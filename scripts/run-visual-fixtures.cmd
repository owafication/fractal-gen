@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run-visual-fixtures.ps1" %*
exit /b %ERRORLEVEL%
