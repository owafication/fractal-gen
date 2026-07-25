# Mandelbrot Live Wallpaper 1.5.0 UI and monitor update

## Responsive control flow

The control window is divided into three pages so controls remain usable on smaller work areas:

1. **Preview & Preset** edits the staged fractal state and updates the preview immediately.
2. **Wallpaper & Monitors** controls the desktop renderer and explicitly applies the staged preview.
3. **Diagnostics** displays renderer, monitor and precision details without consuming space on action pages.

The window starts centred within the current Windows work area, has a 900×640 minimum size, and uses compact row spacing when vertical space is limited. The separate Settings window uses a two-column 780×590 layout.

## Direct numeric controls

Iterations, maximum frame rate and render scale have editable text fields beside their sliders. Brightness, contrast, saturation and colour offset are on the main preview page with paired text fields so their effect is visible before applying it to the wallpaper. Numeric inputs are clamped to validated application limits.

## Preview versus wallpaper state

Preview edits are staged. The status line reports when preview changes have not yet been applied. **Apply Preview as Live Wallpaper** starts or replaces the live desktop renderer. **Capture Preview as Static** creates a full virtual-desktop image and releases the wallpaper GPU. A static wallpaper does not disable the visible editor preview; the preview pauses only when the control window is hidden or minimised.

## Multi-monitor corrections

Span mode uses one render region covering the entire virtual desktop. After WorkerW/Progman attachment, virtual-desktop screen coordinates are mapped into the host window's client coordinates before the child window is positioned. This covers layouts with negative X/Y coordinates, portrait displays and a primary monitor that is not the top-left display.

Independent assignments are enabled only in Independent mode. Selecting a monitor loads its stored assignment. Applying an assignment rebuilds live per-monitor animation controllers; while stopped or showing a saved static image, it is stored for the next live wallpaper start.

## Automatic Journey safety and variety

The journey has twelve seed locations and retains at least eight distinct validated targets for the standard Mandelbrot recurrence. Every accepted target must:

- have an escaping centre point rather than a black interior centre;
- contain a sampled mixture of interior and exterior pixels;
- contain meaningful iteration variation around the final viewport; and
- be sufficiently separated from other journey targets.

The controller pans during zoom-in, holds briefly, pans toward the next destination while zooming out, and repeats.

## Precision descriptions

The precision dialog explains Automatic, GPU float32, native GPU float64, split high/low float, double-reference perturbation and arbitrary-reference perturbation. It states the expected compatibility, useful zoom range and setup/performance cost, while retaining independent enable/disable toggles and safe fallback.
