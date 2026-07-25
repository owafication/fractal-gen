# Mandelbrot Live Wallpaper 1.11.7

## Preview hover menu removed

The control panel previously drawn over the preview has been removed completely. The preview is now unobstructed and retains direct drag and mouse-wheel navigation.

Removed implementation includes:

- Overlay controls and panel creation
- Overlay layout and z-order management
- Mouse-triggered overlay display
- Overlay status updates
- Overlay command routing

## Quick Controller cleanup

Removed from the Quick Controller:

- Play
- Stop Desktop

The Quick Controller continues to provide Apply Settings Live, static and slideshow desktop actions, separate preview and desktop animation controls, coordinate tools, image capture, high-resolution rendering, preset loading, the editor, and Exit App.

Desktop stopping remains available through the main editor and system tray controls.
