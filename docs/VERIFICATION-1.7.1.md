# Verification record for 1.7.1

## Implemented and inspected

- Preview hover controls and coordinate display.
- Independent zoom-motion and colour-cycling controls.
- Live, static and slideshow desktop actions from the preview overlay.
- Coordinate jump prompt using `centreX,centreY,scale`.
- Per-preset Automatic Journey waypoint text with travel/hold timing.
- JSON round-trip persistence and validation bounds.
- Motion state retention when wallpaper animations are rebuilt.
- Source-only packaging and version metadata updated to 1.7.1.

## Commands run

- GCC CMake configure/build with `MW_WARNINGS_AS_ERRORS=ON`.
- GCC `ctest --output-on-failure`.
- Clang CMake configure/build with `MW_WARNINGS_AS_ERRORS=ON`.
- Clang `ctest --output-on-failure`.
- Clang AddressSanitizer and UndefinedBehaviorSanitizer build/test.
- `python3 scripts/verify-source.py`.
- ZIP integrity and source-only content inspection.

## Results

All available portable core checks passed.

## Not verified here

The packaging environment is not Windows and has no MSVC/Windows SDK. The Win32 application, preview overlay z-order/input behaviour, WGL rendering, WorkerW integration and desktop actions remain to be compiled and runtime-tested on Windows.
