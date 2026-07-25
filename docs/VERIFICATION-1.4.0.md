# Verification record for 1.4.0

## Ran in the packaging environment

- GCC 14.2 C++20 core configure/build with warnings treated as errors.
- Clang 17 C++20 core configure/build with warnings treated as errors.
- Core test suite under both compilers.
- Clang AddressSanitizer and UndefinedBehaviorSanitizer core build/test.
- `scripts/verify-source.py` source-structure, offline-policy, manifest, and feature-marker checks.
- ZIP integrity test after packaging.

All commands above completed successfully in the packaging environment.

## Not verified here

The packaging environment is Linux and cannot execute or compile the Win32/WGL application target. The following remain Windows runtime checks:

- MSVC compilation of the new precision dialog and OpenGL renderer.
- Native float64 shader compilation on supported drivers.
- Floating-point reference-orbit texture upload.
- Visual correctness and stability of split and perturbation modes on Windows 10/11 GPUs.
- Preview, live wallpaper, static capture, and multi-monitor behaviour at deep scales.
- Performance of 128-, 256-, and 512-bit reference-orbit generation.

Use `scripts/build-release.ps1` on Windows, then follow the deep-zoom runtime checklist in `docs/TESTING.md`.
