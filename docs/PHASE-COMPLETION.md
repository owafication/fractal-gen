> **Historical record:** This file records earlier phase claims. Current phase order, evidence gates and status are owned by `project_docs/IMPLEMENTATION_PLAN.md` and `project_docs/VALIDATION_AND_EVIDENCE.md`.

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


## Phase 20 — Tricorn colour texture foundation (1.12.0)

- Added named Tricorn/Mandelbar and Multicorn power-3/power-4 equation templates.
- Added persisted palette frequency, gamma and linear/smooth mapping controls.
- Added optional stripe-average orbit colouring with density, phase, strength and start-iteration controls.
- Applied the new colour contract to CPU still rendering, OpenGL and Direct3D 11 inputs.
- Added the Cyan Fire Ring palette and Tricorn Cyan Fire Ring scene.
- Added `docs/FRACTAL-STYLE-BUILD-PLAN.md` for the remaining conjugate distance, deep perturbation, post-processing/rotation and Fractal Scout phases.
- Portable GCC/Clang core verification passes; native Windows UI and shader runtime comparison remain external.


## Phase 21 — Tricorn Jacobian distance and edge lighting (1.12.1)

Implemented the second fractal-style build-plan phase:

- exact power-2 Tricorn real two-axis Jacobian tracking;
- largest-singular-value distance estimation;
- strict support gating for unsupported conjugate formulas;
- independent mathematical edge lighting and screen-space bloom;
- one-time migration of older glow settings to preserve appearance;
- CPU, OpenGL and Direct3D 11 renderer contract updates;
- finite-difference and persistence regression tests.

Deep Tricorn perturbation remains the next planned phase.


## Phase 22 — Deep Tricorn perturbation (1.12.2)

- Added exact Tricorn perturbation profile selection.
- Added conjugate double and arbitrary-precision reference orbit generation.
- Added OpenGL and Direct3D 11 conjugate perturbation recurrence.
- Added guarded split-float fallback and deterministic CPU reference refresh.
- Added progressively deeper direct-versus-perturbation regression coverage.

Next: Phase 4 post-processing, tile-overlap calculation, and camera rotation.

## Phase 23 — Post-processing and camera rotation (1.12.3)

- Added persisted bloom threshold, soft knee and radius controls with compatibility defaults.
- Replaced the fixed GPU bloom kernel with bounded horizontal/vertical separable passes in OpenGL and Direct3D 11.
- Made GPU high-resolution tile overlap follow the active bloom radius and anti-alias reconstruction allowance.
- Added persisted camera rotation across preview, desktop, CPU still/static fallback, GPU tiles, drag pan and wheel-zoom anchoring.
- Added global-pixel mapping, rotated tile-centre, overlap, persistence and rotated interaction regression fixtures.

Next: Phase 5 Fractal Scout candidate discovery.

## Phase 24 — Fractal Scout (1.12.4)

- Added a deterministic bounded candidate search around the current preview camera.
- Added boundary-mix, iteration-variance, edge-density, detail and symmetry scoring.
- Added CPU-rendered ranked thumbnails with Fast, Balanced and Detailed resource budgets.
- Added cancellable background search, selected-candidate refinement, temporary preview application and explicit Save as New handoff.
- Added deterministic, cancellation, memory-bound and thumbnail-completeness regression coverage.


## Phase 25 — Targeted multi-scale Fractal Scout (1.12.5)

- Added Balanced, Boundary, Filaments and Symmetry Scout scoring goals.
- Added deterministic logarithmic depth bands around each search position.
- Added position-and-scale near-duplicate suppression with bounded deterministic relaxation.
- Added the Search target selector and diversity reporting to the Scout dialog.
- Added score-profile, depth-band, suppression, determinism and limit regression coverage.


## Phase 26 — Windows Fractal Scout build correction (1.12.6)

- Added the missing direct `Core/DeepZoom.h` dependency to the Win32 Fractal Scout dialog.
- Corrected the MSVC undeclared `CameraCentreX` / `CameraCentreY` failure and its cascading stream ambiguity.
- Added a source regression check for the Windows-only translation-unit dependency.
- Recorded later save-path, desktop-mode, journey, dropdown, window-behaviour, and live-palette requests without implementing them.

## 1.13.0 desktop/output UX

Implemented the explicit desktop-mode Apply flow, safe default launch mode, shared image output directory/format/compression settings, Journey Settings access, scrollable Equation Editor dropdowns, independent Settings/Equation/Palette windows, and live Palette Editor preview updates.


## 1.13.1 Windows Journey Settings build correction

Implemented:

- Public read-only journey-script validation contract.
- Journey Settings no longer calls the private animation parser.
- MSVC C2248 regression source check and core validation fixtures.
