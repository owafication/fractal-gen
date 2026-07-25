# Mandelbrot Live Wallpaper 1.9.0 — Advanced Equation and Visual Rendering

## Equation engine

Version 1.9.0 expands the data-only equation model without accepting arbitrary shader source or executable expressions.

Escape-time recurrences support:

- Integer powers from 1 through 12.
- Independent real and imaginary absolute-value transforms.
- Complex conjugation and real/imaginary component swapping.
- Complex `sin`, `cos`, `exp`, and guarded `log` transforms.
- Complex coefficients for the powered, linear, parameter, constant, reciprocal, and iteration-dependent terms.
- Rational terms of the form `lambda / z^q`, with `q` from 0 through 12.
- Initial `z0` modes: zero, fixed value, parameter value, or a supported-family critical point.
- Julia mode with a fixed complex parameter.
- Configurable escape radius.
- Bounded coefficient animation and a bounded randomise action.

For `A*z^p + c`, critical-point mode uses `z0 = 0`, which is the critical point for the Multibrot parameter family. For `A*z^p + lambda/z^q`, it uses the principal solution of `z^(p+q) = q*lambda/(p*A)`. Other branches remain available through fixed `z0` values.

## Newton convergence mode

Newton mode renders convergence basins for `z^degree = target`, with:

- Degree 2 through 12.
- Complex target and relaxation values.
- Configurable convergence tolerance.
- Root-basin colouring and iteration-based shading.

## Colouring and finish

The renderer adds selectable:

- Smooth escape colouring.
- Point, cross, or circle orbit traps.
- Analytic-polynomial distance estimation with safe fallback when transforms make the derivative unsupported.
- Newton basin colouring.
- Depth shading.
- A screen-space glow pass using a bounded 3x3 bloom kernel.

## Equation presets

Named equation presets are stored independently from wallpaper presets. The equation editor can load examples, save new equation presets, update/delete saved entries, and randomise a bounded equation. Imported wallpaper presets continue to contain structured numeric and enum data only.

## Controls

- Preview overlay controls are persistent rather than temporary.
- The vertical navigation rail now contains Preview, Desktop, Status, Settings, Palette, Quick, and Equations buttons.
- The Quick Controller has a single Play/Stop toggle. The separate pause and redundant stop buttons were removed.

## Precision compatibility

Float32 and native float64 paths support the advanced equation operations. Split-float and perturbation deep-zoom modes remain restricted to compatible quadratic parameter maps. Automatic precision falls back safely rather than applying an invalid perturbation formula.
