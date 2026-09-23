param([string]$ReportDirectory = 'artifacts\ph11-installer')

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$evidence = [IO.Path]::GetFullPath((Join-Path $root $ReportDirectory))
if (Test-Path -LiteralPath $evidence) { throw 'Use a fresh report directory; existing evidence is never deleted.' }
if (Get-Process -Name MandelbrotWallpaper -ErrorAction SilentlyContinue) {
    throw 'An application is already running; it will not be closed by this validator.'
}
New-Item -ItemType Directory -Path $evidence | Out-Null
$install = Join-Path $evidence 'installed'
$appData = Join-Path $evidence 'appdata'
$testId = [guid]::NewGuid().ToString().ToUpperInvariant()
$testName = "Mandelbrot PH11 Isolated $testId"
$uninstallKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\{$testId}_is1"
$steps = [Collections.Generic.List[object]]::new()
function Record([string]$name, [string]$detail) {
    $steps.Add([ordered]@{name=$name; status='Passed'; detail=$detail})
}
function Run-Bounded([string]$file, [string[]]$arguments, [int]$expected = 0) {
    $child = Start-Process -FilePath $file -ArgumentList $arguments -PassThru -WindowStyle Hidden
    if (!$child.WaitForExit(120000)) {
        throw "Installer process $($child.Id) exceeded 120 seconds; left running for inspection."
    }
    if ($child.ExitCode -ne $expected) { throw "Installer exit code $($child.ExitCode), expected $expected." }
}

Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public static class IsolatedInstallerWindow {
    private delegate bool EnumCallback(IntPtr window, IntPtr state);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumCallback callback, IntPtr state);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint pid);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetClassName(IntPtr window, StringBuilder name, int size);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    public static IntPtr Find(uint pid) {
        IntPtr found = IntPtr.Zero;
        EnumWindows((window, state) => {
            uint owner; GetWindowThreadProcessId(window, out owner);
            if (owner != pid) return true;
            var name = new StringBuilder(256); GetClassName(window, name, 256);
            if (name.ToString() != "MandelbrotLiveWallpaperControl") return true;
            found = window; return false;
        }, IntPtr.Zero);
        return found;
    }
}
'@

