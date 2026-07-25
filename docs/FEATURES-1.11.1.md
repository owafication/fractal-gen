# Mandelbrot Live Wallpaper 1.11.1

## Split preview and desktop animation controls

The preview hover menu now exposes four independently labelled runtime controls:

- Start/Stop Preview Zoom
- Start/Stop Preview Colours
- Start/Stop Desktop Zoom
- Start/Stop Desktop Colours

Preview controls affect only the preview animation. Desktop controls affect only the live wallpaper renderer and retain their selected runtime state when a live wallpaper is started or replaced.

## Send the visible preview to the desktop

**Send Preview to Desktop** creates a transient live-wallpaper snapshot from the exact camera and colour frame currently visible in the preview, together with the current equation, palette and rendering settings. It sends that snapshot to all active desktop displays without creating or overwriting a saved preset. Desktop zoom and colour controls determine whether motion continues after the snapshot is applied.

**Apply Settings Live** remains available for starting the desktop from the current editor values rather than the currently animated preview frame.

## Hover coordinate display removed

The hover menu status line now reports only desktop state and whether preview changes are pending. It no longer displays centre X, centre Y or scale. **Copy Coordinates** and **Jump to Coordinates…** remain available.
