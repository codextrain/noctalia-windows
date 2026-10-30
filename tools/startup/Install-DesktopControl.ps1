# PowerShell · Install-DesktopControl.ps1 · desktop controls · Windows 11
param([Parameter(Mandatory=$true)][string]$RuntimeRoot)
$ErrorActionPreference='Stop'
$RuntimeRoot=(Resolve-Path -LiteralPath $RuntimeRoot).Path
$launcher=Join-Path $RuntimeRoot 'Vitoria-Start.exe'
if(!(Test-Path -LiteralPath $launcher -PathType Leaf)){throw 'Compile desktop-start.rs como Vitoria-Start.exe na pasta de execução.'}
if(!(Test-Path -LiteralPath (Join-Path $RuntimeRoot 'NiriWin/niriwin.exe'))){throw 'NiriWin não encontrado na pasta de execução.'}
if(!(Test-Path -LiteralPath (Join-Path $RuntimeRoot 'Noctalia-DragDrop/runtime-path.txt'))){throw 'runtime-path.txt não encontrado.'}
$iconPath=Join-Path $RuntimeRoot 'vitoria-wolf.ico'
$wolf=Join-Path $PSScriptRoot '../../assets/vitoria-wolf.png'
if(Test-Path -LiteralPath $wolf) {
 Add-Type -AssemblyName System.Drawing
 $image=[Drawing.Image]::FromFile((Resolve-Path -LiteralPath $wolf).Path)
 $bitmap=[Drawing.Bitmap]::new($image,64,64)
 $handle=$bitmap.GetHicon()
 $icon=[Drawing.Icon]::FromHandle($handle)
 $stream=[IO.File]::Create($iconPath)
 try {$icon.Save($stream)} finally {$stream.Dispose();$icon.Dispose();$bitmap.Dispose();$image.Dispose()}
}
$wsh=New-Object -ComObject WScript.Shell
function Add-ControlShortcut([string]$path,[string]$argument,[string]$description) {
 $shortcut=$wsh.CreateShortcut($path)
 $shortcut.TargetPath=$launcher
 $shortcut.Arguments=$argument
 $shortcut.WorkingDirectory=$RuntimeRoot
 $shortcut.Description=$description
 $shortcut.IconLocation=if(Test-Path -LiteralPath $iconPath){$iconPath+',0'}else{$launcher+',0'}
 $shortcut.Save()
}
Add-ControlShortcut (Join-Path $RuntimeRoot 'Desativar Vitoria e NiriWin.lnk') '--disable' 'Desativar Vitória Shell e NiriWin e restaurar o Windows'
$programs=[Environment]::GetFolderPath('Programs')
Add-ControlShortcut (Join-Path $programs 'Vitória Shell.lnk') '--toggle' 'Ativar ou desativar Vitória Shell e NiriWin juntos'
# Remove only the two desktop links created by earlier versions of this installer.
foreach($name in @('Ativar ou desativar Vitoria.lnk','Ativar Vitoria e NiriWin.lnk')) {
 $old=Join-Path ([Environment]::GetFolderPath('Desktop')) $name
 if(Test-Path -LiteralPath $old) {
  $link=$wsh.CreateShortcut($old)
  if($link.TargetPath -eq $launcher -and $link.Arguments -in @('--toggle','--enable')){Remove-Item -LiteralPath $old}
 }
}
