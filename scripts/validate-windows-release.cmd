@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0validate-windows-release.ps1" %*
exit /b %ERRORLEVEL%
