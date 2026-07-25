# Verification record for 1.8.0

## Verified in the build environment

- GCC core build with warnings treated as errors.
- Clang core build with warnings treated as errors.
- Core unit tests.
- AddressSanitizer and UndefinedBehaviorSanitizer core test run.
- Source structure and offline-policy verification.
- Source-only archive integrity and compiled-artifact exclusion.
- Static implementation checks for vertical navigation, one-line coordinates, Preset Library, Quick Controller, settings grouping and tray/hover entry points.

## Windows verification still required

The Win32 UI, modeless Quick Controller, Settings layout, WGL rendering, WorkerW desktop attachment and system-tray interaction must be compiled and exercised on Windows 10/11. Portable checks cannot prove those platform-specific behaviours.
