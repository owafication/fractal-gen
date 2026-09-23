# Mandelbrot Live Wallpaper

A native, offline Windows 10/11 desktop application that renders an animated Mandelbrot set behind desktop icons.

## Project governance

For implementation work, read [`AGENTS.md`](AGENTS.md) and [`project_docs/PROJECT_INDEX.md`](project_docs/PROJECT_INDEX.md) first. Historical feature and verification files remain evidence records; current roadmap, state, validation, risk and release contracts are routed through the project index. Release-facing source version fields are now aligned at 1.13.1 and checked during CMake configuration and CTest. Native Windows/MSVC and runtime verification are still required before a release-ready claim.

## Implemented scope

- GPU Mandelbrot rendering through Direct3D 11/HLSL by default, with automatic OpenGL/GLSL fallback and a static CPU-rendered fallback.
- Responsive control window with a compact vertical Preview/Desktop/Status navigation rail, a resizable live preview, wheel zoom, drag pan, reset view, smooth colouring, ten built-in palettes and ten built-in presets.
- Runtime complete-project undo and redo with labelled Preview-page buttons, Ctrl+Z/Ctrl+Y, coalesced scalar gestures, atomic structural snapshots for palette/equation/journey/preset/import/dialog actions, redo-branch truncation and bounded memory.
- Runtime general-animation timeline editor with typed target tracks, keyframe time/interpolation editing, authoritative Add Current Value, Clamp/Loop/PingPong timing, scrub/play/pause/stop preview and strict loss-aware Journey conversion. Timelines are not persisted in PH07.
- Pop-out custom palette editor with colour swatches, ordered colour stops, add/edit/remove/reorder controls, named reusable palette presets, JSON persistence, palette frequency/gamma/mapping controls, and optional stripe-average orbit texture.
- Advanced pop-out equation editor for bounded data-only escape-time, Julia, rational and Newton recurrences. It supports independent integer powers 1–12 for both `z` and `c`, complex coefficients, fixed/critical initial values, reciprocal powers, independent absolute components, conjugation, component swapping, `sin`/`cos`/`exp`/guarded `log`, iteration terms, fixed Julia parameters, Newton roots, named equation presets, bounded randomisation and coefficient animation.
- Automatic Journey with validated generated destinations plus exact user-defined ordered routes. Each custom row supplies a coordinate string, transition time and hold time before the route continues directly to the next row; Continuous Zoom, Static Animated Colour and Manual View modes remain available.
- Frame limiting, render-scale reduction and iteration limits with paired sliders and editable numeric fields, 1x–4x supersampling, and selectable deep-zoom precision strategies.
- Toggleable deep-zoom strategies: GPU float32, native GPU float64, split high/low float, double-reference perturbation, and 128/256/512-bit CPU-reference perturbation, with automatic capability-based fallback.
- Battery Saver, Balanced, High Quality and Custom profiles.
- Configurable adaptive resource protection: sustained low-FPS, process-CPU and working-set limits pause both wallpaper and preview, then allow one bounded resume probe after a stable cooldown.
- Invisible-frame suppression skips Mandelbrot shader execution when camera and palette movement remain below configurable screen-space thresholds, while accumulating movement until it becomes visible.
- WorkerW/Progman desktop attachment without modifying Explorer or the user’s wallpaper file.
- Pause, resume, stop, Explorer reattachment checks, display-change recovery and one bounded renderer restart.
- Full-resolution static capture with a dedicated slideshow manager: assignable capture folder, explicit image list, existing-BMP import, folder scan, remove/reorder/select-current controls, sequential or shuffle playback, and editable interval.
- Preview-tab **Render Hi-Res...** workflow with coordinate-string input, independent width/height and DPI metadata, PNG/TIFF/BMP output, cancellable background progress, GPU Direct3D 11 tile-band export by default, explicit OpenGL tile-band fallback, CPU scanline-tiled fallback, fixed still-frame animation phase, automatic resolution-aware iteration depth, and bloom-radius-aware overlap cropping.
- Automatic zoom targets are checked for escaping boundary detail so journeys do not finish inside a black Mandelbrot interior region.
- Full-screen, battery, Remote Desktop, session lock, sleep/resume and desktop-visibility awareness.
- Mirror, Span and Independent monitor modes. Span maps the full virtual desktop into the attached WorkerW host, including negative monitor coordinates. Independent mode stores a preset assignment per display device and exposes assignment controls only when that mode is active.
- Local versioned JSON settings, strict preset import, atomic saves and corrupt-file preservation.
- A movable modeless Quick Controller that remains available when the main editor is minimised to tray. It exposes live/static/slideshow application, independent preview/desktop zoom and colour controls, coordinate jump/copy, image capture, high-resolution rendering, preset loading and editor access.
- Persisted camera rotation shared by preview, desktop, CPU stills, GPU tiles, drag panning and zoom anchoring, plus configurable separable bloom threshold, soft knee and radius.
- **Fractal Scout** performs a cancellable, resource-bounded multi-scale search around the current view, supports Balanced, Boundary, Filament and Symmetry targets, suppresses near-duplicate results, shows rendered thumbnails, supports iterative refinement, and only saves through an explicit Save as New action.
- Resizable, per-monitor-DPI-aware dialogs with scrollable small-screen layouts, system message fonts, keyboard mnemonics, reliable Enter/Escape handling, focus-following scroll, high-contrast-safe native controls, equation tooltips, and remembered per-user dialog size and position.
- Rotating local diagnostics with open, copy and clear actions. No analytics or network access.
- Inno Setup installer definition and portable release packaging script.

