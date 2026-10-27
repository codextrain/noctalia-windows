// Rust · config.rs · validated configuration · Windows 11 · stable Rust
use std::{fs, path::Path};
#[derive(Clone, Debug)]
pub struct Config {
    pub gap: i32,
    pub width: i32,
    pub poll: u64,
    pub top: i32,
    pub modifiers: u32,
    pub navigation_modifiers: u32,
    pub bar_class: String,
    pub align_to_bar: bool,
    pub round_corners: bool,
    pub exclude: Vec<String>,
}
impl Default for Config {
    fn default() -> Self {
        Self {
            gap: 12,
            width: 55,
            poll: 250,
            top: 0,
            modifiers: 7,
            navigation_modifiers: 8,
            bar_class: "Qt683QWindowToolSaveBits".into(),
            align_to_bar: true,
            round_corners: true,
            exclude: vec!["RainmeterMeterWindow".into(), "NoctaliaShell".into()],
        }
    }
}
impl Config {
    pub fn load(p: &Path) -> Result<Self, String> {
        let s = fs::read_to_string(p).map_err(|e| format!("{}: {e}", p.display()))?;
        Self::parse(&s)
    }
    fn parse(s: &str) -> Result<Self, String> {
        let mut c = Self::default();
        for (i, line) in s.lines().enumerate() {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                continue;
            }
            let (k, v) = line
                .split_once('=')
                .ok_or_else(|| format!("line {}: expected key=value", i + 1))?;
            let number = || {
                v.trim()
                    .parse::<i32>()
                    .map_err(|_| format!("invalid number: {k}"))
            };
            match k.trim() {
                "gap" => c.gap = number()?,
                "bar_class" => c.bar_class = v.trim().to_owned(),
                "align_to_bar" => {
                    c.align_to_bar = v
                        .trim()
                        .parse()
                        .map_err(|_| "align_to_bar expects true/false")?
                }
                "round_corners" => {
                    c.round_corners = v
                        .trim()
                        .parse()
                        .map_err(|_| "round_corners expects true/false")?
                }
                "width_percent" => c.width = number()?,
                "hotkey_modifiers" => {
                    c.modifiers = number()?.try_into().map_err(|_| "negative modifiers")?
                }
                "navigation_modifiers" => {
                    c.navigation_modifiers = number()?
                        .try_into()
                        .map_err(|_| "negative navigation modifiers")?
                }
                "poll_ms" => c.poll = number()?.try_into().map_err(|_| "negative poll_ms")?,
                "reserve_top" => c.top = number()?,
                "exclude_classes" => {
                    c.exclude = v
                        .split(';')
                        .map(|s| s.trim().to_owned())
                        .filter(|s| !s.is_empty())
                        .collect()
                }
                _ => return Err(format!("unknown key: {k}")),
            }
        }
        if !(0..=100).contains(&c.gap)
            || !(20..=100).contains(&c.width)
            || !(100..=5000).contains(&c.poll)
            || !(0..=500).contains(&c.top)
            || !(3..=7).contains(&c.modifiers)
            || c.modifiers & 3 != 3
            || ![1, 3, 5, 7, 8].contains(&c.navigation_modifiers)
            || c.bar_class.is_empty()
        {
            return Err("configuration outside allowed ranges".into());
        }
        Ok(c)
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn validation() {
        assert!(Config::parse("gap=-1").is_err());
        assert!(Config::parse("poll_ms=-1").is_err());
        assert!(Config::parse("width_percent=101").is_err());
        assert!(Config::parse("gapp=4").is_err());
        assert_eq!(
            Config::parse("navigation_modifiers=1")
                .unwrap()
                .navigation_modifiers,
            1
        );
        assert!(Config::parse("navigation_modifiers=0").is_err());
        assert_eq!(
            Config::parse("navigation_modifiers=8")
                .unwrap()
                .navigation_modifiers,
            8
        );
        assert!(!Config::parse("round_corners=false").unwrap().round_corners);
        assert!(Config::parse("align_to_bar=maybe").is_err());
        assert_eq!(Config::parse("gap=8\nwidth_percent=70").unwrap().gap, 8);
    }
}
