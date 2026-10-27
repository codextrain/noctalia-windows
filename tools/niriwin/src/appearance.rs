// Rust · appearance.rs · percentage corner control · Windows 11 · stable Rust
use std::{fs, path::Path};
#[derive(Clone, Copy)]
pub struct Decorations {
    pub win_drag: bool,
    pub hide_taskbar: bool,
    pub spotlight: bool,
    pub borders: bool,
    pub active: u32,
    pub inactive: u32,
    pub animate: bool,
}
pub fn colorref(text: &str) -> Option<u32> {
    let digits = text.strip_prefix('#')?;
    if digits.len() != 6 {
        return None;
    }
    let rgb = u32::from_str_radix(digits, 16).ok()?;
    Some(((rgb & 255) << 16) | (rgb & 0xff00) | ((rgb >> 16) & 255))
}
pub fn decorations(path: &Path) -> Result<Decorations, String> {
    let root: serde_json::Value =
        serde_json::from_str(&fs::read_to_string(path).map_err(|e| e.to_string())?)
            .map_err(|e| e.to_string())?;
    let palette = root
        .pointer("/windows/border_palette_json")
        .and_then(|v| v.as_str())
        .and_then(|v| serde_json::from_str::<serde_json::Value>(v).ok())
        .unwrap_or_default();
    Ok(Decorations {
        win_drag: root
            .pointer("/windows/win_drag_enabled")
            .and_then(|v| v.as_bool())
            .unwrap_or(true),
        hide_taskbar: root
            .pointer("/windows/hide_taskbar_permanently")
            .and_then(|v| v.as_bool())
            .unwrap_or(false),
        spotlight: root
            .pointer("/windows/alt_click_spotlight_enabled")
            .and_then(|v| v.as_bool())
            .unwrap_or(true),
        borders: root
            .pointer("/windows/border_enabled")
            .and_then(|v| v.as_bool())
            .unwrap_or(true),
        active: palette
            .get("active")
            .and_then(|v| v.as_str())
            .and_then(colorref)
            .unwrap_or(0xe49767),
        inactive: palette
            .get("inactive")
            .and_then(|v| v.as_str())
            .and_then(colorref)
            .unwrap_or(0x55504b),
        animate: root
            .pointer("/windows/layout_animation_enabled")
            .and_then(|v| v.as_bool())
            .unwrap_or(true)
            && root
                .pointer("/shell/animation_enabled")
                .and_then(|v| v.as_bool())
                .unwrap_or(true),
    })
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Corners {
    Native(Option<u32>),
    Percent(u8),
}
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct BlurSettings {
    pub enabled: bool,
    pub intensity: u8,
}
pub fn blur_parse(text: &str) -> Result<BlurSettings, String> {
    let root: serde_json::Value = serde_json::from_str(text).map_err(|e| e.to_string())?;
    if !root.is_object() {
        return Err("Noctalia configuration must be an object".into());
    }
    let enabled = match root.pointer("/windows/blur_enabled") {
        None => false,
        Some(v) => v.as_bool().ok_or("invalid blur_enabled")?,
    };
    let intensity = match root.pointer("/windows/blur_intensity_percent") {
        None => 50,
        Some(v) => v
            .as_u64()
            .filter(|v| *v <= 100)
            .ok_or("invalid blur intensity")? as u8,
    };
    Ok(BlurSettings { enabled, intensity })
}
pub fn blur_load(path: &Path) -> Result<BlurSettings, String> {
    match fs::read_to_string(path) {
        Ok(text) => blur_parse(&text),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(BlurSettings {
            enabled: false,
            intensity: 50,
        }),
        Err(e) => Err(e.to_string()),
    }
}
pub fn parse(text: &str, fallback: Option<u32>) -> Result<Corners, String> {
    let root: serde_json::Value = serde_json::from_str(text).map_err(|e| e.to_string())?;
    if !root.is_object() {
        return Err("Noctalia configuration must be an object".into());
    }
    if let Some(value) = root.pointer("/windows/corner_rounding_percent") {
        return value
            .as_u64()
            .filter(|v| *v <= 100)
            .map(|v| Corners::Percent(v as u8))
            .ok_or_else(|| "invalid corner percentage".into());
    }
    match root.pointer("/windows/corner_preference") {
        None => Ok(Corners::Native(fallback)),
        Some(value) => value
            .as_u64()
            .filter(|v| *v <= 3)
            .map(|v| Corners::Native(Some(v as u32)))
            .ok_or_else(|| "invalid corner_preference".into()),
    }
}
pub fn load(path: &Path, fallback: Option<u32>) -> Result<Corners, String> {
    match fs::read_to_string(path) {
        Ok(text) => parse(&text, fallback),
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => Ok(Corners::Native(fallback)),
        Err(e) => Err(e.to_string()),
    }
}
/// Percentage of a 32-physical-pixel radius, clamped for small windows.
pub fn radius(percent: u8, width: i32, height: i32) -> i32 {
    if percent == 0 {
        return 0;
    }
    ((percent.min(100) as i32 * 32 + 50) / 100)
        .max(1)
        .min(width.min(height).max(0) / 2)
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn theme_colors_use_windows_bgr_and_reject_invalid_values() {
        assert_eq!(colorref("#123456"), Some(0x563412));
        assert_eq!(colorref("#FFFFFF"), Some(0xffffff));
        for value in ["red", "#abc", "#gg7788", "#ff123456", ""] {
            assert_eq!(colorref(value), None);
        }
    }
    #[test]
    fn blur_settings_validated() {
        assert_eq!(
            blur_parse("{}").unwrap(),
            BlurSettings {
                enabled: false,
                intensity: 50
            }
        );
        for percent in [0, 25, 100] {
            assert_eq!(blur_parse(&format!("{{\"windows\":{{\"blur_enabled\":true,\"blur_intensity_percent\":{percent}}}}}")).unwrap(),BlurSettings{enabled:true,intensity:percent});
        }
        for json in [
            "{",
            "[]",
            "{\"windows\":{\"blur_enabled\":1}}",
            "{\"windows\":{\"blur_intensity_percent\":101}}",
        ] {
            assert!(blur_parse(json).is_err());
        }
    }
    #[test]
    fn percentage_and_fallback() {
        for p in [0, 1, 25, 50, 100] {
            assert_eq!(
                parse(
                    &format!("{{\"windows\":{{\"corner_rounding_percent\":{p}}}}}"),
                    Some(3)
                )
                .unwrap(),
                Corners::Percent(p)
            );
        }
        assert_eq!(parse("{}", None).unwrap(), Corners::Native(None));
    }
    #[test]
    fn bad_values() {
        for text in [
            "{",
            "[]",
            "{\"windows\":{\"corner_rounding_percent\":101}}",
            "{\"windows\":{\"corner_rounding_percent\":null}}",
            "{\"windows\":{\"corner_rounding_percent\":-1}}",
            "{\"windows\":{\"corner_rounding_percent\":1.5}}",
        ] {
            assert!(parse(text, Some(3)).is_err());
        }
    }
    #[test]
    fn actual_radii() {
        assert_eq!(radius(0, 1000, 800), 0);
        assert_eq!(radius(25, 1000, 800), 8);
        assert_eq!(radius(50, 1000, 800), 16);
        assert_eq!(radius(100, 1000, 800), 32);
        assert_eq!(radius(100, 20, 10), 5);
    }
}
