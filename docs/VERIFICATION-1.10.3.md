# Verification — 1.10.3

## Automated portable checks

The core still renderer is included in `MandelbrotCore` and is covered by `tests/CoreTests.cpp`.

The test verifies:

- top-down sequential row delivery;
- exact requested row and pixel counts;
- monotonic progress ending at the requested height;
- bounded preview dimensions and complete preview storage;
- preserved computation tile width;
- peak full-resolution working memory limited to one row plus one tile;
- no full-resolution frame allocation for a multi-row image;
- deterministic cancellation behavior.

The existing source verifier additionally checks:

- the Preview-tab button and command handler;
- PNG, TIFF and BMP WIC encoder GUIDs;
- DPI metadata support;
- worker-thread, progress and cancellation wiring;
- tiled renderer integration;
- bounded preview and Save As flow;
- Windows Imaging Component and COM link libraries;
- release, installer and manifest version consistency.

## Verification limitation

Linux GCC, Clang and sanitizer builds exercise the portable renderer and all existing core tests. They do not compile or execute the Win32 dialog or Windows Imaging Component encoder. Final confirmation of WIC PNG/TIFF/BMP output, progress UI, cancellation latency, DPI metadata and Save As behavior requires the Windows MSVC build and runtime test.
