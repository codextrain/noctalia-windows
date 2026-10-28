# PowerShell · Install-Voice.ps1 · offline PC voice helper · Windows 11 · Python 3.12
param([Parameter(Mandatory=$true)][string]$InstallRoot,[string]$Python='python')
$ErrorActionPreference='Stop'
$runtime=Join-Path $InstallRoot 'voice-build'
& $Python -m venv $runtime
if($LASTEXITCODE -ne 0){throw 'Falha ao criar ambiente Python.'}
$py=Join-Path $runtime 'Scripts/python.exe'
& $py -m pip install -r (Join-Path $PSScriptRoot 'requirements.txt') pyinstaller==6.16.0
if($LASTEXITCODE -ne 0){throw 'Falha ao instalar dependencias.'}
$modelRoot=Join-Path $InstallRoot 'voice-model'
New-Item -ItemType Directory -Force $modelRoot | Out-Null
$archive=Join-Path $runtime 'model.zip'
Invoke-WebRequest 'https://alphacephei.com/vosk/models/vosk-model-small-pt-0.3.zip' -OutFile $archive
Expand-Archive -LiteralPath $archive -DestinationPath $modelRoot -Force
& $py -m PyInstaller --noconfirm --onedir --windowed --name vitoria-voice --collect-all vosk --collect-all sounddevice --distpath $InstallRoot --workpath (Join-Path $runtime 'build') --specpath $runtime (Join-Path $PSScriptRoot 'vitoria_voice.py')
if($LASTEXITCODE -ne 0){throw 'Falha ao gerar helper de voz.'}
'Instalado. Integre o helper ao iniciador antes de ativar a voz.'
