# Mandelbrot Live Wallpaper 1.6.0 static slideshow update

## Dedicated slideshow editor

**Manage Slideshow…** on the **Wallpaper & Monitors** tab opens a modal editor for the complete static wallpaper playlist.

- The capture folder is editable and can be selected with the Windows folder picker.
- **Add Images…** imports one or more existing local BMP paths.
- **Add BMPs from Folder** scans the assigned folder without recursing into unrelated directories.
- Entries can be removed, moved up or down, and explicitly selected as the current image.
- **Use Selected Now** saves the list and starts static wallpaper playback from that entry.
- Timed playback supports Sequential and Shuffle order with an interval from 10 seconds to 24 hours.
- Missing or invalid files are skipped without executing file content. Removing a list entry never deletes the source image.

## Capture actions

- **Capture Preview as Static** captures the current preview at virtual-desktop resolution, adds it to the list, uses it immediately, and releases the wallpaper GPU context.
- **Add Preview to Slideshow** captures to the assigned folder and adds the result while restoring the previous live/stopped state.

The list is persisted in settings schema version 6 and bounded to 512 unique local paths. The application-generated format remains uncompressed 32-bit BMP for predictable offline loading through the existing GDI path.
