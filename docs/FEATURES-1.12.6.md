# Mandelbrot Live Wallpaper 1.12.6

## Windows build correction

This release corrects a native Windows compilation failure in the Fractal Scout dialog. The dialog formats candidate coordinates with `CameraCentreX` and `CameraCentreY`; their declaration is provided by `Core/DeepZoom.h`, which is now included directly by the translation unit that uses them.

No application behaviour or rendering contract was intentionally changed.
