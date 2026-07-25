# Mandelbrot Live Wallpaper 1.10.3 — High-resolution output

## Preview-tab entry point

The Preview tab includes **Render Hi-Res and Save...**. It opens a responsive, per-monitor-DPI-aware dialog using the same keyboard, scrolling and remembered-placement support as the other application dialogs.

The dialog accepts:

- coordinates in `centreX,centreY,scale` form;
- independent output width and height;
- DPI metadata;
- PNG, TIFF or BMP output.

No arbitrary application-level resolution or DPI maximum is imposed. Inputs are positive 32-bit values because Windows Imaging Component and the application process use 32-bit image dimensions. Actual completion remains subject to encoder support, available disk space, one-scanline memory and operating-system limits.

## Background rendering

Rendering runs on a dedicated worker thread. The dialog remains responsive and reports progress from 0–100%. **Cancel Render** sets an atomic cancellation request checked between tiles and at bounded intervals within each tile. A cancelled or failed temporary output is deleted and cannot be previewed or saved.

Closing the dialog during a render requests cancellation, joins the worker, removes temporary output and then closes. This prevents worker access to destroyed window state.

## Tiled memory model

`src/Core/StillImageRenderer.*` implements the portable still-frame renderer. It:

- computes 256-pixel horizontal tiles;
- assembles and emits one top-down scanline at a time;
- never allocates a full-resolution pixel frame;
- retains only one full output row, one computation tile and a bounded preview;
- checks cancellation at row, tile and 16-pixel intervals;
- exposes monotonic row progress and peak working-pixel statistics.

The output encoder receives rows sequentially. This means memory grows with output width, not width multiplied by height. A very wide image still requires one encoded scanline, which is an unavoidable constraint of the selected WIC encoders.

## PNG, TIFF and BMP

The Windows dialog uses Windows Imaging Component rather than adding a third-party image library. PNG, TIFF and BMP are encoded as 24-bit BGR output with the selected DPI stored through `IWICBitmapFrameEncode::SetResolution`.

The completed image is written to a private temporary file. **Save As...** copies that already-encoded file to the selected location without re-rendering. The temporary file is removed when it is replaced or when the dialog closes.

## Preview behavior

The renderer builds a bounded top-down preview with a maximum size of 1600×1000 while rows are produced. **Preview** displays this bounded image rather than decoding or allocating the full output. This keeps preview memory independent of very large export dimensions.

## Compatibility

The still renderer uses the current preview snapshot: compensated camera coordinates, equation, iterations, anti-aliasing level, palette, custom palette, colour offset, brightness, contrast, saturation, interior colour, orbit-trap/distance/Newton colouring, glow contribution and depth shading. Animated equation coefficients are frozen at one phase when Render is pressed. It is CPU-rendered and does not require the live OpenGL preview to be available.
