# Verification record for 1.6.1

## Implemented correction

- Corrected the Win32/MSVC `LONG` and `int` type mismatch in `SlideshowDialog.cpp` by explicitly converting the measured text width before calling `std::max`.
- Added a source-verification regression guard for the exact ambiguous expression.
- Updated build, installer, executable resource, and package metadata to version 1.6.1.

## Verification performed in the packaging environment

- Source structure and offline-policy verification.
- GCC C++20 core build with warnings treated as errors.
- Clang C++20 core build with warnings treated as errors.
- Core test suite through CTest.
- AddressSanitizer and UndefinedBehaviorSanitizer core build and tests.
- ZIP integrity and source-only archive inspection.

## Windows verification limit

The packaging environment cannot compile or run the Win32 UI. The reported MSVC error is directly addressed at the failing source line, but the complete Windows executable must be rebuilt on Windows to verify the exact Visual Studio 18 toolchain and SDK combination.
