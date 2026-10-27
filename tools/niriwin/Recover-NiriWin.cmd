@echo off
cd /d "%~dp0"
niriwin.exe --stop
timeout /t 2 /nobreak >nul
niriwin.exe --recover
pause
