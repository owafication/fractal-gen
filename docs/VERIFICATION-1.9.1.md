# Verification — 1.9.1

## Fix

Removed a stale `previewOverlayHideAt_` assignment from `AppWindow::ShowPreviewOverlay`.
The hover/quick controls are intended to remain persistent, and no deadline member or
hide timer is used by the 1.9 UI. The stale reference caused MSVC error C2065.

## Checks performed in the packaging environment

- GCC C++20 core build with warnings treated as errors.
- Clang C++20 core build with warnings treated as errors.
- Core test suite with both compilers.
- AddressSanitizer and UndefinedBehaviorSanitizer core test run.
- Source-only archive scan and ZIP integrity check.

The Win32 GUI and OpenGL executable require a Windows/MSVC build for final runtime verification.
