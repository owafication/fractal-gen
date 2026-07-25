#!/usr/bin/env python3
from pathlib import Path
import re
import sys

root = Path(__file__).resolve().parents[1]
required = [
    "CMakeLists.txt",
    "scripts/build-release.ps1",
    "scripts/build-windows.cmd",
    "src/App/main.cpp",
    "src/App/AdaptivePerformanceDialog.cpp",
    "src/App/AppWindow.cpp",
    "src/App/HighResRenderDialog.cpp",
    "src/App/PaletteEditorDialog.cpp",
    "src/App/EquationEditorDialog.cpp",
    "src/App/PrecisionDialog.cpp",
    "src/App/PresetManagerDialog.cpp",
    "src/App/QuickControllerWindow.cpp",
    "src/App/SettingsDialog.cpp",
    "src/App/SlideshowDialog.cpp",
    "src/App/app.manifest",
    "src/App/resources.rc",
    "src/Core/AdaptivePerformance.cpp",
    "src/Core/Models.cpp",
    "src/Core/DeepZoom.cpp",
    "src/Core/SettingsStore.cpp",
    "src/Core/StillImageRenderer.cpp",
    "src/Rendering/OpenGLRenderer.cpp",
    "src/WindowsIntegration/DesktopHost.cpp",
    "src/WindowsIntegration/WallpaperController.cpp",
    "installer/MandelbrotWallpaper.iss",
    "README.md",
    "LICENSE",
]
missing = [path for path in required if not (root / path).is_file()]
if missing:
    print("Missing required files:", *missing, sep="\n- ")
    sys.exit(1)

text = "\n".join(path.read_text(encoding="utf-8", errors="ignore") for path in (root / "src").rglob("*.*") if path.suffix in {".cpp", ".h"})
for forbidden in [r"https?://", r"WinHttp", r"InternetOpen", r"curl_easy", r"ShellExecute.*https"]:
    if re.search(forbidden, text, re.IGNORECASE):
        print(f"Forbidden network-related pattern found: {forbidden}")
        sys.exit(1)

checks = {
    "WorkerW integration": "WorkerW",
    "GPU shader": "sampleFractal",
    "full-screen detection": "IsForegroundWindowFullscreen",
    "session lock": "WTS_SESSION_LOCK",
    "static fallback": "BuildStaticFallback",
    "settings corruption preservation": ".corrupt-",
    "mirror mode": "MonitorMode::Mirror",
    "span mode": "MonitorMode::Span",
    "independent mode": "MonitorMode::Independent",
    "custom palette editor": "PaletteEditorDialog",
    "custom palette shader": "uCustomPalette",
    "static render capture": "CaptureAndUseStatic",
    "static render slideshow": "cycleEnabled",
    "slideshow manager dialog": "SlideshowDialog",
    "assignable static capture folder": "storageDirectory",
    "slideshow image ordering": "StaticSlideshowOrder",
    "slideshow add current preview": "Add Preview to Slideshow",
    "safe auto zoom target": "FindInterestingFractalTarget",
    "palette colour swatches": "DrawColourItem",
    "saved palette presets": "customPalettePresets",
    "ordered journey transition": "journeyLegStartCamera_",
    "bounded equation editor": "EquationEditorDialog",
    "equation shader uniforms": "uEquationQuadratic",
    "equation JSON persistence": "EquationToJson",
    "deep zoom precision dialog": "PrecisionDialog",
    "native float64 shader": "uCentreD",
    "split precision shader": "directSplit",
    "perturbation shader": "vec3 perturb",
    "arbitrary reference orbit": "BuildReferenceOrbitArbitrary",
    "precision persistence": "arbitraryPrecisionBits",
    "precision dialog": "PrecisionDialog",
    "native float64 precision": "kDoubleFragmentShader",
    "split high-low precision": "directSplit",
    "perturbation renderer": "UploadReferenceOrbit",
    "arbitrary precision reference": "BuildReferenceOrbitArbitrary",
    "precision settings persistence": "arbitraryPrecisionBits",
    "vertical preview navigation": "NavigationPreviewButton",
    "explicit wallpaper apply flow": "Apply Preview as Live Wallpaper",
    "paired iteration input": "IterationsEdit",
    "paired frame-rate input": "FpsEdit",
    "paired render-scale input": "RenderScaleEdit",
    "live visual controls": "BrightnessEdit",
    "span host coordinate mapping": "MapDesktopRectToHost",
    "independent assignment feedback": "Monitor Assignment Applied",
    "boundary-rich journey validation": "IsBoundaryRichFractalTarget",
    "precision mode descriptions": "DescriptionForMode",
    "adaptive resource dialog": "AdaptivePerformanceDialog",
    "sustained overload controller": "AdaptivePerformanceController",
    "invisible equation suppression": "VisibleChangeDetector",
    "process resource sampling": "processCpuPercent",
    "quick preview zoom control": "QuickControllerCommands::TogglePreviewZoom",
    "quick preview colour control": "QuickControllerCommands::TogglePreviewColours",
    "quick desktop zoom control": "QuickControllerCommands::ToggleDesktopZoom",
    "quick desktop colour control": "QuickControllerCommands::ToggleDesktopColours",
    "quick desktop actions": "QuickControllerCommands::SlideshowDesktop",
    "preview coordinate jump": "JumpToCoordinates",
    "quick coordinate info": "coordinatesLabel_",
    "custom journey waypoint persistence": "automaticJourneyWaypoints",
    "custom journey waypoint parser": "ParseJourneyScript",
    "independent zoom motion": "SetMotionEnabled",
    "preset library popup": "PresetManagerDialog",
    "single-line coordinate editor": "CoordinatesEdit",
    "persistent quick controller": "QuickControllerWindow",
    "quick controller tray command": "TrayCommands::Controller",
    "settings graphics grouping": "PerformanceProfileCombo",
    "settings monitor assignment": "ApplyMonitorAssignment",
    "zero-GPU user pause snapshot": "PauseAndReleaseGpu",
}

