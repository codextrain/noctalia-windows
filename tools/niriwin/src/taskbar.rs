// Reversible taskbar hiding. Auto-hide is explicitly OFF: hovering cannot reveal it.
use crate::{layout::Rect, win::*};
use serde_json::{json, Value};
use std::{
    fs,
    path::{Path, PathBuf},
};
#[repr(C)]
struct AppBarData {
    size: u32,
    window: Handle,
    callback: u32,
    edge: u32,
    rect: R,
    param: isize,
}
#[link(name = "shell32")]
extern "system" {
    fn SHAppBarMessage(message: u32, data: *mut AppBarData) -> usize;
}
#[link(name = "user32")]
extern "system" {
    fn FindWindowW(class: *const u16, title: *const u16) -> Handle;
}
fn primary() -> Handle {
    unsafe { FindWindowW(wide("Shell_TrayWnd").as_ptr(), std::ptr::null()) }
}
fn state() -> u32 {
    let mut data = AppBarData {
        size: std::mem::size_of::<AppBarData>() as u32,
        window: primary(),
        callback: 0,
        edge: 0,
        rect: R::default(),
        param: 0,
    };
    unsafe { SHAppBarMessage(4, &mut data) as u32 & 3 }
}
fn set_state(value: u32) {
    let mut data = AppBarData {
        size: std::mem::size_of::<AppBarData>() as u32,
        window: primary(),
        callback: 0,
        edge: 0,
        rect: R::default(),
        param: value as isize,
    };
    unsafe { SHAppBarMessage(10, &mut data) };
}
fn is_tray(h: Handle) -> bool {
    matches!(
        class(h).as_str(),
        "Shell_TrayWnd" | "Shell_SecondaryTrayWnd"
    )
}
fn edge(bounds: Rect, r: Rect) -> Option<&'static str> {
    if r.width() >= bounds.width() / 2 && r.height() > 0 && r.height() <= 200 {
        if (r.bottom - bounds.bottom).abs() <= 4 || (r.top - bounds.bottom).abs() <= r.height() + 4
        {
            return Some("bottom");
        }
        if (r.top - bounds.top).abs() <= 4 || (r.bottom - bounds.top).abs() <= r.height() + 4 {
            return Some("top");
        }
    } else if r.height() >= bounds.height() / 2 && r.width() > 0 && r.width() <= 200 {
        if (r.left - bounds.left).abs() <= 4 || (r.right - bounds.left).abs() <= r.width() + 4 {
            return Some("left");
        }
        if (r.right - bounds.right).abs() <= 4 || (r.left - bounds.right).abs() <= r.width() + 4 {
            return Some("right");
        }
    }
    None
}
fn reclaim(mut work: Rect, bounds: Rect, edge: &str, bar: Option<Rect>) -> Rect {
    match edge {
        "bottom" => work.bottom = bounds.bottom,
        "top" => work.top = bounds.top,
        "left" => work.left = bounds.left,
        "right" => work.right = bounds.right,
        _ => {}
    }
    if let Some(bar) = bar {
        work.top = work.top.max(bar.bottom);
    }
    work
}
fn rect_json(r: Rect) -> Value {
    json!([r.left, r.top, r.right, r.bottom])
}
fn parse_rect(value: &Value) -> Result<Rect, String> {
    let a = value
        .as_array()
        .filter(|v| v.len() == 4)
        .ok_or("invalid taskbar rectangle")?;
    let mut n = [0; 4];
    for (i, v) in a.iter().enumerate() {
        n[i] = v
            .as_i64()
            .and_then(|n| i32::try_from(n).ok())
            .filter(|n| n.unsigned_abs() <= 1000000)
            .ok_or("invalid taskbar coordinate")?;
    }
    let r = Rect {
        left: n[0],
        top: n[1],
        right: n[2],
        bottom: n[3],
    };
    if r.width() <= 0 || r.height() <= 0 || r.width() > 100000 || r.height() > 100000 {
        return Err("invalid taskbar dimensions".into());
    }
    Ok(r)
}
#[derive(Clone)]
struct MonitorOriginal {
    name: String,
    bounds: Rect,
    work: Rect,
}
#[derive(Clone)]
struct TrayOriginal {
    class: String,
    monitor: String,
    visible: bool,
    edge: String,
}
#[derive(Clone)]
struct Original {
    state: u32,
    monitors: Vec<MonitorOriginal>,
    trays: Vec<TrayOriginal>,
}
impl Original {
    fn capture() -> Self {
        let mut original = Self {
            state: state(),
            monitors: vec![],
            trays: vec![],
        };
        original.discover();
        original
    }
    fn discover(&mut self) -> bool {
        let mut changed = false;
        let ms = monitors();
        for m in &ms {
            if !self.monitors.iter().any(|o| o.name == m.name) {
                self.monitors.push(MonitorOriginal {
                    name: m.name.clone(),
                    bounds: m.bounds,
                    work: m.work,
                });
                changed = true;
            }
        }
        for h in windows().into_iter().filter(|h| is_tray(*h)) {
            let Some(m) = ms
                .iter()
                .find(|m| m.id == unsafe { MonitorFromWindow(h, 2) })
            else {
                continue;
            };
            let class = class(h);
            if !self
                .trays
                .iter()
                .any(|o| o.class == class && o.monitor == m.name)
            {
                let Some(edge) = rect(h).and_then(|r| edge(m.bounds, r)) else {
                    continue;
                };
                self.trays.push(TrayOriginal {
                    class,
                    monitor: m.name.clone(),
                    visible: unsafe { IsWindowVisible(h) != 0 },
                    edge: edge.into(),
                });
                changed = true;
            }
        }
        changed
    }
    fn value(&self) -> Value {
        json!({"version":1,"state":self.state,
        "monitors":self.monitors.iter().map(|m|json!({"name":m.name,"bounds":rect_json(m.bounds),"work":rect_json(m.work)})).collect::<Vec<_>>(),
        "trays":self.trays.iter().map(|t|json!({"class":t.class,"monitor":t.monitor,"visible":t.visible,"edge":t.edge})).collect::<Vec<_>>()})
    }
    fn parse(value: Value) -> Result<Self, String> {
        if value.get("version").and_then(Value::as_u64) != Some(1) {
            return Err("invalid taskbar recovery version".into());
        }
        let state = value
            .get("state")
            .and_then(Value::as_u64)
            .filter(|s| *s <= 3)
            .ok_or("invalid taskbar state")? as u32;
        let records = value
            .get("monitors")
            .and_then(Value::as_array)
            .filter(|m| m.len() <= 64)
            .ok_or("invalid monitor records")?;
        let mut ms = Vec::new();
        for m in records {
            let name = m
                .get("name")
                .and_then(Value::as_str)
                .filter(|n| !n.is_empty() && n.len() <= 128)
                .ok_or("invalid monitor name")?
                .to_owned();
            let bounds = parse_rect(&m["bounds"])?;
            let work = parse_rect(&m["work"])?;
            if work.left < bounds.left
                || work.right > bounds.right
                || work.top < bounds.top
                || work.bottom > bounds.bottom
            {
                return Err("work area outside saved monitor".into());
            }
            ms.push(MonitorOriginal { name, bounds, work });
        }
        let records = value
            .get("trays")
            .and_then(Value::as_array)
            .filter(|t| t.len() <= 128)
            .ok_or("invalid taskbar records")?;
        let mut trays = Vec::new();
        for t in records {
            let class = t
                .get("class")
                .and_then(Value::as_str)
                .filter(|c| matches!(*c, "Shell_TrayWnd" | "Shell_SecondaryTrayWnd"))
                .ok_or("invalid tray class")?
                .to_owned();
            let monitor = t
                .get("monitor")
                .and_then(Value::as_str)
                .filter(|n| ms.iter().any(|m| m.name == *n))
                .ok_or("unknown saved monitor")?
                .to_owned();
            let edge = t
                .get("edge")
                .and_then(Value::as_str)
                .filter(|e| matches!(*e, "top" | "bottom" | "left" | "right"))
                .ok_or("invalid taskbar edge")?
                .to_owned();
            let visible = t
                .get("visible")
                .and_then(Value::as_bool)
                .ok_or("invalid taskbar visibility")?;
            trays.push(TrayOriginal {
                class,
                monitor,
                visible,
                edge,
            });
        }
        Ok(Self {
            state,
            monitors: ms,
            trays,
        })
    }
}
pub struct Taskbar {
    path: PathBuf,
    original: Option<Original>,
}
impl Taskbar {
    pub fn active(&self) -> bool {
        self.original.is_some()
    }
    pub fn new(path: PathBuf) -> Self {
        Self {
            path,
            original: None,
        }
    }
    fn save(&self) -> Result<(), String> {
        let Some(original) = &self.original else {
            return Ok(());
        };
        let tmp = self.path.with_extension("tmp");
        fs::write(
            &tmp,
            serde_json::to_vec(&original.value()).map_err(|e| e.to_string())?,
        )
        .map_err(|e| e.to_string())?;
        fs::rename(tmp, &self.path).map_err(|e| e.to_string())
    }
    pub fn update(&mut self, enabled: bool) -> Result<(), String> {
        // If the replacement shell is gone, give the Windows shell back.
        let replacement = enabled
            && windows()
                .into_iter()
                .any(|h| class(h).starts_with("Qt") && is_noctalia(h));
        if !replacement {
            return self.restore();
        }
        if self.original.is_none() {
            self.original = Some(Original::capture());
            self.save()?;
        }
        if self.original.as_mut().is_some_and(Original::discover) {
            self.save()?;
        }
        if state() & 1 != 0 {
            set_state(state() & !1);
        }
        for h in windows().into_iter().filter(|h| is_tray(*h)) {
            if unsafe { IsWindowVisible(h) != 0 } {
                unsafe { ShowWindowAsync(h, 0) };
            }
        }
        Ok(())
    }
    // Give managed windows the freed strip without fighting Explorer's global
    // app-bar work-area calculation or changing unrelated applications' settings.
    pub fn expand_monitors(&self, ms: &mut [Monitor], bar_class: &str) {
        let Some(original) = &self.original else {
            return;
        };
        for m in ms {
            let bar = top_bar(m, bar_class);
            for t in original.trays.iter().filter(|t| t.monitor == m.name) {
                m.work = reclaim(m.work, m.bounds, &t.edge, bar);
            }
        }
    }
    pub fn restore(&mut self) -> Result<(), String> {
        let Some(original) = &self.original else {
            return Ok(());
        };
        set_state(original.state);
        let ms = monitors();
        let mut expected_visible = Vec::new();
        for h in windows().into_iter().filter(|h| is_tray(*h)) {
            let monitor = ms
                .iter()
                .find(|m| m.id == unsafe { MonitorFromWindow(h, 2) })
                .map(|m| m.name.as_str());
            let visible = original
                .trays
                .iter()
                .find(|t| Some(t.monitor.as_str()) == monitor && t.class == class(h))
                .is_none_or(|t| t.visible);
            unsafe { ShowWindowAsync(h, if visible { 8 } else { 0 }) };
            if visible && original.state & 1 == 0 {
                expected_visible.push(h);
            }
        }
        // Wait for Explorer's asynchronous show request before clearing recovery
        // state, so an immediate restart cannot snapshot our own hidden window.
        for _ in 0..20 {
            if expected_visible
                .iter()
                .all(|h| unsafe { IsWindow(*h) == 0 || IsWindowVisible(*h) != 0 })
            {
                break;
            }
            std::thread::sleep(std::time::Duration::from_millis(10));
        }
        if expected_visible
            .iter()
            .any(|h| unsafe { IsWindow(*h) != 0 && IsWindowVisible(*h) == 0 })
        {
            return Err("Explorer did not restore the taskbar; recovery journal retained".into());
        }
        if self.path.exists() {
            fs::remove_file(&self.path).map_err(|e| e.to_string())?;
        }
        self.original = None;
        Ok(())
    }
}
impl Drop for Taskbar {
    fn drop(&mut self) {
        let _ = self.restore();
    }
}
pub fn recover(path: &Path) -> Result<(), String> {
    if !path.exists() {
        return Ok(());
    }
    let value = serde_json::from_str(&fs::read_to_string(path).map_err(|e| e.to_string())?)
        .map_err(|e| e.to_string())?;
    let mut taskbar = Taskbar::new(path.into());
    taskbar.original = Some(Original::parse(value)?);
    taskbar.restore()
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn reclaim_bottom_preserves_top_bar_and_other_edges() {
        let bounds = Rect {
            left: -1920,
            top: 188,
            right: 0,
            bottom: 1268,
        };
        let work = Rect {
            left: -1920,
            top: 224,
            right: 0,
            bottom: 1220,
        };
        let bar = Rect {
            left: -1920,
            top: 188,
            right: 0,
            bottom: 224,
        };
        let expanded = reclaim(work, bounds, "bottom", Some(bar));
        assert_eq!(expanded.top, 224);
        assert_eq!(expanded.bottom, 1268);
        assert_eq!(expanded.left, -1920);
        assert_eq!(
            edge(
                bounds,
                Rect {
                    top: 1220,
                    ..bounds
                }
            ),
            Some("bottom")
        );
    }
    #[test]
    fn journal_validates_and_roundtrips_without_touching_system() {
        let original = Original {
            state: 2,
            monitors: vec![MonitorOriginal {
                name: "DISPLAY1".into(),
                bounds: Rect {
                    left: 0,
                    top: 0,
                    right: 1920,
                    bottom: 1080,
                },
                work: Rect {
                    left: 0,
                    top: 36,
                    right: 1920,
                    bottom: 1032,
                },
            }],
            trays: vec![TrayOriginal {
                class: "Shell_TrayWnd".into(),
                monitor: "DISPLAY1".into(),
                visible: true,
                edge: "bottom".into(),
            }],
        };
        assert_eq!(
            Original::parse(original.value()).unwrap().monitors[0]
                .work
                .bottom,
            1032
        );
        let mut invalid = original.value();
        invalid["trays"][0]["class"] = json!("other");
        assert!(Original::parse(invalid).is_err());
        assert!(parse_rect(&json!([0, 0, -1, 10])).is_err());
    }
}
