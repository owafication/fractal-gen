# Mandelbrot Live Wallpaper

A native, offline Windows 10/11 desktop application that renders an animated Mandelbrot set behind desktop icons.

## v1.11.7

<img width="900" alt="fractal-gen" src="https://github.com/user-attachments/assets/06437913-e262-47e0-9435-6da304ba2df1" />




## Implemented scope

- GPU Mandelbrot rendering through Direct3D 11/HLSL by default, with automatic OpenGL/GLSL fallback and a static CPU-rendered fallback.
- Responsive control window with a compact vertical Preview/Desktop/Status navigation rail, a resizable live preview, wheel zoom, drag pan, reset view, smooth colouring, ten built-in palettes and ten built-in presets.
- Pop-out custom palette editor with colour swatches, ordered colour stops, add/edit/remove/reorder controls, named reusable palette presets, JSON persistence, and GPU interpolation.
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
- Preview-tab **Render Hi-Res and Save** workflow with coordinate-string input, independent width/height and DPI metadata, PNG/TIFF/BMP output, cancellable background progress, GPU Direct3D 11 tile-band export by default, explicit OpenGL tile-band fallback, CPU scanline-tiled fallback, fixed still-frame animation phase, and automatic resolution-aware iteration depth.
- Automatic zoom targets are checked for escaping boundary detail so journeys do not finish inside a black Mandelbrot interior region.
- Full-screen, battery, Remote Desktop, session lock, sleep/resume and desktop-visibility awareness.
- Mirror, Span and Independent monitor modes. Span maps the full virtual desktop into the attached WorkerW host, including negative monitor coordinates. Independent mode stores a preset assignment per display device and exposes assignment controls only when that mode is active.
- Local versioned JSON settings, strict preset import, atomic saves and corrupt-file preservation.
- A persistent control strip over the live preview with independent preview/desktop zoom and colour controls, live/static/slideshow desktop actions, desktop stop, coordinate jumping, coordinate copying, image saving, high-resolution rendering, a Quick Controller shortcut, and current desktop status information.
- A movable modeless Quick Controller that remains available when the main editor is minimised to tray. Its single Play button changes to Stop while active; the former separate pause control is removed.
- Resizable, per-monitor-DPI-aware dialogs with scrollable small-screen layouts, system message fonts, keyboard mnemonics, reliable Enter/Escape handling, focus-following scroll, high-contrast-safe native controls, equation tooltips, and remembered per-user dialog size and position.
- Rotating local diagnostics with open, copy and clear actions. No analytics or network access.
- Inno Setup installer definition and portable release packaging script.

## Built-in equation and colour libraries

The equation editor includes 45 bounded data-only presets. These cover Mandelbrot and Multibrot powers, Burning Ship and Tricorn variants, multiple Julia constants, Newton basins, rational maps, orbit traps, distance colouring, coefficient animation, transcendental transforms, powered `c` terms and complex coefficient variants.

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

The palette editor exposes 30 reusable built-in palettes. Eight are matched to the supplied reference artwork: Electric Blue and Gold, Cyan Aurora, Magenta Nebula, Golden Halo, Deep Cyan, Crimson Web, Ice Lightning and Toxic Green. Complete scene presets combine those equations and palettes so they can be loaded without configuring the two libraries separately. Built-in entries are read-only; load one and use **Save as New** to create an editable custom copy.

## Build requirements

- Windows 10 or Windows 11, x64.
- Visual Studio 2022 Build Tools or Visual Studio 2022 with **Desktop development with C++**.
- CMake 3.24 or later.
- Optional: Inno Setup 6 to produce the installer.

No third-party runtime or package-manager dependency is required.

## Render across 3 displays

<img width="1080" alt="static-render-1785048857384" src="https://github.com/user-attachments/assets/6f905026-0fc2-4c1c-9891-fa0584a6d8a5" />

## Phone Wallpaper Renders

<p align="center">
  <img width="300" alt="phone wallpaper low res6" src="https://github.com/user-attachments/assets/68f28179-30db-44ad-99a3-653b4a8fb5d2" />
  <img width="300" alt="phone wallpaper low res5" src="https://github.com/user-attachments/assets/f18733a5-b398-4b85-8b65-23ad1de25976" />
  <img width="300" alt="phone wallpaper low res" src="https://github.com/user-attachments/assets/d076776d-29db-426c-8f91-c3d987e5e92c" />

</p>

## Build

Open PowerShell in the repository root:

```powershell
Set-ExecutionPolicy -Scope Process Bypass
.\scripts\build-release.ps1
```

The script configures an x64 Release build, runs the core tests, creates a portable ZIP, and builds the installer when Inno Setup is available.

Manual build:

