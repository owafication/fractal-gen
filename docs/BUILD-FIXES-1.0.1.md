# Build fixes in source package 1.0.1

- Fixed MSVC type deduction errors in `OpenGLRenderer.cpp` by explicitly converting Win32 `LONG` rectangle dimensions to `int` before `std::max`.
- Replaced lossy `std::wstring` to `std::string` monitor-name conversions with UTF-8 conversion.
- Fixed edit-control class detection to compare string contents instead of string-literal addresses.
- Replaced deprecated `_wgetenv` use with `_wdupenv_s` and explicit memory release.
- Made `scripts/build-release.ps1` stop immediately when configure, build, or tests return a non-zero exit code.

Verification performed in the packaging environment:

- `python3 scripts/verify-source.py` passed.
- CMake configured the platform-independent core with GCC 14 and warnings as errors.
- Core library and test executable built successfully.
- `ctest` passed 1/1 tests.

The Windows GUI target requires compilation and runtime verification on Windows with MSVC and a Windows SDK.
