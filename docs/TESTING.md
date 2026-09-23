# Testing

**Status:** Current compatibility entry point. Canonical validation policy is `project_docs/VALIDATION_AND_EVIDENCE.md`; native Windows execution is `docs/testing/WINDOWS-RELEASE-VALIDATION.md`; visual testing is `docs/testing/VISUAL-REGRESSION.md`.

## Available checks

```powershell
python .\scripts\verify-source.py
.\scripts\validate-windows-release.ps1
```

The Windows workflow resolves the canonical Visual Studio configure/build/test presets, checks executable metadata and the portable package, and runs startup/shutdown against isolated application data. `tests/CoreTests.cpp` covers core math, models/settings, bounded data handling, animation, deep zoom, adaptive policy, still rendering and feature regressions. `tests/PathsTests.cpp` verifies validation-state isolation. Source markers and portable tests still do not prove Win32, GPU, WIC, WorkerW or interactive UI behaviour.

## Required release layers

1. Version-surface scan.
2. Native x64 MSVC configure/build/resource/link.
3. Core CTest from that build.
4. Windows startup/dialog/shutdown smoke test.
5. Static/live/slideshow/journey desktop-mode matrix.
6. D3D11/OpenGL/CPU fallback and precision checks.
7. Persistence/migration/cancellation checks.
8. Production-renderer visual and tiled-seam fixtures.
9. Portable ZIP, installer and upgrade validation.

## Current evidence limit

Historical 1.13.0/1.13.1 documents report prior portable compiler/sanitizer passes. BR-20260727-03 reran GNU warnings-as-errors compilation and CTest 3/3, including path isolation. Native MSVC, Windows runtime, GPU, desktop-mode, installer and upgrade evidence remain unproven.
