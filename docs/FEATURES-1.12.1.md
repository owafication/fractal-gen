# Mandelbrot Live Wallpaper 1.12.1

## Tricorn distance estimation

The exact power-2 Tricorn/Mandelbar parameter-map template now has a dedicated non-analytic distance estimator. The renderer tracks derivatives with respect to both real pixel axes and uses the largest singular value of the resulting real 2×2 Jacobian as the local stretch.

Support is deliberately restricted to the validated recurrence:

```text
z(n+1) = conjugate(z(n))² + c
z(0) = 0
```

Higher Multicorn powers, Julia variants, animated coefficients, reciprocal terms, and transformed conjugate formulas continue to render through the safe direct path without a fabricated distance value.

## Independent edge lighting and bloom

Equation appearance now exposes two separate controls:

- **Bloom 0–4** — existing screen-space GPU blur/bloom.
- **Edge light 0–4** — narrow mathematical boundary lighting driven by distance estimation or, for orbit-trap colouring, the selected trap distance.

Older custom presets did not contain an edge-light field. During loading, their old glow value is copied to edge lighting once so the previous combined appearance is retained. Subsequent saves persist both values independently.

## Tuned Tricorn scene

`Tricorn Cyan Fire Ring` now uses:

- distance-estimation colouring;
- edge-light strength 0.65;
- bloom strength 0.30;
- the existing cyan-fire palette, stripe average, smooth palette phase, 28× repetition, and 4× anti-aliasing.
