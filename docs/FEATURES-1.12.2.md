# Mandelbrot Live Wallpaper 1.12.2

## Deep Tricorn perturbation

The exact power-2 Tricorn/Mandelbar equation can now use the same deep-zoom precision selector as the classic quadratic Mandelbrot profile.

Implemented capabilities:

- Double-reference Tricorn orbits.
- 128–512-bit fixed-point Tricorn reference orbits.
- Conjugate perturbation in OpenGL and Direct3D 11.
- Tricorn-aware split-float direct rendering.
- Explicit profile gating that rejects higher Multicorn powers and transformed conjugate equations.
- Perturbation-delta guards with split-float GPU fallback.
- Deterministic CPU reference refresh when a sample exceeds the bounded perturbation region.
- Per-tile high-resolution reference generation through the existing tile-centred camera path.

The supported recurrence is the exact parameter-plane profile:

```text
z(n+1) = conjugate(z(n))^2 + c
z(0) = 0
```

Other anti-holomorphic equations continue through the safe direct precision path until independently validated perturbation recurrences are added.
