# Verification — 1.13.0

## Scope

Desktop-mode selection, default startup mode, shared image-output settings, WIC image encoding/decoding, Journey Settings access, scrollable equation dropdowns, independent editor windows, and live Palette Editor preview.

## Automated verification

Passed in the available environment:

- Source regression verifier covering the new model, persistence, UI, codec and editor contracts
- Settings round-trip fixtures for default desktop mode, output format and compression/quality
- Validation fixtures for schema version 9 and compression/quality clamping
- GCC 14 Release build with warnings treated as errors
- GCC core tests
- Clang 17 Release build with warnings treated as errors
- Clang core tests
- Clang AddressSanitizer and UndefinedBehaviorSanitizer core tests
- Clean-package verification excluding build output
- ZIP integrity validation

## Windows-specific review

Static source inspection covers:

- WIC PNG/JPEG/TIFF/BMP encoder and decoder wiring
- Default folder use by static captures and high-resolution Save As
- Desktop selector command routing
- Removal of the Preview Animation control
- Independent tool-window styles for Settings, Equation, Palette, and Journey dialogs
- Live Palette Editor callback wiring
- Vertical-scroll styles on Equation Editor dropdowns

## Verification limit

This environment does not provide the Windows SDK, MSBuild, a Win32 desktop, or GPU runtime. Native MSVC compilation and interactive validation of the new Windows controls and WIC encoder behaviour remain to be run on Windows with `scripts/build-release.ps1`.