cmake_text = (root / "CMakeLists.txt").read_text(encoding="utf-8")
resource_text = (root / "src/App/resources.rc").read_text(encoding="utf-8")
if "/MANIFEST:NO" not in cmake_text or "/MANIFESTUAC" in cmake_text:
    print("MSVC manifest generation is not safely disabled for the resource-embedded manifest.")
    sys.exit(1)
if resource_text.count("RT_MANIFEST") != 1:
    print("Expected exactly one resource-embedded application manifest.")
    sys.exit(1)

app_window_text = (root / "src/App/AppWindow.cpp").read_text(encoding="utf-8")
app_window_header_text = (root / "src/App/AppWindow.h").read_text(encoding="utf-8")
if "std::string(text.begin(), text.end())" in app_window_text:
    print("Unsafe wide-to-narrow fallback remains in AppWindow.cpp.")
    sys.exit(1)

missing_checks = [name for name, token in checks.items() if token not in text]
if missing_checks:
    print("Required implementation markers missing:", *missing_checks, sep="\n- ")
    sys.exit(1)


slideshow_text = (root / "src/App/SlideshowDialog.cpp").read_text(encoding="utf-8")
if "std::max(widest, size.cx + 28)" in slideshow_text:
    print("MSVC-ambiguous LONG/int std::max remains in SlideshowDialog.cpp.")
    sys.exit(1)
if "std::max(widest, static_cast<int>(size.cx) + 28)" not in slideshow_text:
    print("Expected explicit slideshow text-width conversion is missing.")
    sys.exit(1)

print("Source structure and offline-policy checks passed.")

renderer_text = (root / "src/Rendering/OpenGLRenderer.cpp").read_text(encoding="utf-8")
for incompatible in [
    "int samples = clamp(uAA, 1, 4);",
    "int samples = int(clamp(float(uAA), 1.0, 4.0));",
]:
    if incompatible in renderer_text:
        print("GLSL 1.20-incompatible anti-aliasing conversion remains in the fragment shader.")
        sys.exit(1)
if not re.search(r"int\s+samples\s*=\s*uAA\s*;", renderer_text):
    print("Expected CPU-clamped GLSL 1.20 anti-aliasing assignment is missing.")
    sys.exit(1)
if "fallbackWidth_ = 480" in (root / "src/WindowsIntegration/WallpaperController.cpp").read_text(encoding="utf-8"):
    print("Low-resolution fixed CPU fallback remains enabled.")
    sys.exit(1)
if "message == WM_PAINT" not in app_window_text or "GPU preview unavailable" not in app_window_text:
    print("Preview failure paint handling is missing.")
    sys.exit(1)

if "ColourCycleButton" not in app_window_text or "SetColourCyclingEnabled" not in text:
    print("Colour-cycling play/pause control or animation propagation is missing.")
    sys.exit(1)
if "colourCyclingEnabled" not in (root / "src/Core/SettingsStore.cpp").read_text(encoding="utf-8"):
    print("Colour-cycling state persistence is missing.")
    sys.exit(1)

settings_dialog_text = (root / "src/App/SettingsDialog.cpp").read_text(encoding="utf-8")
if "MaximumIterationsEdit" not in settings_dialog_text:
    print("Maximum-iterations text input is missing from SettingsDialog.cpp.")
    sys.exit(1)
if "4096" not in (root / "src/App/PaletteEditorDialog.cpp").read_text(encoding="utf-8"):
    print("Custom palette safety bound is missing.")
    sys.exit(1)
palette_editor_text = (root / "src/App/PaletteEditorDialog.cpp").read_text(encoding="utf-8")
if "LBS_OWNERDRAWFIXED" not in palette_editor_text or "Save / Update" not in palette_editor_text:
    print("Palette swatches or saved-palette controls are missing.")
    sys.exit(1)
animation_text = (root / "src/Core/Animation.cpp").read_text(encoding="utf-8")
if "JourneyPhase::Transition" not in animation_text or "Interpolate(journeyLegStartCamera_, target.camera" not in animation_text:
    print("Automatic Journey direct transition phase is missing.")
    sys.exit(1)

if "JourneyTargetCount() >= 8" not in (root / "tests/CoreTests.cpp").read_text(encoding="utf-8"):
    print("Automatic Journey target-variety coverage is missing.")
    sys.exit(1)
if "IsInterestingFractalTarget(point.camera.centreX" not in animation_text:
    print("Automatic Journey can still accept a black interior centre point.")
    sys.exit(1)
if "Desktop actions are available from the preview hover menu and Quick Controller" not in app_window_text:
    print("Desktop actions were not moved to the hover menu and Quick Controller.")
    sys.exit(1)
if "run shader code" not in (root / "tests/CoreTests.cpp").read_text(encoding="utf-8"):
    print("Equation import security coverage is missing.")
    sys.exit(1)
slideshow_text = (root / "src/App/SlideshowDialog.cpp").read_text(encoding="utf-8")
for marker in ["Add Images...", "Add BMPs from Folder", "Move Up", "Move Down", "Set as Current", "Use Selected Now", "Shuffle"]:
    if marker not in slideshow_text:
        print(f"Static slideshow editor marker missing: {marker}")
        sys.exit(1)
if "StaticStorageDirectory()" not in app_window_text or "AddPreviewToSlideshow()" not in app_window_text:
    print("Static capture folder routing or add-to-slideshow action is missing.")
    sys.exit(1)
print("Feature checks for palettes, equations, journey motion, static slideshow management, safe zoom targets, and selectable deep-zoom precision passed.")


waypoint_markers = [
    "centreX,centreY,scale,transitionSeconds,holdSeconds",
    "points.size() < 128U",
    "32768U",
    "std::clamp(values[3], 1.0, 3600.0)",
]
for marker in waypoint_markers:
    if marker not in text:
        print(f"Automatic Journey waypoint validation marker missing: {marker}")
        sys.exit(1)
