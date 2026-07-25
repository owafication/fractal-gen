# UI redline implementation — 1.8.0

## Main editor

- Replaced the horizontal page tabs with a compact vertical Preview/Desktop/Status navigation rail.
- Removed explanatory copy and nonessential preset-management buttons from the Preview page.
- Replaced separate centre X, centre Y and scale fields with one validated `centreX, centreY, scale` field.
- Added a Preset Library pop-out for load, save-new, update, delete, restore, import and export actions.
- Kept palette, animation, equation, iterations, zoom speed, colour speed, brightness, contrast, saturation and colour offset together on Preview.
- Moved live/static/slideshow/stop actions to the preview hover strip and Quick Controller.
- Moved FPS, status and current coordinates into the hover/status surfaces.

## Settings

- Graphics: performance profile, maximum FPS, render scale, iterations, anti-aliasing, precision, resource protection and resume delay.
- System behaviour: startup, launch, tray, full-screen, battery, Remote Desktop, desktop visibility, lock and reduced-motion controls.
- Monitors and journey: Mirror/Span/Independent mode, per-monitor preset assignment and custom timed Automatic Journey waypoint text.

## Quick Controller

The modeless Quick Controller is a normal movable tool window and is not topmost. It can remain visible while the main editor is hidden in the system tray. It provides:

- Play / Resume
- Pause
- Save Image
- Load Preset
- Open Editor
- Stop
- Current coordinates
- Preview/desktop FPS and process CPU/memory status

The system-tray context menu and preview hover menu can open it.

## Rendering behaviour

The preview renderer now stops submitting frames whenever the main editor is hidden or minimised. The live desktop renderer remains active, while the Quick Controller continues to receive resource and status updates.

## Zero-GPU user pause

When the user pauses a live wallpaper, the controller captures the current desktop framebuffer, displays that captured frame through the existing static paint path, and releases the OpenGL wallpaper renderer. Animation state is not advanced while paused. Resume performs one bounded renderer initialisation and continues from the paused animation state; if it fails, the captured frame remains in place and the error is reported.
