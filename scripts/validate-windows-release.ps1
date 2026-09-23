[CmdletBinding()]
param(
    [switch]$SkipInstaller,
    [switch]$SkipRuntimeSmoke,
    [ValidateRange(5, 120)]
    [int]$StartupTimeoutSeconds = 30,
    [string]$ReportDirectory
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $PSScriptRoot
$Version = '1.13.1'
$BuildScript = Join-Path $PSScriptRoot 'build-release.ps1'
$Timestamp = Get-Date -Format 'yyyyMMdd-HHmmss'
if ([string]::IsNullOrWhiteSpace($ReportDirectory)) {
    $ReportDirectory = Join-Path $Root "artifacts\windows-validation\$Timestamp"
}
$ReportDirectory = [System.IO.Path]::GetFullPath($ReportDirectory)
New-Item -ItemType Directory -Force -Path $ReportDirectory | Out-Null
$LogPath = Join-Path $ReportDirectory 'validation.log'
$JsonPath = Join-Path $ReportDirectory 'report.json'
$MarkdownPath = Join-Path $ReportDirectory 'report.md'
$Steps = [System.Collections.Generic.List[object]]::new()
$OverallStatus = 'Automated checks passed'
$FailureMessage = $null
$StartedAt = Get-Date

function Add-Step {
    param(
        [string]$Name,
        [ValidateSet('Passed', 'Failed', 'Skipped', 'Unproven')]
        [string]$Status,
        [string]$Detail
    )
    $Steps.Add([pscustomobject]@{
        name = $Name
        status = $Status
        detail = $Detail
    })
}

function Invoke-RecordedNative {
    param(
        [string]$Name,
        [string]$FilePath,
        [string[]]$Arguments
    )
    "`n=== $Name ===" | Tee-Object -FilePath $LogPath -Append
    "Command: $FilePath $($Arguments -join ' ')" | Tee-Object -FilePath $LogPath -Append
    $output = & $FilePath @Arguments 2>&1
    $exitCode = $LASTEXITCODE
    $output | ForEach-Object { $_.ToString() } | Tee-Object -FilePath $LogPath -Append
    if ($exitCode -ne 0) {
        Add-Step $Name 'Failed' "Exit code $exitCode. See validation.log."
        throw "$Name failed with exit code $exitCode."
    }
    Add-Step $Name 'Passed' 'Command completed successfully.'
}

function Resolve-PythonCommand {
    if (Get-Command py -ErrorAction SilentlyContinue) {
        return [pscustomobject]@{ File = 'py'; Prefix = @('-3') }
    }
    if (Get-Command python -ErrorAction SilentlyContinue) {
        return [pscustomobject]@{ File = 'python'; Prefix = @() }
    }
    throw 'Python 3 was not found as py or python.'
}

function Write-Reports {
    $finishedAt = Get-Date
    $gitBranch = $null
    $gitCommit = $null
    if (Test-Path (Join-Path $Root '.git')) {
        Push-Location $Root
        try {
            $gitBranch = (git branch --show-current 2>$null).Trim()
            $gitCommit = (git rev-parse HEAD 2>$null).Trim()
        } finally {
            Pop-Location
        }
    }

    $manualChecks = @(
        'Windows 10 22H2 runtime smoke',
        'Windows 11 runtime smoke',
        'Settings, Equation, Palette, Journey, Quick Controller, Scout, Slideshow and High-Resolution dialogs',
        'File-backed static/slideshow/exported-MP4 modes, mute/loop and visible failure stop/detach',
        'D3D11 and OpenGL runtime shader/render paths',
        'Mixed-DPI, negative-coordinate and multi-monitor arrangements',
        'Explorer restart, display reconnect, lock/unlock, sleep/wake and Remote Desktop',
        'Installer install, upgrade and uninstall behaviour'
    )

    $report = [ordered]@{
        reportId = "BR-$((Get-Date).ToString('yyyyMMdd'))-WINDOWS-$Timestamp"
        objective = 'PH-01 native Windows release validation'
        version = $Version
        status = $OverallStatus
        phaseStatus = 'In progress pending the manual Windows matrix'
        startedAt = $StartedAt.ToString('o')
        finishedAt = $finishedAt.ToString('o')
        machine = $env:COMPUTERNAME
        os = [System.Environment]::OSVersion.VersionString
        powershell = $PSVersionTable.PSVersion.ToString()
        gitBranch = $gitBranch
        gitCommit = $gitCommit
        reportDirectory = $ReportDirectory
        failure = $FailureMessage
        steps = $Steps
        manualChecksRemaining = $manualChecks
    }
    $report | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 $JsonPath

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add('# Windows Release Validation Report')
    $lines.Add('')
    $lines.Add("**Status:** $OverallStatus")
    $lines.Add("**Version:** $Version")
    $lines.Add("**Started:** $($StartedAt.ToString('o'))")
    $lines.Add("**Finished:** $($finishedAt.ToString('o'))")
    if ($gitBranch) { $lines.Add("**Git:** ``$gitBranch`` at ``$gitCommit``") }
    if ($FailureMessage) { $lines.Add("**Failure:** $FailureMessage") }
    $lines.Add('')
    $lines.Add('## Automated steps')
    $lines.Add('')
    $lines.Add('| Step | Status | Detail |')
    $lines.Add('|---|---|---|')
    foreach ($step in $Steps) {
        $detail = $step.detail.Replace('|', '\|').Replace("`r", ' ').Replace("`n", ' ')
        $lines.Add("| $($step.name) | $($step.status) | $detail |")
    }
    $lines.Add('')
    $lines.Add('## Manual checks remaining')
    $lines.Add('')
    foreach ($check in $manualChecks) { $lines.Add("- [ ] $check") }
    $lines.Add('')
    $lines.Add('Automated startup smoke uses an isolated `MW_APPDATA_DIR`; it does not load or overwrite the normal user settings file.')
    $lines | Set-Content -Encoding UTF8 $MarkdownPath
}

try {
    if ($env:OS -ne 'Windows_NT') {
        throw 'This validation workflow requires native Windows.'
    }

    foreach ($required in @('cmake', 'git')) {
        if (-not (Get-Command $required -ErrorAction SilentlyContinue)) {
            throw "Required command not found: $required"
        }
    }
    $Python = Resolve-PythonCommand
    Add-Step 'Tool preflight' 'Passed' "CMake, Git and Python were found. PowerShell $($PSVersionTable.PSVersion)."

    Push-Location $Root
    try {
        if (Test-Path '.git') {
            $dirty = @(git status --porcelain)
            if ($LASTEXITCODE -ne 0) { throw 'git status failed.' }
            if ($dirty.Count -gt 0) {
                throw 'Working tree is not clean. Commit or stash changes before release validation.'
            }
            Add-Step 'Repository cleanliness' 'Passed' 'Git working tree is clean.'
        } else {
            Add-Step 'Repository cleanliness' 'Unproven' 'No .git directory is present in this source copy.'
        }

        $pythonArgs = @($Python.Prefix) + @('scripts\verify-source.py')
        Invoke-RecordedNative 'Source and offline-policy verification' $Python.File $pythonArgs

        $powerShellExe = (Get-Process -Id $PID).Path
        $buildArgs = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $BuildScript)
        if ($SkipInstaller) { $buildArgs += '-SkipInstaller' }
        Invoke-RecordedNative 'Native build, CTest and packaging' $powerShellExe $buildArgs

        $Exe = Join-Path $Root 'build\Release\MandelbrotWallpaper.exe'
        $PortableZip = Join-Path $Root "dist\Mandelbrot-Live-Wallpaper-$Version-win-x64.zip"
        if (-not (Test-Path $Exe)) { throw "Expected executable missing: $Exe" }
        if (-not (Test-Path $PortableZip)) { throw "Expected portable package missing: $PortableZip" }

        $versionInfo = (Get-Item $Exe).VersionInfo
        $embeddedFileVersion = "$($versionInfo.FileMajorPart).$($versionInfo.FileMinorPart).$($versionInfo.FileBuildPart)"
        $embeddedProductVersion = "$($versionInfo.ProductMajorPart).$($versionInfo.ProductMinorPart).$($versionInfo.ProductBuildPart)"
        if ($embeddedFileVersion -ne $Version -or $embeddedProductVersion -ne $Version) {
            throw "Embedded executable version mismatch: file=$embeddedFileVersion product=$embeddedProductVersion expected=$Version"
        }
        Add-Step 'Executable metadata' 'Passed' "File and product versions are $Version."

        Add-Type -AssemblyName System.IO.Compression.FileSystem
        $archive = [System.IO.Compression.ZipFile]::OpenRead($PortableZip)
        try {
            $entries = @($archive.Entries | ForEach-Object { $_.FullName.Replace('\', '/') })
            foreach ($requiredEntry in @('MandelbrotWallpaper.exe', 'README.md', 'LICENSE')) {
                if ($entries -notcontains $requiredEntry) {
                    throw "Portable package is missing $requiredEntry."
                }
            }
            $forbidden = @($entries | Where-Object {
                $_ -match '(^|/)(settings\.json|logs?|build|dist)(/|$)' -or $_ -match '\.(pdb|ilk|obj|user)$'
            })
            if ($forbidden.Count -gt 0) {
                throw "Portable package contains forbidden local/build artifacts: $($forbidden -join ', ')"
            }
            Add-Step 'Portable package inspection' 'Passed' "$($entries.Count) entries inspected; required files present and no forbidden local/build artifacts found."
        } finally {
            $archive.Dispose()
        }

        if ($SkipRuntimeSmoke) {
            Add-Step 'Isolated startup/shutdown smoke' 'Skipped' 'Skipped by command-line option.'
        } else {
            if (-not ('MandelbrotNativeWindow' -as [type])) {
                Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class MandelbrotNativeWindow {
    public delegate bool EnumWindowsProc(IntPtr window, IntPtr parameter);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool PostMessage(IntPtr window, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool EnumWindows(EnumWindowsProc callback, IntPtr parameter);
    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    public static extern int GetClassName(IntPtr window, System.Text.StringBuilder className, int maxCount);
    public static IntPtr FindWindowByClass(string expectedClassName) {
        return FindWindowForProcess(0, expectedClassName);
    }
    public static IntPtr FindWindowForProcess(uint targetProcessId, string expectedClassName) {
        IntPtr match = IntPtr.Zero;
        EnumWindows(delegate(IntPtr window, IntPtr parameter) {
            uint owner = 0;
            GetWindowThreadProcessId(window, out owner);
            if (targetProcessId != 0 && owner != targetProcessId) return true;
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
            if ([MandelbrotNativeWindow]::FindWindowByClass('MandelbrotLiveWallpaperControl') -ne [IntPtr]::Zero) {
                throw 'Another Mandelbrot Live Wallpaper instance is already running. Close it before the smoke test.'
            }

            $isolatedAppData = Join-Path $ReportDirectory 'appdata'
            New-Item -ItemType Directory -Force -Path $isolatedAppData | Out-Null
            $previousOverride = $env:MW_APPDATA_DIR
            $process = $null
            try {
                $env:MW_APPDATA_DIR = $isolatedAppData
                $process = Start-Process -FilePath $Exe -WorkingDirectory (Split-Path -Parent $Exe) -PassThru
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
                $wmCommand = 0x0111
                $trayExitCommand = 41008
                if (-not [MandelbrotNativeWindow]::PostMessage($window, $wmCommand, [IntPtr]$trayExitCommand, [IntPtr]::Zero)) {
                    throw 'Failed to post the application Exit command.'
                }
                if (-not $process.WaitForExit(15000)) {
                    throw 'Application did not exit cleanly within 15 seconds.'
                }
                if ($process.ExitCode -ne 0) {
                    throw "Application smoke test exited with code $($process.ExitCode)."
                }

                $logFiles = @(Get-ChildItem -Path (Join-Path $isolatedAppData 'logs') -Filter '*.log' -File -ErrorAction SilentlyContinue)
                $logText = ($logFiles | Get-Content -ErrorAction SilentlyContinue) -join "`n"
                if ($logText -notmatch 'Application startup\.' -or $logText -notmatch 'Application shutdown\.') {
                    throw 'Isolated log does not contain both startup and shutdown markers.'
                }
                Add-Step 'Isolated startup/shutdown smoke' 'Passed' 'Main window appeared, the normal message/render loop ran for two seconds, the application accepted its Exit command and logged clean shutdown using isolated app data.'
            } finally {
                if ($process -and -not $process.HasExited) {
                    Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
                }
                $env:MW_APPDATA_DIR = $previousOverride
            }
        }

        $installerPath = Join-Path $Root "dist\Mandelbrot-Live-Wallpaper-$Version-Setup-x64.exe"
        if ($SkipInstaller) {
            Add-Step 'Installer artifact' 'Skipped' 'Installer generation was skipped by command-line option.'
        } elseif (Test-Path -LiteralPath $installerPath -PathType Leaf) {
            $installer = Get-Item -LiteralPath $installerPath
            if ($installer.Length -le 0) { throw 'The generated installer is empty.' }
            $installerVersion = $installer.VersionInfo.FileVersion.Trim()
            if ($installerVersion -ne $Version) {
                throw "Installer file version mismatch: expected=$Version actual=$installerVersion"
            }
            $installerHash = (Get-FileHash -Algorithm SHA256 -LiteralPath $installerPath).Hash.ToLowerInvariant()
            $installerSignature = (Get-AuthenticodeSignature -LiteralPath $installerPath).Status.ToString()
            Add-Step 'Installer artifact' 'Passed' "Inno Setup produced $($installer.Length) bytes; file version=$installerVersion; sha256=$installerHash; Authenticode=$installerSignature. Install/upgrade/uninstall runtime checks remain manual."
        } else {
            Add-Step 'Installer artifact' 'Unproven' 'Expected installer was not produced; Inno Setup may be unavailable. See validation.log.'
        }
    } finally {
        Pop-Location
    }
} catch {
    $OverallStatus = 'Failed'
    $FailureMessage = $_.Exception.Message
    if ($Steps.Count -eq 0 -or $Steps[$Steps.Count - 1].status -ne 'Failed') {
        Add-Step 'Validation workflow' 'Failed' $FailureMessage
    }
    $FailureMessage | Tee-Object -FilePath $LogPath -Append | Out-Null
    Write-Error $FailureMessage -ErrorAction Continue
} finally {
    Write-Reports
    Write-Host "Validation report: $MarkdownPath"
    Write-Host "Machine-readable report: $JsonPath"
}

if ($OverallStatus -eq 'Failed') { exit 1 }
