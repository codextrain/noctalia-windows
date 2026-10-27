// Rust · effects.rs · reversible system acrylic · Windows 11 · stable Rust
use crate::win::*;
use std::{
    collections::{BTreeMap, BTreeSet},
    fs,
    path::{Path, PathBuf},
};
#[derive(Clone)]
struct Original {
    pid: u32,
    class: String,
    backdrop: u32,
}
impl Original {
    fn valid(&self, h: Handle) -> bool {
        (unsafe { IsWindow(h) != 0 }) && pid(h) == self.pid && class(h) == self.class
    }
}
pub struct Blur {
    path: PathBuf,
    originals: BTreeMap<Handle, Original>,
}
impl Blur {
    pub fn new(path: PathBuf) -> Self {
        Self {
            path,
            originals: BTreeMap::new(),
        }
    }
    pub fn applied_count(&self) -> usize {
        self.originals
            .iter()
            .filter(|(h, o)| o.valid(**h) && dwm_get_u32(**h, 38) == Some(3))
            .count()
    }
    pub fn applied_shell_count(&self) -> usize {
        self.originals
            .iter()
            .filter(|(h, o)| {
                o.valid(**h)
                    && o.class.starts_with("Qt")
                    && is_noctalia(**h)
                    && dwm_get_u32(**h, 38) == Some(3)
            })
            .count()
    }
    fn save(&self) -> Result<(), String> {
        if self.originals.is_empty() {
            if self.path.exists() {
                fs::remove_file(&self.path).map_err(|e| e.to_string())?;
            }
            return Ok(());
        }
        let windows:Vec<_>=self.originals.iter().map(|(h,o)|serde_json::json!({"hwnd":h,"pid":o.pid,"class":o.class,"backdrop":o.backdrop})).collect();
        let bytes = serde_json::to_vec(&serde_json::json!({"version":1,"windows":windows}))
            .map_err(|e| e.to_string())?;
        let temporary = self.path.with_extension("tmp");
        fs::write(&temporary, bytes).map_err(|e| e.to_string())?;
        fs::rename(temporary, &self.path).map_err(|e| e.to_string())
    }
    pub fn update(&mut self, targets: &BTreeSet<Handle>) -> Result<(), String> {
        let old: Vec<_> = self.originals.keys().copied().collect();
        for h in old {
            if !targets.contains(&h) {
                let original = &self.originals[&h];
                if !original.valid(h)
                    || dwm_get_u32(h, 38) == Some(original.backdrop)
                    || dwm_set_u32(h, 38, original.backdrop)
                {
                    self.originals.remove(&h);
                }
            }
        }
        self.save()?;
        for h in targets {
            if !self.originals.contains_key(h) {
                if let Some(backdrop) = dwm_get_u32(*h, 38).filter(|v| *v <= 4) {
                    self.originals.insert(
                        *h,
                        Original {
                            pid: pid(*h),
                            class: class(*h),
                            backdrop,
                        },
                    );
                    self.save()?;
                } else {
                    continue;
                }
            }
            if self.originals[h].valid(*h) && dwm_get_u32(*h, 38) != Some(3) {
                dwm_set_u32(*h, 38, 3);
            }
        }
        Ok(())
    }
    pub fn restore(&mut self) -> Result<(), String> {
        self.update(&BTreeSet::new())?;
        if self.originals.is_empty() {
            Ok(())
        } else {
            Err("some backdrop restores failed; blur-recovery.json retained".into())
        }
    }
}
pub fn recover(path: &Path) -> Result<(), String> {
    if !path.exists() {
        return Ok(());
    }
    let value: serde_json::Value =
        serde_json::from_str(&fs::read_to_string(path).map_err(|e| e.to_string())?)
            .map_err(|e| e.to_string())?;
    if value.get("version").and_then(|v| v.as_u64()) != Some(1) {
        return Err("invalid blur recovery version".into());
    }
    let windows = value
        .get("windows")
        .and_then(|v| v.as_array())
        .ok_or("invalid blur recovery records")?;
    let mut blur = Blur::new(path.into());
    for w in windows {
        let h = w
            .get("hwnd")
            .and_then(|v| v.as_i64())
            .and_then(|v| isize::try_from(v).ok())
            .filter(|v| *v > 0)
            .ok_or("invalid blur hwnd")?;
        let pid = w
            .get("pid")
            .and_then(|v| v.as_u64())
            .and_then(|v| u32::try_from(v).ok())
            .ok_or("invalid blur pid")?;
        let class = w
            .get("class")
            .and_then(|v| v.as_str())
            .ok_or("invalid blur class")?
            .to_owned();
        let backdrop = w
            .get("backdrop")
            .and_then(|v| v.as_u64())
            .filter(|v| *v <= 4)
            .ok_or("invalid blur backdrop")? as u32;
        blur.originals.insert(
            h,
            Original {
                pid,
                class,
                backdrop,
            },
        );
    }
    blur.restore()
}
