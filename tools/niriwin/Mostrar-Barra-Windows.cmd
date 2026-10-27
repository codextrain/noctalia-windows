@echo off
cd /d "%~dp0"
rem Emergency restore: stopping NiriWin also restores window positions.
niriwin.exe --stop
timeout /t 2 /nobreak >nul
niriwin.exe --recover
pause
