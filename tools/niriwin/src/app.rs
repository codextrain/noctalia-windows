// Rust · app.rs · manager and recovery · Windows 11 x64 · stable Rust
use crate::{
    config::Config,
    layout::{self, Rect},
    win::*,
};
use std::{
    collections::HashMap,
    fs,
    path::{Path, PathBuf},
    sync::atomic::{AtomicBool, Ordering},
    time::{Duration, Instant},
};
static STOP: AtomicBool = AtomicBool::new(false);
unsafe extern "system" fn ctrl(_: u32) -> i32 {
    STOP.store(true, Ordering::SeqCst);
    1
}
#[derive(Clone, Debug)]
struct Saved {
    h: Handle,
    pid: u32,
    class: String,
    r: Rect,
    parked: bool,
    monitor: Handle,
    workspace: usize,
    corner: Option<u32>,
    border: Option<u32>,
    border_applied: Option<u32>,
    clip_allowed: bool,
    clipped: bool,
    clip_state: Option<(i32, i32, i32)>,
}
impl Saved {
    fn valid(&self) -> bool {
        (unsafe { IsWindow(self.h) != 0 }) && pid(self.h) == self.pid && class(self.h) == self.class
    }
    fn in_workspace(&self) -> bool {
        workspace_member(
            self.parked,
            unsafe { IsWindowVisible(self.h) != 0 },
            unsafe { IsIconic(self.h) != 0 },
            cloaked(self.h),
        )
    }
}
// Minimize keeps membership; hiding to the tray and changing Windows desktops do not.
fn workspace_member(parked: bool, visible: bool, _minimized: bool, cloaked: bool) -> bool {
    !cloaked && (parked || visible)
}
fn protected(h: Handle, m: &Monitor) -> bool {
    (unsafe { IsZoomed(h) != 0 }) || rect(h).is_some_and(|r| layout::covers(r, m.bounds))
}
fn eligible(h: Handle, c: &Config, only: &Option<String>) -> bool {
    let name = class(h);
    if only.as_ref().is_some_and(|s| s != &name)
        || name == c.bar_class
        || c.exclude.contains(&name)
        || [
            "Shell_TrayWnd",
            "Shell_SecondaryTrayWnd",
            "Progman",
            "WorkerW",
            "ConsoleWindowClass",
            "Windows.UI.Core.CoreWindow",
        ]
        .contains(&name.as_str())
    {
        return false;
    }
    unsafe {
        IsWindowVisible(h) != 0
            && GetWindow(h, 4) == 0
            && GetWindowTextLengthW(h) > 0
            && GetWindowLongW(h, -16) as u32 & 0x00040000 != 0
            && GetWindowLongW(h, -20) as u32 & (0x80 | 0x08000000) == 0
            && !cloaked(h)
            // Maximized windows may carry a system-created region. They are
            // still protected by the manager; --retile explicitly restores them.
            && (IsZoomed(h) != 0 || !has_custom_region(h))
            && pid(h) != std::process::id()
    }
}
fn atomic_write(p: &Path, s: &str) -> Result<(), String> {
    let t = p.with_extension("tmp");
    fs::write(&t, s).map_err(|e| e.to_string())?;
    fs::rename(t, p).map_err(|e| e.to_string())
}
fn save(p: &Path, v: &[Saved]) -> Result<(), String> {
    let mut s = String::from("NIRIWIN1\n");
    for w in v {
        if w.class.contains(['\t', '\n', '\r']) {
            return Err("unsupported window class".into());
        }
        s.push_str(&format!(
            "{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
            w.h,
            w.pid,
            w.r.left,
            w.r.top,
            w.r.right,
            w.r.bottom,
            w.parked as u8,
            w.class,
            w.corner
                .map(|v| v.to_string())
                .unwrap_or_else(|| "-".into()),
            w.clip_allowed as u8,
            w.clipped as u8,
            w.border
                .map(|v| v.to_string())
                .unwrap_or_else(|| "-".into())
        ));
    }
    atomic_write(p, &s)
}
fn load(p: &Path) -> Result<Vec<Saved>, String> {
    let s = fs::read_to_string(p).map_err(|e| e.to_string())?;
    let mut lines = s.lines();
    if lines.next() != Some("NIRIWIN1") {
        return Err("invalid recovery header".into());
    }
    let mut v = Vec::new();
    for l in lines {
        let f: Vec<_> = l.split('\t').collect();
        if ![8, 9, 11, 12].contains(&f.len()) {
            return Err("invalid recovery record".into());
        }
        let n = |i: usize| {
            f[i].parse::<i64>()
                .map_err(|_| "invalid recovery number".to_string())
        };
        let r = Rect {
            left: n(2)?.try_into().map_err(|_| "coordinate overflow")?,
            top: n(3)?.try_into().map_err(|_| "coordinate overflow")?,
            right: n(4)?.try_into().map_err(|_| "coordinate overflow")?,
            bottom: n(5)?.try_into().map_err(|_| "coordinate overflow")?,
        };
        if r.width() <= 0 || r.height() <= 0 || r.width() > 100000 || r.height() > 100000 {
            return Err("invalid recovery rectangle".into());
        }
        v.push(Saved {
            h: n(0)? as Handle,
            pid: n(1)?.try_into().map_err(|_| "invalid pid")?,
            r,
            parked: n(6)? == 1,
            class: f[7].into(),
            monitor: 0,
            workspace: 0,
            corner: if f.len() >= 9 && f[8] != "-" {
                let value = f[8]
                    .parse::<u32>()
                    .map_err(|_| "invalid corner preference")?;
                if value > 3 {
                    return Err("invalid corner preference".into());
                }
                Some(value)
            } else {
                None
            },
            border: if f.len() >= 12 && f[11] != "-" {
                Some(f[11].parse().map_err(|_| "invalid border color")?)
            } else {
                None
            },
            border_applied: None,
            clip_allowed: f.len() >= 11 && f[9] == "1",
            clipped: f.len() >= 11 && f[9] == "1" && f[10] == "1",
            clip_state: None,
        });
    }
    Ok(v)
}
fn restore(v: &[Saved]) -> bool {
    let monitors = monitors();
    let mut success = true;
    for w in v {
        if !w.valid() {
            continue;
        }
        if w.clipped && !clear_region(w.h) {
            eprintln!("region restoration failed: {}", w.h);
            success = false;
        }
        if let Some(original) = w.corner {
            if dwm_get_u32(w.h, 33) != Some(original) && !dwm_set_u32(w.h, 33, original) {
                eprintln!("corner restoration failed: {}", w.h);
                success = false;
            }
        }
        if let Some(original) = w.border {
            if dwm_get_u32(w.h, 34) != Some(original) && !dwm_set_u32(w.h, 34, original) {
                success = false;
            }
        }
        let m = monitors
            .iter()
            .find(|m| m.id == unsafe { MonitorFromWindow(w.h, 2) });
        if m.is_some_and(|m| protected(w.h, m)) {
            continue;
        }
        if !w.parked && unsafe { IsIconic(w.h) != 0 } {
            continue;
        }
        if w.parked {
            unsafe { ShowWindowAsync(w.h, 4) };
        }
        let mut r = w.r;
        // A removed monitor must not strand a recovered window outside the desktop.
        if !monitors.iter().any(|m| {
            r.left < m.work.right
                && r.right > m.work.left
                && r.top < m.work.bottom
                && r.bottom > m.work.top
        }) {
            if let Some(m) = monitors.first() {
                r = Rect {
                    left: m.work.left + 12,
                    top: m.work.top + 12,
                    right: m.work.left + 12 + r.width().min(m.work.width() - 24),
                    bottom: m.work.top + 12 + r.height().min(m.work.height() - 24),
                };
            }
        }
        if !place(w.h, r) {
            eprintln!("restore failed: {} error {}", w.h, unsafe {
                GetLastError()
            });
            success = false;
        }
    }
    success
}
struct Lock(Handle);
impl Drop for Lock {
    fn drop(&mut self) {
        unsafe { CloseHandle(self.0) };
    }
}
struct Hotkeys(Vec<i32>);
impl Drop for Hotkeys {
    fn drop(&mut self) {
        for id in &self.0 {
            unsafe { UnregisterHotKey(0, *id) };
        }
    }
}
fn lock() -> Result<Lock, String> {
    let h = unsafe { CreateMutexW(std::ptr::null(), 0, wide("Local\\NiriWin.MVP.v1").as_ptr()) };
    if h == 0 {
        return Err("cannot create single-instance lock".into());
    }
    if unsafe { GetLastError() } == 183 {
        unsafe { CloseHandle(h) };
        return Err("NiriWin is running; use --stop before recovery".into());
    }
    Ok(Lock(h))
}
pub fn main() -> Result<(), String> {
    let args: Vec<String> = std::env::args().skip(1).collect();
    if args.is_empty() || args.iter().any(|a| a == "--help") {
        println!("NiriWin 0.1\n--dry-run | --run [--seconds N] [--only-class CLASS]\n--pause | --resume | --stop | --recover\n--config PATH (default: beside executable)\nAlt+Left/Right: focus; Ctrl+Alt+Shift; P: pause/restore; R: reload; Q: restore/exit\nNo startup installation. Default is help; --run explicitly enables movement.");
        return Ok(());
    }
    let mut run = false;
    let mut dry = false;
    let mut command: Option<String> = None;
    let mut recover = false;
    let mut seconds = None;
    let mut only = None;
    let base = std::env::current_exe()
        .map_err(|e| e.to_string())?
        .parent()
        .unwrap()
        .to_path_buf();
    let mut config = base.join("niriwin.conf");
    let mut i = 0;
    while i < args.len() {
        match args[i].as_str() {
            "--run" => run = true,
            "--dry-run" => dry = true,
            "--pause" => command = Some("pause".into()),
            "--resume" => command = Some("resume".into()),
            "--stop" => command = Some("stop".into()),
            "--focus-left" => command = Some("left".into()),
            "--focus-right" => command = Some("right".into()),
            "--retile" => command = Some("retile".into()),
            "--spotlight-window" => {
                i += 1;
                let handle = args
                    .get(i)
                    .ok_or("missing window handle")?
                    .parse::<isize>()
                    .map_err(|_| "invalid window handle")?;
                if handle <= 0 {
                    return Err("invalid window handle".into());
                }
                command = Some(format!("spotlight:{handle}"));
            }
            "--recover" => recover = true,
            "--config" | "--seconds" | "--only-class" => {
                let key = &args[i];
                i += 1;
                let value = args.get(i).ok_or("missing argument")?;
                match key.as_str() {
                    "--config" => config = PathBuf::from(value),
                    "--seconds" => {
                        seconds = Some(value.parse::<u64>().map_err(|_| "invalid seconds")?)
                    }
                    _ => only = Some(value.clone()),
                }
            }
            a => return Err(format!("unknown argument {a}")),
        }
        i += 1;
    }
    if [run, dry, command.is_some(), recover]
        .into_iter()
        .filter(|v| *v)
        .count()
        != 1
    {
        return Err("choose exactly one operation".into());
    }
    let state = base.join("state");
    fs::create_dir_all(&state).map_err(|e| e.to_string())?;
    let journal = state.join("recovery.tsv");
    let blur_journal = state.join("blur-recovery.json");
    let taskbar_journal = state.join("taskbar-recovery.json");
    let control = state.join("command.txt");
    if let Some(cmd) = command {
        return atomic_write(&control, &cmd);
    }
    let _lock = lock()?;
    unsafe { SetProcessDpiAwarenessContext(-4) };
    if recover {
        crate::taskbar::recover(&taskbar_journal)?;
        crate::effects::recover(&blur_journal)?;
        if !journal.exists() {
            println!("No recovery journal.");
            return Ok(());
        }
        let v = load(&journal)?;
        if restore(&v) {
            fs::remove_file(journal).map_err(|e| e.to_string())?;
            println!("Recovery requested for {} records.", v.len());
            return Ok(());
        }
        return Err("some restores failed; journal retained".into());
    }
    let mut c = Config::load(&config)?;
    let noctalia_config = std::env::var_os("NOCTALIA_CONFIG")
        .map(PathBuf::from)
        .unwrap_or_else(|| {
            PathBuf::from(std::env::var_os("LOCALAPPDATA").unwrap_or_default())
                .join("noctalia-win/config.json")
        });
    let mut corner_preference =
        crate::appearance::Corners::Native(if c.round_corners { Some(3) } else { None });
    if dry {
        for m in monitors() {
            println!("Monitor {} work {:?}", m.id, m.work);
            if c.align_to_bar {
                println!("  bar={:?}", top_bar(&m, &c.bar_class));
            }
            for h in windows() {
                if unsafe { MonitorFromWindow(h, 2) } == m.id && eligible(h, &c, &only) {
                    println!(
                        "  hwnd={h} class={} protected={}",
                        class(h),
                        protected(h, &m)
                    );
                }
            }
        }
        return Ok(());
    }
    if journal.exists() || blur_journal.exists() || taskbar_journal.exists() {
        return Err("recovery journal exists: run --recover first".into());
    }
    let mut keys = Hotkeys(Vec::new());
    for (id, key) in [(1, 0x25), (2, 0x27), (3, 0x50), (4, 0x51), (5, 0x52)] {
        if id <= 2 && c.navigation_modifiers == 8 {
            continue;
        }
        let modifiers = if id <= 2 {
            c.navigation_modifiers
        } else {
            c.modifiers
        };
        if unsafe { RegisterHotKey(0, id, 0x4000 | modifiers, key) } == 0 {
            return Err(format!("shortcut conflict (id {id}); no windows moved"));
        }
        keys.0.push(id);
    }
    let _ = fs::remove_file(&control);
    unsafe { SetConsoleCtrlHandler(Some(ctrl), 1) };
    STOP.store(false, Ordering::SeqCst);
    println!("NiriWin running. Ctrl+Alt+Shift+P restores/pauses; Ctrl+Alt+Shift+Q restores/exits.");
    let mut v: Vec<Saved> = Vec::new();
    let mut blur = crate::effects::Blur::new(blur_journal);
    let mut taskbar = crate::taskbar::Taskbar::new(taskbar_journal);
    let mut blur_settings = crate::appearance::BlurSettings {
        enabled: false,
        intensity: 50,
    };
    let mut focus: HashMap<Handle, Handle> = HashMap::new();
    let mut pending_focus: HashMap<Handle, (Handle, Instant)> = HashMap::new();
    let mouse = crate::mouse::Gesture::new()?;
    let mut spotlights = crate::spotlight::Spotlights::default();
    let mut paused = false;
    let start = Instant::now();
    let result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(
        || -> Result<(), String> {
            loop {
                if STOP.load(Ordering::SeqCst)
                    || seconds.is_some_and(|s| start.elapsed() >= Duration::from_secs(s))
                {
                    break;
                }
                let mut action = None;
                let mut spotlight_clicks = Vec::new();
                let mut drag_events = Vec::new();
                let mut close_requests = Vec::new();
                for event in mouse.events() {
                    match event {
                        crate::mouse::Event::Spotlight(h) => spotlight_clicks.push(h),
                        crate::mouse::Event::Left => action = Some("left".into()),
                        crate::mouse::Event::Right => action = Some("right".into()),
                        crate::mouse::Event::Close(h) => close_requests.push(h),
                        event => drag_events.push(event),
                    }
                }
                let mut msg = Msg::default();
                while unsafe { PeekMessageW(&mut msg, 0, 0, 0, 1) } != 0 {
                    if msg.message == 0x0312 {
                        action = Some(
                            match msg.wparam {
                                1 => "left",
                                2 => "right",
                                3 => "toggle",
                                4 => "stop",
                                5 => "reload",
                                _ => "",
                            }
                            .to_string(),
                        );
                    }
                }
                if let Ok(s) = fs::read_to_string(&control) {
                    let _ = fs::remove_file(&control);
                    action = Some(s.trim().into());
                }
                if let Some(a) = action.as_deref() {
                    if let Some(h) = a
                        .strip_prefix("spotlight:")
                        .and_then(|value| value.parse::<isize>().ok())
                    {
                        spotlight_clicks.push(h);
                    }
                    match a {
                        "stop" => break,
                        "pause" | "toggle" if !paused => {
                            taskbar.restore()?;
                            blur.restore()?;
                            if !restore(&v) {
                                return Err("pause restoration failed".into());
                            }
                            v.clear();
                            spotlights.clear();
                            mouse.update([]);
                            save(&journal, &v)?;
                            paused = true;
                        }
                        "resume" | "toggle" => paused = false,
                        // Explicit repair only: ordinary maximized windows on the
                        // active monitor. Normal operation still preserves maximization.
                        "retile" => {
                            let monitor = unsafe { MonitorFromWindow(GetForegroundWindow(), 2) };
                            for h in windows() {
                                if eligible(h, &c, &only)
                                    && unsafe {
                                        IsZoomed(h) != 0 && MonitorFromWindow(h, 2) == monitor
                                    }
                                {
                                    unsafe { ShowWindowAsync(h, 9) };
                                }
                            }
                        }
                        "reload" => match Config::load(&config) {
                            Ok(new) => {
                                if new.modifiers != c.modifiers
                                    || new.navigation_modifiers != c.navigation_modifiers
                                {
                                    eprintln!(
                                        "reload rejected: shortcut modifiers require restart"
                                    );
                                } else {
                                    if !restore(&v) {
                                        return Err("reload restoration failed".into());
                                    }
                                    v.clear();
                                    save(&journal, &v)?;
                                    c = new;
                                }
                            }
                            Err(e) => eprintln!("reload rejected: {e}"),
                        },
                        _ => {}
                    }
                }
                if let Ok(settings) = crate::appearance::blur_load(&noctalia_config) {
                    blur_settings = settings;
                }
                if let Ok(preference) = crate::appearance::load(
                    &noctalia_config,
                    if c.round_corners { Some(3) } else { None },
                ) {
                    corner_preference = preference;
                }
                let decoration = crate::appearance::decorations(&noctalia_config).unwrap_or(
                    crate::appearance::Decorations {
                        win_drag: false,
                        hide_taskbar: false,
                        spotlight: false,
                        borders: false,
                        active: 0,
                        inactive: 0,
                        animate: false,
                    },
                );
                taskbar.update(!paused && decoration.hide_taskbar)?;
                let mut ms = monitors();
                taskbar.expand_monitors(&mut ms, &c.bar_class);
                let mut drop_overrides = HashMap::new();
                if !paused {
                    for h in close_requests {
                        if v.iter().any(|w| {
                            w.h == h
                                && w.valid()
                                && unsafe { IsIconic(w.h) == 0 && IsWindowVisible(w.h) != 0 }
                                && ms.iter().any(|m| m.id == w.monitor && !protected(h, m))
                        }) {
                            unsafe { PostMessageW(h, 0x10, 0, 0) };
                        }
                    }
                    for event in drag_events {
                        let h = match event {
                            crate::mouse::Event::DragStart(h)
                            | crate::mouse::Event::CancelDrag(h) => h,
                            crate::mouse::Event::Drop { window, .. } => window,
                            _ => continue,
                        };
                        let Some(i) = v.iter().position(|w| {
                            w.h == h
                                && w.valid()
                                && unsafe { IsIconic(h) == 0 && IsWindowVisible(h) != 0 }
                        }) else {
                            continue;
                        };
                        let old_monitor = v[i].monitor;
                        v[i].parked = false;
                        let old_group = v[i].workspace;
                        focus.insert(old_monitor, h);
                        pending_focus
                            .insert(old_monitor, (h, Instant::now() + Duration::from_secs(1)));
                        unsafe { SetForegroundWindow(h) };
                        match event {
                            crate::mouse::Event::CancelDrag(_) => {
                                drop_overrides.insert(h, old_monitor);
                            }
                            crate::mouse::Event::Drop {
                                x, y, moved: true, ..
                            } => {
                                let Some(dest) = ms.iter().find(|m| {
                                    x >= m.bounds.left
                                        && x < m.bounds.right
                                        && y >= m.bounds.top
                                        && y < m.bounds.bottom
                                }) else {
                                    drop_overrides.insert(h, old_monitor);
                                    continue;
                                };
                                spotlights.clear_group(old_monitor, old_group);
                                if dest.id != old_monitor {
                                    let active = focus
                                        .get(&dest.id)
                                        .and_then(|handle| v.iter().find(|w| w.h == *handle))
                                        .map(|w| w.workspace);
                                    let groups: Vec<_> = v
                                        .iter()
                                        .filter(|w| w.monitor == dest.id && w.in_workspace())
                                        .map(|w| w.workspace)
                                        .collect();
                                    let group = layout::drop_group(&groups, active);
                                    if let Some(partner) = v.iter().find(|w| {
                                        w.h != h
                                            && w.monitor == old_monitor
                                            && w.workspace == old_group
                                            && w.in_workspace()
                                    }) {
                                        focus.insert(old_monitor, partner.h);
                                    } else {
                                        focus.remove(&old_monitor);
                                    }
                                    pending_focus.remove(&old_monitor);
                                    v[i].monitor = dest.id;
                                    v[i].workspace = group;
                                    focus.insert(dest.id, h);
                                    pending_focus.insert(
                                        dest.id,
                                        (h, Instant::now() + Duration::from_secs(1)),
                                    );
                                    spotlights.clear_group(dest.id, group);
                                }
                                drop_overrides.insert(h, dest.id);
                                let group = v[i].workspace;
                                let members: Vec<_> = v
                                    .iter()
                                    .enumerate()
                                    .filter(|(_, w)| {
                                        w.monitor == dest.id
                                            && w.workspace == group
                                            && w.in_workspace()
                                    })
                                    .map(|(i, _)| i)
                                    .collect();
                                if members.len() == 2 {
                                    let slot = usize::from(
                                        x >= (dest.bounds.left + dest.bounds.right) / 2,
                                    );
                                    let wanted = members[slot];
                                    if wanted != i {
                                        v.swap(i, wanted);
                                    }
                                }
                            }
                            _ => {}
                        }
                    }
                }
                v.retain(|w| w.valid());
                spotlights.retain(|monitor, group, h| {
                    v.iter().any(|w| {
                        w.h == h && w.monitor == monitor && w.workspace == group && w.in_workspace()
                    })
                });
                if !decoration.spotlight {
                    spotlights.clear();
                }
                if !paused && decoration.spotlight {
                    for h in spotlight_clicks {
                        if let Some(w) = v.iter().find(|w| {
                            w.h == h
                                && w.valid()
                                && w.in_workspace()
                                && !w.parked
                                && ms.iter().any(|m| m.id == w.monitor && !protected(w.h, m))
                        }) {
                            spotlights.toggle(w.monitor, w.workspace, w.h);
                            focus.insert(w.monitor, w.h);
                            pending_focus
                                .insert(w.monitor, (w.h, Instant::now() + Duration::from_secs(1)));
                        }
                    }
                }
                for w in &mut v {
                    if w.clipped
                        && ms.iter().any(|m| {
                            m.id == unsafe { MonitorFromWindow(w.h, 2) } && protected(w.h, m)
                        })
                        && clear_region(w.h)
                    {
                        w.clipped = false;
                        w.clip_state = None;
                    }
                }
                if !paused {
                    for h in windows() {
                        if v.iter().any(|w| w.h == h) || !eligible(h, &c, &only) {
                            continue;
                        }
                        let id = unsafe { MonitorFromWindow(h, 2) };
                        if let Some(m) = ms.iter().find(|m| m.id == id) {
                            if !protected(h, m) {
                                let original = if unsafe { IsIconic(h) != 0 } {
                                    let mut placement = Placement {
                                        length: std::mem::size_of::<Placement>() as u32,
                                        ..Default::default()
                                    };
                                    if unsafe { GetWindowPlacement(h, &mut placement) } != 0 {
                                        let mut r: Rect = placement.normal.into();
                                        let dx = m.work.left - m.bounds.left;
                                        let dy = m.work.top - m.bounds.top;
                                        r.left += dx;
                                        r.right += dx;
                                        r.top += dy;
                                        r.bottom += dy;
                                        Some(r)
                                    } else {
                                        None
                                    }
                                } else {
                                    rect(h)
                                };
                                if let Some(r) = original {
                                    v.push(Saved {
                                        h,
                                        pid: pid(h),
                                        class: class(h),
                                        r,
                                        parked: false,
                                        monitor: id,
                                        workspace: layout::new_group(
                                            &v.iter()
                                                .filter(|w| w.monitor == id)
                                                .map(|w| w.workspace)
                                                .collect::<Vec<_>>(),
                                        ),
                                        corner: dwm_get_u32(h, 33),
                                        // DWMWA_BORDER_COLOR is set-only on this Windows build.
                                        // DWMWA_COLOR_DEFAULT restores the system-managed border.
                                        border: Some(dwm_get_u32(h, 34).unwrap_or(u32::MAX)),
                                        border_applied: None,
                                        clip_allowed: !has_custom_region(h),
                                        clipped: false,
                                        clip_state: None,
                                    });
                                }
                            }
                        }
                    }
                    // Save originals BEFORE any operation. A crash leaves a recoverable journal.
                    save(&journal, &v)?;
                    for w in &mut v {
                        if let Some(original) = w.border {
                            let allowed = decoration.borders
                                && w.in_workspace()
                                && !ms.iter().any(|m| m.id == w.monitor && protected(w.h, m));
                            let color = if !allowed {
                                original
                            } else if unsafe { GetForegroundWindow() == w.h } {
                                decoration.active
                            } else {
                                decoration.inactive
                            };
                            if w.border_applied != Some(color) && dwm_set_u32(w.h, 34, color) {
                                w.border_applied = Some(color);
                            }
                        }
                    }
                    for m in &ms {
                        if mouse.dragging().is_some() || native_move_active() {
                            continue;
                        }
                        // Do not rearrange anything on a monitor with a fullscreen/maximized app.
                        if windows().iter().any(|h| {
                            (unsafe {
                                IsWindowVisible(*h) != 0
                                    && IsIconic(*h) == 0
                                    && MonitorFromWindow(*h, 2) == m.id
                                    && GetWindow(*h, 4) == 0
                                    && GetWindowLongW(*h, -20) as u32 & 0x80 == 0
                            }) && !cloaked(*h)
                                && (rect(*h).is_some_and(|r| layout::covers(r, m.bounds))
                                    || (unsafe {
                                        GetForegroundWindow() == *h && IsZoomed(*h) != 0
                                    }))
                                && window_title_length(*h) > 0
                                && ![
                                    "Progman",
                                    "WorkerW",
                                    "Shell_TrayWnd",
                                    "Shell_SecondaryTrayWnd",
                                ]
                                .contains(&class(*h).as_str())
                        }) {
                            continue;
                        }
                        for i in 0..v.len() {
                            let old_monitor = v[i].monitor;
                            let next_monitor = if let Some(monitor) = drop_overrides.get(&v[i].h) {
                                *monitor
                            } else if !v[i].parked {
                                unsafe { MonitorFromWindow(v[i].h, 2) }
                            } else if !ms.iter().any(|monitor| monitor.id == old_monitor) {
                                m.id
                            } else {
                                old_monitor
                            };
                            if old_monitor != next_monitor {
                                v[i].workspace = layout::new_group(
                                    &v.iter()
                                        .enumerate()
                                        .filter(|(j, w)| *j != i && w.monitor == next_monitor)
                                        .map(|(_, w)| w.workspace)
                                        .collect::<Vec<_>>(),
                                );
                                v[i].monitor = next_monitor;
                            }
                        }
                        let ids: Vec<usize> = v
                            .iter()
                            .enumerate()
                            .filter(|(_, w)| w.monitor == m.id && w.in_workspace())
                            .map(|(i, _)| i)
                            .collect();
                        if ids.is_empty() {
                            continue;
                        }
                        let fg = unsafe { GetForegroundWindow() };
                        pending_focus.retain(|_, (h, until)| {
                            *until > Instant::now() && unsafe { IsWindow(*h) != 0 }
                        });
                        if pending_focus.get(&m.id).is_some_and(|(h, _)| *h == fg) {
                            pending_focus.remove(&m.id);
                        }
                        if let Some((h, _)) = pending_focus.get(&m.id) {
                            focus.insert(m.id, *h);
                        } else if ids.iter().any(|i| v[*i].h == fg) {
                            focus.insert(m.id, fg);
                        }
                        let mut f = ids
                            .iter()
                            .position(|i| Some(&v[*i].h) == focus.get(&m.id))
                            .unwrap_or(0);
                        let groups: Vec<usize> = ids.iter().map(|i| v[*i].workspace).collect();
                        if unsafe { MonitorFromWindow(fg, 2) } == m.id {
                            match action.as_deref() {
                                Some("left") => f = layout::neighbor_group(&groups, f, false),
                                Some("right") => f = layout::neighbor_group(&groups, f, true),
                                _ => {}
                            }
                        }
                        let target = v[ids[f]].h;
                        let group = v[ids[f]].workspace;
                        if spotlights.get(m.id, group).is_some_and(|h| h != target) {
                            // An explicit external activation (e.g. Alt+Tab) selects
                            // that member while preserving the group's spotlight mode.
                            spotlights.toggle(m.id, group, target);
                        }
                        let spotlight_index = spotlights
                            .get(m.id, group)
                            .and_then(|h| ids.iter().position(|i| v[*i].h == h));
                        if matches!(action.as_deref(), Some("left" | "right"))
                            && unsafe { MonitorFromWindow(fg, 2) } == m.id
                        {
                            pending_focus
                                .insert(m.id, (target, Instant::now() + Duration::from_secs(1)));
                        }
                        focus.insert(m.id, target);
                        let bar = if c.align_to_bar {
                            top_bar(m, &c.bar_class)
                        } else {
                            None
                        };
                        let area = bar
                            .map(|b| layout::bar_area(m.work, b, c.gap))
                            .unwrap_or(m.work);
                        if area.width() < 160 + c.gap * 2 || area.height() < 100 + c.gap * 2 + c.top
                        {
                            continue;
                        }
                        let mut moves = Vec::new();
                        for (idx, dest) in ids.into_iter().zip(layout::spotlight_layout(
                            area,
                            &groups,
                            f,
                            c.gap,
                            c.width,
                            c.top,
                            spotlight_index,
                        )) {
                            let w = &mut v[idx];
                            if !w.valid()
                                || protected(w.h, m)
                                || (!w.parked && unsafe { IsIconic(w.h) != 0 })
                            {
                                continue;
                            }
                            let desired = match corner_preference {
                                crate::appearance::Corners::Native(value) => value.or(w.corner),
                                crate::appearance::Corners::Percent(0) => Some(1),
                                crate::appearance::Corners::Percent(_) => Some(3),
                            };
                            if let Some(desired) = desired {
                                if w.corner.is_some() && dwm_get_u32(w.h, 33) != Some(desired) {
                                    dwm_set_u32(w.h, 33, desired);
                                }
                            }
                            if !matches!(
                                corner_preference,
                                crate::appearance::Corners::Percent(1..=100)
                            ) && w.clipped
                                && clear_region(w.h)
                            {
                                w.clipped = false;
                                w.clip_state = None;
                            }
                            if let Some(r) = dest {
                                if w.parked {
                                    unsafe { ShowWindowAsync(w.h, 4) };
                                    w.parked = false;
                                }
                                let outer = rect(w.h);
                                let destination = match (outer, visible_rect(w.h)) {
                                    (Some(current), Some(frame)) => {
                                        layout::outer_for_visible(r, current, frame)
                                    }
                                    _ => r,
                                };
                                if outer != Some(destination) {
                                    let start = outer
                                        .filter(|r| {
                                            r.left > -30000
                                                && r.top > -30000
                                                && r.width() > 0
                                                && r.height() > 0
                                        })
                                        .unwrap_or(destination);
                                    moves.push((w.h, start, destination));
                                }
                            } else if !w.parked {
                                w.parked = true;
                                save(&journal, &v)?;
                                unsafe { ShowWindowAsync(v[idx].h, 7) };
                            }
                        }
                        if !moves.is_empty() {
                            let steps = if decoration.animate { 10 } else { 1 };
                            for step in 1..=steps {
                                for (h, start, end) in &moves {
                                    let next = layout::interpolate(
                                        *start,
                                        *end,
                                        step as f64 / steps as f64,
                                    );
                                    if !place(*h, next) {
                                        eprintln!("move failed {} error {}", h, unsafe {
                                            GetLastError()
                                        });
                                    }
                                }
                                if steps > 1 {
                                    std::thread::sleep(Duration::from_millis(14));
                                }
                            }
                        }
                        // Percentage clipping is applied only to visible ordinary windows.
                        // Original custom regions are left untouched; journal intent first.
                        let percent =
                            if let crate::appearance::Corners::Percent(value) = corner_preference {
                                value
                            } else {
                                0
                            };
                        if percent > 0 {
                            let candidates: Vec<usize> = v
                                .iter()
                                .enumerate()
                                .filter(|(_, w)| {
                                    w.monitor == m.id
                                        && w.clip_allowed
                                        && !w.parked
                                        && w.valid()
                                        && (unsafe { IsIconic(w.h) == 0 })
                                        && !protected(w.h, m)
                                })
                                .map(|(i, _)| i)
                                .collect();
                            for idx in candidates {
                                if let Some(r) = rect(v[idx].h) {
                                    let radius =
                                        crate::appearance::radius(percent, r.width(), r.height());
                                    let shape = (r.width(), r.height(), radius);
                                    if radius > 0 && v[idx].clip_state != Some(shape) {
                                        let was_clipped = v[idx].clipped;
                                        v[idx].clipped = true;
                                        save(&journal, &v)?;
                                        if rounded_region(v[idx].h, r.width(), r.height(), radius) {
                                            v[idx].clip_state = Some(shape);
                                        } else {
                                            v[idx].clipped = was_clipped;
                                        }
                                    }
                                }
                            }
                        }
                        if pending_focus.contains_key(&m.id) {
                            // Restoring parked windows is asynchronous. Wait briefly so
                            // a still-minimized target cannot bounce us back next tick.
                            for _ in 0..10 {
                                if unsafe {
                                    IsIconic(target) == 0 && SetForegroundWindow(target) != 0
                                } {
                                    break;
                                }
                                std::thread::sleep(Duration::from_millis(10));
                            }
                        }
                    }
                    save(&journal, &v)?;
                }
                let mut blur_targets = std::collections::BTreeSet::new();
                if !paused && blur_settings.enabled && blur_settings.intensity > 0 {
                    for w in &v {
                        if w.valid()
                            && !w.parked
                            && (unsafe { IsIconic(w.h) == 0 })
                            && ms.iter().any(|m| {
                                m.id == unsafe { MonitorFromWindow(w.h, 2) } && !protected(w.h, m)
                            })
                        {
                            blur_targets.insert(w.h);
                        }
                    }
                    // System backdrop paints the transparent margins of these Qt
                    // windows white. Keep the shell excluded until it has a
                    // renderer that supports a bounded backdrop region.
                    blur_targets.retain(|h| !is_noctalia(*h));
                }
                blur.update(&blur_targets)?;
                mouse.configure(
                    !paused && c.navigation_modifiers == 8,
                    !paused && decoration.spotlight,
                    !paused && decoration.win_drag,
                );
                mouse.update(
                    v.iter()
                        .filter(|w| {
                            !paused
                                && (decoration.spotlight
                                    || decoration.win_drag
                                    || c.navigation_modifiers == 8)
                                && w.in_workspace()
                                && ms.iter().any(|m| m.id == w.monitor && !protected(w.h, m))
                        })
                        .map(|w| (w.h, w.pid)),
                );
                let blur_windows = blur.applied_count();
                let blur_shell_windows = blur.applied_shell_count();
                let visible = v.iter().filter(|w| !w.parked && w.in_workspace()).count();
                let mut numbers: HashMap<Handle, usize> = HashMap::new();
                let windows: Vec<String> = v
                    .iter()
                    .filter(|w| w.in_workspace())
                    .filter_map(|w| {
                        let monitor = ms.iter().find(|m| m.id == w.monitor)?;
                        let number = numbers.entry(w.monitor).or_default();
                        *number += 1;
                        let group_keys: std::collections::BTreeSet<usize> = v.iter().filter(|entry| entry.monitor == w.monitor && entry.in_workspace()).map(|entry| entry.workspace).collect();
                        let workspace = group_keys.iter().position(|key| *key == w.workspace).unwrap_or(0) + 1;
                        Some(format!(
                            "{{\"id\":\"{}\",\"monitor\":{},\"monitor_x\":{},\"monitor_y\":{},\"number\":{},\"workspace\":{},\"spotlight\":{},\"border_color\":{},\"active\":{}}}",
                            w.h,
                            json_string(&monitor.name),
                            monitor.bounds.left,
                            monitor.bounds.top,
                            number,
                            workspace,
                            spotlights.get(w.monitor,w.workspace)==Some(w.h),
                            w.border_applied.map(|c|c.to_string()).unwrap_or_else(||"null".into()),
                            focus.get(&w.monitor) == Some(&w.h)
                        ))
                    })
                    .collect();
                let updated = std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .unwrap_or_default()
                    .as_millis();
                let _=atomic_write(&state.join("status.json"),&format!("{{\"schema_version\":1,\"running\":true,\"updated_at_ms\":{updated},\"paused\":{paused},\"taskbar_hidden\":{},\"blur_enabled\":{},\"blur_windows\":{blur_windows},\"blur_shell_windows\":{blur_shell_windows},\"managed\":{},\"visible\":{visible},\"monitors\":{},\"windows\":[{}]}}\n",taskbar.active(),blur_settings.enabled&&blur_settings.intensity>0,v.len(),ms.len(),windows.join(",")));
                std::thread::sleep(Duration::from_millis(c.poll));
            }
            Ok(())
        },
    ));
    mouse.update([]);
    let taskbar_error = taskbar.restore().err();
    let blur_error = blur.restore().err();
    let restored = restore(&v);
    if restored {
        let _ = fs::remove_file(&journal);
    }
    unsafe { SetConsoleCtrlHandler(Some(ctrl), 0) };
    let _ = atomic_write(
        &state.join("status.json"),
        "{\"schema_version\":1,\"running\":false}\n",
    );
    match result {
        Ok(r) => {
            r?;
            if let Some(error) = taskbar_error {
                return Err(error);
            }
            if let Some(error) = blur_error {
                return Err(error);
            }
            if !restored {
                return Err("restore failed: journal retained".into());
            }
            Ok(())
        }
        Err(_) => Err("panic recovered; inspect recovery journal if present".into()),
    }
}
fn window_title_length(h: Handle) -> i32 {
    unsafe { GetWindowTextLengthW(h) }
}
#[cfg(test)]
mod workspace_tests {
    use super::workspace_member;
    #[test]
    fn border_recovery_roundtrip_and_old_journals_remain_readable() {
        let path = std::env::temp_dir().join(format!(
            "niri-border-journal-test-{}.tsv",
            std::process::id()
        ));
        let mut text =
            String::from("NIRIWIN1\n1\t1\t10\t10\t800\t600\t0\tTestClass\t3\t1\t1\t4294967295\n");
        std::fs::write(&path, &text).unwrap();
        let loaded = super::load(&path).unwrap();
        assert_eq!(loaded[0].border, Some(u32::MAX));
        assert!(loaded[0].clipped);
        super::save(&path, &loaded).unwrap();
        assert_eq!(super::load(&path).unwrap()[0].border, Some(u32::MAX));
        text = text.replace("\t4294967295\n", "\n");
        std::fs::write(&path, text).unwrap();
        let old = super::load(&path).unwrap();
        assert_eq!(old[0].border, None);
        assert!(old[0].clipped);
        std::fs::remove_file(path).unwrap();
    }

    #[test]
    fn minimized_windows_keep_membership_but_tray_windows_do_not() {
        assert!(!workspace_member(false, false, false, false));
        assert!(workspace_member(false, true, true, false));
        assert!(workspace_member(false, true, false, false));
    }

    #[test]
    fn manager_parked_windows_stay_reachable_but_other_desktops_are_excluded() {
        assert!(workspace_member(true, true, true, false));
        assert!(workspace_member(true, false, true, false));
        assert!(!workspace_member(true, true, true, true));
    }
}
fn json_string(value: &str) -> String {
    let mut result = String::from("\"");
    for ch in value.chars() {
        match ch {
            '\\' => result.push_str("\\\\"),
            '"' => result.push_str("\\\""),
            c if c.is_control() => result.push_str(&format!("\\u{:04x}", c as u32)),
            c => result.push(c),
        }
    }
    result.push('"');
    result
}
