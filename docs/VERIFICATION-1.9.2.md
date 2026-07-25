# Verification — 1.9.2

## Fix

The Windows executable and core tests completed successfully, but the release script
failed after creating the portable ZIP because a PowerShell pipeline containing exactly
one Inno Setup path was treated as a scalar under `Set-StrictMode`. The script no longer
uses `.Count` or indexed access on that pipeline result. It selects the first matching
compiler path directly and checks it against `$null`.

## Expected Windows result

- The portable ZIP is created in `dist`.
- If Inno Setup 6 is installed, the installer is built.
- If Inno Setup 6 is absent, the script reports that the installer was skipped and exits successfully.

## Checks performed in the packaging environment

- GCC C++20 core build with warnings treated as errors.
- Clang C++20 core build with warnings treated as errors.
- Core test suite with both compilers.
- AddressSanitizer and UndefinedBehaviorSanitizer core test run.
- Source verification including the PowerShell packaging regression.
- Source-only archive and ZIP-integrity checks.

The full Windows release script should be rerun on Windows to confirm the final packaging path.