## Built-in equation and colour libraries

The equation editor includes 47 bounded data-only presets. These cover Mandelbrot and Multibrot powers, Burning Ship, named Tricorn/Mandelbar and Multicorn variants, multiple Julia constants, Newton basins, rational maps, orbit traps, distance colouring, coefficient animation, transcendental transforms, powered `c` terms and complex coefficient variants.

The eight equations supplied in the reference comparison are included directly:

```text
z² + c
z² + 1.2c
z² + c + 0.5
1.2z² + c
z² + 0.5z + c
z + c²
|z|² + c
z² - c
```

The palette editor exposes reusable built-in palettes, including the `Cyan Fire Ring` palette added for high-frequency Tricorn boundary colour. Eight are matched to the supplied reference artwork: Electric Blue and Gold, Cyan Aurora, Magenta Nebula, Golden Halo, Deep Cyan, Crimson Web, Ice Lightning and Toxic Green. Complete scene presets combine those equations and palettes so they can be loaded without configuring the two libraries separately. Built-in entries are read-only; load one and use **Save as New** to create an editable custom copy.

Current roadmap progress adds deterministic PNG frame sequences and optional external-FFmpeg H.264/MP4 encoding without changing the 1.13.1 release version or settings schema. Version 1.13.1 corrects the native Windows Journey Settings build by keeping journey parsing private and exposing a public validation contract. Version 1.13.0 adds explicit desktop-mode selection, configurable image output defaults, a dedicated journey editor, independent modeless settings/equation/palette windows, scrollable equation selectors, and live palette preview. Version 1.12.6 corrected the native Windows Fractal Scout build, while 1.12.5 added targeted scoring, logarithmic multi-scale depth bands and deterministic near-duplicate suppression. The earlier Tricorn colour, distance, perturbation, bloom, tiled-export and rotation phases remain intact. See `docs/FRACTAL-STYLE-BUILD-PLAN.md` for the rendering roadmap and completed extension phases.

## Build requirements

- Windows 10 or Windows 11, x64.
- Visual Studio 2026 Build Tools or Visual Studio 2026 with **Desktop development with C++**.
- CMake 3.24 or later.
- Optional: Inno Setup 6 to produce the installer.
- The reviewed Boost.Multiprecision headers are already included under `third_party/`; no package-manager or network fetch is required.

No third-party runtime or package-manager dependency is required for rendering, wallpaper operation or PNG frame export. The source tree vendors a hash-locked Boost.Multiprecision 1.83.0 standalone header subset under BSL-1.0 for an independent high-precision reference backend; it adds no DLL or installer runtime. MP4 export optionally uses a user-supplied FFmpeg executable with `libx264` and MP4 muxer capabilities; it is not bundled, downloaded or remembered by the application.

