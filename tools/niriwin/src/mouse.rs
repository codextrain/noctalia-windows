// Win gestures run on a dedicated hook thread; no layout, I/O or blocking sends in hooks.
use crate::win::*;
use std::{
    cell::RefCell,
    collections::{HashMap, HashSet},
    sync::{
        atomic::{AtomicIsize, Ordering},
        mpsc::{self, Receiver, SyncSender},
        Arc, Mutex,
    },
    thread,
};
#[repr(C)]
#[derive(Clone, Copy, Debug)]
struct Point {
    x: i32,
    y: i32,
}
#[repr(C)]
struct MouseEvent {
    point: Point,
    data: u32,
    flags: u32,
    time: u32,
    extra: usize,
}
#[repr(C)]
struct KeyEvent {
    key: u32,
    scan: u32,
    flags: u32,
    time: u32,
    extra: usize,
}
#[repr(C)]
struct KeyboardInput {
    key: u16,
    scan: u16,
    flags: u32,
    time: u32,
    extra: usize,
}
#[repr(C)]
struct Input {
    kind: u32,
    pad: u32,
    key: KeyboardInput,
    tail: [u8; 8],
}
const MASK: usize = 0x4e495249;
#[derive(Clone, Copy, Debug)]
pub enum Event {
    Spotlight(Handle),
    Left,
    Right,
    Close(Handle),
    DragStart(Handle),
    Drop {
        window: Handle,
        x: i32,
        y: i32,
        moved: bool,
    },
    CancelDrag(Handle),
}
#[derive(Default)]
struct Shared {
    targets: HashMap<Handle, u32>,
    win_keys: bool,
    spotlight: bool,
    drag: bool,
}
type Targets = Arc<Mutex<Shared>>;
struct Drag {
    window: Handle,
    start: Point,
    rect: crate::layout::Rect,
    moved: bool,
}
struct Context {
    targets: Targets,
    sender: SyncSender<Event>,
    swallow_release: bool,
    drag: Option<Drag>,
    active: Arc<AtomicIsize>,
    win_used: bool,
    keys: HashSet<u32>,
}
thread_local! {static CONTEXT:RefCell<Option<Context>>=const{RefCell::new(None)};}
#[link(name = "user32")]
extern "system" {
    fn SetWindowsHookExW(
        kind: i32,
        callback: Option<unsafe extern "system" fn(i32, usize, isize) -> isize>,
        module: Handle,
        thread: u32,
    ) -> Handle;
    fn UnhookWindowsHookEx(h: Handle) -> i32;
    fn CallNextHookEx(h: Handle, code: i32, w: usize, l: isize) -> isize;
    fn GetAsyncKeyState(key: i32) -> i16;
    fn WindowFromPoint(p: Point) -> Handle;
    fn GetAncestor(h: Handle, kind: u32) -> Handle;
    fn GetMessageW(msg: *mut Msg, h: Handle, min: u32, max: u32) -> i32;
    fn PostThreadMessageW(thread: u32, message: u32, w: usize, l: isize) -> i32;
    fn SendInput(count: u32, inputs: *const Input, size: i32) -> u32;
}
#[link(name = "kernel32")]
extern "system" {
    fn GetCurrentThreadId() -> u32;
    fn GetModuleHandleW(name: *const u16) -> Handle;
}
unsafe fn valid(targets: &Shared, h: Handle) -> bool {
    targets.targets.get(&h).is_some_and(|p| *p == pid(h))
        && IsWindowVisible(h) != 0
        && IsZoomed(h) == 0
        && IsIconic(h) == 0
}
unsafe fn mask_win_release(key: u32) -> bool {
    let make = |vk, flags| Input {
        kind: 1,
        pad: 0,
        key: KeyboardInput {
            key: vk,
            scan: 0,
            flags,
            time: 0,
            extra: MASK,
        },
        tail: [0; 8],
    };
    let inputs = [make(0xe8, 0), make(0xe8, 2), make(key as u16, 2)];
    SendInput(3, inputs.as_ptr(), std::mem::size_of::<Input>() as i32) == 3
}
unsafe extern "system" fn keyboard(code: i32, w: usize, l: isize) -> isize {
    if code < 0 || l == 0 {
        return CallNextHookEx(0, code, w, l);
    }
    let event = &*(l as *const KeyEvent);
    if event.extra == MASK {
        return CallNextHookEx(0, code, w, l);
    }
    let down = w == 0x100 || w == 0x104;
    let up = w == 0x101 || w == 0x105;
    let used = CONTEXT.with(|cell| {
        let Ok(mut slot) = cell.try_borrow_mut() else {
            return false;
        };
        let Some(c) = slot.as_mut() else { return false };
        if up && c.keys.remove(&event.key) {
            return true;
        }
        if up && matches!(event.key, 0x5b | 0x5c) && c.win_used {
            c.win_used = false;
            return mask_win_release(event.key);
        }
        if down && c.keys.contains(&event.key) {
            return true;
        }
        if down && event.key == 0x1b && c.drag.is_some() {
            let drag = c.drag.take().unwrap();
            place(drag.window, drag.rect);
            c.active.store(0, Ordering::Release);
            let _ = c.sender.try_send(Event::CancelDrag(drag.window));
            c.keys.insert(event.key);
            return true;
        }
        if !down
            || !matches!(event.key, 0x25 | 0x27 | 0x51)
            || !(GetAsyncKeyState(0x5b) < 0 || GetAsyncKeyState(0x5c) < 0)
            || [0x10, 0x11, 0x12].iter().any(|k| GetAsyncKeyState(*k) < 0)
        {
            return false;
        }
        let Ok(targets) = c.targets.try_lock() else {
            return false;
        };
        let h = GetAncestor(GetForegroundWindow(), 2);
        if !targets.win_keys || !valid(&targets, h) {
            return false;
        }
        let action = match event.key {
            0x25 => Event::Left,
            0x27 => Event::Right,
            _ => Event::Close(h),
        };
        if c.sender.try_send(action).is_err() {
            return false;
        }
        c.keys.insert(event.key);
        c.win_used = true;
        true
    });
    if used {
        1
    } else {
        CallNextHookEx(0, code, w, l)
    }
}
unsafe extern "system" fn mouse(code: i32, w: usize, l: isize) -> isize {
    if code < 0 || l == 0 {
        return CallNextHookEx(0, code, w, l);
    }
    let event = &*(l as *const MouseEvent);
    let consumed = CONTEXT.with(|cell| {
        let Ok(mut slot) = cell.try_borrow_mut() else {
            return false;
        };
        let Some(c) = slot.as_mut() else { return false };
        if w == 0x202 && c.swallow_release {
            c.swallow_release = false;
            if let Some(drag) = c.drag.take() {
                c.active.store(0, Ordering::Release);
                let _ = c.sender.try_send(Event::Drop {
                    window: drag.window,
                    x: event.point.x,
                    y: event.point.y,
                    moved: drag.moved,
                });
            }
            return true;
        }
        if w == 0x200 {
            if let Some(drag) = c.drag.as_mut() {
                let dx = event.point.x - drag.start.x;
                let dy = event.point.y - drag.start.y;
                drag.moved |= dx.abs() >= 6 || dy.abs() >= 6;
                if drag.moved && IsWindow(drag.window) != 0 {
                    SetWindowPos(
                        drag.window,
                        0,
                        drag.rect.left + dx,
                        drag.rect.top + dy,
                        0,
                        0,
                        0x4011,
                    );
                }
                // Let the cursor move normally. Only the drag's button events
                // are swallowed; blocking WM_MOUSEMOVE would freeze the pointer.
                return false;
            }
        }
        if w != 0x201 || [0x10, 0x11].iter().any(|k| GetAsyncKeyState(*k) < 0) {
            return false;
        }
        let alt = GetAsyncKeyState(0x12) < 0;
        let win = GetAsyncKeyState(0x5b) < 0 || GetAsyncKeyState(0x5c) < 0;
        if alt == win {
            return false;
        }
        let root = GetAncestor(WindowFromPoint(event.point), 2);
        let Ok(targets) = c.targets.try_lock() else {
            return false;
        };
        if !valid(&targets, root) {
            return false;
        }
        if win && targets.drag {
            let Some(r) = rect(root) else { return false };
            if c.sender.try_send(Event::DragStart(root)).is_err() {
                return false;
            }
            c.drag = Some(Drag {
                window: root,
                start: event.point,
                rect: r,
                moved: false,
            });
            c.active.store(root, Ordering::Release);
            c.win_used = true;
        } else if alt && targets.spotlight {
            if c.sender.try_send(Event::Spotlight(root)).is_err() {
                return false;
            }
        } else {
            return false;
        }
        c.swallow_release = true;
        true
    });
    if consumed {
        1
    } else {
        CallNextHookEx(0, code, w, l)
    }
}
pub struct Gesture {
    targets: Targets,
    receiver: Receiver<Event>,
    active: Arc<AtomicIsize>,
    thread_id: u32,
    thread: Option<thread::JoinHandle<()>>,
}
impl Gesture {
    pub fn new() -> Result<Self, String> {
        let targets = Arc::new(Mutex::new(Shared::default()));
        let snapshot = targets.clone();
        let active = Arc::new(AtomicIsize::new(0));
        let flag = active.clone();
        let (sender, receiver) = mpsc::sync_channel(64);
        let (ready, result) = mpsc::sync_channel(1);
        let thread = thread::spawn(move || unsafe {
            let id = GetCurrentThreadId();
            let mut msg = Msg::default();
            PeekMessageW(&mut msg, 0, 0, 0, 0);
            CONTEXT.with(|c| {
                *c.borrow_mut() = Some(Context {
                    targets: snapshot,
                    sender,
                    swallow_release: false,
                    drag: None,
                    active: flag,
                    win_used: false,
                    keys: HashSet::new(),
                })
            });
            let module = GetModuleHandleW(std::ptr::null());
            let pointer = SetWindowsHookExW(14, Some(mouse), module, 0);
            let keys = SetWindowsHookExW(13, Some(keyboard), module, 0);
            if pointer == 0 || keys == 0 {
                if pointer != 0 {
                    UnhookWindowsHookEx(pointer);
                }
                if keys != 0 {
                    UnhookWindowsHookEx(keys);
                }
                let _ = ready.send(Err(format!("input hook failed: {}", GetLastError())));
                return;
            }
            let _ = ready.send(Ok(id));
            while GetMessageW(&mut msg, 0, 0, 0) > 0 {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            UnhookWindowsHookEx(pointer);
            UnhookWindowsHookEx(keys);
            CONTEXT.with(|c| *c.borrow_mut() = None);
        });
        match result.recv().map_err(|e| e.to_string())? {
            Ok(thread_id) => Ok(Self {
                targets,
                receiver,
                active,
                thread_id,
                thread: Some(thread),
            }),
            Err(error) => {
                let _ = thread.join();
                Err(error)
            }
        }
    }
    pub fn configure(&self, win_keys: bool, spotlight: bool, drag: bool) {
        if let Ok(mut shared) = self.targets.lock() {
            shared.win_keys = win_keys;
            shared.spotlight = spotlight;
            shared.drag = drag;
        }
    }
    pub fn update(&self, targets: impl IntoIterator<Item = (Handle, u32)>) {
        if let Ok(mut shared) = self.targets.lock() {
            shared.targets = targets.into_iter().collect();
        }
    }
    pub fn events(&self) -> Vec<Event> {
        self.receiver.try_iter().collect()
    }
    pub fn dragging(&self) -> Option<Handle> {
        let h = self.active.load(Ordering::Acquire);
        (h != 0).then_some(h)
    }
}
impl Drop for Gesture {
    fn drop(&mut self) {
        self.update([]);
        unsafe {
            PostThreadMessageW(self.thread_id, 0x12, 0, 0);
        };
        if let Some(thread) = self.thread.take() {
            let _ = thread.join();
        }
    }
}
