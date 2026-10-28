#![windows_subsystem="windows"]
// Rust · desktop-start.rs · silent logon launcher · Windows 11 · rustc
use std::{fs,path::Path,process::{Command,Stdio},os::windows::process::CommandExt,time::Instant};
#[repr(C)] struct Entry {size:u32,usage:u32,pid:u32,heap:usize,module:u32,threads:u32,parent:u32,priority:i32,flags:u32,name:[u16;260]}
#[link(name="kernel32")] extern "system" {fn CreateToolhelp32Snapshot(flags:u32,pid:u32)->isize;fn Process32FirstW(h:isize,e:*mut Entry)->i32;fn Process32NextW(h:isize,e:*mut Entry)->i32;fn CloseHandle(h:isize)->i32;}
fn running(name:&str)->bool {unsafe {let h=CreateToolhelp32Snapshot(2,0);if h == -1{return false;}let mut e:Entry=std::mem::zeroed();e.size=std::mem::size_of::<Entry>() as u32;let mut ok=Process32FirstW(h,&mut e);let mut found=false;while ok!=0 {let n=e.name.iter().position(|x|*x==0).unwrap_or(260);if String::from_utf16_lossy(&e.name[..n]).eq_ignore_ascii_case(name){found=true;break;}ok=Process32NextW(h,&mut e);}CloseHandle(h);found}}
fn uri(p:&Path)->String {format!("file:///{}",p.to_string_lossy().replace('\\',"/").replace('%',"%25").replace(' ',"%20").replace('#',"%23"))}
fn command(p:&Path)->Command {let mut c=Command::new(p);c.creation_flags(0x08000000).stdin(Stdio::null()).stdout(Stdio::null()).stderr(Stdio::null());c}
fn main(){let base=std::env::current_exe().unwrap().parent().unwrap().to_owned();let start=Instant::now();let result=(||->Result<(),Box<dyn std::error::Error>>{
 let shell=base.join("Noctalia-DragDrop");let exe=fs::read_to_string(shell.join("runtime-path.txt"))?;let exe=Path::new(exe.trim());
 if !running("noctalia.exe") {
  fs::create_dir_all(shell.join("logs"))?;
  let p=command(exe).env("NOCTALIA_VOICE_STATE",uri(&shell.join("voice-state.json"))).env("NOCTALIA_QML_DIR",shell.join("source/qml")).env("NOCTALIA_OPEN","").env("NOCTALIA_SETTINGS_PAGE","").env("NOCTALIA_NIRIWIN_STATUS",uri(&base.join("NiriWin/state/status.json"))).env("NOCTALIA_GPU_STATE",uri(&shell.join("gpu-state.json"))).env("QML_XHR_ALLOW_FILE_READ","1").stderr(fs::File::create(shell.join("logs/noctalia-error.log"))?).spawn()?;
  let voice=shell.join("vitoria-voice/vitoria-voice.exe");
  if voice.exists(){command(&voice).arg("--parent").arg(p.id().to_string()).arg("--state").arg(shell.join("voice-state.json")).arg("--model").arg(shell.join("voice-model/vosk-model-small-pt-0.3")).arg("--config").arg(Path::new(&std::env::var("LOCALAPPDATA")?).join("noctalia-win/config.json")).spawn()?;}
  let gpu=shell.join("noctalia-gpu.exe");if gpu.exists(){command(&gpu).arg("--parent").arg(p.id().to_string()).arg("--state").arg(shell.join("gpu-state.json")).spawn()?;}
 }
 let niri=base.join("NiriWin");let exe=niri.join("niriwin.exe");
 if !running("niriwin.exe") {
  if ["recovery.tsv","blur-recovery.json","taskbar-recovery.json","opacity-recovery.json"].iter().any(|n|niri.join("state").join(n).exists()) && !command(&exe).arg("--recover").current_dir(&niri).status()?.success(){return Err("Niri recovery failed".into());}
  command(&exe).arg("--run").current_dir(&niri).stderr(fs::File::create(niri.join("state/startup-error.log"))?).spawn()?;
 }
 Ok(())})();let _=fs::write(base.join("inicio-nativo.log"),format!("Launch took {:?}; result: {:?}\n",start.elapsed(),result));}