```powershell
cmake -S . -B build -A x64 -DMW_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

## Run

Launch `build\Release\MandelbrotWallpaper.exe`. Edit the live preview on **Preview**, then move the pointer over the preview and choose **Live to Desktop**, **Static to Desktop**, or **Slideshow to Desktop**. Preview changes are intentionally staged until they are explicitly applied to the desktop.

Use **Render Hi-Res and Save...** on the Preview tab for still output. The dialog accepts `centreX,centreY,scale`, width, height, DPI and PNG/TIFF/BMP format. GPU Direct3D 11 is the default renderer and OpenGL remains selectable as a compatibility fallback. Both GPU paths use the active precision strategy where supported and render overlapping GPU tiles, assembles a bounded scanline band and streams rows to the encoder, so the requested image is not constrained to a single GPU texture or full-frame readback allocation. The preset iteration count is treated as a minimum and is raised automatically when the camera scale and output resolution resolve finer detail, up to the existing 4096 cap. CPU scanline tiling remains available as a compatibility fallback.

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

Use **Pause Colours** beside the Colour speed slider to freeze the palette at its current offset without pausing camera movement. The button changes to **Play Colours** and resumes from the same colour position. This global state applies to both the preview and active wallpaper and persists across restarts.

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

Choose **Edit Palette…** beside the palette selector. Add, edit, remove, and reorder colour stops in the pop-out editor. The renderer interpolates the list in order and wraps the final colour back to the first so colour cycling remains continuous. Each colour row includes a visual swatch beside its hexadecimal value. Enter a name and choose **Save / Update** to store the current colour list as a reusable palette preset. Saved palettes are data-only, persist independently of fractal presets, and can be loaded, renamed, updated, or deleted from the same dialog.

The editor is designed for arbitrary-length palettes. A 4096-stop safety bound prevents malformed imported JSON from consuming unbounded memory; the GPU uploads one texel per stop when hardware limits allow and safely resamples only palettes larger than the device texture limit.

## Static wallpaper and slideshow

On **Wallpaper & Monitors**:

- **Capture Preview as Static** saves the exact current camera and colour position at full virtual-desktop resolution, adds it to the slideshow list, displays it behind the icons, and releases the wallpaper GPU renderer.
- **Add Preview to Slideshow** captures and adds the current preview to the list while restoring the previous live/stopped wallpaper state afterward.
- **Manage Slideshow…** opens the dedicated editor. Choose the folder used by new captures, add existing 32-bit BMP files, scan the selected folder, remove or reorder entries, choose the current image, set a 10–86400 second interval, and select Sequential or Shuffle playback. **Use Selected Now** starts the static wallpaper from the chosen entry.

The default capture folder is `%LOCALAPPDATA%\MandelbrotLiveWallpaper\static-renders`, but it can be changed to any accessible local folder. Removing an entry does not delete its file. The slideshow is safety-bounded to 512 paths, skips missing or invalid images, and does not restart the GPU between transitions.

## Automatic zoom target safety

Continuous Zoom and Automatic Journey validate destination coordinates before zooming. Interior points and fast-escaping featureless points are replaced with a nearby slowly escaping boundary point. Automatic Journey pans while zooming into a boundary target, briefly holds at its deepest view, then pans toward the next target while zooming back out before repeating. Manual pan and wheel zoom remain unrestricted.

## Control flow in 1.8

- A compact vertical rail provides **Preview**, **Desktop**, **Status**, **Settings**, **Palette**, **Quick**, and **Equations** actions without consuming vertical space at the top of the control panel.
- **Preview** contains the one-line `centreX, centreY, scale` editor, palette, animation, equation, iterations, zoom/colour speed and live brightness/contrast/saturation/offset controls.
- **Preset Library…** opens a dedicated manager for loading, saving, updating, deleting, restoring, importing and exporting presets.
- **Desktop** is deliberately compact. Desktop actions live in the preview hover strip and Quick Controller; the page links to slideshow management and Settings.
- **Settings** contains graphics quality, frame-rate, render scale, precision, adaptive resource protection, startup/pause behaviour, monitor mode and independent monitor assignments.
- **Status** contains renderer, resource, precision, monitor-layout and log information.
- The movable **Quick Controller** can remain open while the main editor is hidden. It provides a single **Play/Stop** toggle, save image, load preset and open editor controls with basic coordinates and resource diagnostics.


## Preset library and coordinates

The Preview page uses one editable coordinate field in the format `centreX, centreY, scale`. The same format is accepted by **Jump…** in the preview hover menu. Invalid or non-positive scale values are rejected without changing the current camera.

Choose **Preset Library…** to load a preset or maintain the local library. Built-ins remain read-only; custom presets can be saved, updated or deleted. Import and export remain JSON data-only operations.

## Preview hover controls and journey waypoints

The preview displays a persistent control strip. Preview zoom and preview colour cycling have their own controls, while desktop zoom and desktop colour cycling have separate controls labelled for the desktop. Every app launch begins with both preview and desktop animation stopped; the saved preset animation mode is retained and starts only when the user presses the relevant control. **Apply Settings Live**, static, slideshow, stop, jump, copy, image and high-resolution actions remain available. Preset creation and naming are kept in the Preset Library dialog.

Custom Automatic Journey destinations are edited under **Settings → Monitors and journey**. Use one ordered destination per line:

```text
centreX,centreY,scale,transitionSeconds,holdSeconds
```

Example:

```text
-0.743643887037151,0.131825904205330,0.004,16,2
0.285,0.01,0.028,14,2
```

Custom coordinates are followed exactly in their entered order: transition to the destination, hold, then transition directly to the next row. Transition time is constrained to 1–3600 seconds, hold time to 0–3600 seconds, and at most 128 destinations are accepted from 32768 bytes of local preset data. Automatically generated journeys still validate destinations for visible boundary detail.