if "SetMotionEnabled(false)" not in (root / "tests/CoreTests.cpp").read_text(encoding="utf-8"):
    print("Independent zoom-motion regression coverage is missing.")
    sys.exit(1)
if "automaticJourneyWaypoints == custom.automaticJourneyWaypoints" not in (root / "tests/CoreTests.cpp").read_text(encoding="utf-8"):
    print("Automatic Journey waypoint persistence coverage is missing.")
    sys.exit(1)
print("Quick controls and custom Automatic Journey waypoint checks passed.")

models_text = (root / "src/Core/Models.h").read_text(encoding="utf-8")
settings_store_text = (root / "src/Core/SettingsStore.cpp").read_text(encoding="utf-8")
if "int schemaVersion{8}" not in models_text or '"adaptive"' not in settings_store_text:
    print("Adaptive settings schema version 8 persistence is missing.")
    sys.exit(1)
if "GetProcessMemoryInfo" not in text or "GetProcessTimes" not in text:
    print("Windows process resource sampling is incomplete.")
    sys.exit(1)
if "ShouldRender(descriptors" not in (root / "src/WindowsIntegration/WallpaperController.cpp").read_text(encoding="utf-8"):
    print("Wallpaper invisible-frame suppression is not connected before renderer submission.")
    sys.exit(1)
if "previewChangeDetector_.ShouldRender" not in app_window_text:
    print("Preview invisible-frame suppression is not connected before renderer submission.")
    sys.exit(1)
print("Adaptive resource protection and invisible equation-work suppression checks passed.")

# 1.8 UI redline structure checks.
for required_file in [
    "src/App/PresetManagerDialog.cpp",
    "src/App/PresetManagerDialog.h",
    "src/App/QuickControllerWindow.cpp",
    "src/App/QuickControllerWindow.h",
]:
    if not (root / required_file).is_file():
        print(f"1.8 UI source file missing: {required_file}")
        sys.exit(1)
if "WC_TABCONTROLW" in app_window_text:
    print("The old horizontal tab control remains in the main UI.")
    sys.exit(1)
for marker in [
    "NavigationPreviewButton",
    "PresetLibraryButton",
    "CoordinatesEdit",
    "NavigationControllerButton",
    "OpenQuickController",
    "QuickControllerCommands::ApplySettingsLive",
]:
    if marker not in app_window_text:
        print(f"1.8 main UI marker missing: {marker}")
        sys.exit(1)
for marker in [
    "PerformanceProfileCombo",
    "FrameRateEdit",
    "RenderScaleEdit",
    "MonitorModeCombo",
    "ApplyAssignmentButton",
    "StartWindowsCheck",
]:
    if marker not in settings_dialog_text:
        print(f"1.8 settings marker missing: {marker}")
        sys.exit(1)
if 'part.find_first_not_of(L" \\t\\r\\n")' not in app_window_text:
    print("Coordinate parsing whitespace literal is malformed.")
    sys.exit(1)
if "src/App/PresetManagerDialog.cpp" not in cmake_text or "src/App/QuickControllerWindow.cpp" not in cmake_text:
    print("New 1.8 Windows UI modules are not included in CMake.")
    sys.exit(1)
print("1.8 vertical navigation, Preset Library, Quick Controller and consolidated Settings checks passed.")

# 1.9 advanced equation and persistent-control checks.
advanced_markers = {
    "powers 1-12": "int power{2}",
    "rational reciprocal power": "reciprocalPower",
    "Julia fixed parameter": "juliaParameter",
    "initial z modes": "InitialZMode",
    "complex transforms": "EquationUnaryTransform",
    "Newton convergence": "newtonMode",
    "orbit traps": "OrbitTrapType",
    "distance estimation": "distanceEstimate",
    "glow post process": "kPostProcessFragmentShader",
    "saved equation presets": "customEquationPresets",
    "coefficient animation": "animateCoefficients",
    "persistent quick controls": "QuickControllerWindow",
    "settings navigation button": "NavigationSettingsButton",
    "palette navigation button": "NavigationPaletteButton",
    "quick navigation button": "NavigationControllerButton",
    "equation navigation button": "NavigationEquationButton",
}
missing_advanced = [name for name, token in advanced_markers.items() if token not in text]
if missing_advanced:
    print("1.9 advanced feature markers missing:", *missing_advanced, sep="\n- ")
    sys.exit(1)
quick_text = (root / "src/App/QuickControllerWindow.cpp").read_text(encoding="utf-8")
quick_header = (root / "src/App/QuickControllerWindow.h").read_text(encoding="utf-8")
if 'L"Play / Resume"' in quick_text or 'QuickControllerCommands::Pause' in text or 'constexpr unsigned Stop =' in quick_header:
    print("Quick Controller still contains the redundant pause/stop controls.")
    sys.exit(1)
if "EquationEditorDialog::Show(" not in app_window_text or "settings_.customEquationPresets" not in app_window_text:
    print("Independent saved equation presets are not connected to the equation editor.")
    sys.exit(1)
print("1.9 advanced equations, colouring and Quick Controller checks passed.")

# 1.9.1 Windows build regression: removed overlay deadline member stays absent.
if "previewOverlayHideAt_" in app_window_text or "previewOverlayHideAt_" in (root / "src/App/AppWindow.h").read_text(encoding="utf-8"):
    print("Stale previewOverlayHideAt_ reference would break the MSVC Win32 build.")
    sys.exit(1)
print("1.9.1 persistent-overlay MSVC declaration regression check passed.")

# 1.9.2 release packaging regression: a single Inno Setup path must not be
# treated as an object that is assumed to expose .Count under StrictMode.
build_release = (root / "scripts/build-release.ps1").read_text(encoding="utf-8")
if "$IsccCandidates.Count" in build_release or "$IsccCandidates[0]" in build_release:
    print("Release script still assumes a pipeline result is always an array.")
    sys.exit(1)
if "Select-Object -First 1" not in build_release or "if ($null -ne $Iscc)" not in build_release:
    print("Release script is missing the scalar-safe Inno Setup lookup.")
    sys.exit(1)
