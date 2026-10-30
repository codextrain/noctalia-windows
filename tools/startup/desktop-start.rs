#![windows_subsystem = "windows"]
// Rust · desktop-start.rs · silent logon launcher · Windows 11 · rustc
use std::{
    fs,
    os::windows::process::CommandExt,
    path::Path,
    process::{Command, Stdio},
    thread,
    time::{Duration, Instant},
};
#[repr(C)]
struct Entry {
    size: u32,
    usage: u32,
    pid: u32,
    heap: usize,
    module: u32,
    threads: u32,
    parent: u32,
    priority: i32,
    flags: u32,
    name: [u16; 260],
}
#[link(name = "kernel32")]
extern "system" {
    fn CreateToolhelp32Snapshot(flags: u32, pid: u32) -> isize;
    fn Process32FirstW(h: isize, e: *mut Entry) -> i32;
    fn Process32NextW(h: isize, e: *mut Entry) -> i32;
    fn CloseHandle(h: isize) -> i32;
    fn CreateMutexW(a: *const u8, owner: i32, name: *const u16) -> isize;
    fn WaitForSingleObject(h: isize, ms: u32) -> u32;
    fn ReleaseMutex(h: isize) -> i32;
}
fn running(name: &str) -> bool {
    unsafe {
        let h = CreateToolhelp32Snapshot(2, 0);
        if h == -1 {
            return false;
        }
        let mut e: Entry = std::mem::zeroed();
        e.size = std::mem::size_of::<Entry>() as u32;
        let mut ok = Process32FirstW(h, &mut e);
        let mut found = false;
        while ok != 0 {
            let n = e.name.iter().position(|x| *x == 0).unwrap_or(260);
            if String::from_utf16_lossy(&e.name[..n]).eq_ignore_ascii_case(name) {
                found = true;
                break;
            }
            ok = Process32NextW(h, &mut e);
        }
        CloseHandle(h);
        found
    }
}
fn uri(p: &Path) -> String {
    format!(
        "file:///{}",
        p.to_string_lossy()
            .replace('\\', "/")
            .replace('%', "%25")
            .replace(' ', "%20")
            .replace('#', "%23")
    )
}
fn command(p: &Path) -> Command {
    let mut c = Command::new(p);
    c.creation_flags(0x08000000)
        .stdin(Stdio::null())
        .stdout(Stdio::null())
        .stderr(Stdio::null());
    c
}
fn launch(base: &Path) -> Result<(), Box<dyn std::error::Error>> {
    let shell = base.join("Noctalia-DragDrop");
    let exe = fs::read_to_string(shell.join("runtime-path.txt"))?;
    let exe = Path::new(exe.trim());
    if !running("noctalia.exe") {
        fs::create_dir_all(shell.join("logs"))?;
        let p = command(exe)
            .env(
                "NOCTALIA_DESKTOP_CONTROL",
                uri(&base.join("Desativar Vitoria e NiriWin.lnk")),
            )
            .env("NOCTALIA_VOICE_STATE", uri(&shell.join("voice-state.json")))
            .env("NOCTALIA_QML_DIR", shell.join("source/qml"))
            .env("NOCTALIA_OPEN", "")
            .env("NOCTALIA_SETTINGS_PAGE", "")
            .env(
                "NOCTALIA_NIRIWIN_STATUS",
                uri(&base.join("NiriWin/state/status.json")),
            )
            .env("NOCTALIA_GPU_STATE", uri(&shell.join("gpu-state.json")))
            .env("QML_XHR_ALLOW_FILE_READ", "1")
            .stderr(fs::File::create(shell.join("logs/noctalia-error.log"))?)
            .spawn()?;
        let voice = shell.join("vitoria-voice/vitoria-voice.exe");
        if voice.exists() {
            command(&voice)
                .arg("--parent")
                .arg(p.id().to_string())
                .arg("--state")
                .arg(shell.join("voice-state.json"))
                .arg("--model")
                .arg(shell.join("voice-model/vosk-model-small-pt-0.3"))
                .arg("--config")
                .arg(Path::new(&std::env::var("LOCALAPPDATA")?).join("noctalia-win/config.json"))
                .spawn()?;
        }
        let gpu = shell.join("noctalia-gpu.exe");
        if gpu.exists() {
            command(&gpu)
                .arg("--parent")
                .arg(p.id().to_string())
                .arg("--state")
                .arg(shell.join("gpu-state.json"))
                .spawn()?;
        }
    }
    let niri = base.join("NiriWin");
    let exe = niri.join("niriwin.exe");
    if !running("niriwin.exe") {
        if [
            "recovery.tsv",
            "blur-recovery.json",
            "taskbar-recovery.json",
            "opacity-recovery.json",
        ]
        .iter()
        .any(|n| niri.join("state").join(n).exists())
            && !command(&exe)
                .arg("--recover")
                .current_dir(&niri)
                .status()?
                .success()
        {
            return Err("Niri recovery failed".into());
        }
        command(&exe)
            .arg("--run")
            .current_dir(&niri)
            .stderr(fs::File::create(niri.join("state/startup-error.log"))?)
            .spawn()?;
    }
    Ok(())
}

