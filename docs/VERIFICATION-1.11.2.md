# Verification — 1.11.2

This document records checks executed in the packaging environment. Native Win32/OpenGL runtime verification is listed separately because it was not available here.

## Source regression coverage

`scripts/verify-source.py` checks that:

- the GPU exporter queries the device render limit and uses bounded overlapping tiles;
- full-resolution GPU window/texture allocation and full-frame readback code are absent;
- completed tile bands are streamed row-by-row to WIC;
- tile camera placement uses the shared compensated deep-zoom helper;
- animated coefficient time is fixed across the complete still;
- automatic resolution-aware iteration scaling is connected to both CPU and GPU exporters;
- core tests cover quality scaling, the 4096 cap, disabling automatic scaling, and top-down tile-camera quadrants;
- 1.11.2 release metadata is consistent.

## Automated core coverage

The core suite verifies:

- deep, high-resolution requests receive an iteration budget above the preset minimum;
- the automatic budget remains at or below 4096;
- disabling scaling preserves the preset iteration count;
- tile cameras preserve horizontal extent, top-down vertical orientation and tile-relative scale;
- the existing CPU scanline renderer remains bounded, cancellable and row ordered.

## Packaging-environment results

Executed successfully after the final patch:

```text
python3 scripts/verify-source.py
GCC 14 Release core build with warnings treated as errors
CTest: MandelbrotCoreTests passed under GCC
Clang 17 Release core build with warnings treated as errors
CTest: MandelbrotCoreTests passed under Clang
```

## Platform limit

The packaging environment cannot compile or run the Win32 GUI and OpenGL exporter. Final Windows verification should render a multi-tile image with visible glow, compare adjacent tile boundaries at 100% zoom, cancel during a later tile band, test PNG/TIFF/BMP output, and confirm GPU memory remains bounded while output dimensions exceed the reported single-texture limit.
