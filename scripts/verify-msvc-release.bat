@echo off
setlocal EnableExtensions

call "%MW_VCVARS64%"
if errorlevel 1 exit /b %errorlevel%

cmake --preset codex-msvc-release
if errorlevel 1 exit /b %errorlevel%

cmake --build --preset codex-msvc-release --parallel
if errorlevel 1 exit /b %errorlevel%

ctest --test-dir build-codex-runtime -C Release --output-on-failure
