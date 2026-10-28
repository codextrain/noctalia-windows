// Rust · transparency.rs · reversible opacity · Windows 11 · stable Rust
use crate::win::*;
use std::{
    collections::{BTreeMap, BTreeSet},
    fs,
    path::PathBuf,
};
#[link(name = "user32")]
extern "system" {
    fn SetWindowLongW(h: Handle, index: i32, value: i32) -> i32;
    fn GetLayeredWindowAttributes(h: Handle, key: *mut u32, alpha: *mut u8, flags: *mut u32)
        -> i32;
    fn SetLayeredWindowAttributes(h: Handle, key: u32, alpha: u8, flags: u32) -> i32;
}

#[cfg(test)]
static APPLY_COUNT: std::sync::atomic::AtomicUsize = std::sync::atomic::AtomicUsize::new(0);

struct Original {
    pid: u32,
    class: String,
    style: i32,
}
pub struct Transparency {
    path: PathBuf,
    originals: BTreeMap<Handle, Original>,
}
impl Transparency {
    pub fn new(path: PathBuf) -> Self {
        Self {
            path,
            originals: BTreeMap::new(),
        }
    }
    fn save(&self) -> Result<(), String> {
        if self.originals.is_empty() {
            if self.path.exists() {
                fs::remove_file(&self.path).map_err(|e| e.to_string())?;
            }
            return Ok(());
        }
        let items: Vec<_> = self
            .originals
            .iter()
            .map(|(h, o)| serde_json::json!({"hwnd":h,"pid":o.pid,"class":o.class,"style":o.style}))
            .collect();
        let tmp = self.path.with_extension("tmp");
        fs::write(
            &tmp,
            serde_json::to_vec(&serde_json::json!({"version":1,"windows":items})).unwrap(),
        )
        .map_err(|e| e.to_string())?;
        fs::rename(tmp, &self.path).map_err(|e| e.to_string())
    }
    pub fn update(&mut self, targets: &BTreeSet<Handle>, percent: u8) -> Result<(), String> {
        let old: Vec<_> = self.originals.keys().copied().collect();
        for h in old {
            if percent == 0 || !targets.contains(&h) {
                let o = &self.originals[&h];
                if unsafe { IsWindow(h) } != 0 && pid(h) == o.pid && class(h) == o.class {
                    unsafe {
                        SetLayeredWindowAttributes(h, 0, 255, 2);
                        let current = GetWindowLongW(h, -20);
                        SetWindowLongW(h, -20, current & !0x80000);
                    }
                    if unsafe { GetWindowLongW(h, -20) } & 0x80000 != 0 {
                        continue;
                    }
                }
                self.originals.remove(&h);
            }
        }
        self.save()?;
        if percent == 0 {
            return Ok(());
        }
        for &h in targets {
            if let std::collections::btree_map::Entry::Vacant(e) = self.originals.entry(h) {
                let style = unsafe { GetWindowLongW(h, -20) };
                // Keep app-owned layered windows, including their own alpha/color keys, untouched.
                if style & 0x80000 != 0 {
                    continue;
                }
                e.insert(Original {
                    pid: pid(h),
                    class: class(h),
                    style,
                });
                self.save()?;
            }
            let o = &self.originals[&h];
            if pid(h) == o.pid && class(h) == o.class {
                unsafe {
                    let current = GetWindowLongW(h, -20);
                    let desired = ((100 - percent.min(30)) as u16 * 255 / 100) as u8;
                    let (mut key, mut alpha, mut flags) = (0, 0, 0);
                    let unchanged = current & 0x80000 != 0
                        && GetLayeredWindowAttributes(h, &mut key, &mut alpha, &mut flags) != 0
                        && flags == 2
                        && alpha == desired;
                    if !unchanged {
                        if current & 0x80000 == 0 {
                            SetWindowLongW(h, -20, current | 0x80000);
                        }
                        #[cfg(test)]
                        APPLY_COUNT.fetch_add(1, std::sync::atomic::Ordering::SeqCst);
                        SetLayeredWindowAttributes(h, 0, desired, 2);
                    }
                }
            }
        }
        Ok(())
    }
    pub fn restore(&mut self) -> Result<(), String> {
        self.update(&BTreeSet::new(), 0)?;
        if self.originals.is_empty() {
            Ok(())
        } else {
            Err("opacity recovery retained".into())
        }
    }
    pub fn recover(path: PathBuf) -> Result<(), String> {
        if !path.exists() {
            return Ok(());
        }
        let root: serde_json::Value =
            serde_json::from_str(&fs::read_to_string(&path).map_err(|e| e.to_string())?)
                .map_err(|e| e.to_string())?;
        if root["version"] != 1 {
            return Err("invalid opacity journal".into());
        }
        let mut effect = Self::new(path);
        for w in root["windows"]
            .as_array()
            .ok_or("invalid opacity records")?
        {
            let h = w["hwnd"]
                .as_i64()
                .and_then(|h| isize::try_from(h).ok())
                .filter(|h| *h > 0)
                .ok_or("invalid opacity handle")?;
            let pid = w["pid"]
                .as_u64()
                .and_then(|v| u32::try_from(v).ok())
                .ok_or("invalid opacity pid")?;
            let style = w["style"]
                .as_i64()
                .and_then(|v| i32::try_from(v).ok())
                .filter(|v| v & 0x80000 == 0)
                .ok_or("invalid opacity style")?;
            effect.originals.insert(
                h,
                Original {
                    pid,
                    class: w["class"].as_str().ok_or("invalid opacity class")?.into(),
                    style,
                },
            );
        }
        effect.restore()
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[link(name = "user32")]
    extern "system" {
        fn CreateWindowExW(
            ex: u32,
            class: *const u16,
            title: *const u16,
            style: u32,
            x: i32,
            y: i32,
            w: i32,
            h: i32,
            parent: Handle,
            menu: Handle,
            instance: Handle,
            param: *const std::ffi::c_void,
        ) -> Handle;
        fn DestroyWindow(h: Handle) -> i32;
        fn GetLayeredWindowAttributes(
            h: Handle,
            key: *mut u32,
            alpha: *mut u8,
            flags: *mut u32,
        ) -> i32;
    }
    #[test]
    fn native_opacity_restores_recovers_and_preserves_app_owned_layers() {
        unsafe {
            let cls: Vec<u16> = "STATIC\0".encode_utf16().collect();
            let h = CreateWindowExW(
                0,
                cls.as_ptr(),
                cls.as_ptr(),
                0x00cf0000,
                0,
                0,
                320,
                200,
                0,
                0,
                0,
                std::ptr::null_mut(),
            );
            assert_ne!(h, 0);
            let path =
                std::env::temp_dir().join(format!("niri-opacity-test-{}.json", std::process::id()));
            let mut fx = Transparency::new(path.clone());
            let targets = [h].into_iter().collect();
            fx.update(&targets, 8).unwrap();
            let (mut key, mut alpha, mut flags) = (0, 0, 0);
            assert_ne!(
                GetLayeredWindowAttributes(h, &mut key, &mut alpha, &mut flags),
                0
            );
            assert_eq!(alpha, 234);
            let applied = APPLY_COUNT.load(std::sync::atomic::Ordering::SeqCst);
            for _ in 0..200 {
                fx.update(&targets, 8).unwrap();
            }
            assert_eq!(
                APPLY_COUNT.load(std::sync::atomic::Ordering::SeqCst),
                applied,
                "unchanged polling must not reapply style/alpha"
            );
            fx.update(&targets, 9).unwrap();
            assert_eq!(
                APPLY_COUNT.load(std::sync::atomic::Ordering::SeqCst),
                applied + 1
            );

            assert!(path.exists());
            fx.restore().unwrap();
            assert_eq!(GetWindowLongW(h, -20) & 0x80000, 0);
            assert!(!path.exists());
            fx.update(&targets, 12).unwrap();
            drop(fx);
            Transparency::recover(path.clone()).unwrap();
            assert_eq!(GetWindowLongW(h, -20) & 0x80000, 0);
            assert!(!path.exists());
            SetWindowLongW(h, -20, GetWindowLongW(h, -20) | 0x80000);
            SetLayeredWindowAttributes(h, 0, 199, 2);
            let mut fx = Transparency::new(path);
            fx.update(&targets, 8).unwrap();
            fx.restore().unwrap();
            assert_ne!(
                GetLayeredWindowAttributes(h, &mut key, &mut alpha, &mut flags),
                0
            );
            assert_eq!(alpha, 199);
            assert_ne!(GetWindowLongW(h, -20) & 0x80000, 0);
            DestroyWindow(h);
        }
    }
}