print("1.9.2 scalar-safe release packaging regression check passed.")

# 1.9.3 navigation-rail construction regression: declarations and layout are
# insufficient unless the four action buttons are actually created.
for member, control_id in [
    ("navigationSettingsButton_", "NavigationSettingsButton"),
    ("navigationPaletteButton_", "NavigationPaletteButton"),
    ("navigationControllerButton_", "NavigationControllerButton"),
    ("navigationEquationButton_", "NavigationEquationButton"),
]:
    if member + " = MakeControl(" not in app_window_text or control_id not in app_window_text:
        print(f"Navigation rail button is declared but not constructed: {member}")
        sys.exit(1)
print("1.9.3 navigation action-button construction check passed.")


# 1.9.4 compact controls regression: the removed preview overlay must stay absent,
# and its useful actions must remain available through the Quick Controller.
for removed in [
    "PreviewOverlay", "previewOverlay", "ShowPreviewOverlay", "UpdatePreviewOverlay",
    'L"PREVIEW HOVER CONTROLS"',
]:
    if removed in app_window_text or removed in app_window_header_text:
        print(f"Removed preview hover-menu marker remains: {removed}")
        sys.exit(1)
for marker in [
    "QuickControllerCommands::ApplySettingsLive",
    "QuickControllerCommands::TogglePreviewZoom",
    "QuickControllerCommands::ToggleDesktopZoom",
    "QuickControllerCommands::JumpToCoordinates",
    "OpenQuickController",
]:
    if marker not in quick_text and marker not in quick_header and marker not in app_window_text:
        print(f"Quick Controller replacement marker missing: {marker}")
        sys.exit(1)
print("1.9.4 removed preview overlay and Quick Controller replacement checks passed.")

# 1.10 responsive and accessible Win32 dialog regression checks.
for required_file in ["src/App/DialogSupport.cpp", "src/App/DialogSupport.h"]:
    if not (root / required_file).is_file():
        print(f"Responsive-dialog support file missing: {required_file}")
        sys.exit(1)
if "src/App/DialogSupport.cpp" not in cmake_text:
    print("Responsive-dialog support is not included in the Windows target.")
    sys.exit(1)

dialog_files = [
    "AdaptivePerformanceDialog.cpp",
    "EquationEditorDialog.cpp",
    "PaletteEditorDialog.cpp",
    "PrecisionDialog.cpp",
    "PresetManagerDialog.cpp",
    "SettingsDialog.cpp",
    "SlideshowDialog.cpp",
]
for filename in dialog_files:
    dialog_text = (root / "src/App" / filename).read_text(encoding="utf-8")
    for marker in [
        'App/DialogSupport.h',
        'ResponsiveDialogLayout layout',
        'layout.Initialise',
        'WM_GETMINMAXINFO',
        'WM_SIZE',
        'WM_DPICHANGED',
        'WS_EX_CONTROLPARENT',
        'WS_THICKFRAME',
        'ProcessModalDialogMessage',
        'RememberDialogPlacement',
    ]:
        if marker not in dialog_text:
            print(f"Responsive/accessibility marker missing from {filename}: {marker}")
            sys.exit(1)
    if re.search(r'CreateFontW\(-1[56]', dialog_text):
        print(f"Fixed-pixel dialog font remains in {filename}.")
        sys.exit(1)

support_text = (root / "src/App/DialogSupport.cpp").read_text(encoding="utf-8")
for marker in [
    "CreateResponsiveDialogFont",
    "AccessibleControlStyle",
    "EnsureFocusedControlVisible",
    "SetScrollInfo",
    "DialogPlacementStore",
    'dialog-layout.txt',
    "DialogTooltipManager::Initialise",
    "GetSysColor",
]:
    source_pool = support_text + palette_editor_text
    if marker not in source_pool:
        print(f"Responsive/accessibility support marker missing: {marker}")
        sys.exit(1)

if "DialogTooltipManager tooltips" not in (root / "src/App/EquationEditorDialog.cpp").read_text(encoding="utf-8"):
    print("Advanced equation controls are missing accessible explanatory tooltips.")
    sys.exit(1)
if "ScaleDialogMetric(28, state->dpi)" not in palette_editor_text:
    print("Owner-drawn palette rows are not DPI-scaled.")
    sys.exit(1)
slideshow_text = (root / "src/App/SlideshowDialog.cpp").read_text(encoding="utf-8")
if "ScaleDialogMetric(23, state->dpi)" not in slideshow_text:
    print("Slideshow rows are not DPI-scaled.")
    sys.exit(1)
quick_text = (root / "src/App/QuickControllerWindow.cpp").read_text(encoding="utf-8")
for marker in ["ProcessDialogMessage", "WS_EX_CONTROLPARENT", "WM_DPICHANGED", "RememberDialogPlacement"]:
    if marker not in quick_text:
        print(f"Quick Controller accessibility marker missing: {marker}")
        sys.exit(1)
if "quickController_.ProcessDialogMessage(message)" not in app_window_text:
    print("Quick Controller keyboard dialog navigation is not connected to the main message loop.")
    sys.exit(1)
manifest_text = (root / "src/App/app.manifest").read_text(encoding="utf-8")
if 'PerMonitorV2,PerMonitor' not in manifest_text:
    print("Per-monitor-v2 DPI awareness is missing from the application manifest.")
    sys.exit(1)
if 'assemblyIdentity version="1.11.7.0"' not in manifest_text:
    print("Application manifest identity was not updated to 1.11.7.0.")
    sys.exit(1)
for marker in [
    "horizontalOffset_ = MulDiv(horizontalOffset_",
    "verticalOffset_ = MulDiv(verticalOffset_",
    "std::ifstream input{Path()}",
]:
    if marker not in support_text:
        print(f"Responsive dialog support regression marker missing: {marker}")
        sys.exit(1)
