# PowerShell · build.ps1 · reproducible build · Windows 11 · Rust + linker in PATH
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    cargo test --locked
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
    cargo build --locked --release
    if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
    Copy-Item -LiteralPath target\release\niriwin.exe -Destination niriwin.exe -Force
} finally { Pop-Location }
