# Runtime rendering fixes 1.0.4

- Removed the remaining GLSL 1.20 float-to-integer anti-aliasing conversion. The CPU already validates the uniform to the supported range.
- Restored animated GPU rendering for the preview and wallpaper on drivers that reject float-to-int shader constructors.
- Increased the static CPU fallback from a fixed 480x270 image to an aspect-correct bounded render of up to 1280 pixels wide or 720 pixels tall.
- Enabled halftone scaling for the CPU fallback to reduce blocky enlargement.
- Added explicit renderer failure details and fallback mode to copied diagnostics.
- Added clean, fail-fast Windows build scripts to the source archive.
