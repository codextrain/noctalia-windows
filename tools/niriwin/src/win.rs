// Rust · win.rs · Win32 ABI x64 · Windows 11 · stable Rust GNU/MSVC
#![allow(non_snake_case, dead_code)]
use crate::layout::Rect;
use std::ffi::c_void;
pub type Handle = isize;
#[repr(C)]
#[derive(Default)]
pub struct Placement {
    pub length: u32,
    pub flags: u32,
    pub show: u32,
    pub min_x: i32,
    pub min_y: i32,
    pub max_x: i32,
    pub max_y: i32,
    pub normal: R,
}
#[repr(C)]
pub struct WindowClass {
    pub style: u32,
    pub proc: Option<unsafe extern "system" fn(Handle, u32, usize, isize) -> isize>,
    pub class_extra: i32,
    pub window_extra: i32,
    pub instance: Handle,
    pub icon: Handle,
    pub cursor: Handle,
    pub background: Handle,
    pub menu: *const u16,
    pub name: *const u16,
}
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct R {
    pub left: i32,
    pub top: i32,
    pub right: i32,
    pub bottom: i32,
}
impl From<R> for Rect {
    fn from(r: R) -> Self {
        Self {
            left: r.left,
            top: r.top,
            right: r.right,
            bottom: r.bottom,
        }
    }
}
#[repr(C)]
pub struct MonitorInfo {
    pub size: u32,
    pub monitor: R,
    pub work: R,
    pub flags: u32,
    pub device: [u16; 32],
}
#[repr(C)]
#[derive(Default)]
pub struct Msg {
    pub hwnd: Handle,
    pub message: u32,
    pub wparam: usize,
    pub lparam: isize,
    pub time: u32,
    pub x: i32,
    pub y: i32,
    pub private: u32,
}
#[link(name = "user32")]
extern "system" {
    pub fn GetWindowPlacement(h: Handle, placement: *mut Placement) -> i32;
    pub fn OpenProcess(access: u32, inherit: i32, pid: u32) -> Handle;
    pub fn QueryFullProcessImageNameW(
        process: Handle,
        flags: u32,
        name: *mut u16,
        length: *mut u32,
    ) -> i32;
    pub fn GetWindowRgn(h: Handle, region: Handle) -> i32;
    pub fn SetWindowRgn(h: Handle, region: Handle, redraw: i32) -> i32;
    pub fn SendMessageTimeoutW(
        h: Handle,
        message: u32,
        wparam: usize,
        lparam: isize,
        flags: u32,
        timeout: u32,
        result: *mut usize,
    ) -> isize;
    pub fn RegisterClassW(class: *const WindowClass) -> u16;
    pub fn DefWindowProcW(h: Handle, msg: u32, w: usize, l: isize) -> isize;
    pub fn CreateWindowExW(
        ex: u32,
        class: *const u16,
        title: *const u16,
        style: u32,
        x: i32,
        y: i32,
        w: i32,
        height: i32,
        parent: Handle,
        menu: Handle,
        instance: Handle,
        param: *const c_void,
    ) -> Handle;
    pub fn DestroyWindow(h: Handle) -> i32;
    pub fn TranslateMessage(msg: *const Msg) -> i32;
    pub fn DispatchMessageW(msg: *const Msg) -> isize;
    pub fn EnumWindows(cb: unsafe extern "system" fn(Handle, isize) -> i32, data: isize) -> i32;
    pub fn EnumDisplayMonitors(
        dc: Handle,
        clip: *const R,
        cb: unsafe extern "system" fn(Handle, Handle, *mut R, isize) -> i32,
        data: isize,
    ) -> i32;
    pub fn GetMonitorInfoW(h: Handle, info: *mut MonitorInfo) -> i32;
    pub fn MonitorFromWindow(h: Handle, flags: u32) -> Handle;
    pub fn GetWindowRect(h: Handle, r: *mut R) -> i32;
    pub fn IsWindowVisible(h: Handle) -> i32;
    pub fn IsWindow(h: Handle) -> i32;
    pub fn IsIconic(h: Handle) -> i32;
    pub fn IsZoomed(h: Handle) -> i32;
    pub fn GetWindow(h: Handle, cmd: u32) -> Handle;
    pub fn GetWindowLongW(h: Handle, index: i32) -> i32;
    pub fn GetClassNameW(h: Handle, out: *mut u16, max: i32) -> i32;
    pub fn GetWindowTextLengthW(h: Handle) -> i32;
    pub fn GetWindowThreadProcessId(h: Handle, pid: *mut u32) -> u32;
    pub fn GetForegroundWindow() -> Handle;
    pub fn SetForegroundWindow(h: Handle) -> i32;
    pub fn PostMessageW(h: Handle, message: u32, w: usize, l: isize) -> i32;
    pub fn SetWindowPos(
        h: Handle,
        after: Handle,
        x: i32,
        y: i32,
        w: i32,
        height: i32,
        flags: u32,
    ) -> i32;
    pub fn ShowWindowAsync(h: Handle, cmd: i32) -> i32;
    pub fn RegisterHotKey(h: Handle, id: i32, mods: u32, key: u32) -> i32;
    pub fn UnregisterHotKey(h: Handle, id: i32) -> i32;
    pub fn PeekMessageW(msg: *mut Msg, h: Handle, min: u32, max: u32, remove: u32) -> i32;
    pub fn SetProcessDpiAwarenessContext(context: isize) -> i32;
}
pub fn is_noctalia(h: Handle) -> bool {
    unsafe {
        let process = OpenProcess(0x1000, 0, pid(h));
        if process == 0 {
            return false;
        }
        let mut name = [0u16; 1024];
        let mut len = name.len() as u32;
        let ok = QueryFullProcessImageNameW(process, 0, name.as_mut_ptr(), &mut len) != 0;
        CloseHandle(process);
        ok && String::from_utf16_lossy(&name[..len as usize])
            .to_lowercase()
            .ends_with("\\noctalia.exe")
    }
}
#[link(name = "gdi32")]
extern "system" {
    pub fn CreateRectRgn(left: i32, top: i32, right: i32, bottom: i32) -> Handle;
    pub fn CreateRoundRectRgn(
        left: i32,
        top: i32,
        right: i32,
        bottom: i32,
        ellipse_width: i32,
        ellipse_height: i32,
    ) -> Handle;
    pub fn DeleteObject(object: Handle) -> i32;
    pub fn EqualRgn(first: Handle, second: Handle) -> i32;
}
pub fn has_custom_region(h: Handle) -> bool {
    unsafe {
        let region = CreateRectRgn(0, 0, 0, 0);
        if region == 0 {
            return true;
        }
        let result = GetWindowRgn(h, region);
        DeleteObject(region);
        result != 0
    }
}
pub fn clear_region(h: Handle) -> bool {
    responsive(h) && unsafe { SetWindowRgn(h, 0, 1) != 0 }
}
fn responsive(h: Handle) -> bool {
    let mut result = 0;
    unsafe { SendMessageTimeoutW(h, 0, 0, 0, 3, 100, &mut result) != 0 }
}
pub fn rounded_region(h: Handle, width: i32, height: i32, radius: i32) -> bool {
    if width <= 0 || height <= 0 || radius <= 0 || !responsive(h) {
        return false;
    }
    unsafe {
        let region = CreateRoundRectRgn(0, 0, width + 1, height + 1, radius * 2, radius * 2);
        if region == 0 {
            return false;
        }
        if SetWindowRgn(h, region, 1) == 0 {
            DeleteObject(region);
            false
        } else {
            true
        }
    }
}
#[link(name = "kernel32")]
extern "system" {
    pub fn LoadLibraryExW(name: *const u16, file: Handle, flags: u32) -> Handle;
    pub fn GetProcAddress(module: Handle, name: *const u8) -> *const c_void;
    pub fn CreateMutexW(attrs: *const c_void, owner: i32, name: *const u16) -> Handle;
    pub fn GetLastError() -> u32;
    pub fn CloseHandle(h: Handle) -> i32;
    pub fn SetConsoleCtrlHandler(
        handler: Option<unsafe extern "system" fn(u32) -> i32>,
        add: i32,
    ) -> i32;
}
pub fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(Some(0)).collect()
}
pub fn class(h: Handle) -> String {
    let mut s = [0u16; 256];
    let n = unsafe { GetClassNameW(h, s.as_mut_ptr(), 256) };
    String::from_utf16_lossy(&s[..n.max(0) as usize])
}
pub fn pid(h: Handle) -> u32 {
    let mut p = 0;
    unsafe { GetWindowThreadProcessId(h, &mut p) };
    p
}
pub fn rect(h: Handle) -> Option<Rect> {
    let mut r = R::default();
    if unsafe { GetWindowRect(h, &mut r) } != 0 {
        Some(r.into())
    } else {
        None
    }
}
pub fn cloaked(h: Handle) -> bool {
    dwm_get_u32(h, 14).is_none_or(|v| v != 0)
}
pub fn visible_rect(h: Handle) -> Option<Rect> {
    if unsafe { IsIconic(h) != 0 } {
        return None;
    }
    static FUNCTION: std::sync::OnceLock<usize> = std::sync::OnceLock::new();
    let address = *FUNCTION.get_or_init(|| unsafe {
        let module = dwm_module();
        if module == 0 {
            0
        } else {
            GetProcAddress(module, c"DwmGetWindowAttribute".as_ptr().cast()) as usize
        }
    });
    if address == 0 {
        return None;
    }
    let function: unsafe extern "system" fn(Handle, u32, *mut c_void, u32) -> i32 =
        unsafe { std::mem::transmute(address) };
    let mut bounds = R::default();
    if unsafe {
        function(
            h,
            9,
            &mut bounds as *mut _ as *mut c_void,
            std::mem::size_of::<R>() as u32,
        )
    } == 0
    {
        Some(bounds.into())
    } else {
        None
    }
}
fn dwm_module() -> Handle {
    static MODULE: std::sync::OnceLock<Handle> = std::sync::OnceLock::new();
    *MODULE.get_or_init(|| unsafe { LoadLibraryExW(wide("dwmapi.dll").as_ptr(), 0, 0x800) })
}
pub fn dwm_get_u32(h: Handle, attribute: u32) -> Option<u32> {
    static FUNCTION: std::sync::OnceLock<usize> = std::sync::OnceLock::new();
    let p = *FUNCTION.get_or_init(|| unsafe {
        // LOAD_LIBRARY_SEARCH_SYSTEM32 avoids loading a DLL from the package folder.
        let m = dwm_module();
        if m == 0 {
            0
        } else {
            GetProcAddress(m, c"DwmGetWindowAttribute".as_ptr().cast()) as usize
        }
    });
    if p == 0 {
        return None;
    }
    let f: unsafe extern "system" fn(Handle, u32, *mut c_void, u32) -> i32 =
        unsafe { std::mem::transmute(p) };
    let mut v = 0u32;
    if unsafe { f(h, attribute, &mut v as *mut _ as *mut c_void, 4) } == 0 {
        Some(v)
    } else {
        None
    }
}
pub fn dwm_set_u32(h: Handle, attribute: u32, value: u32) -> bool {
    static FUNCTION: std::sync::OnceLock<usize> = std::sync::OnceLock::new();
    let p = *FUNCTION.get_or_init(|| unsafe {
        let m = dwm_module();
        if m == 0 {
            0
        } else {
            GetProcAddress(m, c"DwmSetWindowAttribute".as_ptr().cast()) as usize
        }
    });
    if p == 0 {
        return false;
    }
    let f: unsafe extern "system" fn(Handle, u32, *const c_void, u32) -> i32 =
        unsafe { std::mem::transmute(p) };
    unsafe { f(h, attribute, &value as *const _ as *const c_void, 4) == 0 }
}
pub fn top_bar(m: &Monitor, class_name: &str) -> Option<Rect> {
    windows()
        .into_iter()
        .filter(|h| {
            class(*h) == class_name
                && (unsafe {
                    IsWindowVisible(*h) != 0
                        && IsIconic(*h) == 0
                        && MonitorFromWindow(*h, 2) == m.id
                })
                && !cloaked(*h)
        })
        .filter_map(rect)
        .filter_map(|r| crate::layout::bounded_top_bar(m.bounds, r))
        .max_by_key(|r| r.width())
}
pub fn place(h: Handle, r: Rect) -> bool {
    unsafe {
        SetWindowPos(
            h,
            0,
            r.left,
            r.top,
            r.width(),
            r.height(),
            0x0014 | 0x0200 | 0x4000,
        ) != 0
    }
}
#[derive(Clone, Debug)]
pub struct Monitor {
    pub id: Handle,
    pub name: String,
    pub bounds: Rect,
    pub work: Rect,
}
pub fn monitors() -> Vec<Monitor> {
    unsafe extern "system" fn cb(h: Handle, _: Handle, _: *mut R, data: isize) -> i32 {
        let v = &mut *(data as *mut Vec<Monitor>);
        let mut i = MonitorInfo {
            size: std::mem::size_of::<MonitorInfo>() as u32,
            monitor: R::default(),
            work: R::default(),
            flags: 0,
            device: [0; 32],
        };
        if GetMonitorInfoW(h, &mut i) != 0 {
            v.push(Monitor {
                id: h,
                name: String::from_utf16_lossy(
                    &i.device[..i.device.iter().position(|c| *c == 0).unwrap_or(32)],
                ),
                bounds: i.monitor.into(),
                work: i.work.into(),
            });
        }
        1
    }
    let mut v = Vec::new();
    unsafe { EnumDisplayMonitors(0, std::ptr::null(), cb, &mut v as *mut _ as isize) };
    v.sort_by_key(|m: &Monitor| (m.bounds.left, m.bounds.top));
    v
}
pub fn windows() -> Vec<Handle> {
    unsafe extern "system" fn cb(h: Handle, data: isize) -> i32 {
        (*(data as *mut Vec<Handle>)).push(h);
        1
    }
    let mut v = Vec::new();
    unsafe { EnumWindows(cb, &mut v as *mut _ as isize) };
    v
}