$status = 'Failed'
$failure = $null
$app = $null
try {
    $original = Get-Content -LiteralPath (Join-Path $root 'installer\MandelbrotWallpaper.iss') -Raw
    if ($original -match 'taskkill|Type: filesandordirs' -or $original -notmatch 'AppMutex=Local\\MandelbrotLiveWallpaper.SingleInstance') {
        throw 'Installer safety preflight failed.'
    }
    $isolated = $original.Replace('AppId={{20F1F29B-52A3-4BEE-AB7E-BCE7F6100C39}', "AppId={{$testId}")
    $isolated = $isolated.Replace('#define AppName "Mandelbrot Live Wallpaper"', "#define AppName `"$testName`"")
    $isolated = $isolated.Replace('DefaultDirName={localappdata}\Programs\MandelbrotLiveWallpaper', "DefaultDirName=$install")
    $isolated = $isolated.Replace('OutputBaseFilename=Mandelbrot-Live-Wallpaper-{#AppVersion}-Setup-x64', 'OutputBaseFilename=IsolatedSetup')
    $isolated = $isolated.Replace('..\', "$root\")
    $definition = Join-Path $evidence 'isolated.iss'
    Set-Content -LiteralPath $definition -Value $isolated -Encoding utf8
    $compiler = (Get-Command ISCC.exe -CommandType Application).Source
    & $compiler "/DSourceExe=$root\build\Release\MandelbrotWallpaper.exe" "/DOutputDir=$evidence" $definition *> (Join-Path $evidence 'compile.log')
    if ($LASTEXITCODE -ne 0) { throw "Inno compiler failed: $LASTEXITCODE" }
    Record 'Build isolated installer' "Distinct AppId {$testId}, product name, directory and shortcut; production payload and safety directives retained."

    $setup = Join-Path $evidence 'IsolatedSetup.exe'
    Run-Bounded $setup @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOCLOSEAPPLICATIONS',"/DIR=`"$install`"", "/LOG=`"$evidence\install.log`"")
    $installedExe = Join-Path $install 'MandelbrotWallpaper.exe'
    if (!(Test-Path -LiteralPath $uninstallKey) -or
        (Get-FileHash -LiteralPath $installedExe).Hash -ne (Get-FileHash -LiteralPath "$root\build\Release\MandelbrotWallpaper.exe").Hash) {
        throw 'Installed payload or isolated registration mismatch.'
    }
    Record 'Install' 'Per-user isolated registration and exact executable hash verified.'
    $sentinel = Join-Path $install 'user-created-output.txt'
    Set-Content -LiteralPath $sentinel -Value 'This untracked output must survive reinstall and uninstall.'
    $sentinelHash = (Get-FileHash -LiteralPath $sentinel).Hash

    $previousData = $env:MW_APPDATA_DIR
    try {
        $env:MW_APPDATA_DIR = $appData
        $app = Start-Process -FilePath $installedExe -WorkingDirectory $install -PassThru -WindowStyle Hidden
    } finally { $env:MW_APPDATA_DIR = $previousData }
    $deadline = (Get-Date).AddSeconds(30)
    $window = [IntPtr]::Zero
    while ((Get-Date) -lt $deadline -and !$app.HasExited) {
        $window = [IsolatedInstallerWindow]::Find([uint32]$app.Id)
        $log = Join-Path $appData 'logs\mandelbrot-wallpaper.log'
        if ($window -ne [IntPtr]::Zero -and (Test-Path -LiteralPath $log) -and
            (Get-Content -LiteralPath $log -Raw) -match 'Direct3D 11 renderer started') { break }
        Start-Sleep -Milliseconds 200
    }
    if ($window -eq [IntPtr]::Zero -or $app.HasExited -or
        (Get-Content -LiteralPath $log -Raw) -notmatch 'Direct3D 11 renderer started') { throw 'Installed launch smoke failed.' }
    Record 'Installed launch' 'Owned process exposed its main window and initialized production D3D11 with isolated app data.'
    $uninstaller = Join-Path $install 'unins000.exe'
    Run-Bounded $uninstaller @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$evidence\uninstall-running-refused.log`"") 1
    if ($app.HasExited -or !(Test-Path -LiteralPath $installedExe)) { throw 'Running-application protection failed.' }
    Record 'Running uninstall refusal' 'AppMutex refused silent uninstall with exit 1; the owned application remained running and installed.'
    [void][IsolatedInstallerWindow]::PostMessage($window, 0x111, [IntPtr]41008, [IntPtr]::Zero)
    if (!$app.WaitForExit(15000) -or (Get-Content -LiteralPath $log -Raw) -notmatch 'Application shutdown\.') { throw 'Installed application did not exit cleanly.' }
    Record 'Installed shutdown' 'Production Exit command and clean shutdown marker verified.'
    $settings = Join-Path $appData 'settings.json'
    $settingsHash = (Get-FileHash -LiteralPath $settings).Hash

    Run-Bounded $setup @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/NOCLOSEAPPLICATIONS',"/DIR=`"$install`"", "/LOG=`"$evidence\reinstall.log`"")
    if ((Get-FileHash -LiteralPath $settings).Hash -ne $settingsHash -or (Get-FileHash -LiteralPath $sentinel).Hash -ne $sentinelHash) { throw 'Same-version reinstall changed user data.' }
    Record 'Same-version reinstall' 'Version 1.13.1 over 1.13.1 preserved isolated settings and the untracked output. This is not preceding-release upgrade evidence.'

    Run-Bounded $uninstaller @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',"/LOG=`"$evidence\uninstall.log`"")
    if ((Test-Path -LiteralPath $installedExe) -or (Test-Path -LiteralPath $uninstallKey) -or
        (Get-FileHash -LiteralPath $settings).Hash -ne $settingsHash -or (Get-FileHash -LiteralPath $sentinel).Hash -ne $sentinelHash) {
        throw 'Uninstall cleanup or user-data preservation failed.'
    }
    Record 'Uninstall' 'Tracked executable and isolated registration removed; isolated settings and the untracked user output preserved.'
    $status = 'Passed'
} catch {
    $failure = $_.Exception.Message
} finally {
    if ($app -and !$app.HasExited) {
        $ownedWindow = [IsolatedInstallerWindow]::Find([uint32]$app.Id)
        if ($ownedWindow -ne [IntPtr]::Zero) {
            [void][IsolatedInstallerWindow]::PostMessage($ownedWindow, 0x111, [IntPtr]41008, [IntPtr]::Zero)
            [void]$app.WaitForExit(10000)
        }
    }
    [ordered]@{status=$status; failure=$failure; testAppId=$testId; installDirectory=$install; steps=@($steps.ToArray()); limitations=@('Isolated AppId/name/directory; normal user installation is not modified.', 'Same-version reinstall only; previous-release upgrade remains unproven.')} |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'report.json')
    $lines = @('# Isolated installer validation', '', "Status: $status", '', "Test AppId: {$testId}", '')
    foreach ($step in $steps) { $lines += "- Passed: $($step.name). $($step.detail)" }
    if ($failure) { $lines += "`nFailure: $failure" }
    $lines += "`nPreceding-release upgrade, normal-AppId install and signing remain unproven."
    $lines | Set-Content -LiteralPath (Join-Path $evidence 'report.md')
}
if ($failure) { throw $failure }
Write-Output "Isolated installer validation passed: $evidence\report.md"
