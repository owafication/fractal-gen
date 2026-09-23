setlocal EnableExtensions EnableDelayedExpansion

rem Adds the located MSVC x64 compiler and CMake directories to the current
rem user's persistent Path.  It deliberately does not modify the machine Path.

set "VS_ROOT=G:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools"
set "VCVARS64=%VS_ROOT%\VC\Auxiliary\Build\vcvars64.bat"
set "MSVC_BIN="
for /d %%D in ("%VS_ROOT%\VC\Tools\MSVC\*") do (
    if exist "%%~fD\bin\Hostx64\x64\cl.exe" set "MSVC_BIN=%%~fD\bin\Hostx64\x64"
)

if not defined MSVC_BIN (
    echo ERROR: Could not find cl.exe beneath "%VS_ROOT%".
    exit /b 1
)

set "CMAKE_BIN="
for %%D in (
    "G:\Program Files\CMake\bin"
    "C:\Program Files\CMake\bin"
    "%VS_ROOT%\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
) do (
    if not defined CMAKE_BIN if exist "%%~fD\cmake.exe" set "CMAKE_BIN=%%~fD"
)

if not defined CMAKE_BIN (
    echo ERROR: Could not find cmake.exe in the configured locations.
    echo Add its bin directory to this script, then run it again.
    exit /b 1
)

powershell.exe -NoProfile -ExecutionPolicy Bypass -Command "$msvc=$env:MSVC_BIN; $cmake=$env:CMAKE_BIN; $old=[Environment]::GetEnvironmentVariable('Path','User'); $parts=@($old -split ';' | Where-Object { $_ }); foreach($entry in @($msvc,$cmake)){ if($parts -notcontains $entry){ $parts += $entry } }; [Environment]::SetEnvironmentVariable('Path',($parts -join ';'),'User'); [Environment]::SetEnvironmentVariable('MW_VCVARS64',$env:VCVARS64,'User'); [Environment]::SetEnvironmentVariable('MW_CMAKE_BIN',$cmake,'User')"
if errorlevel 1 (
    echo ERROR: Windows did not update the user environment variables.
    exit /b 1
)

echo Added to User Path:
echo   %MSVC_BIN%
echo   %CMAKE_BIN%
echo Saved MW_VCVARS64=%VCVARS64%
echo.
echo Close and reopen Command Prompt, PowerShell, Visual Studio, and Codex before using the new Path.
echo For a full MSVC build environment, still run:
echo   call "%VCVARS64%"
