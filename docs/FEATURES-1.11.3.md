# Mandelbrot Live Wallpaper 1.11.3

## Direct3D 11 rendering

- Added a native Direct3D 11/HLSL renderer for the preview, live desktop wallpaper and high-resolution still exporter.
- Direct3D 11 is attempted first on Windows. If device creation or shader compilation fails, the application automatically falls back to the existing OpenGL renderer.
- The GPU facade preserves the existing render, resize, capture, diagnostics, precision-capability and shutdown contracts.
- Direct3D 11 supports float32, split high/low float, double-reference perturbation and arbitrary-precision CPU-reference perturbation. Native shader float64 remains an OpenGL capability where the driver exposes it.
- Device removal and presentation failures are surfaced instead of silently leaving a stale frame.

## High-resolution export

The renderer list now provides:

1. GPU Direct3D 11 tiled — default.
2. GPU OpenGL tiled — compatibility fallback.
3. CPU scanline tiled — final compatibility fallback.

Both GPU backends retain the bounded overlapping tile-band pipeline introduced in 1.11.2, including fixed animation phase, resolution-aware iteration depth, bounded working memory and row-streamed WIC encoding.

## Build integration

The Windows target now links `d3d11`, `dxgi` and `d3dcompiler` in addition to the existing OpenGL and Windows libraries. No external DirectX SDK package is required beyond a current Windows SDK.
