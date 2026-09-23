[CmdletBinding()]
param(
    [switch]$SkipInstaller
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root 'build'
$Dist = Join-Path $Root 'dist'
$Version = '1.13.1'
$VersionCheckScript = Join-Path $Root 'cmake\VerifyVersionConsistency.cmake'

function Invoke-Native {
    param([string]$FailureMessage, [scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$FailureMessage Exit code: $LASTEXITCODE."
    }
}

Push-Location $Root
try {
    if (Test-Path $Build) { Remove-Item -Recurse -Force $Build }
    New-Item -ItemType Directory -Force -Path $Build, $Dist | Out-Null

    Write-Host 'Checking release version consistency...'
    Invoke-Native 'Version consistency check failed.' {
        cmake "-DROOT_DIR=$Root" "-DEXPECTED_VERSION=$Version" -P $VersionCheckScript
    }

    Write-Host 'Configuring x64 Release build from CMakePresets.json...'
    Invoke-Native 'CMake configuration failed.' {
        cmake --preset windows-msvc-release
    }

    Write-Host 'Building...'
    Invoke-Native 'Build failed.' {
        cmake --build --preset windows-msvc-release --parallel
    }

    Write-Host 'Running core and release-gate tests...'
    Invoke-Native 'Tests failed.' {
        ctest --preset windows-msvc-release
    }

    $Exe = Join-Path $Build 'Release\MandelbrotWallpaper.exe'
    if (-not (Test-Path $Exe)) { throw "Expected executable was not produced: $Exe" }

    $VersionInfo = (Get-Item $Exe).VersionInfo
    $EmbeddedFileVersion = "$($VersionInfo.FileMajorPart).$($VersionInfo.FileMinorPart).$($VersionInfo.FileBuildPart)"
    $EmbeddedProductVersion = "$($VersionInfo.ProductMajorPart).$($VersionInfo.ProductMinorPart).$($VersionInfo.ProductBuildPart)"
    if ($EmbeddedFileVersion -ne $Version) {
        throw "Executable file version mismatch. Expected $Version, found $EmbeddedFileVersion."
    }
    if ($EmbeddedProductVersion -ne $Version) {
        throw "Executable product version mismatch. Expected $Version, found $EmbeddedProductVersion."
    }
    Write-Host "Embedded executable version verified: $Version"

    $Stage = Join-Path $Build 'package'
    if (Test-Path $Stage) { Remove-Item -Recurse -Force $Stage }
    New-Item -ItemType Directory -Force -Path $Stage | Out-Null
    Copy-Item $Exe $Stage
    Copy-Item -Path @('README.md', 'LICENSE') -Destination $Stage
    Copy-Item -Path 'docs' -Destination (Join-Path $Stage 'docs') -Recurse
    if (Test-Path 'assets\presets') {
        New-Item -ItemType Directory -Force -Path (Join-Path $Stage 'assets') | Out-Null
        Copy-Item 'assets\presets' (Join-Path $Stage 'assets\presets') -Recurse
    }

    $PortableZip = Join-Path $Dist "Mandelbrot-Live-Wallpaper-$Version-win-x64.zip"
    if (Test-Path $PortableZip) { Remove-Item -Force $PortableZip }
    Compress-Archive -Path (Join-Path $Stage '*') -DestinationPath $PortableZip -CompressionLevel Optimal
    Write-Host "Portable package: $PortableZip"

    if (-not $SkipInstaller) {
        $Iscc = @(
            "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
            "$env:ProgramFiles\Inno Setup 6\ISCC.exe"
        ) | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1
        if ($null -eq $Iscc) {
            $IsccCommand = Get-Command ISCC.exe -CommandType Application -ErrorAction SilentlyContinue |
                Select-Object -First 1
            if ($null -ne $IsccCommand) { $Iscc = $IsccCommand.Source }
        }
        if ($null -ne $Iscc) {
            & $Iscc "/DSourceExe=$Exe" "/DOutputDir=$Dist" 'installer\MandelbrotWallpaper.iss'
            if ($LASTEXITCODE -ne 0) { throw "Installer build failed. Exit code: $LASTEXITCODE." }
        } else {
            Write-Host 'Inno Setup 6 not found; installer build skipped.'
        }
    }
} finally {
    Pop-Location
}