print("1.10 responsive, scrollable, DPI-aware and keyboard-accessible dialog checks passed.")

# 1.10.1 Quick Controller and persistent preview command regression checks.
quick_required_groups = [
    ['QuickControllerCommands::ToggleZoom', 'QuickControllerCommands::TogglePreviewZoom'],
    ['QuickControllerCommands::ToggleColours', 'QuickControllerCommands::TogglePreviewColours'],
    ['QuickControllerCommands::CopyCoordinates'],
    ['QuickControllerCommands::ExitApp'],
    ['L"&Copy Coordinates"', 'L"Copy Coordinates"'],
    ['L"E&xit App"', 'L"Exit App"'],
    ['zoomMotionEnabled ? L"Pause Zoom" : L"Start Zoom"', 'previewZoomMotionEnabled ? L"Stop Preview Zoom" : L"Start Preview Zoom"'],
    ['colourCyclingEnabled ? L"Pause Colours" : L"Start Colours"', 'previewColourCyclingEnabled ? L"Stop Preview Colours" : L"Start Preview Colours"'],
]
for group in quick_required_groups:
    if not any(marker in quick_text or marker in quick_header or marker in app_window_text for marker in group):
        print(f"1.10.1 Quick Controller marker missing: {group[0]}")
        sys.exit(1)
for marker in [
    'QuickControllerCommands::ApplySettingsLive',
    'QuickControllerCommands::CopyCoordinates',
    'QuickControllerCommands::SaveImage',
    'QuickControllerCommands::ApplySettingsLive) SetWallpaper()',
    'QuickControllerCommands::CopyCoordinates) CopyCoordinates()',
    'QuickControllerCommands::SaveImage) AddPreviewToSlideshow()',
]:
    if marker not in quick_text and marker not in quick_header and marker not in app_window_text:
        print(f"1.10.1 Quick Controller command missing: {marker}")
        sys.exit(1)
for removed in [
    'QuickControllerCommands::SavePresetAs',
    'L"Save Preset &As..."',
    'PreviewOverlaySaveAsButton',
    'L"Save Preset As..."',
]:
    if removed in quick_text or removed in quick_header or removed in app_window_text or removed in app_window_header_text:
        print(f"Removed compact-menu preset action remains: {removed}")
        sys.exit(1)
if app_window_text.count('ShowSelectedTab();') < 1 or 'ShowSelectedTab();\n    ShowSelectedTab();' in app_window_text:
    print("Duplicate main-window tab initialisation remains.")
    sys.exit(1)
print("1.10.1 Quick Controller command checks passed.")

# 1.10.3 background, tiled, multi-format high-resolution render checks.
high_res_text = (root / "src/App/HighResRenderDialog.cpp").read_text(encoding="utf-8")
still_renderer_text = (root / "src/Core/StillImageRenderer.cpp").read_text(encoding="utf-8")
still_renderer_header = (root / "src/Core/StillImageRenderer.h").read_text(encoding="utf-8")
for marker in [
    'L"Render Hi-Res and Save..."',
    'RenderHighResButton) OpenHighResRenderDialog()',
    'HighResRenderDialog::Show',
]:
    if marker not in app_window_text:
        print(f"1.10.3 Preview-tab high-resolution render marker missing: {marker}")
        sys.exit(1)
for marker in [
    'GUID_ContainerFormatPng',
    'GUID_ContainerFormatTiff',
    'GUID_ContainerFormatBmp',
    'SetResolution',
    'std::thread',
    'cancelRequested',
    'PROGRESS_CLASSW',
    'kRenderProgressMessage',
    'RenderStillImageTiled',
    'SaveCompletedOutput',
    'StillRenderPreview',
]:
    if marker not in high_res_text:
        print(f"1.10.3 high-resolution dialog marker missing: {marker}")
        sys.exit(1)
for marker in [
    'tileStart += tileWidth',
    'std::vector<std::uint32_t> row',
    'std::vector<std::uint32_t> tile',
    'peakWorkingPixels',
    'Still render cancelled.',
    'previewMaximumWidth',
]:
    if marker not in still_renderer_text and marker not in still_renderer_header:
        print(f"1.10.3 tiled still-render marker missing: {marker}")
        sys.exit(1)
for marker in ['src/Core/StillImageRenderer.cpp', 'src/App/HighResRenderDialog.cpp', 'windowscodecs', 'ole32']:
    if marker not in cmake_text:
        print(f"1.10.3 CMake integration marker missing: {marker}")
        sys.exit(1)
if "$Version = '1.11.7'" not in (root / "scripts/build-release.ps1").read_text(encoding="utf-8"):
    print("Release packaging version was not updated to 1.11.7.")
    sys.exit(1)
if '#define AppVersion "1.11.7"' not in (root / "installer/MandelbrotWallpaper.iss").read_text(encoding="utf-8"):
    print("Installer version was not updated to 1.11.7.")
    sys.exit(1)
print("1.10.3 tiled background PNG/TIFF/BMP high-resolution render checks passed.")


# 1.11.0 expanded equation, palette and scene libraries plus reliable preset saves.
models_header_text = (root / "src/Core/Models.h").read_text(encoding="utf-8")
models_text = (root / "src/Core/Models.cpp").read_text(encoding="utf-8")
math_text = (root / "src/Core/MandelbrotMath.cpp").read_text(encoding="utf-8")
settings_store_text = (root / "src/Core/SettingsStore.cpp").read_text(encoding="utf-8")
deep_zoom_text = (root / "src/Core/DeepZoom.cpp").read_text(encoding="utf-8")
equation_editor_text = (root / "src/App/EquationEditorDialog.cpp").read_text(encoding="utf-8")
palette_editor_text = (root / "src/App/PaletteEditorDialog.cpp").read_text(encoding="utf-8")
tests_text = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")
for marker in [
    "int parameterPower{1};",
    "BuiltInPalettePresets",
]:
    if marker not in models_header_text:
        print(f"1.11.0 model marker missing: {marker}")
        sys.exit(1)
