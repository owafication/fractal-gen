# Architecture

**Status:** Current compatibility overview. Canonical state and rendering contracts are owned by:

- `docs/architecture/PROJECT-STATE-AND-PARAMETERS.md`
- `docs/architecture/RENDERING-AND-EXPORT-CONTRACTS.md`
- `project_docs/DATA_AND_PERSISTENCE.md`

## Current structure

- `src/Core`: platform-neutral C++20 models, validation, JSON/settings, fractal math, deep zoom, animation, adaptive policy, Scout and still-image rendering.
- `src/Rendering`: D3D11/OpenGL implementations and GPU facade.
- `src/WindowsIntegration`: WorkerW/Progman attachment, display/system state, WIC image codec, startup, tray and wallpaper controller.
- `src/App`: native Win32 main window, preview and modeless/tool dialogs.
- `src/Infrastructure`: local paths and rotating diagnostics.
- `tests/CoreTests.cpp`: platform-neutral core tests.

The build produces a static `MandelbrotCore` library and, on Windows, one `MandelbrotWallpaper` executable.

## Current state flow

`AppSettings` (schema 9) and custom assets are loaded through `SettingsStore`. `AppWindow` edits a working preset for preview; copies are passed to renderers and `WallpaperController`. Renderer inputs use `RenderRegion` and `RenderOptions`. Long-running Scout and high-resolution rendering use bounded request/progress/cancellation contracts.

## Security and failure boundaries

- Imported equations/presets remain bounded data; no imported shader/script source is executed.
- Settings parse depth/size and numeric values are bounded.
- Settings writes use temporary replacement and preserve corrupt input.
- Desktop integration attaches an app-owned window and leaves the configured Windows wallpaper unchanged.
- GPU fallback/recovery is bounded; no uncontrolled restart loop.
- Logs are local and rotating.

## Planned direction

Add production-renderer visual fixtures, immutable snapshot/parameter adapters and render fingerprints before undo, general animation or video export. Current models remain authoritative during adapter migration; project/user/runtime state must not become parallel mutable authorities.
