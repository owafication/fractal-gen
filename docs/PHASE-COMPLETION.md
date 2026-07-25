# Phase Completion Record

## Phase 1 — Rendering prototype

Implemented:

- Native resizable preview window.
- GPU shader Mandelbrot evaluation.
- Wheel zoom, drag pan and reset.
- Palette, iteration, FPS and render-scale controls.
- Smooth colouring, supersampling and error handling.

Automated core math is verified. GPU runtime behaviour remains Windows-hardware dependent.

## Phase 2 — Animation

Implemented:

- Continuous Zoom.
- Automatic Journey with eased logarithmic zoom-in, panning at the same time, a bounded deep-view hold, then a panning zoom-out toward the next target. Boundary-detail validation avoids black interior endpoints.
- Static Animated Colour.
- Manual View.
- Ten built-in presets and custom preset lifecycle.
- Arbitrary-length custom palette editing with per-row colour swatches, named reusable palette presets, validated local persistence and GPU interpolation.

## Phase 3 — Windows wallpaper integration

Implemented:

- Borderless virtual-desktop wallpaper host.
- WorkerW/Progman attachment and reversible detach.
- Start, pause, resume and stop.
- Explorer attachment revalidation and display-change recovery.
- One renderer restart and static fallback.

Physical Windows verification is still required because WorkerW is version-sensitive.

## Phase 4 — Settings and system tray

Implemented:

- Main controls and advanced settings dialog.
- Tray menu.
- Versioned local JSON persistence and corruption recovery.
- Per-user Start with Windows.
- Performance profiles, direct maximum-iterations input and diagnostic logging.
- Static render capture and a dedicated local slideshow manager with an assignable capture folder, selectable/reorderable image list, existing-BMP import, sequential/shuffle playback and configurable interval.

## Phase 5 — System awareness

Implemented:

- Full-screen detection.
- Battery pause and battery quality reduction.
- Session lock/unlock.
- Sleep/resume.
- Remote Desktop pause.
- Desktop visibility pause and configurable resume delay.

## Phase 6 — Multi-monitor

Implemented:

- Mirror.
- Span.
- Independent preset/animation assignment by display device.
- Mixed-resolution and virtual-coordinate region mapping.
- Display/DPI rebuild handling.

Runtime testing across common monitor layouts is unverified in this environment.

## Phase 7 — Packaging

Implemented:

- Application icon and version resource.
- Per-monitor-V2 manifest.
- Portable release ZIP script.
- Inno Setup installer and uninstaller definition.
- MIT licence, default preset reference and user/developer documentation.

The installer executable itself was not generated because this environment lacks Windows, Visual Studio and Inno Setup.

## Phase 8 — Adaptive rendering protection (1.7.0)

Implemented sustained low-FPS, process-CPU and working-set pause gates; bounded stable resume; configurable resource thresholds; and screen-space suppression of Mandelbrot equation work for visually unchanged frames. Platform-independent policy and tests pass in the available environment. Windows process metrics, WGL behaviour and target-machine threshold tuning remain runtime verification items.


## Phase 9 — Preview overlay and editable journey waypoints (1.7.1)

- Added hover controls over the preview for zoom, colours and desktop output actions.
- Added coordinate jump and current-camera display.
- Added bounded, persisted Automatic Journey waypoint scripts with travel and hold durations.
- Added regression coverage for waypoint persistence, validation and independent motion pausing.
- Portable core verification passed; Windows runtime verification remains external.

## Phase 10 — Redlined UI restructuring and Quick Controller (1.8.0)

- Replaced horizontal tabs with a compact vertical navigation rail.
- Consolidated preview coordinates into one validated field.
- Added the Preset Library manager.
- Moved graphics, monitor and system behaviour configuration into Settings.
- Added the modeless Quick Controller and tray/hover entry points.
- Suppressed invisible preview rendering while the main editor is hidden.

## Phase 11 — Advanced equations and visual finish (1.9.0)

- Added integer powers through degree 12, reciprocal powers, iteration terms, Julia mode, fixed and critical initial values, conjugation, component swapping, independent absolute components, and bounded complex sine/cosine/exponential/logarithm transforms.
- Added Newton convergence basins for degrees 2–12.
- Added independent named equation presets, bounded randomisation and coefficient animation.
- Added smooth escape, orbit-trap, distance-estimation and Newton-basin colouring, depth shading and a bounded screen-space glow pass.
- Made the preview controller persistent and moved Settings, Palette, Quick and Equations actions onto the vertical navigation rail.
- Simplified the Quick Controller to one Play/Stop toggle.
- Portable core compiler, test and sanitizer verification passed. Windows shader/UI/runtime verification remains external.

## Phase 12 — Responsive and accessible dialogs (1.10.0)

- Added shared DPI-aware, resizable and scrollable dialog layout support.
- Added keyboard mnemonics, Enter/Escape handling, focus-following scroll, system fonts and remembered placement.
- Applied the support to the equation, palette, settings, preset, slideshow, precision, adaptive-resource and Quick Controller windows.

## Phase 13 — Quick and hover command completion (1.10.1)

- Added Quick Controller zoom and colour toggles, Save Preset As, Save Image, Copy Coordinates and Exit App.
- Added Set Live, Save Preset As, Copy Coordinates and Save Image to the preview overlay.
- Removed duplicated initial page layout.

## Phase 14 — Tiled high-resolution still output (1.10.3)

- Added the Preview-tab Render Hi-Res and Save workflow.
- Added PNG, TIFF and BMP output through Windows Imaging Component with DPI metadata.
- Added a cancellable background worker and progress UI.
- Added a portable scanline-tiled renderer that retains one full output row, one 256-pixel computation tile and a bounded preview instead of allocating the full-resolution frame.
- Added bounded preview and Save As actions after successful completion.
- Added core tests for row ordering, exact output counts, progress, cancellation, writer failures, bounded preview storage and peak working-pixel limits.

## Phase 15 — Expanded equation and palette libraries (1.11.0)

Completed:

- added independent powered-`c` recurrence support with persistence, CPU evaluation, OpenGL uniforms, summaries and editor controls;
- added all eight supplied reference formulas as equation and scene presets;
- expanded the built-in equation library to 45 entries;
- added 30 reusable built-in palette presets, including eight reference-matched palettes;
- expanded the complete built-in scene library to 36 entries;
- kept built-in equations and palettes read-only while allowing editable custom copies;
- strengthened custom scene saving with name prompting, explicit error reporting and rollback after persistence failure;
- retained the complete requested preview hover menu, including high-resolution rendering;
- added core and source regression checks for library counts, unique identifiers, powered-parameter evaluation and save wiring.

## Phase 18 — Preset popup naming and structured journeys (1.11.5)

- Moved Save-as-new naming into the Preset Library popup.
- Removed redundant preset save/rename and preview-transfer actions from compact menus.
- Made preview and desktop animation opt-in on every app launch.
- Changed custom Automatic Journey scripts to exact ordered transition/hold routes.
