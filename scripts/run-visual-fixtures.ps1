[CmdletBinding()]
param(
    [string]$OutputDirectory,
    [string]$Fixture,
    [string]$BaselineDirectory,
    [ValidateRange(0, 255)]
    [int]$BaselineMaximumChannelError = 0,
    [ValidateRange(0.0, 1.0)]
    [double]$BaselineMaximumDifferingRatio = 0.0,
    [ValidateRange(-1.0, 1.0)]
    [double]$BaselineMinimumSsim = 1.0,
    [switch]$SkipSeamCheck,
    [switch]$SkipMutationCheck
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Root 'build-visual'
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = Join-Path $Root 'test_artifacts\visual'
}
$OutputDirectory = [System.IO.Path]::GetFullPath($OutputDirectory)

function Invoke-Native {
    param([string]$FailureMessage, [scriptblock]$Command)
    & $Command
    if ($LASTEXITCODE -ne 0) {
        throw "$FailureMessage Exit code: $LASTEXITCODE."
    }
}

Push-Location $Root
try {
    Invoke-Native 'Visual fixture configuration failed.' {
        cmake -S $Root -B $Build `
            -DMW_BUILD_TESTS=ON `
            -DMW_BUILD_VISUAL_FIXTURES=ON `
            -DMW_WARNINGS_AS_ERRORS=ON
    }
    Invoke-Native 'Visual fixture build failed.' {
        cmake --build $Build --config Release --parallel --target MandelbrotVisualFixtures
    }

    $ExecutableCandidates = @(
        (Join-Path $Build 'Release\MandelbrotVisualFixtures.exe'),
        (Join-Path $Build 'MandelbrotVisualFixtures.exe'),
        (Join-Path $Build 'MandelbrotVisualFixtures')
    )
    $Executable = $ExecutableCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ($null -eq $Executable) {
        throw "Visual fixture executable was not produced under $Build."
    }

    $Arguments = @('--output-dir', $OutputDirectory)
    if (-not [string]::IsNullOrWhiteSpace($Fixture)) {
        $Arguments += @('--fixture', $Fixture)
    }
    if (-not [string]::IsNullOrWhiteSpace($BaselineDirectory)) {
        $Arguments += @(
            '--baseline-dir', [System.IO.Path]::GetFullPath($BaselineDirectory),
            '--baseline-max-channel-error', $BaselineMaximumChannelError.ToString(),
            '--baseline-max-differing-ratio', $BaselineMaximumDifferingRatio.ToString([System.Globalization.CultureInfo]::InvariantCulture),
            '--baseline-minimum-ssim', $BaselineMinimumSsim.ToString([System.Globalization.CultureInfo]::InvariantCulture)
        )
    }
    if ($SkipSeamCheck) { $Arguments += '--no-seam-check' }
    if ($SkipMutationCheck) { $Arguments += '--no-mutation-check' }

    Invoke-Native 'Visual fixture validation failed.' {
        & $Executable @Arguments
    }
} finally {
    Pop-Location
}
