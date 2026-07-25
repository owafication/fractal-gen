# Mandelbrot Live Wallpaper 1.2.0 feature update

## Palette editor

- The colour list is owner-drawn and shows a filled colour swatch beside each hexadecimal RGB value.
- Named palette presets are stored separately from fractal presets in the versioned local settings file.
- The editor can load, create, rename/update, and delete up to 256 saved palettes.
- Each saved palette contains between 2 and 4096 validated colour stops.
- Cancelling the palette editor discards palette-library edits; choosing OK commits both the current palette and the saved palette library.

## Automatic Journey

Automatic Journey now uses an explicit repeating sequence:

1. Pan while logarithmically zooming from a wide camera to an interesting boundary target.
2. Hold briefly at the deepest target view.
3. Pan toward the next target while logarithmically zooming back out.
4. Continue the pan while zooming into the next target.

Targets continue to pass the existing Mandelbrot boundary-interest check, so automatic travel does not intentionally finish on a solid black interior point.

## Settings schema

The settings schema is version 3. Versions 1 and 2 remain accepted and migrate automatically when settings are next saved.
