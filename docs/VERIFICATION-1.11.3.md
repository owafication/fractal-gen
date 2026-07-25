# Verification — 1.11.3

## Automated checks available in the packaging environment

- Source regression script verifies the Direct3D 11 device, shader, texture, staging-readback and fallback integration markers.
- Core library builds and tests remain compiler-independent and are run with warnings treated as errors under GCC and Clang.
- Release metadata, installer metadata and archive integrity are checked.

## Windows verification still required

This environment does not include the Windows SDK runtime, a Win32 desktop, Direct3D hardware or an OpenGL driver. Final Windows verification should therefore cover:

- Preview and live wallpaper start with Direct3D 11 and identify the active adapter in diagnostics.
- OpenGL is used automatically when Direct3D 11 initialisation is forced to fail.
- Direct3D 11 and OpenGL output have matching orientation, camera framing, palettes and equation behaviour.
- Split-float and both perturbation modes remain stable at deep zoom.
- Direct3D device removal or display-driver reset produces an actionable error and the existing recovery path can restart rendering.
- High-resolution Direct3D 11 export crosses multiple tiles without seams and PNG/TIFF/BMP output has correct orientation.
- Explicit OpenGL and CPU selections still complete successfully.

Passing core tests does not prove Win32, HLSL compilation, driver compatibility or GPU output correctness.
