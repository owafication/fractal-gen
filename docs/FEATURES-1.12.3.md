# Mandelbrot Live Wallpaper 1.12.3

## Adjustable bloom

The equation editor now separates bloom intensity from its extraction and blur shape:

- Bloom intensity: 0–4
- Brightness threshold: 0–4
- Soft knee: 0–2
- Radius: 0–16 render pixels

Older presets retain the previous bloom behaviour through compatibility defaults: threshold `0.22`, hard knee `0`, and radius `1`.

OpenGL and Direct3D 11 now use two bounded separable passes instead of a fixed 3×3 square sample. The first pass extracts bright pixels while blurring horizontally; the second blurs vertically and composites against the retained fractal frame.

## Seam-safe high-resolution export

GPU tiled export calculates overlap from the selected bloom radius and anti-alias reconstruction allowance. Each expanded tile is post-processed before only its valid central area is copied into the streamed output band. This avoids using a fixed one-pixel overlap for larger bloom kernels.

## Camera rotation

Presets now store view rotation in degrees. Rotation is available in **Settings → Graphics** and is applied consistently to:

- Preview rendering
- Live desktop rendering
- Direct3D 11 and OpenGL paths
- CPU still rendering and static fallback
- GPU tiled high-resolution export
- Preview drag panning
- Mouse-wheel zoom anchoring
- Visual-change detection

The coordinate-string contract remains `centreX,centreY,scale`; rotation is stored separately so existing coordinate links and presets remain compatible.
