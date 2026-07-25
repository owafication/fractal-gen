# Mandelbrot Live Wallpaper 1.3.0 equation editor

## Safe recurrence model

The renderer now supports a bounded, data-only complex recurrence:

```text
z(n+1) = A*z(n)^2 + B*z(n) + C*c + D
```

`A`, `B`, `C`, and `D` each have editable real and imaginary components. Values are validated as finite numbers from -8 to 8. Presets cannot contain executable expressions, scripts, or arbitrary shader source.

This model includes the requested examples:

- `z = z + 1.2c`: `A=0`, `B=1`, `C=1.2`, `D=0`.
- `z = 1.2 + z + c`: `A=0`, `B=1`, `C=1`, `D=1.2`.

It also includes examples for classic Mandelbrot, scaled `c`, a constant offset, scaled `z²`, a linear `z` term, `-c`, and Burning Ship-style absolute real/imaginary components.

## UI

The main window has an **Equation** row that opens a modal equation editor. The editor provides:

- a live textual equation summary and live update of the main preview;
- real and imaginary inputs for all four coefficients;
- example formulas;
- optional `|Re(z)|` and `|Im(z)|` transforms;
- a one-click reset to the classic Mandelbrot recurrence.

## Rendering and persistence

The equation is part of each fractal preset and is included in preset import/export and local settings. It is applied consistently to:

- the live preview;
- live wallpaper rendering;
- span, mirror, and independent monitor regions;
- static image capture;
- the bounded CPU fallback;
- automatic target selection.

The settings schema is version 4. Versions 1–3 remain accepted and migrate to the classic equation when no equation data exists.
