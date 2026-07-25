# Architecture

## Goal

Keep fractal rendering, animation, settings, Windows desktop attachment, system-state detection and user-interface concerns independently reviewable.

## Components

### Core

`src/Core` is platform-neutral and contains:

- Typed settings and preset models.
- Validation, compensated camera coordinates and bounded deep-zoom precision settings.
- Built-in presets and performance profiles.
- Mandelbrot escape/smooth-colouring reference math.
- Compensated camera coordinates and CPU reference-orbit generation for deep zoom.
- Animation state, logarithmic zoom interpolation and automatic journeys.
- A bounded JSON parser and versioned settings/preset serialisation.

The core has no Windows, graphics or network dependency and is covered by `MandelbrotCoreTests`.

### Rendering

`src/Rendering/GpuRenderer.*` is the renderer facade used by preview, desktop and still export. It attempts `Direct3D11Renderer` first on Windows and falls back to `OpenGLRenderer` when Direct3D device creation, HLSL compilation or an automatic-mode render fails. Explicit high-resolution backend choices do not silently change backend.

Both implementations evaluate user settings through fixed shaders; user data never becomes shader source. Off-screen render targets implement real render-scale reduction before the image is post-processed and presented. Direct3D 11 supports float32, split high/low float and both CPU-reference perturbation modes. OpenGL additionally uses native shader float64 when the driver exposes it. Arbitrary-precision orbit arithmetic stays in the platform-neutral `Core/DeepZoom` module.

The renderer supports multiple regions, allowing one wallpaper host to draw:

- Mirror: the same camera in each monitor viewport.
- Span: one camera across the virtual desktop.
- Independent: one animation/preset per monitor viewport.

The renderer exposes independently selectable precision paths behind the same `RenderOptions` contract: float32, native shader float64, split-float compensated arithmetic, and GPU perturbation using either a double or bounded fixed-point CPU reference orbit. Automatic mode resolves the strategy from zoom depth, hardware capability, equation compatibility and user candidate toggles. The renderer makes one recovery attempt after a context/device failure. The wallpaper controller switches to a static CPU-generated Mandelbrot image if recovery fails.

### Windows integration

`src/WindowsIntegration` contains:

- `DesktopHost`: locates Progman/WorkerW and attaches/detaches the wallpaper host.
- `DisplayManager`: monitor bounds, orientation implications and DPI discovery.
- `WallpaperController`: virtual-desktop host, rendering regions, Explorer reattachment and static fallback.
- `SystemStateMonitor`: full-screen, power, lock, Remote Desktop and desktop visibility state.
- `StartupManager`: per-user HKCU Run registration.
- `TrayIcon`: notification-area lifecycle and commands.

No module patches Explorer, writes system files or requires administrator privileges.

### Application UI

`src/App` contains the native Win32 control window, live preview and advanced settings dialog. UI actions modify typed models, validate them, then pass copies to rendering and wallpaper services.

### Infrastructure

`src/Infrastructure` provides user-local paths and size-limited rotating logs. Logs intentionally exclude window titles, keystrokes, desktop filenames and unrelated application activity.

## Runtime flow

1. Load versioned settings from `%LOCALAPPDATA%`.
2. Preserve an invalid file with a `.corrupt-<timestamp>.json` suffix and load defaults when needed.
3. Start the preview GPU renderer.
4. On **Set as Wallpaper**, create one virtual-desktop window and attach it behind the desktop icon host.
5. The application timer enforces the selected frame limit and updates preview/wallpaper animation.
6. System state may pause rendering without destroying the wallpaper host.
7. Stop or exit detaches and destroys the host, revealing the unchanged prior Windows wallpaper.

## Failure containment

- Invalid imported JSON is rejected before it enters application state.
- Numeric fields are finite-checked and clamped.
- JSON input size and nesting depth are bounded.
- Settings saves use a temporary file and atomic replacement.
- GPU startup and runtime recovery are each bounded; there is no restart loop.
- Explorer attachment is periodically revalidated without modifying Explorer.
- Shutdown destroys GPU contexts before windows and releases tray/session resources.

## Equation editor

`EquationSettings` is a validated data model shared by core escape calculations and the GPU renderer. The UI never compiles user-provided shader text. `EquationEditorDialog` edits bounded complex coefficients, `SettingsStore` serialises them as JSON numbers, and the active GPU backend passes them through fixed GLSL uniforms or an HLSL constant buffer.

## Deep zoom

`src/Core/DeepZoom.*` owns compensated camera helpers and CPU reference-orbit generation. Arbitrary reference arithmetic is bounded fixed-point data, not executable input. `PrecisionDialog` edits the global strategy and candidate toggles. Settings schema version 8 persists these values together with the static slideshow folder, playback order, selected image, and adaptive rendering thresholds. The active Direct3D 11 or OpenGL renderer reports its active strategy and hardware capabilities in diagnostics.


## Adaptive rendering

`src/Core/AdaptivePerformance.*` contains two platform-independent controls. `AdaptivePerformanceController` evaluates sustained low-FPS, process-CPU and working-set conditions and exposes bounded pause/resume decisions. `VisibleChangeDetector` compares camera and palette state against the last frame submitted to the renderer, converting movement into screen-space pixels before allowing another equation pass.

Windows process metrics are sampled in `SystemStateMonitor`; policy remains in Core. `AppWindow` coordinates preview and wallpaper pauses, while `WallpaperController` owns its own visible-change detector so the desktop renderer can remain idle independently of the preview.