for marker in [
    '"Reference: Scaled c — z^2 + 1.2c"',
    '"Reference: Constant add — z^2 + c + 0.5"',
    '"Reference: Scaled z and c — 1.2z^2 + c"',
    '"Reference: Add to z — z^2 + 0.5z + c"',
    '"Reference: Swap z and c — z + c^2"',
    '"Reference: Absolute z — |z|^2 + c"',
    '"Reference: Minus c — z^2 - c"',
    'make("reference-blue-gold"',
    'make("reference-cyan-aurora"',
    'make("reference-magenta-nebula"',
    'make("reference-golden-halo"',
    'make("reference-deep-cyan"',
    'make("reference-crimson-web"',
    'make("reference-ice-lightning"',
    'make("reference-toxic-green"',
    'addScene("reference-swap-crimson"',
]:
    if marker not in models_text:
        print(f"1.11.0 built-in library marker missing: {marker}")
        sys.exit(1)
for marker in [
    "equation.parameterPower",
    "PowInteger(c, equation.parameterPower)",
]:
    if marker not in math_text:
        print(f"1.11.0 powered-parameter math marker missing: {marker}")
        sys.exit(1)
for marker in [
    "uniform int uParameterPower",
    "cpowInt(c,uParameterPower)",
    "cpowD(c,uParameterPower)",
    'uniform1i("uParameterPower", region.equation.parameterPower)',
]:
    if marker not in renderer_text:
        print(f"1.11.0 powered-parameter OpenGL marker missing: {marker}")
        sys.exit(1)
for marker in [
    '"parameterPower"',
]:
    if marker not in settings_store_text:
        print(f"1.11.0 equation persistence marker missing: {marker}")
        sys.exit(1)
if "equation.parameterPower == 1" not in deep_zoom_text:
    print("1.11.0 incompatible perturbation guard for powered c terms is missing.")
    sys.exit(1)
for marker in [
    "ParameterPowerEdit",
    'L"c power"',
    'L"C · c^r"',
    "equation.parameterPower",
]:
    if marker not in equation_editor_text:
        print(f"1.11.0 equation editor marker missing: {marker}")
        sys.exit(1)
for marker in [
    "BuiltInPalettePresets()",
    "builtInPaletteCount",
    'L"Built-in — "',
    "CustomPaletteLibrary",
]:
    if marker not in palette_editor_text:
        print(f"1.11.0 palette library marker missing: {marker}")
        sys.exit(1)
for marker in [
    "bool AppWindow::SaveSettings(std::string* errorOut)",
    "settings_.customPresets.pop_back();",
    'L"Preset saved successfully and selected."',
    "QuickControllerCommands::RenderHighRes",
]:
    if marker not in app_window_text:
        print(f"1.11.0 preset-save or high-resolution command marker missing: {marker}")
        sys.exit(1)
