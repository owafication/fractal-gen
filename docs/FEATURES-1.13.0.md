# Mandelbrot Live Wallpaper 1.13.0

## Desktop mode selector

The Desktop page now provides an explicit mode selector and Apply button. Available modes are:

- None
- Static image
- Live image
- Slide show
- Journey

Changing the selection alone does not alter the desktop. The selected mode is started only after **Apply** is pressed. **Set as default** stores the selected mode for the next application launch. The safe default remains **None**.

The Preview page's Animation selector was removed. Preview and desktop zoom/colour playback remain controlled from the Quick Controller, while Journey has its own settings dialog.

## Journey settings

A dedicated **Journey Settings...** dialog is available from the Preview page, Desktop page, Settings window, and Quick Controller. It edits structured rows in the form:

```text
X,Y,Scale,TransitionSeconds,HoldSeconds
```

## Default image output

Settings now includes:

- A user-selected default folder for static captures, saved images, and slideshow captures
- PNG, JPEG, TIFF, or BMP output
- A 1–100 compression/quality value

JPEG uses image quality. PNG and TIFF map the value to the compression/filter options available through Windows Imaging Component. BMP is uncompressed.

High-resolution rendering uses the same default folder, format, and compression/quality setting when the dialog opens. Its Save As action still permits changing the final file path.

The slideshow manager and static wallpaper loader now accept PNG, JPEG, TIFF, and BMP files.

## Editor workflow

Settings, Equation, Palette, and Journey windows are independent tool windows rather than owner-disabled modal windows. The main preview remains navigable while those windows are open.

Equation Editor dropdown controls now include vertical scrolling.

Palette Editor changes are applied to the preview as controls change and immediately when a saved palette is loaded. Cancelling restores the preset state that existed when the editor opened.
