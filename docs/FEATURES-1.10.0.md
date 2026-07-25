# Mandelbrot Live Wallpaper 1.10.0

## Responsive and accessible dialogs

Version 1.10.0 replaces the fixed-pixel modal-window behaviour with shared responsive Win32 dialog support.

- Every application dialog is resizable and uses the existing per-monitor-v2 DPI context.
- Controls and system message fonts are scaled when a dialog opens or crosses to a monitor with a different DPI.
- Dialogs are constrained to the current monitor work area instead of opening partly off-screen.
- Horizontal and vertical scrolling keep all controls reachable when display size, text scaling, or DPI leaves insufficient room.
- Enlarged dialogs expand list boxes, multiline editors, group boxes, labels, and single-line fields without stretching ordinary push-button heights.
- Keyboard tab navigation uses `WS_EX_CONTROLPARENT` and automatic tab stops for interactive native controls.
- Enter activates the focused push button or the dialog's default action. Escape performs the dialog's cancel or close action.
- Tabbing to a control automatically scrolls it into view.
- Important actions include keyboard mnemonics.
- Native system colours remain in use, including the owner-drawn palette list, preserving Windows high-contrast behaviour.
- Advanced equation settings now have explanatory hover tooltips for powers, rational terms, transforms, initial values, Julia/Newton controls, colouring, glow, depth, and coefficient animation.
- List controls expose descriptive window text for assistive technologies.
- Dialog and Quick Controller size and position are remembered in `%LOCALAPPDATA%\MandelbrotLiveWallpaper\dialog-layout.txt` using bounded, local-only values and an atomic replacement write.
- The coordinate text prompt and modeless Quick Controller use the same responsive infrastructure.

Native Windows file, folder, and colour pickers remain system-provided so they retain the operating system's accessibility behaviour.