for marker in [
    "TestBuiltInEquationAndPaletteLibraries",
    "palettes.size() >= 30U",
    "names.size() >= 45U",
    "scenes.size() >= 35U",
]:
    if marker not in tests_text:
        print(f"1.11.0 core regression test marker missing: {marker}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.11.7' not in cmake_text:
    print("CMake project version was not updated to 1.11.7.")
    sys.exit(1)
print("1.11.0 expanded equation/palette/scene library and preset persistence checks passed.")


# 1.11.1 independent preview/desktop runtime controls, now exposed through
# the Quick Controller after removal of the preview hover menu.
for marker in [
    "QuickControllerCommands::TogglePreviewZoom",
    "QuickControllerCommands::TogglePreviewColours",
    "QuickControllerCommands::ToggleDesktopZoom",
    "QuickControllerCommands::ToggleDesktopColours",
    'L"Stop Preview Zoom"', 'L"Stop Preview Colours"',
    'L"Stop Desktop Zoom"', 'L"Stop Desktop Colours"',
    "desktopZoomMotionEnabled_", "desktopColourCyclingEnabled_",
    "previewColourCyclingEnabled_",
    "TogglePreviewZoomMotion()", "TogglePreviewColourCycling()",
    "ToggleDesktopZoomMotion()", "ToggleDesktopColourCycling()",
]:
    if marker not in app_window_text and marker not in app_window_header_text and marker not in quick_text and marker not in quick_header:
        print(f"1.11.1 split runtime-control marker missing: {marker}")
        sys.exit(1)
for removed in [
    "PreviewOverlay", "previewOverlay", "PreviewOverlaySendPreviewButton",
    "SendPreviewToDesktop", 'L"Send Preview to Desktop"',
    'snapshot.id = "preview-live-session"',
]:
    if removed in app_window_text or removed in app_window_header_text:
        print(f"Removed preview-overlay action remains: {removed}")
        sys.exit(1)
preview_toggle_start = app_window_text.index("void AppWindow::TogglePreviewZoomMotion()")
preview_toggle_end = app_window_text.index("void AppWindow::ToggleDesktopZoomMotion()")
if "wallpaperController_." in app_window_text[preview_toggle_start:preview_toggle_end]:
    print("Preview zoom toggle still changes the desktop renderer.")
    sys.exit(1)
colour_toggle_start = app_window_text.index("void AppWindow::TogglePreviewColourCycling()")
colour_toggle_end = app_window_text.index("void AppWindow::StopWallpaper()")
if "wallpaperController_." in app_window_text[colour_toggle_start:colour_toggle_end]:
    print("Preview colour toggle still changes the desktop renderer.")
    sys.exit(1)
print("1.11.1 independent preview/desktop Quick Controller checks passed.")

# 1.11.2 bounded GPU tile-band export and resolution-aware still quality.
renderer_header_text = (root / "src/Rendering/OpenGLRenderer.h").read_text(encoding="utf-8")
for marker in [
    "MaximumRenderDimension() const noexcept",
    "maximumBandPixels",
    "overlapPixels = 1U",
    "CameraForStillRenderTile",
    "options.timeSeconds = request.timeSeconds",
    'L"GPU OpenGL tiled - compatibility fallback"',
    "bandPixels",
    "encoder.WriteRow(row, error)",
]:
    if marker not in high_res_text and marker not in renderer_header_text:
        print(f"1.11.2 GPU tile-band marker missing: {marker}")
        sys.exit(1)
for marker in [
    "ResolveStillRenderQuality",
    "detailStopsBeyond1080p",
    "referencePixelSpan = 3.0 / 1080.0",
    "quality.maximumIterations",
    "CameraForStillRenderTile",
]:
    if marker not in still_renderer_text and marker not in still_renderer_header:
        print(f"1.11.2 resolution-aware still-quality marker missing: {marker}")
        sys.exit(1)
for marker in [
    "TestStillRenderQualityAndTileCamera",
    "Deep high-resolution stills should receive a larger automatic iteration budget.",
    "Automatic iteration scaling should be explicitly bypassable.",
    "Top-down GPU tiles should map the top half to positive imaginary coordinates.",
]:
    if marker not in tests_text:
        print(f"1.11.2 core regression marker missing: {marker}")
        sys.exit(1)
for obsolete in [
    "const int hostWidth",
    "const int hostHeight",
    "BuildPreviewFromTopDownPixels",
    "std::vector<std::uint32_t> topDown",
]:
    if obsolete in high_res_text:
        print(f"1.11.2 obsolete full-frame GPU allocation remains: {obsolete}")
        sys.exit(1)
for required_doc in ["docs/FEATURES-1.11.2.md", "docs/VERIFICATION-1.11.2.md"]:
    if not (root / required_doc).is_file():
        print(f"1.11.2 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.11.7' not in cmake_text:
    print("CMake project version was not updated to 1.11.7.")
    sys.exit(1)
print("1.11.2 GPU tile-band export and resolution-aware still-quality checks passed.")


# 1.11.3 Direct3D 11 renderer facade, automatic fallback and explicit hi-res backends.
d3d_header_text = (root / "src/Rendering/Direct3D11Renderer.h").read_text(encoding="utf-8")
d3d_text = (root / "src/Rendering/Direct3D11Renderer.cpp").read_text(encoding="utf-8")
gpu_header_text = (root / "src/Rendering/GpuRenderer.h").read_text(encoding="utf-8")
gpu_text = (root / "src/Rendering/GpuRenderer.cpp").read_text(encoding="utf-8")
for marker in [
    "D3D11CreateDeviceAndSwapChain",
    'CompileShader("FractalMain", "ps_5_0"',
    "DXGI_FORMAT_R8G8B8A8_UNORM",
    "UploadReferenceOrbit",
    "D3D11_USAGE_STAGING",
    "CapturePixels",
    "D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION",
    "Direct3D 11 shader compilation failed",
]:
    if marker not in d3d_text and marker not in d3d_header_text:
        print(f"1.11.3 Direct3D 11 marker missing: {marker}")
        sys.exit(1)
for marker in [
    "GpuBackendPreference::Automatic",
    "GpuBackendPreference::Direct3D11",
    "GpuBackendPreference::OpenGL",
    "ActiveGpuBackend::Direct3D11",
    "ActiveGpuBackend::OpenGL",
    "trying OpenGL",
]:
    if marker not in gpu_text and marker not in gpu_header_text:
        print(f"1.11.3 GPU facade marker missing: {marker}")
        sys.exit(1)
for marker in [
    'L"GPU Direct3D 11 tiled - default, supports huge renders"',
    'L"GPU OpenGL tiled - compatibility fallback"',
    "GpuBackendPreference gpuPreference",
    "backend != RenderBackend::CpuTiled",
]:
    if marker not in high_res_text:
        print(f"1.11.3 hi-res backend marker missing: {marker}")
        sys.exit(1)
for marker in [
    "src/Rendering/Direct3D11Renderer.cpp",
    "src/Rendering/GpuRenderer.cpp",
    "d3d11",
    "dxgi",
    "d3dcompiler",
]:
    if marker not in cmake_text:
        print(f"1.11.3 build integration marker missing: {marker}")
        sys.exit(1)
for rel in [
    "src/App/AppWindow.h",
    "src/WindowsIntegration/WallpaperController.h",
]:
    text = (root / rel).read_text(encoding="utf-8")
    if 'Rendering/GpuRenderer.h' not in text or "GpuRenderer" not in text:
        print(f"1.11.3 renderer facade was not connected: {rel}")
        sys.exit(1)
for required_doc in ["docs/FEATURES-1.11.3.md", "docs/VERIFICATION-1.11.3.md"]:
    if not (root / required_doc).is_file():
        print(f"1.11.3 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.11.7' not in cmake_text:
    print("CMake project version was not updated to 1.11.7.")
    sys.exit(1)
print("1.11.3 Direct3D 11 renderer and fallback checks passed.")


# 1.11.7 Preset Library naming, static runtime defaults and structured journeys.
preset_dialog_text = (root / "src/App/PresetManagerDialog.cpp").read_text(encoding="utf-8")
animation_text = (root / "src/Core/Animation.cpp").read_text(encoding="utf-8")
animation_header_text = (root / "src/Core/Animation.h").read_text(encoding="utf-8")
settings_dialog_text = (root / "src/App/SettingsDialog.cpp").read_text(encoding="utf-8")
for marker in [
    "SaveAsNameEdit",
    'L"Name for Save Preview as New"',
    "EM_SETLIMITTEXT, 120",
    "std::string& saveAsName",
    "state.saveAsName = saveAsName",
    "saveAsName = state.saveAsName",
    'L"Enter a name for the new preset."',
    "name.size() > 120U",
]:
    if marker not in preset_dialog_text and marker not in (root / "src/App/PresetManagerDialog.h").read_text(encoding="utf-8"):
        print(f"1.11.7 preset-popup naming marker missing: {marker}")
        sys.exit(1)
for marker in [
    "SaveAsNewPreset(saveAsName)",
    "const std::string& requestedName",
    "name.size() > 120U",
]:
    if marker not in app_window_text and marker not in app_window_header_text:
        print(f"1.11.7 named save-as marker missing: {marker}")
        sys.exit(1)
for removed in [
    "PreviewOverlayPresetNameEdit",
    "PreviewOverlayRenameButton",
    "RenameSelectedPreset",
    "PreviewOverlaySaveAsButton",
    "PreviewOverlaySendPreviewButton",
    "SendPreviewToDesktop",
    "QuickControllerCommands::SavePresetAs",
]:
    if removed in app_window_text or removed in app_window_header_text or removed in quick_text or removed in quick_header:
        print(f"1.11.7 removed UI action remains: {removed}")
        sys.exit(1)
static_launch_text = "\n".join([
    app_window_header_text,
    app_window_text,
    (root / "src/Core/Models.h").read_text(encoding="utf-8"),
    (root / "src/WindowsIntegration/WallpaperController.h").read_text(encoding="utf-8"),
    (root / "src/WindowsIntegration/WallpaperController.cpp").read_text(encoding="utf-8"),
])
for marker in [
    "bool zoomMotionEnabled_{false};",
    "bool previewColourCyclingEnabled_{false};",
    "bool desktopZoomMotionEnabled_{false};",
    "bool desktopColourCyclingEnabled_{false};",
    "runtimeSettings.general.colourCyclingEnabled = desktopColourCyclingEnabled_",
    "bool colourCyclingEnabled{false};",
    "bool motionEnabled_{false};",
    "preserveRuntimeColourState",
]:
    if marker not in static_launch_text:
        print(f"1.11.7 static-launch marker missing: {marker}")
        sys.exit(1)
for marker in [
    "journey_ = scriptedPoints",
    "journeyLegStartCamera_",
    "JourneyPhase::Transition",
    "JourneyPhase::Hold",
    "Interpolate(journeyLegStartCamera_, target.camera",
    "journeyIndex_ = (journeyIndex_ + 1U) % journey_.size()",
    "A custom script is an exact ordered route",
]:
    if marker not in animation_text and marker not in animation_header_text:
        print(f"1.11.7 structured-journey marker missing: {marker}")
        sys.exit(1)
for obsolete in ["BuildJourneyExitCamera", "JourneyPhase::ZoomOut", "journeyExitCamera_", "journeyWideCamera_"]:
    if obsolete in animation_text or obsolete in animation_header_text:
        print(f"1.11.7 obsolete journey behaviour remains: {obsolete}")
        sys.exit(1)
for marker in [
    'L"Ordered Automatic Journey (optional)',
    "TransitionSeconds,HoldSeconds",
]:
    if marker not in settings_dialog_text:
        print(f"1.11.7 journey editor marker missing: {marker}")
        sys.exit(1)
for marker in [
    "A structured custom journey should contain exactly the supplied destinations.",
    "The first custom destination should be reached exactly after its transition time.",
    "After the hold, the journey should transition directly to the next exact coordinate.",
]:
    if marker not in tests_text:
        print(f"1.11.7 structured-journey test marker missing: {marker}")
        sys.exit(1)
for required_doc in ["docs/FEATURES-1.11.7.md", "docs/VERIFICATION-1.11.7.md"]:
    if not (root / required_doc).is_file():
        print(f"1.11.7 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.11.7' not in cmake_text:
    print("CMake project version was not updated to 1.11.7.")
    sys.exit(1)
print("1.11.7 preset-popup naming, static launch and structured journey checks passed.")


# 1.11.7 Preview hover menu removed; Play and Stop Desktop removed from Quick Controller.
for marker in [
    "ApplySettingsLive", "StaticDesktop", "TogglePreviewZoom",
    "TogglePreviewColours", "JumpToCoordinates", "ToggleDesktopZoom",
    "ToggleDesktopColours", "SlideshowDesktop", "RenderHighRes",
    'L"Apply Settings Live"', 'L"Static Desktop"', 'L"Slideshow Desktop"',
    'L"Jump to Coordinates..."', 'L"Render Hi-Res..."',
    'L"Start Preview Zoom"', 'L"Start Preview Colours"',
    'L"Start Desktop Zoom"', 'L"Start Desktop Colours"',
]:
    if marker not in quick_text and marker not in quick_header:
        print(f"1.11.7 Quick Controller marker missing: {marker}")
        sys.exit(1)
for marker in [
    "QuickControllerCommands::ApplySettingsLive",
    "QuickControllerCommands::StaticDesktop",
    "QuickControllerCommands::TogglePreviewZoom",
    "QuickControllerCommands::TogglePreviewColours",
    "QuickControllerCommands::JumpToCoordinates",
    "QuickControllerCommands::ToggleDesktopZoom",
    "QuickControllerCommands::ToggleDesktopColours",
    "QuickControllerCommands::SlideshowDesktop",
    "QuickControllerCommands::RenderHighRes",
    "UpdateQuickController();",
]:
    if marker not in app_window_text and marker not in quick_text:
        print(f"1.11.7 app integration marker missing: {marker}")
        sys.exit(1)
for removed in [
    "PreviewOverlay", "previewOverlay", "ShowPreviewOverlay", "UpdatePreviewOverlay",
    "QuickControllerCommands::Play", "QuickControllerCommands::StopDesktop",
    "playButton_", "stopDesktopButton_", 'L"Play"', 'L"Stop Desktop"',
]:
    if removed in app_window_text or removed in app_window_header_text or removed in quick_text or removed in quick_header:
        print(f"1.11.7 removed control remains: {removed}")
        sys.exit(1)
for required_doc in ["docs/FEATURES-1.11.7.md", "docs/VERIFICATION-1.11.7.md"]:
    if not (root / required_doc).is_file():
        print(f"1.11.7 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.11.7' not in cmake_text:
    print("CMake project version was not updated to 1.11.7.")
    sys.exit(1)
print("1.11.7 hover-menu removal and Quick Controller cleanup checks passed.")
