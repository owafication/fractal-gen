# Mandelbrot Live Wallpaper 1.11.4

## Preview preset-name editor

- Added a preset-name text box to the hover controls over the live preview.
- Added a **Rename Preset** action beside the text box.
- Custom preset renames preserve the preset ID and all rendering settings, so monitor assignments and saved references remain intact.
- Preset names are trimmed, limited to the existing 120-byte persisted-name contract, and saved through the normal atomic settings path.
- Built-in presets remain read-only. The entered name is retained as the suggested name when creating a custom copy with **Save Preset As**.
- Rename failures roll back the in-memory name and restore the previous UI value.
