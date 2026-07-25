# Mandelbrot Live Wallpaper 1.1.0 feature update

## Custom palette editor

- **Edit Palette…** opens a modal Win32 palette editor.
- Colour stops can be added, edited with the Windows colour picker, removed, and reordered.
- The stop list is persisted in preset JSON as `customPaletteColours`.
- The renderer converts the list to a repeating 1D GPU texture, retaining one texel per stop when hardware limits allow, and interpolates continuously between adjacent stops.
- Imported palettes are data-only, validated, clamped to valid RGBA values, and safety-bounded to 4096 stops.

## Maximum iterations input

**Open Settings** now includes a numeric **Maximum iterations** field. Valid values are normalised to 32–4096 and applied to the active preset and performance settings. Higher values increase boundary calculation depth but do not increase coordinate precision.

## Static capture and slideshow

- **Set Static Image** renders the current preview camera and colour offset at 100% virtual-desktop resolution.
- The result is stored as a local 32-bit BMP under the application data directory.
- After capture, the wallpaper OpenGL context is released, the preview is frozen, and the wallpaper is displayed through GDI, eliminating continuous fractal rendering.
- Settings can cycle through saved captures at a validated interval of 10–86400 seconds.
- Image history is validated, bounded to 256 local paths, and persisted in settings schema version 2.

## Safer automatic zoom targets

Automatic destinations are checked with Mandelbrot escape calculations. A destination must escape slowly enough to indicate boundary detail; interior black points and featureless fast-escaping points are rejected. The controller searches nearby rings for a suitable point and uses a known boundary-rich fallback only when no local candidate is found. Manual navigation is unchanged.

## Compatibility

Settings schema version 1 is migrated to version 2 on load. Existing presets without `customPaletteColours` continue to use their selected built-in palette.
