param(
    [string]$Executable = (Join-Path (Split-Path -Parent $PSScriptRoot) 'build\Release\MandelbrotWallpaper.exe'),
    [string]$ReportDirectory = (Join-Path $env:TEMP 'mw-ph01-runtime-smoke'),
    [ValidateRange(5, 120)]
    [int]$StartupTimeoutSeconds = 60,
    [string]$CapturePath,
    [switch]$DiagnosticOnly
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

if (-not (Test-Path -LiteralPath $Executable)) {
    throw "Executable not found: $Executable"
}

New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
$appData = Join-Path $ReportDirectory 'appdata'
New-Item -ItemType Directory -Force -Path $appData | Out-Null

if (-not ('MandelbrotNativeWindow' -as [type])) {
    Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class MandelbrotNativeWindow {
    public delegate bool EnumWindowsProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern IntPtr FindWindow(string className, string windowName);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr parameter);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern int GetClassName(IntPtr window, System.Text.StringBuilder className, int maxCount);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern int GetWindowText(IntPtr window, System.Text.StringBuilder text, int maxCount);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool GetWindowRect(IntPtr window, out RECT rectangle);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool PrintWindow(IntPtr window, IntPtr deviceContext, uint flags);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left; public int Top; public int Right; public int Bottom; }
    public static IntPtr FindWindowForProcess(uint targetProcessId, string expectedClassName) {
        IntPtr match = IntPtr.Zero;
        EnumWindows(delegate(IntPtr window, IntPtr parameter) {
            uint owner = 0;
            GetWindowThreadProcessId(window, out owner);
            if (owner != targetProcessId) return true;
            var className = new System.Text.StringBuilder(256);
            GetClassName(window, className, className.Capacity);
            if (string.Equals(className.ToString(), expectedClassName, StringComparison.Ordinal)) {
                match = window;
                return false;
            }
            return true;
        }, IntPtr.Zero);
        return match;
    }
}
'@
}

if ([MandelbrotNativeWindow]::FindWindow('MandelbrotLiveWallpaperControl', $null) -ne [IntPtr]::Zero) {
    throw 'Another Mandelbrot Live Wallpaper instance is already running.'
}

