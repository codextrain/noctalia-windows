# PowerShell · Start-Noctalia.ps1 · QML development launcher · Windows 11
param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [string]$Page = 'bar',
    [switch]$Background,
    [string]$NiriStatusPath = ''
)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $Executable).Path
$root = Split-Path $PSScriptRoot -Parent
$previous = @(Get-Process -Name noctalia -ErrorAction SilentlyContinue | Where-Object { try { $_.Path -eq $exe } catch { $false } })
if ($previous.Count) {
    Start-Process -FilePath $exe -ArgumentList 'quit' -WindowStyle Hidden -Wait
    foreach ($process in $previous) {
        if (-not $process.HasExited) { $process | Wait-Process -Timeout 10 -ErrorAction Stop }
    }
}
$env:NOCTALIA_QML_DIR = Join-Path $root 'qml'
$env:NOCTALIA_OPEN = if ($Background) { '' } else { 'settings' }
$env:NOCTALIA_SETTINGS_PAGE = if ($Background) { '' } else { $Page }
$env:QML_XHR_ALLOW_FILE_READ = '1'
if ($NiriStatusPath) { $env:NOCTALIA_NIRIWIN_STATUS = ([uri][IO.Path]::GetFullPath($NiriStatusPath)).AbsoluteUri }
$cache = Join-Path $env:LOCALAPPDATA 'noctalia-win\dev-launcher'
New-Item -ItemType Directory -Force -Path $cache | Out-Null
$helper = Join-Path (Split-Path $exe -Parent) 'noctalia-gpu.exe'
if (Test-Path -LiteralPath $helper) {
    $gpuState = Join-Path $cache 'gpu-state.json'
    $env:NOCTALIA_GPU_STATE = ([uri]$gpuState).AbsoluteUri
}
$process = Start-Process -FilePath $exe -WindowStyle Hidden -RedirectStandardError (Join-Path $cache 'noctalia-error.log') -PassThru
if (Test-Path -LiteralPath $helper) {
    Start-Process -FilePath $helper -ArgumentList @('--parent', $process.Id, '--state', ('"' + $gpuState + '"')) -WindowStyle Hidden -RedirectStandardError (Join-Path $cache 'gpu-error.log') | Out-Null
}
