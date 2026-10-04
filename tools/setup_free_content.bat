@echo off
setlocal
powershell.exe -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_free_content.ps1" %*
exit /b %ERRORLEVEL%
