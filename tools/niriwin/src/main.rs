// Rust · main.rs · NiriWin MVP · Windows 11 x64 · stable Rust
#[cfg(windows)]
mod app;
mod appearance;
mod config;
#[cfg(windows)]
mod effects;
mod layout;
#[cfg(windows)]
mod mouse;
mod spotlight;
#[cfg(windows)]
mod taskbar;
#[cfg(windows)]
mod transparency;
#[cfg(windows)]
mod win;
fn main() {
    #[cfg(windows)]
    if let Err(e) = app::main() {
        eprintln!("NiriWin: {e}");
        std::process::exit(1);
    }
    #[cfg(not(windows))]
    {
        eprintln!("NiriWin requires Windows 11 x64");
        std::process::exit(1);
    }
}
