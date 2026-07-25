# Verification record for 1.6.0

Verified in the available Linux build environment:

- GCC core build with warnings treated as errors.
- Clang core build with warnings treated as errors.
- Core settings round-trip tests for slideshow folder, order, interval, current index and image paths.
- Settings migration to schema version 6.
- Source archive integrity and absence of compiled artifacts.

Requires Windows runtime verification:

- Win32 slideshow dialog layout and keyboard navigation.
- Windows folder and multi-file pickers.
- Full-resolution capture into a user-selected folder.
- Sequential and shuffle transitions behind desktop icons.
- Restore of a live wallpaper after **Add Preview to Slideshow**.
