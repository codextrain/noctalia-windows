// Rust · noctalia-gpu.rs · read-only GPU telemetry · Windows 11 x64 · stable Rust
use std::{
    collections::BTreeMap,
    ffi::c_void,
    fs,
    path::PathBuf,
    time::{Duration, SystemTime, UNIX_EPOCH},
};
type Handle = isize;
#[repr(C)]
#[derive(Clone, Copy)]
struct Value {
    status: u32,
    value: f64,
}
#[repr(C)]
struct Item {
    name: *const u16,
    value: Value,
}
#[link(name = "kernel32")]
extern "system" {
    fn LoadLibraryExW(name: *const u16, file: Handle, flags: u32) -> Handle;
    fn GetProcAddress(module: Handle, name: *const u8) -> *const c_void;
    fn OpenProcess(access: u32, inherit: i32, pid: u32) -> Handle;
    fn WaitForSingleObject(handle: Handle, timeout: u32) -> u32;
    fn CloseHandle(handle: Handle) -> i32;
}
fn pdh_library() -> Handle {
    static MODULE: std::sync::OnceLock<Handle> = std::sync::OnceLock::new();
    *MODULE.get_or_init(|| unsafe { LoadLibraryExW(wide("pdh.dll").as_ptr(), 0, 0x800) })
}
macro_rules! pdh {
 ($name:ident($($arg:ident:$kind:ty),* $(,)?))=>{
  #[allow(non_snake_case)]unsafe fn $name($($arg:$kind),*)->u32 {
   static POINTER:std::sync::OnceLock<usize>=std::sync::OnceLock::new();
   let pointer=*POINTER.get_or_init(||{let module=pdh_library();if module==0{0}else{GetProcAddress(module,concat!(stringify!($name),"\0").as_ptr()) as usize}});
   if pointer==0{return 127;}
   let function:unsafe extern "system" fn($($kind),*)->u32=std::mem::transmute(pointer);function($($arg),*)
  }
 };
}
pdh!(PdhOpenQueryW(source: *const u16, data: usize, query: *mut Handle));
pdh!(PdhAddEnglishCounterW(query: Handle, path: *const u16, data: usize, counter: *mut Handle));
pdh!(PdhCollectQueryData(query: Handle));
pdh!(
    PdhGetFormattedCounterArrayW(
        counter: Handle,
        format: u32,
        bytes: *mut u32,
        count: *mut u32,
        items: *mut c_void,
    )
);
pdh!(PdhCloseQuery(query: Handle));
fn wide(s: &str) -> Vec<u16> {
    s.encode_utf16().chain(Some(0)).collect()
}
struct Sampler {
    query: Handle,
    counter: Handle,
}
impl Drop for Sampler {
    fn drop(&mut self) {
        unsafe { PdhCloseQuery(self.query) };
    }
}
fn busiest(samples: impl IntoIterator<Item = (String, f64)>) -> Option<f64> {
    let mut engines: BTreeMap<String, f64> = BTreeMap::new();
    for (name, value) in samples {
        if !value.is_finite() || value < 0.0 {
            continue;
        }
        let key = name
            .strip_prefix("pid_")
            .and_then(|tail| tail.split_once('_').map(|(_, engine)| engine))
            .unwrap_or(&name);
        *engines.entry(key.to_owned()).or_default() += value;
    }
    engines
        .values()
        .copied()
        .reduce(f64::max)
        .map(|v| v.clamp(0.0, 100.0))
}
impl Sampler {
    fn new() -> Result<Self, u32> {
        unsafe {
            let mut query = 0;
            let status = PdhOpenQueryW(std::ptr::null(), 0, &mut query);
            if status != 0 {
                return Err(status);
            }
            let mut sampler = Self { query, counter: 0 };
            let status = PdhAddEnglishCounterW(
                query,
                wide(r"\GPU Engine(*)\Utilization Percentage").as_ptr(),
                0,
                &mut sampler.counter,
            );
            if status != 0 {
                return Err(status);
            }
            PdhCollectQueryData(query);
            Ok(sampler)
        }
    }
    fn sample(&self) -> Result<Option<f64>, u32> {
        unsafe {
            let status = PdhCollectQueryData(self.query);
            if status != 0 {
                return Err(status);
            }
            let mut bytes = 0u32;
            let mut count = 0u32;
            let format = 0x200 | 0x8000;
            let status = PdhGetFormattedCounterArrayW(
                self.counter,
                format,
                &mut bytes,
                &mut count,
                std::ptr::null_mut(),
            );
            if status != 0x800007d2 && status != 0 {
                return Err(status);
            }
            if bytes == 0 || bytes > 32 * 1024 * 1024 {
                return Ok(None);
            }
            // u64 storage provides the alignment required by PDH's pointer/double records.
            let mut storage = vec![0u64; (bytes as usize).div_ceil(8)];
            let status = PdhGetFormattedCounterArrayW(
                self.counter,
                format,
                &mut bytes,
                &mut count,
                storage.as_mut_ptr().cast(),
            );
            if status != 0 {
                return Err(status);
            }
            if count as usize > bytes as usize / std::mem::size_of::<Item>() {
                return Ok(None);
            }
            let items = std::slice::from_raw_parts(storage.as_ptr().cast::<Item>(), count as usize);
            let base = storage.as_ptr() as usize;
            let end = base + bytes as usize;
            let mut samples = Vec::new();
            for item in items {
                if item.value.status > 1 {
                    continue;
                }
                let pointer = item.name as usize;
                if pointer < base || pointer >= end || !pointer.is_multiple_of(2) {
                    continue;
                }
                let name = std::slice::from_raw_parts(item.name, ((end - pointer) / 2).min(1024));
                let length = name.iter().position(|v| *v == 0).unwrap_or(name.len());
                samples.push((String::from_utf16_lossy(&name[..length]), item.value.value));
            }
            Ok(busiest(samples))
        }
    }
}
fn main() {
    if let Err(error) = run() {
        eprintln!("noctalia-gpu: {error}");
        std::process::exit(1);
    }
}
fn publish(state: &std::path::Path, value: &serde_json::Value) -> Result<(), String> {
    let bytes = serde_json::to_vec(value).map_err(|e| e.to_string())?;
    let temporary = state.with_extension("tmp");
    fs::write(&temporary, &bytes).map_err(|e| e.to_string())?;
    for _ in 0..5 {
        match fs::rename(&temporary, state) {
            Ok(()) => return Ok(()),
            Err(error)
                if error.kind() == std::io::ErrorKind::PermissionDenied
                    || error.kind() == std::io::ErrorKind::AlreadyExists =>
            {
                std::thread::sleep(Duration::from_millis(10))
            }
            Err(error) => return Err(error.to_string()),
        }
    }
    // Qt can keep a file open without delete-sharing on Windows. Telemetry is
    // disposable, so an in-place fallback is preferable to losing the reader.
    // The consumer validates JSON and retains its last fresh sample on a short read.
    fs::write(state, bytes).map_err(|e| e.to_string())
}
fn run() -> Result<(), String> {
    let args: Vec<_> = std::env::args().skip(1).collect();
    let mut state = None;
    let mut parent = None;
    let mut limit = None;
    let mut i = 0;
    while i < args.len() {
        let key = &args[i];
        i += 1;
        let value = args.get(i).ok_or("missing argument")?;
        match key.as_str() {
            "--state" => state = Some(PathBuf::from(value)),
            "--parent" => parent = Some(value.parse::<u32>().map_err(|_| "invalid parent")?),
            "--samples" => limit = Some(value.parse::<u32>().map_err(|_| "invalid samples")?),
            _ => return Err(format!("unknown option {key}")),
        }
        i += 1;
    }
    let state = state.ok_or("--state PATH required")?;
    if let Some(directory) = state.parent() {
        fs::create_dir_all(directory).map_err(|e| e.to_string())?;
    }
    let owner = if let Some(pid) = parent {
        let handle = unsafe { OpenProcess(0x00100000, 0, pid) };
        if handle == 0 {
            return Err("parent process unavailable".into());
        }
        Some(handle)
    } else {
        None
    };
    let mut sampler = Sampler::new().ok();
    let mut failures = 0;
    let mut samples = 0;
    let mut write_error_reported = false;
    loop {
        if owner.is_some_and(|h| unsafe { WaitForSingleObject(h, 0) != 258 }) {
            break;
        }
        std::thread::sleep(Duration::from_secs(1));
        if sampler.is_none() {
            sampler = Sampler::new().ok();
        }
        let result = sampler.as_ref().map(|s| s.sample());
        let (utilization, error) = match result {
            Some(Ok(value)) => (value, None),
            Some(Err(code)) => (None, Some(format!("PDH 0x{code:08x}"))),
            None => (None, Some("GPU counters unavailable".to_owned())),
        };
        if utilization.is_none() {
            failures += 1;
        } else {
            failures = 0;
        }
        if failures >= 5 {
            sampler = None;
            failures = 0;
        }
        let now = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap_or_default()
            .as_millis() as u64;
        let value = serde_json::json!({"version":1,"running":true,"updated_at_ms":now,"available":utilization.is_some(),"gpu_percent":utilization,"source":"pdh_gpu_engine","error":error});
        if let Err(error) = publish(&state, &value) {
            if !write_error_reported {
                eprintln!("state update failed; retrying: {error}");
                write_error_reported = true;
            }
        } else {
            write_error_reported = false;
        }
        samples += 1;
        if limit.is_some_and(|n| samples >= n) {
            break;
        }
    }
    if let Some(owner) = owner {
        unsafe { CloseHandle(owner) };
    }
    Ok(())
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn sums_contexts_per_engine_then_uses_busiest() {
        assert_eq!(
            busiest([
                (String::from("pid_1_luid_A_phys_0_eng_0_3D"), 20.0),
                (String::from("pid_2_luid_A_phys_0_eng_0_3D"), 30.0),
                (String::from("pid_1_luid_A_phys_0_eng_1_Copy"), 10.0)
            ]),
            Some(50.0)
        );
    }
    #[test]
    fn unavailable_is_not_zero_and_values_are_bounded() {
        assert_eq!(busiest(Vec::new()), None);
        assert_eq!(busiest([("engine".into(), 0.0)]), Some(0.0));
        assert_eq!(busiest([("engine".into(), 200.0)]), Some(100.0));
        assert_eq!(busiest([("engine".into(), f64::NAN)]), None);
    }
}
