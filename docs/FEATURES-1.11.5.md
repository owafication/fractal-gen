# Mandelbrot Live Wallpaper 1.11.5

## Preset naming

- The Preset Library popup now contains a bounded **Name for Save Preview as New** text box.
- **Save Preview as New** uses that value directly.
- Preset-name editing, **Save Preset As**, and **Send Preview to Desktop** were removed from the preview hover menu.
- **Save Preset As** was removed from the Quick Controller.

## Static launch behaviour

Each process starts with preview zoom, preview colour cycling, desktop zoom, and desktop colour cycling stopped. Preset animation modes and speeds are not changed. The user starts each runtime animation explicitly from the hover controls.

## Structured Automatic Journey

Custom journey rows use:

```text
centreX,centreY,scale,transitionSeconds,holdSeconds
```

Rows are followed exactly in order. The renderer transitions from its current camera to the row coordinate, holds there, then transitions directly to the next row. Custom coordinates are not redirected and fallback destinations are not inserted. Automatically generated journeys retain boundary-safety selection.
