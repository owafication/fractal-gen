# Mandelbrot Live Wallpaper 1.12.4

## Fractal Scout

The Preview tab now includes **Fractal Scout...**, a bounded location-discovery workflow that uses the current equation and visual style.

### Search workflow

1. Open Fractal Scout from the Preview tab.
2. Choose Fast, Balanced, or Detailed search quality.
3. Scout scores a deterministic candidate pool around the current camera.
4. Select a ranked result to inspect its thumbnail, score metrics, and coordinate string.
5. Use **Refine Around Selected** to search progressively deeper around that result.
6. Choose **Use in Preview** to stage the camera without saving, or **Use & Save as New...** to enter the existing preset-save workflow.

### Candidate scoring

Candidates are ranked using:

- escaped/interior boundary mix;
- normalised iteration variance;
- horizontal and vertical edge density;
- escaped-orbit detail;
- optional horizontal or rotational symmetry similarity.

The same inputs produce the same candidate cameras, scores, order, and CPU-rendered thumbnails.

### Resource and data safety

- Candidate pools, sample grids, iterations, thumbnail dimensions, and retained BGRA memory have hard ceilings.
- Search and thumbnail rendering run on a cancellable worker thread.
- Cancellation discards partial results.
- Candidate presets have no saved ID and do not modify settings.
- Existing preset identity and style are preserved when applying a result to the preview.
- Persistence occurs only after the explicit **Use & Save as New...** action and normal preset-name confirmation.