$previousOverride = $env:MW_APPDATA_DIR
$process = $null
try {
    $env:MW_APPDATA_DIR = $appData
    $process = Start-Process -FilePath $Executable -WorkingDirectory (Split-Path -Parent $Executable) -PassThru
    if ($DiagnosticOnly) {
        $started = Get-Date
        $window = [IntPtr]::Zero
        while (((Get-Date) - $started).TotalSeconds -lt $StartupTimeoutSeconds) {
            if ($process.HasExited) { break }
            $window = [MandelbrotNativeWindow]::FindWindowForProcess(
                [uint32]$process.Id, 'MandelbrotLiveWallpaperControl')
            if ($window -ne [IntPtr]::Zero) { break }
            Start-Sleep -Milliseconds 200
            $process.Refresh()
        }
        $process.Refresh()
        $windows = [System.Collections.Generic.List[string]]::new()
        $callback = [MandelbrotNativeWindow+EnumWindowsProc]{
            param([IntPtr]$window, [IntPtr]$parameter)
            [uint32]$owner = 0
            [void][MandelbrotNativeWindow]::GetWindowThreadProcessId($window, [ref]$owner)
            $className = [Text.StringBuilder]::new(256)
            $windowText = [Text.StringBuilder]::new(256)
            [void][MandelbrotNativeWindow]::GetClassName($window, $className, $className.Capacity)
            [void][MandelbrotNativeWindow]::GetWindowText($window, $windowText, $windowText.Capacity)
            $windows.Add("owner=$owner handle=$window visible=$([MandelbrotNativeWindow]::IsWindowVisible($window)) class=$className title=$windowText")
            return $true
        }
        [void][MandelbrotNativeWindow]::EnumWindows($callback, [IntPtr]::Zero)
        $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
        Write-Output "DIAGNOSTIC process=$($process.Id) exited=$($process.HasExited) classWindow=$window elapsedSeconds=$elapsed windows=$($windows.Count)"
        $windows | Write-Output
        return
    }
    $deadline = (Get-Date).AddSeconds($StartupTimeoutSeconds)
    $window = [IntPtr]::Zero
    while ((Get-Date) -lt $deadline) {
        if ($process.HasExited) { break }
        $window = [MandelbrotNativeWindow]::FindWindowForProcess(
            [uint32]$process.Id, 'MandelbrotLiveWallpaperControl')
        if ($window -ne [IntPtr]::Zero) { break }
        Start-Sleep -Milliseconds 200
        $process.Refresh()
    }
    if ($process.HasExited) {
        throw "Application exited before creating the main window. Exit code: $($process.ExitCode)."
    }
    if ($window -eq [IntPtr]::Zero) {
        throw "Main window was not found within $StartupTimeoutSeconds seconds."
    }

    Start-Sleep -Seconds 2
    if (-not [string]::IsNullOrWhiteSpace($CapturePath)) {
        [MandelbrotNativeWindow+RECT]$bounds = [MandelbrotNativeWindow+RECT]::new()
        if (-not [MandelbrotNativeWindow]::GetWindowRect($window, [ref]$bounds)) {
            throw 'Could not obtain the application window bounds for capture.'
        }
        $width = $bounds.Right - $bounds.Left
        $height = $bounds.Bottom - $bounds.Top
        if ($width -le 0 -or $height -le 0) { throw 'Application window has invalid capture bounds.' }
        Add-Type -AssemblyName System.Drawing
        $bitmap = [System.Drawing.Bitmap]::new($width, $height)
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        try {
            try {
                $graphics.CopyFromScreen($bounds.Left, $bounds.Top, 0, 0, $bitmap.Size)
            } catch {
                $deviceContext = $graphics.GetHdc()
                try {
                    if (-not [MandelbrotNativeWindow]::PrintWindow($window, $deviceContext, 2)) {
                        throw 'Could not capture the application window.'
                    }
                } finally {
                    $graphics.ReleaseHdc($deviceContext)
                }
            }
        } finally {
            $graphics.Dispose()
        }
        try {
            $captureParent = Split-Path -Parent $CapturePath
            if ($captureParent) { New-Item -ItemType Directory -Force -Path $captureParent | Out-Null }
            $bitmap.Save($CapturePath, [System.Drawing.Imaging.ImageFormat]::Png)
        } finally {
            $bitmap.Dispose()
        }
        Write-Output "Captured application window: $CapturePath"
    }
    if (-not [MandelbrotNativeWindow]::PostMessage($window, 0x0111, [IntPtr]41008, [IntPtr]::Zero)) {
        throw 'Failed to post the application Exit command.'
    }
    if (-not $process.WaitForExit(15000)) {
        throw 'Application did not exit cleanly within 15 seconds.'
    }
    if ($process.ExitCode -ne 0) {
        throw "Application smoke test exited with code $($process.ExitCode)."
    }

    $logs = @(Get-ChildItem -Path (Join-Path $appData 'logs') -Filter '*.log' -File -ErrorAction SilentlyContinue)
    $logText = ($logs | Get-Content -ErrorAction SilentlyContinue) -join [Environment]::NewLine
    if ($logText -notmatch 'Application startup\.' -or $logText -notmatch 'Application shutdown\.') {
        throw 'Isolated log does not contain both startup and shutdown markers.'
    }

    Write-Output "Runtime smoke passed. ExitCode=$($process.ExitCode); Logs=$($logs.Count); ReportDirectory=$ReportDirectory"
}
finally {
    if ($process -and -not $process.HasExited) {
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
    }
    $env:MW_APPDATA_DIR = $previousOverride
}