fn wait_stopped(name: &str) -> Result<(), Box<dyn std::error::Error>> {
    let until = Instant::now() + Duration::from_secs(12);
    while running(name) {
        if Instant::now() >= until {
            return Err(
                format!("{name} não respondeu; nenhum processo foi encerrado à força.").into(),
            );
        }
        thread::sleep(Duration::from_millis(100));
    }
    Ok(())
}
// The running shell can hide Explorer again during Niri recovery. Reveal trays
// only after both managers have finished their normal shutdown.
fn show_taskbars() {
    #[link(name = "user32")]
    extern "system" {
        fn EnumWindows(
            callback: unsafe extern "system" fn(isize, isize) -> i32,
            param: isize,
        ) -> i32;
        fn GetClassNameW(h: isize, text: *mut u16, count: i32) -> i32;
        fn ShowWindowAsync(h: isize, show: i32) -> i32;
    }
    unsafe extern "system" fn reveal(h: isize, _: isize) -> i32 {
        let mut text = [0u16; 128];
        let count = GetClassNameW(h, text.as_mut_ptr(), 128);
        let class = String::from_utf16_lossy(&text[..count.max(0) as usize]);
        if class == "Shell_TrayWnd" || class == "Shell_SecondaryTrayWnd" {
            ShowWindowAsync(h, 8);
        }
        1
    }
    unsafe {
        EnumWindows(reveal, 0);
    }
}
fn disable(base: &Path) -> Result<(), Box<dyn std::error::Error>> {
    let niri = base.join("NiriWin");
    let exe = niri.join("niriwin.exe");
    if running("niriwin.exe") {
        if !command(&exe)
            .arg("--stop")
            .current_dir(&niri)
            .status()?
            .success()
        {
            return Err("Falha ao solicitar parada do NiriWin".into());
        }
        wait_stopped("niriwin.exe")?;
    }
    // Recovery also handles a prior crash, restoring taskbar, opacity and windows.
    if !command(&exe)
        .arg("--recover")
        .current_dir(&niri)
        .status()?
        .success()
    {
        return Err("Falha ao restaurar janelas; Vitória foi preservado".into());
    }
    if running("noctalia.exe") {
        let path = fs::read_to_string(base.join("Noctalia-DragDrop/runtime-path.txt"))?;
        if !command(Path::new(path.trim()))
            .arg("quit")
            .status()?
            .success()
        {
            return Err("Falha ao solicitar saída do Vitória".into());
        }
        wait_stopped("noctalia.exe")?;
    }
    show_taskbars();
    fs::write(base.join("desktop-disabled.flag"), "Disabled by user\n")?;
    Ok(())
}
fn action(mode: &str, disabled: bool) -> Result<&'static str, &'static str> {
    match mode {
        "--enable" => Ok("enable"),
        "--disable" => Ok("disable"),
        "--toggle" => Ok(if disabled { "enable" } else { "disable" }),
        "" => Ok(if disabled { "skip" } else { "enable" }),
        _ => Err("Use --enable, --disable ou --toggle"),
    }
}
fn main() {
    let base = std::env::current_exe()
        .unwrap()
        .parent()
        .unwrap()
        .to_owned();
    let started = Instant::now();
    let name: Vec<u16> = "Local\\VitoriaShell-DesktopControl\0"
        .encode_utf16()
        .collect();
    let mutex = unsafe { CreateMutexW(std::ptr::null(), 0, name.as_ptr()) };
    if mutex == 0 {
        return;
    }
    let wait = unsafe { WaitForSingleObject(mutex, 0) };
    if wait != 0 && wait != 128 {
        unsafe { CloseHandle(mutex) };
        return;
    }
    let result = (|| -> Result<(), Box<dyn std::error::Error>> {
        let flag = base.join("desktop-disabled.flag");
        let mode = std::env::args().nth(1).unwrap_or_default();
        match action(&mode, flag.exists())? {
            "disable" => disable(&base)?,
            "enable" => {
                launch(&base)?;
                if flag.exists() {
                    fs::remove_file(flag)?;
                }
            }
            _ => {}
        }
        Ok(())
    })();
    let _ = fs::write(
        base.join("inicio-nativo.log"),
        format!(
            "Action took {:?}; result: {:?}\n",
            started.elapsed(),
            result
        ),
    );
    if let Err(e) = result {
        #[link(name = "user32")]
        extern "system" {
            fn MessageBoxW(h: isize, text: *const u16, title: *const u16, flags: u32) -> i32;
        }
        let text: Vec<u16> = format!("Não foi possível concluir a alteração: {e}\0")
            .encode_utf16()
            .collect();
        let title: Vec<u16> = "Vitória Shell\0".encode_utf16().collect();
        unsafe {
            MessageBoxW(0, text.as_ptr(), title.as_ptr(), 0x10);
        }
    }
    unsafe {
        ReleaseMutex(mutex);
        CloseHandle(mutex);
    }
}
#[cfg(test)]
mod tests {
    use super::action;
    #[test]
    fn login_respects_disabled() {
        assert_eq!(action("", true), Ok("skip"));
        assert_eq!(action("", false), Ok("enable"));
    }
    #[test]
    fn explicit_actions() {
        for disabled in [true, false] {
            assert_eq!(action("--enable", disabled), Ok("enable"));
            assert_eq!(action("--disable", disabled), Ok("disable"));
        }
    }
    #[test]
    fn toggle_and_invalid() {
        assert_eq!(action("--toggle", true), Ok("enable"));
        assert_eq!(action("--toggle", false), Ok("disable"));
        assert!(action("--bad", false).is_err());
    }
}
