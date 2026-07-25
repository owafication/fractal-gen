# Adaptive rendering and resource protection — 1.7.0

## Objective

Prevent the live wallpaper from continuing expensive fractal evaluation when the process is overloaded or when animation changes are too small to affect the displayed image.

## Adaptive resource protection

`AdaptivePerformanceController` evaluates bounded, sustained conditions rather than reacting to one slow frame:

- minimum rendered FPS, used as the portable GPU-saturation signal;
- process CPU percentage, normalised across logical processors;
- process working-set memory;
- separate sustain periods for FPS, CPU and memory;
- a stable CPU/memory cooldown before one resume probe.

The controller pauses the live wallpaper and preview together. A repeated overload pauses them again instead of entering an uncontrolled restart loop. All triggers, thresholds and durations are configurable through **Settings → Resource protection**.

Direct GPU utilisation is not read through vendor-specific APIs. Sustained low FPS is used as the cross-vendor indication that the selected resolution, iteration count, precision mode or anti-aliasing level is too expensive.

## Invisible-frame suppression

`VisibleChangeDetector` compares each requested frame with the last frame actually submitted to the renderer. It converts camera pan and logarithmic zoom changes into screen pixels and compares palette movement using a circular colour-offset distance.

When neither movement exceeds the configured visible threshold, the app:

- advances only the lightweight animation state;
- does not invoke the Mandelbrot shader;
- retains the last presented image;
- accumulates sub-threshold movement against the last rendered frame;
- resumes equation rendering automatically once the accumulated change becomes visible.

A visual-content revision includes the equation, palette, colour controls, iteration count, render scale, anti-aliasing and precision settings, so an explicit setting change always invalidates the cached frame.

## Defaults

- Adaptive protection: enabled.
- Low-FPS pause: below 8 FPS for 5 seconds, after renderer warm-up.
- High-CPU pause: above 65% of total process CPU capacity for 5 seconds.
- High-memory pause: above 2048 MB working set for 5 seconds.
- Resume cooldown: 8 seconds of stable CPU and memory.
- Invisible-frame suppression: enabled.
- Camera threshold: 0.25 screen pixels.
- Palette threshold: 0.001 of one palette cycle.