- Deterministic PNG frame-sequence export with rational timing, verified temporary promotion, resumable manifests, progress and cancellation.
- Optional H.264/MP4 encoding of verified frame sequences through direct fixed-argument FFmpeg execution, bounded logs, cancellation, decode verification and source-preserving failure handling.

## Build

Open PowerShell in the repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\build-release.ps1
```

The release script uses the `windows-msvc-release` configure/build/test presets from `CMakePresets.json`, treats warnings as errors, runs CTest, verifies the executable version, creates a portable ZIP, and builds the installer when Inno Setup is available.

For the PH-01 native release gate, use the report-producing workflow:

```powershell
.\scripts\validate-windows-release.ps1
```

From Command Prompt, the equivalent wrapper is:

```cmd
scripts\validate-windows-release.cmd
```

The validation workflow also inspects the portable package and runs a clean startup/shutdown smoke with application data redirected into its report folder. It does not load or overwrite the normal user settings file. Full dialog, desktop-mode, GPU, display, lifecycle and installer checks remain a recorded manual matrix in `docs/testing/WINDOWS-RELEASE-VALIDATION.md`.

Manual preset-based build:

```powershell
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel
ctest --preset windows-msvc-release
```

## Run

Launch `build\Release\MandelbrotWallpaper.exe`. Edit the live preview on **Preview**, then use the main controls or **Quick Controller** to apply the current settings as a live, static, or slideshow desktop. Preview changes are intentionally staged until they are explicitly applied to the desktop.

Use **Animation Timeline...** on Preview or Desktop to open the PH07 runtime editor. Add tracks from the stable target registry, add the current project value as a keyframe, edit selected keyframe time/interpolation, set duration and loop mode, and scrub or play the candidate through the existing preview renderer. Cancel restores the prior preview. General timelines are session-only; **Tracks → Journey** is available only for an exact camera-centre/scale subset and applies the existing Journey text only after OK.

Use **Render Hi-Res...** on the Preview tab for still output. The dialog accepts `centreX,centreY,scale`, width, height, DPI and PNG/TIFF/BMP format. GPU Direct3D 11 is the default renderer and OpenGL remains selectable as a compatibility fallback. Both GPU paths use the active precision strategy where supported and render overlapping GPU tiles, assembles a bounded scanline band and streams rows to the encoder, so the requested image is not constrained to a single GPU texture or full-frame readback allocation. The preset iteration count is treated as a minimum and is raised automatically when the camera scale and output resolution resolve finer detail, up to the existing 4096 cap. CPU scanline tiling remains available as a compatibility fallback.

The application does not overwrite the Windows wallpaper setting. Stopping the live wallpaper destroys its desktop child window, revealing the previous wallpaper unchanged.

## Data locations

Settings and logs are stored under:

```text
%LOCALAPPDATA%\MandelbrotLiveWallpaper\
```

Imported presets are JSON data only. Equations are stored as bounded numeric coefficients; arbitrary expressions, scripts, and downloaded shader code are not executed. Unknown executable fields are ignored and unsupported typed values are rejected.

## Rendering troubleshooting

If the wallpaper looks heavily pixelated, check **Copy Diagnostics**. `Wallpaper renderer mode: static CPU fallback` means the GPU shader did not start. The fallback is intentionally static and bounded to protect CPU usage; it is not the normal animated rendering path. Version 1.0.4 removes a GLSL 1.20 float-to-integer conversion rejected by some Windows OpenGL drivers and reports the complete renderer error in diagnostics.

For maximum normal-render detail, select **High Quality**, set **Render scale** to 100%, and increase **Iterations** for deeper views. Anti-aliasing smooths edges but does not replace render resolution.

## Deep-zoom precision

Open **Settings → Configure precision…** to select **Automatic**, **GPU float32**, **native GPU float64**, **split high/low float**, **double-reference perturbation**, or **arbitrary-reference perturbation**. Automatic mode changes strategy as zoom depth increases and only uses candidates that are enabled and available.

Camera centres retain compensated high/low components. Arbitrary-reference mode computes one bounded 128, 256, or 512-bit CPU reference orbit and renders pixel deltas on the GPU. Burning Ship-style absolute-value equations cannot safely use the analytic perturbation path and fall back to another enabled strategy.

The current source caps scale at `1e-32`, maximum configured zoom at `1e30`, iterations at 4096, and reference precision at 512 bits. These are explicit engineering limits rather than a claim of unlimited zoom. The precision dialog explains the expected quality, compatibility and cost of every mode. See `docs/FEATURES-1.5.0.md`.


## Adaptive resource protection

Open **Settings → Resource protection → Configure adaptive pause…** to control the self-throttling behaviour. The app can pause both the live wallpaper and preview after sustained low FPS, high process CPU use, or high process working-set memory. It resumes only after CPU and memory remain below their limits for the configured stable period. If the same overload returns, it pauses again rather than looping continuously.

The Direct3D 11 and OpenGL paths do not depend on vendor-specific GPU monitoring APIs. Sustained low FPS is used as the cross-vendor signal that GPU work is too expensive. CPU percentage and working-set memory are measured for this process only. Current values, adaptive state and pause reason are included in the main status and copied diagnostics.

Enable **Skip equation rendering until camera or colour movement becomes visible** to retain the last rendered image whenever the next animation step is below the configured pixel and palette thresholds. The comparison is made against the last frame actually submitted to the renderer, so small movements accumulate and automatically trigger a new render once they become visible. Static views with colour cycling paused therefore stop running the fractal equation after their first completed frame.

See `docs/FEATURES-1.7.0.md` for the exact defaults, limits and failure behaviour.

## Windows integration note

WorkerW is an undocumented Explorer implementation detail. The integration is isolated in `src/WindowsIntegration/DesktopHost.*`, checked periodically, and falls back to Progman when the expected WorkerW hierarchy is unavailable. See `docs/WINDOWS-INTEGRATION.md`.

### Colour cycling control

Use the preview and desktop colour controls to freeze or resume palette movement independently without pausing camera motion. Each application launch starts with preview and desktop zoom/colour movement stopped; the saved preset animation settings remain available when explicitly started.

## Advanced equations and visual finish

Open **Equations** on the vertical navigation rail. The editor supports the structured recurrence:

```text
z(n+1) = A·T(z)^p + B·T(z) + C·c + D + E·n + λ/T(z)^q
```

`p` is selectable from 1–12 and `q` from 0–12. `T` may apply independent real/imaginary absolute values, component swapping, conjugation, or one bounded complex transform: sine, cosine, exponential, or guarded logarithm. Initial state options include zero, a fixed complex `z0`, the current parameter, or a supported-family critical point. Julia mode treats each pixel as `z0` and uses a fixed complex `c`.

Newton mode renders convergence basins for `z^degree = target`, with degree 2–12, complex relaxation and a bounded convergence tolerance. Named equation presets are saved independently from wallpaper presets. A bounded randomise action and optional coefficient animation are included.

Colour methods include smooth escape, point/cross/circle orbit traps, analytic-polynomial distance estimation, and Newton basins. Glow uses a bounded screen-space 3×3 post-process pass; depth shading uses iteration/convergence information. Unsupported distance derivatives safely fall back to smooth colouring.

Split-float and perturbation deep zoom remain limited to compatible quadratic parameter maps. Higher powers, rational maps, Julia, Newton and non-analytic transforms use GPU float32 or native float64 according to the precision settings and hardware capability.

## Custom palettes

Choose **Edit Palette…** beside the palette selector. Add, edit, remove, and reorder colour stops in the pop-out editor. Frequency controls repetition, gamma changes phase distribution, linear/smooth mapping changes transitions, and optional stripe-average colouring adds orbit-angle texture through density, phase, strength, and start-iteration fields. The renderer interpolates the list in order and wraps the final colour back to the first so colour cycling remains continuous. Each colour row includes a visual swatch beside its hexadecimal value. Enter a name and choose **Save / Update** to store the current colour list as a reusable palette preset. Saved palettes are data-only, persist independently of fractal presets, and can be loaded, renamed, updated, or deleted from the same dialog.

The editor is designed for arbitrary-length palettes. A 4096-stop safety bound prevents malformed imported JSON from consuming unbounded memory; the GPU uploads one texel per stop when hardware limits allow and safely resamples only palettes larger than the device texture limit.

## Desktop mode and image-output defaults

The Desktop page has an explicit mode selector for **None**, **Static image**, **Live image**, **Slide show**, and **Journey**. Selecting a mode does not change the desktop until **Apply** is pressed. **Set as default** stores the selected mode for the next launch; the default is **None**.

Settings also provide a default output folder, saved-image type (PNG, JPEG, TIFF or BMP), and a 1–100 compression/quality value. These defaults are used by static captures, saved images, slideshow captures and high-resolution export.

## Static wallpaper and slideshow

On **Wallpaper & Monitors**:

- **Capture Preview as Static** saves the exact current camera and colour position at full virtual-desktop resolution, adds it to the slideshow list, displays it behind the icons, and releases the wallpaper GPU renderer.
- **Add Preview to Slideshow** captures and adds the current preview to the list while restoring the previous live/stopped wallpaper state afterward.
- **Manage Slideshow…** opens the dedicated editor. Choose the folder used by new captures, add existing PNG, JPEG, TIFF or BMP files, scan the selected folder, remove or reorder entries, choose the current image, set a 10–86400 second interval, and select Sequential or Shuffle playback. **Use Selected Now** starts the static wallpaper from the chosen entry.

The default capture folder is `%LOCALAPPDATA%\MandelbrotLiveWallpaper\static-renders`, but it can be changed to any accessible local folder. Removing an entry does not delete its file. The slideshow is safety-bounded to 512 paths, skips missing or invalid images, and does not restart the GPU between transitions.

## Automatic zoom target safety

Continuous Zoom and Automatic Journey validate destination coordinates before zooming. Interior points and fast-escaping featureless points are replaced with a nearby slowly escaping boundary point. Automatic Journey pans while zooming into a boundary target, briefly holds at its deepest view, then pans toward the next target while zooming back out before repeating. Manual pan and wheel zoom remain unrestricted.

## Control flow in 1.8

- A compact vertical rail provides **Preview**, **Desktop**, **Status**, **Settings**, **Palette**, **Quick**, and **Equations** actions without consuming vertical space at the top of the control panel.
- **Preview** contains the one-line `centreX, centreY, scale` editor, palette, equation, iterations, zoom/colour speed and live brightness/contrast/saturation/offset controls. Animation is controlled from the Quick Controller and the explicit desktop-mode selector.
- **Preset Library…** opens a dedicated manager for loading, saving, updating, deleting, restoring, importing and exporting presets.
- **Desktop** is deliberately compact. Desktop actions are available from the main window and Quick Controller; the page links to slideshow management and Settings.
- **Settings** contains graphics quality, frame-rate, render scale, precision, adaptive resource protection, startup/pause behaviour, monitor mode and independent monitor assignments.
- **Status** contains renderer, resource, precision, monitor-layout and log information.
- The movable **Quick Controller** can remain open while the main editor is hidden. It provides live/static/slideshow application, independent preview and desktop animation toggles, coordinate jump/copy, still and high-resolution capture, preset loading, editor access, and resource diagnostics.


## Preset library and coordinates

The Preview page uses one editable coordinate field in the format `centreX, centreY, scale`. The same format is accepted by **Jump to Coordinates…** in the Quick Controller. Invalid or non-positive scale values are rejected without changing the current camera.

Choose **Preset Library…** to load a preset or maintain the local library. Built-ins remain read-only; custom presets can be saved, updated or deleted. Import and export remain JSON data-only operations.

## Quick Controller and journey waypoints

The preview remains uncluttered; operational actions are available from the movable Quick Controller and main editor. Preview and desktop zoom/colour animation have independent controls. Every app launch begins with both preview and desktop animation stopped, while the saved preset animation mode remains intact. Preset creation and naming are kept in the Preset Library dialog.

Custom Automatic Journey destinations are edited through **Journey Settings…**, available from Preview, Desktop, Settings and the Quick Controller. Use one ordered destination per line:

```text
centreX,centreY,scale,transitionSeconds,holdSeconds
```

Example:

```text
-0.743643887037151,0.131825904205330,0.004,16,2
0.285,0.01,0.028,14,2
```

Custom coordinates are followed exactly in their entered order: transition to the destination, hold, then transition directly to the next row. Transition time is constrained to 1–3600 seconds, hold time to 0–3600 seconds, and at most 128 destinations are accepted from 32768 bytes of local preset data. Automatically generated journeys still validate destinations for visible boundary detail.

## Desktop modes and image output (1.13.0)

The Desktop page uses an explicit **None / Static image / Live image / Slide show / Journey** selector with an **Apply** button and optional default-launch mode. Settings provides one default folder plus PNG, JPEG, TIFF or BMP output and a 1–100 compression/quality setting for static captures, saved images, slideshow captures and high-resolution output. Journey editing has a dedicated dialog, Equation dropdowns scroll, and the Settings, Equation and Palette windows leave the preview usable while they are open. Palette changes preview live.


## Visual regression fixtures (PH-02)

The source includes a bounded headless CPU fixture command that calls the production tiled still renderer. It renders four canonical scenes twice, compares full-width and tiled output including seam strips, and confirms that a deliberate render-affecting mutation produces a useful failure.

```powershell
.\scripts\run-visual-fixtures.ps1
```

Portable shells can use:

```bash
./scripts/run-visual-fixtures.sh
```

Artifacts are written below `test_artifacts/visual/` and are not committed automatically. Each candidate now includes the exact `mw-render-state-v1` canonical bytes and SHA-256 fingerprint used to identify its render state. Reviewed baselines may be supplied separately at `tests/baselines/visual/<fixture>/cpu/baseline.ppm`; the fixture command never promotes candidate output into that location. D3D11, OpenGL, screen-space bloom and perturbation fixtures remain pending Windows PH-02 verification and do not block PH-03 implementation under the recorded roadmap decision. See `docs/testing/VISUAL-REGRESSION.md`.

## Project-state adapters and fingerprint (PH-03)

The core exposes a bounded stable parameter-key registry plus camera and palette/post value adapters over the existing authoritative `Preset`. Compensated camera low components are preserved. Canonical render identity uses versioned, locale-independent bytes and SHA-256; preset names, IDs, UI/runtime state and paths are excluded. The transactional coordinator owns main-window palette/post scalars, deliberate camera edits and discrete built-in palette selection. Preview pan uses one stable user-gesture token per drag, and wheel zoom coalesces only within a 250 ms monotonic gap. Preset load/import plus Palette, Equation, Settings and Journey dialogs use classified transactional whole-preset replacement over candidate copies. Mutation results carry explicit origin and history eligibility. PH-03 implementation is complete, with native Windows verification for this final slice still pending.

## Complete project undo and redo (PH-04/PH-05)

The Preview page provides labelled Undo/Redo commands and Ctrl+Z/Ctrl+Y over one runtime-only bounded history. Registered scalar/camera changes use stable before/after deltas and retain drag, wheel and palette-thumb coalescing. Structural edits use exact atomic before/after project snapshots, covering custom palette stops, equations, post-processing, journey rows, preset/import application and accepted Palette, Equation, Settings and Journey dialogs. A scalar action that also changes unregistered structure automatically falls back to the atomic form, preventing partial undo. Scout Apply is one undoable camera transaction. History is not persisted; native Windows interaction verification for the exact PH-05 archive remains pending.

## Deterministic general animation evaluator (PH-06)

The platform-neutral core now includes a bounded runtime-only timeline model and immutable evaluator for future general animation. It supports stable timeline/track/keyframe IDs, compensated camera-centre tracks, logarithmic scale, shortest-path rotation, palette/post-processing controls and selected equation values with Step, Linear or Smoothstep interpolation where valid. Clamp, Loop and PingPong time modes are deterministic; duplicate enabled targets and duplicate keyframe times are rejected. Preview, wallpaper and export clocks are independent. No editor or persisted timeline format is included yet, and normal frame evaluation never mutates the project or enters undo history. See `docs/features/ANIMATION-TRACKS-PLAN.md`.
