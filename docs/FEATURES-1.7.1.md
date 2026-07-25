# Preview overlay and custom journey waypoints — 1.7.1

## Preview hover overlay

Moving the pointer over the preview reveals a temporary overlay with:

- Start or stop camera zoom/motion without stopping colour cycling.
- Start or stop colour cycling without stopping camera motion.
- Apply the preview as a live desktop wallpaper.
- Capture and apply the preview as a static wallpaper.
- Start the saved static slideshow.
- Stop the desktop wallpaper.
- Jump to `centreX,centreY,scale`.
- Read the current X, Y and scale values.

The overlay hides after three seconds without preview pointer activity.

## Custom Automatic Journey waypoints

The optional waypoint text belongs to each wallpaper preset and is included in local settings and preset import/export. Entries use:

```text
centreX,centreY,scale,travelSeconds[,holdSeconds]
```

Entries may be placed on separate lines or separated by semicolons. The parser:

- accepts at most 128 entries;
- limits stored input to 32768 bytes;
- clamps X and Y to the supported complex-plane range;
- clamps scale to the renderer precision range;
- clamps travel time to 1–3600 seconds;
- clamps hold time to 0–3600 seconds;
- ignores malformed or non-finite rows;
- applies the existing boundary-rich target validation so automatic travel does not finish on a solid-black interior point.

If no custom entries are supplied, the built-in varied journey candidate set remains active.

## Verification boundary

Core parsing, settings persistence, animation play/pause behaviour, validation and sanitizer checks were run in the packaging environment. The Win32 controls, WGL renderer, WorkerW attachment and desktop hover interaction still require runtime verification on Windows 10 or Windows 11.
