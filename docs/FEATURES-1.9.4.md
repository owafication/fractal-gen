# Mandelbrot Live Wallpaper 1.9.4

## Persistent preview hover menu

The preview now has a visible grouped control panel rendered above the OpenGL child window. It contains:

- Play/stop zoom motion
- Play/stop colour cycling
- Apply live wallpaper
- Capture/apply static wallpaper
- Start slideshow wallpaper
- Stop desktop wallpaper
- Jump to coordinates
- Open Quick Controls
- Current X, Y, scale and desktop state

The overlay is explicitly restored to the top of the sibling-window z-order after every resize or page layout so WGL rendering cannot cover it.
