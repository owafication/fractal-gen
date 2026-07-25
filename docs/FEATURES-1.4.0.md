# Mandelbrot Live Wallpaper 1.4.0 deep zoom

## Added

- A **Deep Zoom Precision** dialog reachable from Settings.
- Directly selectable float32, native GPU float64, split high/low float, double-reference perturbation, and arbitrary-reference perturbation modes.
- Independent toggles controlling which strategies Automatic mode may use.
- Optional safe fallback when the selected strategy is unavailable.
- 128-, 256-, and 512-bit CPU fixed-precision reference-orbit choices.
- Compensated high/low camera centres for retaining small pan offsets.
- Per-frame diagnostics showing requested and active precision plus detected capabilities.
- Precision settings persisted in schema version 5.

## Boundaries

- GPU float64 depends on the OpenGL driver and shader support.
- Perturbation requires floating-point texture support and an analytic recurrence.
- Absolute-value transforms are not differentiated analytically and therefore do not use perturbation.
- The arbitrary-precision CPU orbit is uploaded as four float components per real and imaginary value; GPU delta arithmetic remains bounded and is not an unlimited-zoom claim.
- The preset maximum zoom remains capped at `1e30`.

## Failure behaviour

With safe fallback enabled, an unavailable mode resolves to another enabled strategy. With fallback disabled, rendering stops that frame and reports a clear precision error. No renderer restart loop is introduced.
