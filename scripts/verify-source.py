#!/usr/bin/env python3
from pathlib import Path
import json
import re
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
required = [
    "CMakeLists.txt",
    "CMakePresets.json",
    "scripts/build-release.ps1",
    "scripts/validate-windows-release.ps1",
    "scripts/validate-windows-release.cmd",
    "scripts/build-windows.cmd",
    "src/App/main.cpp",
    "src/App/AdaptivePerformanceDialog.cpp",
    "src/App/AppWindow.cpp",
    "src/App/GeneralAnimationEditorDialog.cpp",
    "src/App/GeneralAnimationEditorDialog.h",
    "src/App/FrameSequenceExportDialog.cpp",
    "src/App/FrameSequenceExportDialog.h",
    "src/App/VideoExportDialog.cpp",
    "src/App/VideoExportDialog.h",
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
    "src/Core/GeneralAnimation.cpp",
    "src/Core/GeneralAnimation.h",
    "src/Core/FrameSequenceExport.cpp",
    "src/Core/FrameSequenceExport.h",
    "src/Core/ExternalVideoExport.cpp",
    "src/Core/ExternalVideoExport.h",
    "src/Tools/ExternalVideoFixtureTool.cpp",
    "src/Core/Models.cpp",
    "src/Core/DeepZoom.cpp",
    "src/Core/Precision/HighPrecisionBackend.cpp",
    "src/Core/Precision/HighPrecisionBackend.h",
    "src/Core/SettingsStore.cpp",
    "src/Core/StillImageRenderer.cpp",
    "src/Infrastructure/Paths.cpp",
    "tests/PathsTests.cpp",
    "src/Rendering/OpenGLRenderer.cpp",
    "src/WindowsIntegration/DesktopHost.cpp",
    "src/WindowsIntegration/ExternalProcess.cpp",
    "src/WindowsIntegration/ExternalProcess.h",
    "src/WindowsIntegration/ImageCodec.cpp",
    "src/WindowsIntegration/ImageCodec.h",
    "src/WindowsIntegration/WallpaperController.cpp",
    "installer/MandelbrotWallpaper.iss",
    "docs/testing/WINDOWS-RELEASE-VALIDATION.md",
    "docs/features/OFFLINE-EXPORT-PLAN.md",
    "project_docs/HIGH_PRECISION_DEPENDENCY_REVIEW.md",
    "scripts/verify-third-party.py",
    "third_party/boost-multiprecision-1.83.0/PACKAGE.json",
    "third_party/boost-multiprecision-1.83.0/FILES.SHA256",
    "third_party/boost-multiprecision-1.83.0/LICENSE_1_0.txt",
    "third_party/boost-multiprecision-1.83.0/include/boost/multiprecision/cpp_bin_float.hpp",
    "README.md",
    "THIRD_PARTY_NOTICES.md",
    "LICENSE",
]
missing = [path for path in required if not (root / path).is_file()]
if missing:
    print("Missing required files:", *missing, sep="\n- ")
    sys.exit(1)

third_party_result = subprocess.run(
    [sys.executable, str(root / "scripts/verify-third-party.py")],
    cwd=root,
    check=False,
)
if third_party_result.returncode != 0:
    sys.exit(third_party_result.returncode)

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
    "file-backed static presentation": "LoadStaticImageByIndex",
    "settings corruption preservation": ".corrupt-",
    "mirror mode": "MonitorMode::Mirror",
    "span mode": "MonitorMode::Span",
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
    "explicit exported-video wallpaper flow": "Use Exported Video...",
    "paired iteration input": "IterationsEdit",
    "paired frame-rate input": "FpsEdit",
    "paired render-scale input": "RenderScaleEdit",
    "live visual controls": "BrightnessEdit",
    "span host coordinate mapping": "MapDesktopRectToHost",
    "boundary-rich journey validation": "IsBoundaryRichFractalTarget",
    "precision mode descriptions": "DescriptionForMode",
    "adaptive resource dialog": "AdaptivePerformanceDialog",
    "sustained overload controller": "AdaptivePerformanceController",
    "invisible equation suppression": "VisibleChangeDetector",
    "process resource sampling": "processCpuPercent",
    "quick preview zoom control": "QuickControllerCommands::TogglePreviewZoom",
    "quick preview colour control": "QuickControllerCommands::TogglePreviewColours",
    "quick video desktop action": "QuickControllerCommands::VideoDesktop",
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
    "file-backed desktop pause": "PausePresentation",
}

for removed_live_route in [
    "Apply Preview as Live Wallpaper",
    "QuickControllerCommands::ApplySettingsLive",
    "QuickControllerCommands::ToggleDesktopZoom",
    "QuickControllerCommands::ToggleDesktopColours",
    "wallpaperController_.Start(instance_",
]:
    if removed_live_route in text:
        print(f"Removed continuously rendered desktop route remains: {removed_live_route}")
        sys.exit(1)

cmake_text = (root / "CMakeLists.txt").read_text(encoding="utf-8")
for required_marker in [
    "MandelbrotBoostMultiprecision",
    "BOOST_MP_STANDALONE",
    "boost-multiprecision-1.83.0",
    "src/Core/Precision/HighPrecisionBackend.cpp",
]:
    if required_marker not in cmake_text:
        print(f"Reviewed high-precision package CMake marker missing: {required_marker}")
        sys.exit(1)

installer_text = (root / "installer/MandelbrotWallpaper.iss").read_text(encoding="utf-8")
if "THIRD_PARTY_NOTICES.md" not in cmake_text or "THIRD_PARTY_NOTICES.md" not in installer_text:
    print("Third-party notices are missing from portable or installer packaging.")
    sys.exit(1)

high_precision_text = (root / "src/Core/Precision/HighPrecisionBackend.cpp").read_text(encoding="utf-8")
for required_marker in [
    "cpp_bin_float<",
    "boost::multiprecision::et_off",
    "BuildIndependentHighPrecisionReferenceOrbit",
    "PerturbationProfile::Unsupported",
    '"1.83.0"',
    '"BSL-1.0"',
]:
    if required_marker not in high_precision_text:
        print(f"Reviewed high-precision backend marker missing: {required_marker}")
        sys.exit(1)

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
if ("Desktop actions are available from the preview hover menu and Quick Controller" not in app_window_text and
        "Desktop actions are available from the Quick Controller" not in app_window_text and
        "Choose a desktop mode, then press Apply" not in app_window_text):
    print("Desktop action guidance is missing from the desktop page.")
    sys.exit(1)
if "run shader code" not in (root / "tests/CoreTests.cpp").read_text(encoding="utf-8"):
    print("Equation import security coverage is missing.")
    sys.exit(1)
slideshow_text = (root / "src/App/SlideshowDialog.cpp").read_text(encoding="utf-8")
for marker in ["Add Images...", "Add Images from Folder", "Move Up", "Move Down", "Set as Current", "Use Selected Now", "Shuffle"]:
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
if "int schemaVersion{12}" not in models_text or '"adaptive"' not in settings_store_text:
    print("Adaptive settings schema version 12 persistence is missing.")
    sys.exit(1)
if "GetProcessMemoryInfo" not in text or "GetProcessTimes" not in text:
    print("Windows process resource sampling is incomplete.")
    sys.exit(1)
wallpaper_controller_text = (root / "src/WindowsIntegration/WallpaperController.cpp").read_text(encoding="utf-8")
if "RenderFrame(" in wallpaper_controller_text or "BuildStaticFallback" in wallpaper_controller_text:
    print("Continuously rendered desktop or CPU-render fallback code remains in the wallpaper controller.")
    sys.exit(1)
for removed_monitor_assignment in ["MonitorMode::Independent", "monitorPresetAssignments", "ApplyMonitorAssignment"]:
    if removed_monitor_assignment in text:
        print(f"Removed per-monitor assignment marker remains: {removed_monitor_assignment}")
        sys.exit(1)
for failure_marker in ["kVideoPlaybackFailedMessage", "TakeRuntimeError", "none of its configured image files could be loaded"]:
    if failure_marker not in text:
        print(f"Fail-closed desktop media marker is missing: {failure_marker}")
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
    "QuickControllerCommands::VideoDesktop",
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
if "Get-Command ISCC.exe -CommandType Application" not in build_release or \
        "$Iscc = $IsccCommand.Source" not in build_release:
    print("Release script cannot discover a per-user or PATH-provided Inno Setup compiler.")
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
    "QuickControllerCommands::VideoDesktop",
    "QuickControllerCommands::TogglePreviewZoom",
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
for marker in [
    "ProcessKeyboardDialogMessage(window_, message)",
    "SetWindowPos(window_",
    "CreateResponsiveDialogFont(mainDpi_)",
]:
    if marker not in app_window_text:
        print(f"Main-window keyboard/DPI regression marker missing: {marker}")
        sys.exit(1)
manifest_text = (root / "src/App/app.manifest").read_text(encoding="utf-8")
if 'PerMonitorV2,PerMonitor' not in manifest_text:
    print("Per-monitor-v2 DPI awareness is missing from the application manifest.")
    sys.exit(1)
if 'assemblyIdentity version="1.13.1.0"' not in manifest_text:
    print("Application manifest identity was not updated to 1.13.1.0.")
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
    'QuickControllerCommands::VideoDesktop',
    'QuickControllerCommands::CopyCoordinates',
    'QuickControllerCommands::SaveImage',
    'QuickControllerCommands::VideoDesktop) SelectVideoWallpaper()',
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
if 'L"Add to Slideshow"' not in quick_text:
    print("Quick Controller slideshow action has a misleading label.")
    sys.exit(1)
for marker in [
    "workingPreset_.exactCamera->centreX.CanonicalText()",
    "workingPreset_.exactCamera->centreY.CanonicalText()",
    "workingPreset_.exactCamera->halfHeight.CanonicalText()",
]:
    if marker not in app_window_text:
        print(f"Exact-coordinate copy regression marker missing: {marker}")
        sys.exit(1)
if "UpdateActionState" not in (root / "src/App/PresetManagerDialog.cpp").read_text(encoding="utf-8"):
    print("Preset Library does not gate custom-only actions by selection.")
    sys.exit(1)
print("1.10.1 Quick Controller command checks passed.")

# 1.10.3 background, tiled, multi-format high-resolution render checks.
high_res_text = (root / "src/App/HighResRenderDialog.cpp").read_text(encoding="utf-8")
high_res_codec_text = (root / "src/WindowsIntegration/ImageCodec.cpp").read_text(encoding="utf-8")
still_renderer_text = (root / "src/Core/StillImageRenderer.cpp").read_text(encoding="utf-8")
still_renderer_header = (root / "src/Core/StillImageRenderer.h").read_text(encoding="utf-8")
for marker in [
    'L"Render Hi-Res..."',
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
    if marker not in high_res_text and marker not in high_res_codec_text:
        print(f"1.10.3 high-resolution dialog/codec marker missing: {marker}")
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
if "$Version = '1.13.1'" not in (root / "scripts/build-release.ps1").read_text(encoding="utf-8"):
    print("Release packaging version was not updated to 1.13.0.")
    sys.exit(1)
if '#define AppVersion "1.13.1"' not in (root / "installer/MandelbrotWallpaper.iss").read_text(encoding="utf-8"):
    print("Installer version was not updated to 1.13.0.")
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
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.11.0 expanded equation/palette/scene library and preset persistence checks passed.")


# 1.11.1 preview runtime controls plus file-backed desktop actions.
for marker in [
    "QuickControllerCommands::TogglePreviewZoom",
    "QuickControllerCommands::TogglePreviewColours",
    "QuickControllerCommands::VideoDesktop",
    'L"Stop Preview Zoom"', 'L"Stop Preview Colours"',
    "previewColourCyclingEnabled_",
    "TogglePreviewZoomMotion()", "TogglePreviewColourCycling()",
]:
    if marker not in app_window_text and marker not in app_window_header_text and marker not in quick_text and marker not in quick_header:
        print(f"1.11.1 split runtime-control marker missing: {marker}")
        sys.exit(1)
for removed in [
    "PreviewOverlay", "previewOverlay", "PreviewOverlaySendPreviewButton",
    "SendPreviewToDesktop", 'L"Send Preview to Desktop"',
    'snapshot.id = "preview-live-session"',
    "desktopZoomMotionEnabled_", "desktopColourCyclingEnabled_",
    "ToggleDesktopZoomMotion()", "ToggleDesktopColourCycling()",
]:
    if removed in app_window_text or removed in app_window_header_text:
        print(f"Removed preview-overlay action remains: {removed}")
        sys.exit(1)
preview_toggle_start = app_window_text.index("void AppWindow::TogglePreviewZoomMotion()")
preview_toggle_end = app_window_text.index("void AppWindow::ApplyPreviewAsSlideshowWallpaper()")
if "wallpaperController_." in app_window_text[preview_toggle_start:preview_toggle_end]:
    print("Preview zoom toggle still changes the file-backed desktop presentation.")
    sys.exit(1)
colour_toggle_start = app_window_text.index("void AppWindow::TogglePreviewColourCycling()")
colour_toggle_end = app_window_text.index("void AppWindow::StopWallpaper()")
if "wallpaperController_." in app_window_text[colour_toggle_start:colour_toggle_end]:
    print("Preview colour toggle still changes the file-backed desktop presentation.")
    sys.exit(1)
print("1.11.1 preview-only animation and file-backed desktop checks passed.")

# 1.11.2 bounded GPU tile-band export and resolution-aware still quality.
renderer_header_text = (root / "src/Rendering/OpenGLRenderer.h").read_text(encoding="utf-8")
for marker in [
    "MaximumRenderDimension() const noexcept",
    "maximumBandPixels",
    "StillRenderTileOverlapPixels",
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
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
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
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
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
    (root / "src/Core/Models.cpp").read_text(encoding="utf-8"),
    (root / "src/WindowsIntegration/WallpaperController.h").read_text(encoding="utf-8"),
    (root / "src/WindowsIntegration/WallpaperController.cpp").read_text(encoding="utf-8"),
])
for marker in [
    "bool zoomMotionEnabled_{false};",
    "bool previewColourCyclingEnabled_{false};",
    "bool colourCyclingEnabled{false};",
    "enum class DesktopMode { None, StaticImage, Slideshow, Video }",
    "StartVideo(HINSTANCE instance",
    "UsingVideo() const noexcept",
    'value == "live-image" || value == "journey"',
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
journey_dialog_text = (root / "src/App/JourneySettingsDialog.cpp").read_text(encoding="utf-8")
for alternatives in [
    ['L"Ordered Automatic Journey (optional)', 'L"Structured route: transition'],
    ["TransitionSeconds,HoldSeconds"],
]:
    if not any(marker in settings_dialog_text or marker in journey_dialog_text for marker in alternatives):
        print(f"1.11.7 journey editor marker missing: {alternatives[0]}")
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
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.11.7 preset-popup naming, static launch and structured journey checks passed.")


# 1.11.7 Preview hover menu removed; Play and Stop Desktop removed from Quick Controller.
for marker in [
    "VideoDesktop", "StaticDesktop", "TogglePreviewZoom",
    "TogglePreviewColours", "JumpToCoordinates",
    "SlideshowDesktop", "RenderHighRes",
    'L"Video Desktop..."', 'L"Static Desktop"', 'L"Slideshow Desktop"',
    'L"Jump to Coordinates..."', 'L"Render Hi-Res..."',
    'L"Start Preview Zoom"', 'L"Start Preview Colours"',
]:
    if marker not in quick_text and marker not in quick_header:
        print(f"1.11.7 Quick Controller marker missing: {marker}")
        sys.exit(1)
for marker in [
    "QuickControllerCommands::VideoDesktop",
    "QuickControllerCommands::StaticDesktop",
    "QuickControllerCommands::TogglePreviewZoom",
    "QuickControllerCommands::TogglePreviewColours",
    "QuickControllerCommands::JumpToCoordinates",
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
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.11.7 hover-menu removal and Quick Controller cleanup checks passed.")

# 1.12.0 Tricorn/Multicorn colour-texture phase.
models_header_120 = (root / "src/Core/Models.h").read_text(encoding="utf-8")
models_source_120 = (root / "src/Core/Models.cpp").read_text(encoding="utf-8")
math_header_120 = (root / "src/Core/MandelbrotMath.h").read_text(encoding="utf-8")
math_source_120 = (root / "src/Core/MandelbrotMath.cpp").read_text(encoding="utf-8")
settings_store_120 = (root / "src/Core/SettingsStore.cpp").read_text(encoding="utf-8")
still_renderer_120 = (root / "src/Core/StillImageRenderer.cpp").read_text(encoding="utf-8")
renderer_types_120 = (root / "src/Rendering/RendererTypes.h").read_text(encoding="utf-8")
open_gl_120 = (root / "src/Rendering/OpenGLRenderer.cpp").read_text(encoding="utf-8")
d3d_120 = (root / "src/Rendering/Direct3D11Renderer.cpp").read_text(encoding="utf-8")
d3d_header_120 = (root / "src/Rendering/Direct3D11Renderer.h").read_text(encoding="utf-8")
palette_dialog_120 = (root / "src/App/PaletteEditorDialog.cpp").read_text(encoding="utf-8")
wallpaper_120 = (root / "src/WindowsIntegration/WallpaperController.cpp").read_text(encoding="utf-8")
high_res_120 = (root / "src/App/HighResRenderDialog.cpp").read_text(encoding="utf-8")

for marker in [
    "enum class PaletteInterpolation { Linear, Smoothstep }",
    "double paletteFrequency{8.0};",
    "double paletteGamma{1.0};",
    "PaletteInterpolation paletteInterpolation{PaletteInterpolation::Linear};",
    "bool stripeAverageEnabled{false};",
    "double stripeDensity{8.0};",
    "double stripePhase{0.0};",
    "double stripeStrength{0.0};",
    "int stripeStartIteration{8};",
]:
    if marker not in models_header_120:
        print(f"1.12.0 model marker missing: {marker}")
        sys.exit(1)
for marker in [
    '"Tricorn / Mandelbar (power 2)"',
    '"Multicorn (power 3)"',
    '"Multicorn (power 4)"',
    '"cyan-fire-ring"',
    '"Tricorn Cyan Fire Ring"',
    "paletteFrequency = 28.0",
    "PaletteInterpolation::Smoothstep",
    "stripeAverageEnabled = true",
]:
    if marker not in models_source_120:
        print(f"1.12.0 built-in library marker missing: {marker}")
        sys.exit(1)
for marker in [
    '{"stripeAverageEnabled", equation.stripeAverageEnabled}',
    '{"stripeDensity", equation.stripeDensity}',
    '{"stripePhase", equation.stripePhase}',
    '{"stripeStrength", equation.stripeStrength}',
    '{"stripeStartIteration", equation.stripeStartIteration}',
    '{"paletteFrequency", preset.paletteFrequency}',
    '{"paletteGamma", preset.paletteGamma}',
    '{"paletteInterpolation", ToString(preset.paletteInterpolation)}',
    "PaletteInterpolationFromString",
]:
    if marker not in settings_store_120:
        print(f"1.12.0 persistence marker missing: {marker}")
        sys.exit(1)
for marker in [
    "double stripeAverage{0.5};",
    "equation.stripeAverageEnabled",
    "equation.stripeDensity * std::arg(z)",
    "stripeSum / static_cast<double>(stripeSamples)",
]:
    if marker not in math_header_120 and marker not in math_source_120:
        print(f"1.12.0 stripe calculation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "position *= preset.paletteFrequency",
    "preset.paletteGamma",
    "PaletteInterpolation::Smoothstep",
    "escape.stripeAverage",
    "(firstIndex + 1U) % palette.size()",
]:
    if marker not in still_renderer_120:
        print(f"1.12.0 CPU colour mapping marker missing: {marker}")
        sys.exit(1)
for marker in [
    "double paletteFrequency{8.0};",
    "double paletteGamma{1.0};",
    "PaletteInterpolation paletteInterpolation",
]:
    if marker not in renderer_types_120:
        print(f"1.12.0 renderer contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "uPaletteFrequency", "uPaletteGamma", "uPaletteInterpolation",
    "uStripeEnabled", "uStripeDensity", "uStripePhase",
    "uStripeStrength", "uStripeStartIteration", "stripeAverage",
]:
    if marker not in open_gl_120:
        print(f"1.12.0 OpenGL marker missing: {marker}")
        sys.exit(1)
for marker in [
    "cPaletteControls", "cStripeControls", "palettePhase",
    "stripeAverage", "constants.paletteControls", "constants.stripeControls",
]:
    if marker not in d3d_120 and marker not in d3d_header_120:
        print(f"1.12.0 Direct3D marker missing: {marker}")
        sys.exit(1)
for marker in [
    "FrequencyEdit", "GammaEdit", "InterpolationCombo", "StripeCheck",
    "StripeDensityEdit", "StripePhaseEdit", "StripeStrengthEdit", "StripeStartEdit",
    'L"Stripe-average texture"',
]:
    if marker not in palette_dialog_120:
        print(f"1.12.0 Palette Editor marker missing: {marker}")
        sys.exit(1)
for text_120, source_name in [
    (app_window_text, "AppWindow"),
    (high_res_120, "HighResRenderDialog"),
    (wallpaper_120, "WallpaperController"),
]:
    for marker in ["paletteFrequency", "paletteGamma", "paletteInterpolation"]:
        if marker not in text_120:
            print(f"1.12.0 render-region propagation missing in {source_name}: {marker}")
            sys.exit(1)
for marker in [
    "Palette frequency, gamma and interpolation should persist.",
    "Stripe-average colouring should return a finite normalised orbit texture value.",
    "The equation template library should expose Tricorn and higher-order Multicorn families.",
    "The tuned Tricorn Cyan Fire Ring scene should include independent distance edge lighting and bloom.",
]:
    if marker not in tests_text:
        print(f"1.12.0 test marker missing: {marker}")
        sys.exit(1)
for required_doc in [
    "docs/FRACTAL-STYLE-BUILD-PLAN.md",
    "docs/FEATURES-1.12.0.md",
    "docs/VERIFICATION-1.12.0.md",
]:
    if not (root / required_doc).is_file():
        print(f"1.12.0 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.12.0 Tricorn/Multicorn colour-texture checks passed.")


# 1.12.1 conjugate Jacobian distance estimation and independent edge lighting.
models_header_121 = (root / "src/Core/Models.h").read_text(encoding="utf-8")
models_source_121 = (root / "src/Core/Models.cpp").read_text(encoding="utf-8")
math_header_121 = (root / "src/Core/MandelbrotMath.h").read_text(encoding="utf-8")
math_source_121 = (root / "src/Core/MandelbrotMath.cpp").read_text(encoding="utf-8")
settings_store_121 = (root / "src/Core/SettingsStore.cpp").read_text(encoding="utf-8")
still_renderer_121 = (root / "src/Core/StillImageRenderer.cpp").read_text(encoding="utf-8")
open_gl_121 = (root / "src/Rendering/OpenGLRenderer.cpp").read_text(encoding="utf-8")
d3d_121 = (root / "src/Rendering/Direct3D11Renderer.cpp").read_text(encoding="utf-8")
d3d_header_121 = (root / "src/Rendering/Direct3D11Renderer.h").read_text(encoding="utf-8")
equation_dialog_121 = (root / "src/App/EquationEditorDialog.cpp").read_text(encoding="utf-8")
build_plan_121 = (root / "docs/FRACTAL-STYLE-BUILD-PLAN.md").read_text(encoding="utf-8")

for marker in [
    "double edgeLightingStrength{0.0};",
    "Screen-space bloom strength",
]:
    if marker not in models_header_121:
        print(f"1.12.1 model marker missing: {marker}")
        sys.exit(1)
for marker in [
    '{"edgeLightingStrength", equation.edgeLightingStrength}',
    'value->Find("edgeLightingStrength")',
    "equation.edgeLightingStrength = equation.glowStrength",
]:
    if marker not in settings_store_121:
        print(f"1.12.1 persistence/migration marker missing: {marker}")
        sys.exit(1)
for marker in [
    "SupportsConjugateDistanceEstimation",
    "JacobianSpectralNorm",
    "derivativeX",
    "derivativeY",
    "largest singular value",
    "equation.power == 2",
    "!equation.animateCoefficients",
]:
    if marker not in math_header_121 and marker not in math_source_121:
        print(f"1.12.1 conjugate-distance marker missing: {marker}")
        sys.exit(1)
for marker in [
    "equation.edgeLightingStrength",
    "escape.distanceEstimate * 80.0",
    "ColouringMethod::OrbitTrap",
]:
    if marker not in still_renderer_121:
        print(f"1.12.1 CPU edge-light marker missing: {marker}")
        sys.exit(1)
for marker in [
    "uEdgeLightingStrength",
    "uConjugateDistanceSupported",
    "jacobianStretch",
    "jacobianStretchD",
    "SupportsConjugateDistanceEstimation(region.equation)",
]:
    if marker not in open_gl_121:
        print(f"1.12.1 OpenGL conjugate-distance marker missing: {marker}")
        sys.exit(1)
for marker in [
    "cDistanceControls",
    "distanceControls",
    "jacobianStretch",
    "SupportsConjugateDistanceEstimation(region.equation)",
    "region.equation.edgeLightingStrength",
]:
    if marker not in d3d_121 and marker not in d3d_header_121:
        print(f"1.12.1 Direct3D conjugate-distance marker missing: {marker}")
        sys.exit(1)
for marker in [
    "EdgeLightEdit",
    'L"Bloom 0–4"',
    'L"Edge light 0–4"',
    "equation.edgeLightingStrength",
]:
    if marker not in equation_dialog_121:
        print(f"1.12.1 Equation Editor marker missing: {marker}")
        sys.exit(1)
for marker in [
    "FiniteDifferenceTricornDistance",
    "The Tricorn Jacobian distance should match a finite-difference reference sample.",
    "Unsupported conjugate formulas should use the safe direct path without a fabricated distance estimate.",
    "Bloom and mathematical edge-light strengths should persist independently.",
    "Legacy glow should migrate once to the independent edge-light field.",
    "Existing analytic Mandelbrot distance-estimation output should remain unchanged.",
    "Tricorn mathematical edge lighting should change CPU output while bloom remains disabled.",
]:
    if marker not in tests_text:
        print(f"1.12.1 test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "**Status: Implemented in 1.12.1**",
    "largest singular value",
    "higher Multicorn powers",
]:
    if marker not in build_plan_121:
        print(f"1.12.1 build-plan status marker missing: {marker}")
        sys.exit(1)
for required_doc in [
    "docs/FEATURES-1.12.1.md",
    "docs/VERIFICATION-1.12.1.md",
]:
    if not (root / required_doc).is_file():
        print(f"1.12.1 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.12.1 Tricorn Jacobian distance and independent edge-light checks passed.")


# 1.12.2 exact Tricorn perturbation and guarded reference refresh.
deep_zoom_header_122 = (root / "src/Core/DeepZoom.h").read_text(encoding="utf-8")
deep_zoom_source_122 = (root / "src/Core/DeepZoom.cpp").read_text(encoding="utf-8")
open_gl_122 = (root / "src/Rendering/OpenGLRenderer.cpp").read_text(encoding="utf-8")
d3d_122 = (root / "src/Rendering/Direct3D11Renderer.cpp").read_text(encoding="utf-8")
build_plan_122 = (root / "docs/FRACTAL-STYLE-BUILD-PLAN.md").read_text(encoding="utf-8")
for marker in [
    "PerturbationProfile",
    "TricornQuadratic",
    "ResolvePerturbationProfile",
    "EvaluatePerturbationSample",
    "referenceRefreshed",
    "maximumRelativeDelta",
    "if (equation.conjugate) z = Conjugate(z)",
    "if (equation.conjugate) working = Conjugate(working)",
    "workingReference = Conjugate(workingReference)",
    "workingQ = Conjugate(workingQ)",
    "relativeDelta > 0.5",
    "BuildReferenceOrbitArbitrary(refreshedCamera",
]:
    if marker not in deep_zoom_header_122 and marker not in deep_zoom_source_122:
        print(f"1.12.2 core perturbation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "cddConj",
    "uConjugate!=0?cddConj(z):z",
    "referenceImaginary=uConjugate!=0?-zi:zi",
    "relativeDelta>0.5",
    "return directSplit(p)",
]:
    if marker not in open_gl_122:
        print(f"1.12.2 OpenGL perturbation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "cddConj",
    "cFlags0.z>0.5?cddConj(z):z",
    "referenceImaginary=cFlags0.z>0.5?-zi:zi",
    "relativeDelta>0.5",
    "return directSplit(p)",
]:
    if marker not in d3d_122:
        print(f"1.12.2 Direct3D perturbation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "The exact power-2 Tricorn profile should enable conjugate perturbation.",
    "The double Tricorn reference orbit should apply conjugation before squaring.",
    "The arbitrary-precision Tricorn reference orbit should apply conjugation before squaring.",
    "Deep Tricorn perturbation should match direct iteration results at progressively deeper scales.",
    "An excessive perturbation delta should refresh the reference orbit instead of continuing unstably.",
]:
    if marker not in tests_text:
        print(f"1.12.2 test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "**Status: Implemented in 1.12.2**",
    "conjugate perturbation recurrence",
    "bounded rebase region",
    "1e-12",
]:
    if marker not in build_plan_122:
        print(f"1.12.2 build-plan marker missing: {marker}")
        sys.exit(1)
for required_doc in [
    "docs/FEATURES-1.12.2.md",
    "docs/VERIFICATION-1.12.2.md",
]:
    if not (root / required_doc).is_file():
        print(f"1.12.2 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.12.2 exact Tricorn perturbation and guarded refresh checks passed.")

# 1.12.3 configurable separable bloom, radius-aware tile overlap, and camera rotation.
models_header_123 = (root / "src/Core/Models.h").read_text(encoding="utf-8")
models_source_123 = (root / "src/Core/Models.cpp").read_text(encoding="utf-8")
settings_store_123 = (root / "src/Core/SettingsStore.cpp").read_text(encoding="utf-8")
still_header_123 = (root / "src/Core/StillImageRenderer.h").read_text(encoding="utf-8")
still_source_123 = (root / "src/Core/StillImageRenderer.cpp").read_text(encoding="utf-8")
animation_header_123 = (root / "src/Core/Animation.h").read_text(encoding="utf-8")
animation_source_123 = (root / "src/Core/Animation.cpp").read_text(encoding="utf-8")
renderer_types_123 = (root / "src/Rendering/RendererTypes.h").read_text(encoding="utf-8")
open_gl_123 = (root / "src/Rendering/OpenGLRenderer.cpp").read_text(encoding="utf-8")
open_gl_header_123 = (root / "src/Rendering/OpenGLRenderer.h").read_text(encoding="utf-8")
d3d_123 = (root / "src/Rendering/Direct3D11Renderer.cpp").read_text(encoding="utf-8")
d3d_header_123 = (root / "src/Rendering/Direct3D11Renderer.h").read_text(encoding="utf-8")
equation_dialog_123 = (root / "src/App/EquationEditorDialog.cpp").read_text(encoding="utf-8")
settings_dialog_123 = (root / "src/App/SettingsDialog.cpp").read_text(encoding="utf-8")
high_res_123 = (root / "src/App/HighResRenderDialog.cpp").read_text(encoding="utf-8")
wallpaper_123 = (root / "src/WindowsIntegration/WallpaperController.cpp").read_text(encoding="utf-8")
adaptive_123 = (root / "src/Core/AdaptivePerformance.cpp").read_text(encoding="utf-8")
build_plan_123 = (root / "docs/FRACTAL-STYLE-BUILD-PLAN.md").read_text(encoding="utf-8")

for marker in [
    "double bloomThreshold{0.22};",
    "double bloomSoftKnee{0.0};",
    "int bloomRadius{1};",
    "double rotationDegrees{0.0};",
]:
    if marker not in models_header_123:
        print(f"1.12.3 model marker missing: {marker}")
        sys.exit(1)
for marker in [
    "preset.rotationDegrees = std::remainder",
    "equation.bloomThreshold",
    "equation.bloomSoftKnee",
    "equation.bloomRadius",
]:
    if marker not in models_source_123:
        print(f"1.12.3 validation marker missing: {marker}")
        sys.exit(1)
for marker in [
    '{"bloomThreshold", equation.bloomThreshold}',
    '{"bloomSoftKnee", equation.bloomSoftKnee}',
    '{"bloomRadius", equation.bloomRadius}',
    '{"rotationDegrees", preset.rotationDegrees}',
    'Find("rotationDegrees")',
]:
    if marker not in settings_store_123:
        print(f"1.12.3 persistence marker missing: {marker}")
        sys.exit(1)
for marker in [
    "ComplexPlanePoint",
    "MapStillRenderSample",
    "StillRenderTileOverlapPixels",
    "double rotationDegrees = 0.0",
]:
    if marker not in still_header_123 and marker not in still_source_123:
        print(f"1.12.3 still mapping marker missing: {marker}")
        sys.exit(1)
for marker in [
    "bloomRadius + reconstructionRadius",
    "qualityPreset.rotationDegrees",
    "const std::uint32_t overlapPixels = StillRenderTileOverlapPixels",
]:
    if marker not in still_source_123 and marker not in high_res_123:
        print(f"1.12.3 tiled export marker missing: {marker}")
        sys.exit(1)
for marker in [
    "double rotationDegrees = 0.0",
    "localX * cosine - localY * sine",
    "localX * sine + localY * cosine",
]:
    if marker not in animation_header_123 and marker not in animation_source_123:
        print(f"1.12.3 interaction rotation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "double rotationDegrees{0.0};",
    "region.rotationDegrees = renderPreset.rotationDegrees",
    "region.rotationDegrees = snapshot.rotationDegrees",
    "MixDouble(hash, preset.rotationDegrees)",
]:
    combined = "\n".join([renderer_types_123, app_window_text, wallpaper_123, adaptive_123])
    if marker not in combined:
        print(f"1.12.3 renderer propagation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "uRotationSinCos",
    "uThreshold",
    "uSoftKnee",
    "uRadius",
    "uDirection",
    "uComposite",
    "blurFramebuffer_",
    "blurTexture_",
    "brightContribution",
]:
    if marker not in open_gl_123 and marker not in open_gl_header_123:
        print(f"1.12.3 OpenGL post-processing marker missing: {marker}")
        sys.exit(1)
for marker in [
    "PostConstants",
    "cTexelDirection",
    "cBloom",
    "cPostPass",
    "OriginalTexture",
    "blurTexture_",
    "blurTargetView_",
    "blurShaderView_",
    "brightContribution",
]:
    if marker not in d3d_123 and marker not in d3d_header_123:
        print(f"1.12.3 Direct3D post-processing marker missing: {marker}")
        sys.exit(1)
for marker in [
    "BloomThresholdEdit",
    "BloomSoftKneeEdit",
    "BloomRadiusEdit",
    'L"Bloom radius 0–16"',
    "RotationDegreesEdit",
    'L"View rotation °"',
]:
    if marker not in equation_dialog_123 and marker not in settings_dialog_123:
        print(f"1.12.3 UI marker missing: {marker}")
        sys.exit(1)
for marker in [
    "Bloom threshold, soft knee and blur radius should persist.",
    "Camera rotation should persist with a preset.",
    "A positive 90-degree view rotation should rotate the right edge toward positive imaginary coordinates.",
    "A rotated GPU tile camera should share the exact global mapping of its tile centre.",
    "GPU tile overlap should cover the configured bloom radius.",
    "GPU tile overlap should include the bloom and reconstruction radii.",
    "Rotated preview panning should move along the rotated complex-plane axis.",
    "Rotated wheel zoom should preserve its anchor along the rotated complex-plane axis.",
]:
    if marker not in tests_text:
        print(f"1.12.3 test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "**Status: Implemented in 1.12.3**",
    "horizontal and vertical separable passes",
    "global-pixel rotation convention",
]:
    if marker not in build_plan_123:
        print(f"1.12.3 build-plan marker missing: {marker}")
        sys.exit(1)
for required_doc in [
    "docs/FEATURES-1.12.3.md",
    "docs/VERIFICATION-1.12.3.md",
]:
    if not (root / required_doc).is_file():
        print(f"1.12.3 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.12.3 configurable bloom, seam-aware tiles, and camera rotation checks passed.")

# 1.12.4 bounded deterministic Fractal Scout.
scout_header_124 = (root / "src/Core/FractalScout.h").read_text(encoding="utf-8")
scout_source_124 = (root / "src/Core/FractalScout.cpp").read_text(encoding="utf-8")
scout_dialog_header_124 = (root / "src/App/FractalScoutDialog.h").read_text(encoding="utf-8")
scout_dialog_124 = (root / "src/App/FractalScoutDialog.cpp").read_text(encoding="utf-8")
build_plan_124 = (root / "docs/FRACTAL-STYLE-BUILD-PLAN.md").read_text(encoding="utf-8")

for marker in [
    "FractalScoutRequest",
    "FractalScoutLimits",
    "FractalScoutMetrics",
    "FractalScoutCandidate",
    "FractalScoutProgress",
    "ResolveFractalScoutLimits",
    "RunFractalScout",
    "candidatePoolSize{36}",
    "resultCount{9}",
]:
    if marker not in scout_header_124:
        print(f"1.12.4 Fractal Scout contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "kGoldenAngle",
    "boundaryMix",
    "iterationVariance",
    "edgeDensity",
    "symmetry",
    "maximumRetainedBytes",
    "std::stable_sort",
    "result.cancelled = true",
    "RenderStillImageTiled",
    "candidate.preset.id.clear()",
]:
    if marker not in scout_source_124:
        print(f"1.12.4 Fractal Scout implementation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "FractalScoutAction",
    "UseInPreview",
    "UseAndSaveAsNew",
]:
    if marker not in scout_dialog_header_124:
        print(f"1.12.4 Fractal Scout dialog contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    'L"Fast — 6 results"',
    'L"Balanced — 9 results"',
    'L"Detailed — 12 results"',
    'L"Refine Around Selected"',
    'L"Use in Preview"',
    'L"Use && Save as New..."',
    "std::atomic_bool cancelRequested",
    "std::thread worker",
    "RunFractalScout(",
    "StretchDIBits",
    "state->searchCamera = state->result.candidates",
]:
    if marker not in scout_dialog_124:
        print(f"1.12.4 Fractal Scout UI marker missing: {marker}")
        sys.exit(1)
for marker in [
    '#include "App/FractalScoutDialog.h"',
    'L"Fractal Scout..."',
    "FractalScoutButton",
    "OpenFractalScout()",
    "ApplyMainWindowCameraMutation(candidate.camera",
    "UseAndSaveAsNew) SaveAsNewPreset()",
]:
    if marker not in app_window_text and marker not in app_window_header_text:
        print(f"1.12.4 AppWindow Scout integration marker missing: {marker}")
        sys.exit(1)
for marker in [
    "Fractal Scout should complete a bounded search",
    "Fractal Scout results should be sorted by descending score",
    "Fractal Scout scoring, identity and thumbnails should be deterministic for identical inputs",
    "Fractal Scout candidates should carry a stable SHA-256 identity",
    "A cancelled Fractal Scout search should not expose partial candidates",
    "Fractal Scout should cap candidate work and retained results",
]:
    if marker not in tests_text:
        print(f"1.12.4 Fractal Scout test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "**Status: Implemented in 1.12.4**",
    "golden-angle candidate search",
    "discards partial candidates after cancellation",
    "Use & Save as New",
]:
    if marker not in build_plan_124:
        print(f"1.12.4 build-plan marker missing: {marker}")
        sys.exit(1)
for required_doc in [
    "docs/FEATURES-1.12.4.md",
    "docs/VERIFICATION-1.12.4.md",
]:
    if not (root / required_doc).is_file():
        print(f"1.12.4 documentation missing: {required_doc}")
        sys.exit(1)
for source_marker in [
    "src/Core/FractalScout.cpp",
    "src/App/FractalScoutDialog.cpp",
]:
    if source_marker not in cmake_text:
        print(f"1.12.4 build integration missing: {source_marker}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.12.4 bounded deterministic Fractal Scout checks passed.")


# 1.12.5 targeted multi-scale Fractal Scout.
scout_header_125 = (root / "src/Core/FractalScout.h").read_text(encoding="utf-8")
scout_source_125 = (root / "src/Core/FractalScout.cpp").read_text(encoding="utf-8")
scout_dialog_125 = (root / "src/App/FractalScoutDialog.cpp").read_text(encoding="utf-8")
build_plan_125 = (root / "docs/FRACTAL-STYLE-BUILD-PLAN.md").read_text(encoding="utf-8")
for marker in [
    "FractalScoutGoal",
    "Balanced",
    "Boundary",
    "Filaments",
    "Symmetry",
    "scaleBandCount{3}",
    "scaleBandSpread{0.38}",
    "minimumResultSeparation{0.20}",
    "suppressedNearDuplicates",
    "CalculateFractalScoutScore",
]:
    if marker not in scout_header_125:
        print(f"1.12.5 Scout contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "std::exp(bandPosition * limits.scaleBandSpread)",
    "AddCompensated(camera.centreX, camera.centreXLow",
    "CandidateSeparation",
    "SelectDiverseResults",
    "threshold *= 0.65",
    "FractalScoutGoal::Boundary",
    "FractalScoutGoal::Filaments",
    "FractalScoutGoal::Symmetry",
    "result.suppressedNearDuplicates",
]:
    if marker not in scout_source_125:
        print(f"1.12.5 Scout implementation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "GoalCombo",
    'L"Search target"',
    'L"Balanced detail"',
    'L"Boundary structures"',
    'L"Fine filaments"',
    'L"Symmetry and rings"',
    "suppressedNearDuplicates",
]:
    if marker not in scout_dialog_125:
        print(f"1.12.5 Scout dialog marker missing: {marker}")
        sys.exit(1)
for marker in [
    "Boundary-targeted Scout scoring should prefer mixed boundary structures.",
    "Filament-targeted Scout scoring should prefer edge-rich detailed structures.",
    "Symmetry-targeted Scout scoring should prefer symmetric structures.",
    "Fractal Scout should suppress near-duplicate ranked candidates before thumbnail rendering.",
    "Fractal Scout should retain candidates from multiple deterministic depth bands.",
    "Repeated Fractal Scout searches should preserve the diversity suppression count.",
    "Fractal Scout should cap depth bands, spread and result separation.",
]:
    if marker not in tests_text:
        print(f"1.12.5 Scout test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "**Status: Implemented in 1.12.5**",
    "logarithmically spaced depth bands",
    "position-and-scale separation",
]:
    if marker not in build_plan_125:
        print(f"1.12.5 build-plan marker missing: {marker}")
        sys.exit(1)
for required_doc in [
    "docs/FEATURES-1.12.5.md",
    "docs/VERIFICATION-1.12.5.md",
]:
    if not (root / required_doc).is_file():
        print(f"1.12.5 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.12.5 targeted multi-scale Fractal Scout checks passed.")


# 1.12.6 Windows Fractal Scout build regression fix.
scout_dialog_126 = (root / "src/App/FractalScoutDialog.cpp").read_text(encoding="utf-8")
for marker in [
    '#include "Core/DeepZoom.h"',
    'CameraCentreX(candidate.preset.camera)',
    'CameraCentreY(candidate.preset.camera)',
]:
    if marker not in scout_dialog_126:
        print(f"1.12.6 Fractal Scout MSVC dependency marker missing: {marker}")
        sys.exit(1)
include_position = scout_dialog_126.find('#include "Core/DeepZoom.h"')
use_position = scout_dialog_126.find('CameraCentreX(candidate.preset.camera)')
if include_position < 0 or use_position < 0 or include_position > use_position:
    print("1.12.6 Fractal Scout helper declaration is not included before use.")
    sys.exit(1)
for required_doc in [
    "docs/FEATURES-1.12.6.md",
    "docs/VERIFICATION-1.12.6.md",
    "docs/DEFERRED-UX-REQUESTS.md",
]:
    if not (root / required_doc).is_file():
        print(f"1.12.6 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.12.6 Windows Fractal Scout build regression checks passed.")


# 1.13.0 desktop mode, image output and live editor UX.
models_header_113 = (root / "src/Core/Models.h").read_text(encoding="utf-8")
models_cpp_113 = (root / "src/Core/Models.cpp").read_text(encoding="utf-8")
settings_store_113 = (root / "src/Core/SettingsStore.cpp").read_text(encoding="utf-8")
settings_dialog_113 = (root / "src/App/SettingsDialog.cpp").read_text(encoding="utf-8")
equation_dialog_113 = (root / "src/App/EquationEditorDialog.cpp").read_text(encoding="utf-8")
palette_dialog_113 = (root / "src/App/PaletteEditorDialog.cpp").read_text(encoding="utf-8")
highres_dialog_113 = (root / "src/App/HighResRenderDialog.cpp").read_text(encoding="utf-8")
slideshow_dialog_113 = (root / "src/App/SlideshowDialog.cpp").read_text(encoding="utf-8")
image_codec_113 = (root / "src/WindowsIntegration/ImageCodec.cpp").read_text(encoding="utf-8")
quick_113 = (root / "src/App/QuickControllerWindow.cpp").read_text(encoding="utf-8")
quick_header_113 = (root / "src/App/QuickControllerWindow.h").read_text(encoding="utf-8")
for marker in [
    "enum class SavedImageFormat { Png, Jpeg, Tiff, Bmp }",
    "enum class DesktopMode { None, StaticImage, Slideshow, Video }",
    "DesktopMode defaultDesktopMode{DesktopMode::None}",
    "SavedImageFormat savedImageFormat{SavedImageFormat::Png}",
    "int compressionQuality{90}",
    "int schemaVersion{12}",
]:
    if marker not in models_header_113:
        print(f"1.13.0 model marker missing: {marker}")
        sys.exit(1)
for marker in [
    'return "static-image"', 'return "slideshow"', 'return "video"',
    'if (value == "jpeg" || value == "jpg")',
]:
    if marker not in models_cpp_113:
        print(f"1.13.0 model conversion marker missing: {marker}")
        sys.exit(1)
for marker in [
    '"defaultDesktopMode"', '"savedImageFormat"', '"compressionQuality"',
    'DesktopModeFromString', 'SavedImageFormatFromString', 'legacyStart',
]:
    if marker not in settings_store_113:
        print(f"1.13.0 settings persistence marker missing: {marker}")
        sys.exit(1)
for marker in [
    'L"Default image folder"', 'BrowseOutputButton', 'OutputFormatCombo',
    'CompressionQualityEdit', 'L"Journey settings..."', 'JourneySettingsDialog::Show',
]:
    if marker not in settings_dialog_113:
        print(f"1.13.0 settings UI marker missing: {marker}")
        sys.exit(1)
for marker in [
    'DesktopModeCombo', 'ApplyDesktopModeButton', 'DefaultDesktopModeCheck',
    'L"None", L"Static image", L"Slide show", L"Video file"',
    'ApplySelectedDesktopMode()', 'ApplyDesktopMode(DesktopMode mode)',
    'settings_.general.defaultDesktopMode != DesktopMode::None',
    'JourneySettingsDialog::Show',
]:
    if marker not in app_window_text and marker not in app_window_header_text:
        print(f"1.13.0 desktop-mode marker missing: {marker}")
        sys.exit(1)
for removed in ['AnimationCombo', 'animationCombo_']:
    if removed in app_window_text or removed in app_window_header_text:
        print(f"1.13.0 removed preview animation selector remains: {removed}")
        sys.exit(1)
for marker in [
    'QuickControllerCommands::JourneySettings', 'L"Journey Settings..."',
]:
    if marker not in quick_113 and marker not in quick_header_113 and marker not in app_window_text:
        print(f"1.13.0 quick journey marker missing: {marker}")
        sys.exit(1)
for dialog_name, dialog_text in [
    ("Settings", settings_dialog_113),
    ("Equation", equation_dialog_113),
    ("Palette", palette_dialog_113),
]:
    owner_associated = 'owner, nullptr, instance' in dialog_text or 'owner,nullptr,instance' in dialog_text
    if 'WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT' not in dialog_text or not owner_associated:
        print(f"1.13.0 {dialog_name} window is not an owner-associated modeless tool window.")
        sys.exit(1)
    if 'EnableWindow(owner, FALSE)' in dialog_text or 'WS_EX_TOPMOST' in dialog_text:
        print(f"1.13.0 {dialog_name} window still blocks or stays above the preview.")
        sys.exit(1)
if (equation_dialog_113.count('CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP') +
        equation_dialog_113.count('CBS_DROPDOWNLIST|WS_VSCROLL|WS_TABSTOP')) < 6:
    print("1.13.0 equation dropdown scrolling markers are incomplete.")
    sys.exit(1)
for marker in ['ApplyLive(EditorState& state', 'state.onChanged', 'notification == EN_CHANGE', 'SavedPaletteCombo && notification == CBN_SELCHANGE']:
    if marker not in palette_dialog_113:
        print(f"1.13.0 live palette marker missing: {marker}")
        sys.exit(1)
for marker in [
    'SavePixelsWithWic', 'LoadPixelsWithWic', 'GUID_ContainerFormatJpeg',
    'GUID_ContainerFormatTiff', 'GUID_ContainerFormatPng', 'compressionQuality',
]:
    if marker not in image_codec_113:
        print(f"1.13.0 image codec marker missing: {marker}")
        sys.exit(1)
for marker in ['SavedImageFormat::Jpeg', 'outputSettings.storageDirectory', 'outputSettings.compressionQuality']:
    if marker not in highres_dialog_113:
        print(f"1.13.0 high-resolution output marker missing: {marker}")
        sys.exit(1)
for marker in [
    'src/App/JourneySettingsDialog.cpp',
    'src/WindowsIntegration/ImageCodec.cpp',
    'windowscodecs',
]:
    if marker not in cmake_text:
        print(f"1.13.0 build integration marker missing: {marker}")
        sys.exit(1)
if 'settings_.staticWallpaper' not in app_window_text or 'HighResRenderDialog::Show' not in app_window_text:
    print("1.13.0 high-resolution output defaults are not passed from AppWindow.")
    sys.exit(1)
for marker in ['L".png"', 'L".jpeg"', 'L".tiff"', 'Add Images from Folder']:
    if marker not in slideshow_dialog_113:
        print(f"1.13.0 slideshow image-format marker missing: {marker}")
        sys.exit(1)
for required_doc in ["docs/FEATURES-1.13.0.md", "docs/VERIFICATION-1.13.0.md"]:
    if not (root / required_doc).is_file():
        print(f"1.13.0 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.0.")
    sys.exit(1)
print("1.13.0 desktop mode, image output and live editor UX checks passed.")


# 1.13.1 native Windows Journey Settings access regression.
animation_header_1131 = (root / "src/Core/Animation.h").read_text(encoding="utf-8")
animation_cpp_1131 = (root / "src/Core/Animation.cpp").read_text(encoding="utf-8")
journey_dialog_1131 = (root / "src/App/JourneySettingsDialog.cpp").read_text(encoding="utf-8")
for marker in [
    "static bool HasValidJourneyScriptTargets",
    "bool AnimationController::HasValidJourneyScriptTargets",
    "return !ParseJourneyScript(script).empty();",
]:
    if marker not in animation_header_1131 and marker not in animation_cpp_1131:
        print(f"1.13.1 public journey validation marker missing: {marker}")
        sys.exit(1)
if "AnimationController::ParseJourneyScript" in journey_dialog_1131:
    print("1.13.1 Journey Settings still calls the private journey parser.")
    sys.exit(1)
if "AnimationController::HasValidJourneyScriptTargets(script)" not in journey_dialog_1131:
    print("1.13.1 Journey Settings is not connected to the public validation contract.")
    sys.exit(1)
for marker in [
    "A valid structured journey row should be accepted through the public validation contract.",
    "Malformed journey rows should be rejected through the public validation contract.",
]:
    if marker not in tests_text:
        print(f"1.13.1 journey validation test marker missing: {marker}")
        sys.exit(1)
for required_doc in ["docs/FEATURES-1.13.1.md", "docs/VERIFICATION-1.13.1.md"]:
    if not (root / required_doc).is_file():
        print(f"1.13.1 documentation missing: {required_doc}")
        sys.exit(1)
if 'project(MandelbrotLiveWallpaper VERSION 1.13.1' not in cmake_text:
    print("CMake project version was not updated to 1.13.1.")
    sys.exit(1)
print("1.13.1 native Windows Journey Settings access regression checks passed.")


# PH-01 repeatable native validation and isolated runtime-state checks.
paths_text = (root / "src/Infrastructure/Paths.cpp").read_text(encoding="utf-8")
paths_tests_text = (root / "tests/PathsTests.cpp").read_text(encoding="utf-8")
windows_validation_text = (root / "scripts/validate-windows-release.ps1").read_text(encoding="utf-8")
build_release_text = (root / "scripts/build-release.ps1").read_text(encoding="utf-8")
windows_interaction_fixture_text = (
    root / "src/Tools/WindowsInteractionFixtureTool.cpp"
).read_text(encoding="utf-8")
for marker in ["MW_APPDATA_DIR", "AppDataOverride()", "if (const auto overridePath = AppDataOverride())"]:
    if marker not in paths_text:
        print(f"PH-01 isolated application-data marker missing: {marker}")
        sys.exit(1)
for marker in ["MandelbrotPathTests", "tests/PathsTests.cpp", "src/Infrastructure/Paths.cpp"]:
    if marker not in cmake_text:
        print(f"PH-01 path-isolation test integration marker missing: {marker}")
        sys.exit(1)
for marker in ["SettingsPath should remain below", "LogDirectory should remain below", "StaticRenderDirectory should remain below"]:
    if marker not in paths_tests_text:
        print(f"PH-01 path-isolation assertion marker missing: {marker}")
        sys.exit(1)
try:
    presets = json.loads((root / "CMakePresets.json").read_text(encoding="utf-8"))
except (OSError, json.JSONDecodeError) as error:
    print(f"CMakePresets.json is invalid: {error}")
    sys.exit(1)
configure_names = {preset.get("name") for preset in presets.get("configurePresets", [])}
build_names = {preset.get("name") for preset in presets.get("buildPresets", [])}
test_names = {preset.get("name") for preset in presets.get("testPresets", [])}
if "windows-msvc-release" not in configure_names or "windows-msvc-release" not in build_names or "windows-msvc-release" not in test_names:
    print("The native Windows configure/build/test preset set is incomplete.")
    sys.exit(1)
for marker in [
    "cmake --preset windows-msvc-release",
    "cmake --build --preset windows-msvc-release",
    "ctest --preset windows-msvc-release",
]:
    if marker not in build_release_text:
        print(f"The release script is not using the canonical native preset: {marker}")
        sys.exit(1)
for marker in [
    "Source and offline-policy verification",
    "Native build, CTest and packaging",
    "Portable package inspection",
    "MW_APPDATA_DIR",
    "MandelbrotLiveWallpaperControl",
    "EnumWindows",
    "GetWindowThreadProcessId",
    "FindWindowForProcess",
    "[uint32]$process.Id",
    "Get-FileHash",
    "Get-FileHash -Algorithm SHA256 -LiteralPath $installerPath",
    "Get-AuthenticodeSignature -LiteralPath $installerPath",
    "Installer file version mismatch",
    "report.json",
    "report.md",
]:
    if marker not in windows_validation_text:
        print(f"PH-01 Windows validation workflow marker missing: {marker}")
        sys.exit(1)
if "[MandelbrotNativeWindow]::FindWindow('MandelbrotLiveWallpaperControl'" in windows_validation_text:
    print("PH-01 Windows validation workflow regressed to unreliable global FindWindow lookup.")
    sys.exit(1)
for relative_path, markers in {
    "project_docs/PROJECT_INDEX.md": ["BR-20260910-06", "Native WIC frame success, matching resume, refusal, visible cancellation and close-while-active cancellation passed"],
    "project_docs/DELIVERY_REPORT.md": ["BR-20260909-02", "PH-01 native installer artifact validation"],
    "project_docs/VALIDATION_AND_EVIDENCE.md": ["BR-20260909-02", "20260909-ph11-installer-final/report.json"],
    "docs/testing/WINDOWS-RELEASE-VALIDATION.md": ["per-user or `PATH`-provided Inno Setup", "20260909-ph11-installer-final/"],
}.items():
    release_document_text = (root / relative_path).read_text(encoding="utf-8")
    for marker in markers:
        if marker not in release_document_text:
            print(f"PH-01 installer artifact evidence missing in {relative_path}: {marker}")
            sys.exit(1)
for marker in [
    "MandelbrotWindowsInteractionFixture",
    "WindowsInteractionFixture",
    "RUN_SERIAL TRUE",
    "test_artifacts/windows-interaction",
]:
    if marker not in cmake_text:
        print(f"Native Windows interaction fixture CMake marker missing: {marker}")
        sys.exit(1)
for marker in [
    "MandelbrotPaletteEditorDialog",
    "MandelbrotAdvancedEquationEditor",
    "MW_APPDATA_DIR",
    "Automatic preview precision did not change",
    "accepting Palette Editor restored the camera",
    "accepting Equation Editor restored the camera",
    "Native Equation Undo/Redo",
    "kMainPresetCombo",
    'find(L"Load Preset")',
    'find(L"Seahorse Valley")',
    "Native preset Load/Undo/Redo",
    "MandelbrotLiveWallpaperSettingsDialog",
    "kSettingsRotation",
    "Edit Project Settings",
    "Native Settings accept/Undo/Redo",
    "MandelbrotJourneySettingsDialog",
    "kJourneyWaypoints",
    'find(L"Edit Journey")',
    "Native Journey accept/Undo/Redo",
    'Core/SettingsStore.h',
    'fixture-import.json',
    'find(L"Import Preset")',
    'find(L"Fixture Imported")',
    "Native preset Import/Undo/Redo",
    "MandelbrotFractalScoutDialog",
    "kScoutResultList",
    'find(L"Apply Scout Camera")',
    "Native Fractal Scout Apply/Undo/Redo",
    "Native Fractal Scout candidate selection and Close preserved",
    "MandelbrotGeneralAnimationEditorDialog",
    "kTimelineScrubber",
    "Native Animation Timeline scrub/play/stop and Cancel",
    "Native Animation Timeline OK retained one runtime track",
    "kTimelineJourneyToTracks",
    "kTimelineTracksToJourney",
    "Native Journey to Tracks and Tracks to Journey preparation passed",
    "MandelbrotFrameSequenceExportDialog",
    "kMainOpenFrameExport",
    "Native Frame Sequence Export completed one production WIC 32x24 PNG",
    "Native Frame Sequence Export resumed its matching manifest with 0 rendered and 1 verified frame reused",
    "Native Frame Sequence Export refused a mismatched manifest and preserved the verified PNG",
    "Native Frame Sequence Export refused an untracked final PNG and preserved its exact contents",
    "Closing an active Frame Sequence Export cancelled and joined its worker",
    "Native Video Export refreshed its typed-sequence summary",
    "MW_TEST_FFMPEG_PATH",
    "MW_TEST_VIDEO_CANCEL_SEQUENCE",
    "Native Video Export completed and verified one MP4",
    "Native Video Export cancellation terminated the owned FFmpeg encode",
    "Native Frame Sequence Export cancellation joined its worker",
    "did not finish constructing its controls",
    "TerminateProcess(process.process",
]:
    if marker not in windows_interaction_fixture_text:
        print(f"Native Windows interaction fixture marker missing: {marker}")
        sys.exit(1)
for relative_path, markers in {
    "project_docs/DELIVERY_REPORT.md": ["BR-20260910-05", "Native Journey and Timeline conversion interaction"],
    "project_docs/VALIDATION_AND_EVIDENCE.md": ["BR-20260910-05", "exactly three camera tracks"],
    "project_docs/UI_WORKFLOWS_AND_ROUTES.md": ["BR-20260910-05", "successful preparation confirmation"],
    "docs/features/ANIMATION-TRACKS-PLAN.md": ["BR-20260910-05", "Journey-to-Tracks produces the three camera tracks"],
}.items():
    interaction_document_text = (root / relative_path).read_text(encoding="utf-8")
    for marker in markers:
        if marker not in interaction_document_text:
            print(f"Native Windows interaction evidence missing in {relative_path}: {marker}")
            sys.exit(1)
for relative_path, markers in {
    "project_docs/DELIVERY_REPORT.md": ["BR-20260910-06", "Native frame-sequence export and cancellation interaction"],
    "project_docs/VALIDATION_AND_EVIDENCE.md": ["BR-20260910-06", "0 rendered, 1 resumed"],
    "project_docs/UI_WORKFLOWS_AND_ROUTES.md": ["BR-20260910-06", "visible Cancel button"],
    "project_docs/IMPLEMENTATION_PLAN.md": ["BR-20260910-06", "matching-manifest resume"],
    "project_docs/TRACEABILITY.md": ["BR-20260910-06", "no `.part` file left"],
    "docs/features/OFFLINE-EXPORT-PLAN.md": ["BR-20260910-06", "separate 1024x1024 runs are cancelled"],
}.items():
    frame_interaction_document_text = (root / relative_path).read_text(encoding="utf-8")
    for marker in markers:
        if marker not in frame_interaction_document_text:
            print(f"Native frame-export interaction evidence missing in {relative_path}: {marker}")
            sys.exit(1)
for relative_path, markers in {
    "project_docs/PROJECT_INDEX.md": ["BR-20260918-01", "Native Video Export typed-summary"],
    "project_docs/DELIVERY_REPORT.md": ["BR-20260918-01", "missing-executable containment"],
    "project_docs/VALIDATION_AND_EVIDENCE.md": ["BR-20260918-01", "21.41 and 21.38 seconds"],
    "project_docs/UI_WORKFLOWS_AND_ROUTES.md": ["BR-20260918-01", "Typing the sequence directory"],
    "project_docs/IMPLEMENTATION_PLAN.md": ["BR-20260918-01", "typed-sequence summary refresh"],
    "project_docs/TRACEABILITY.md": ["BR-20260918-01", "nonexistent-executable refusal"],
    "docs/features/OFFLINE-EXPORT-PLAN.md": ["BR-20260918-01", "Native dialog validation"],
}.items():
    video_interaction_document_text = (root / relative_path).read_text(encoding="utf-8")
    for marker in markers:
        if marker not in video_interaction_document_text:
            print(f"Native video-export interaction evidence missing in {relative_path}: {marker}")
            sys.exit(1)
settings_accept_start = app_window_text.find("void AppWindow::OpenSettings()")
settings_accept_end = app_window_text.find("void AppWindow::OpenDiagnostics()", settings_accept_start)
settings_accept_text = app_window_text[settings_accept_start:settings_accept_end]
for marker in ["SyncNumericEditsFromTracks();", "SyncMainWindowCameraControls();", "PopulateMonitorControls();"]:
    if marker not in settings_accept_text:
        print(f"Accepted Settings main-window synchronization marker missing: {marker}")
        sys.exit(1)
print("PH-01 Windows preset, isolated app-data and release validation workflow checks passed.")

# PH-02 portable production-renderer visual fixture foundation.
visual_header = (root / "src/Core/VisualRegression.h").read_text(encoding="utf-8")
visual_source = (root / "src/Core/VisualRegression.cpp").read_text(encoding="utf-8")
visual_tool = (root / "src/Tools/VisualFixtureTool.cpp").read_text(encoding="utf-8")
visual_doc = (root / "docs/testing/VISUAL-REGRESSION.md").read_text(encoding="utf-8")
for marker in [
    "BuiltInVisualFixtures",
    "RenderVisualFixture",
    "CompareVisualImages",
    "CreateVisualDifferenceImage",
    "structuralSimilarity",
]:
    if marker not in visual_header and marker not in visual_source:
        print(f"PH-02 visual regression core marker missing: {marker}")
        sys.exit(1)
for fixture_id in [
    "standard-mandelbrot",
    "tricorn-cyan-fire-ring",
    "rotated-bloom-state",
    "deep-mandelbrot-perturbation-state",
]:
    if fixture_id not in visual_source:
        print(f"PH-02 canonical fixture missing: {fixture_id}")
        sys.exit(1)
for marker in [
    "RenderStillImageTiled",
    "tiled-versus-full-seams",
    "deliberate-visual-mutation",
    "baseline.ppm",
    "seam-strips",
    "summary.json",
]:
    if marker not in visual_tool:
        print(f"PH-02 visual fixture command marker missing: {marker}")
        sys.exit(1)
for marker in [
    "src/Core/VisualRegression.cpp",
    "MandelbrotVisualFixtures",
    "VisualFixtureSelfCheck",
    "MW_BUILD_VISUAL_FIXTURES",
]:
    if marker not in cmake_text:
        print(f"PH-02 CMake integration marker missing: {marker}")
        sys.exit(1)
for required_file in [
    "scripts/run-visual-fixtures.sh",
    "scripts/run-visual-fixtures.ps1",
    "scripts/run-visual-fixtures.cmd",
]:
    if not (root / required_file).is_file():
        print(f"PH-02 runner missing: {required_file}")
        sys.exit(1)
for marker in [
    "No command copies candidate output",
    "D3D11 WARP",
    "OpenGL",
    "render-state.canonical",
    "canonical SHA-256 render fingerprint",
]:
    if marker not in visual_doc:
        print(f"PH-02 visual regression evidence boundary missing: {marker}")
        sys.exit(1)
print("PH-02 production CPU visual fixture structure and evidence-boundary checks passed.")


# PH-03 bounded parameter adapters and canonical render fingerprint foundation.
project_state_header = (root / "src/Core/ProjectState.h").read_text(encoding="utf-8")
project_state_source = (root / "src/Core/ProjectState.cpp").read_text(encoding="utf-8")
core_tests = (root / "tests/CoreTests.cpp").read_text(encoding="utf-8")
state_doc = (root / "docs/architecture/PROJECT-STATE-AND-PARAMETERS.md").read_text(encoding="utf-8")
for marker in [
    "ProjectParameterDescriptors",
    "ParameterKeyFromStableName",
    "CaptureCameraParameters",
    "CapturePaletteParameters",
    "ApplyCameraParameters",
    "ApplyPaletteParameters",
    "ApplyProjectParameterMutations",
    "ApplyProjectPaletteSelection",
    "ApplyProjectPresetReplacement",
    "ParameterGestureCoalescer",
    "ProjectPresetReplacementKind",
    "ProjectPresetReplacementResult",
    "ParameterMutationOrigin",
    "ParameterMutationResult",
    "historyEligible",
    "HasParameterInvalidation",
    "BuildRenderFingerprint",
    "mw-render-state-v1",
    "SHA-256",
]:
    if marker not in project_state_header and marker not in project_state_source:
        print(f"PH-03 project state/fingerprint marker missing: {marker}")
        sys.exit(1)
for marker in [
    "camera.centre-x-low",
    "palette.selection",
    "palette.frequency",
    "post.brightness",
    "post.contrast",
    "post.saturation",
    "post.bloom-radius",
    "std::bit_cast<std::uint64_t>",
    "Canonicalise negative zero",
]:
    if marker not in project_state_source:
        print(f"PH-03 canonical parameter/number marker missing: {marker}")
        sys.exit(1)
for marker in [
    "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
    "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
    "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1",
    "compensated high/low components exactly",
    "Non-render metadata should not alter",
    "Unknown project parameter names should fail explicitly",
    "Unsupported enum values should be rejected",
    "authoritative 4096-stop palette contract",
    "A rejected batch must leave the authoritative preset unchanged",
    "aggregate only the affected invalidation classes",
    "Duplicate keys in one parameter transaction must be rejected",
    "preserve compensated low components and use model bounds",
    "camera-only transaction should emit only camera viewport invalidation",
    "rejected compensated camera batch must roll back every requested field",
    "Palette selection should have stable discrete identity",
    "Changing the built-in palette should clear custom stops",
    "simple discrete palette change should remain eligible",
    "remain eligible for PH-05 atomic history",
    "Re-selecting the current palette should be a no-op",
    "Unsupported palette values should be rejected without partial mutation",
    "Runtime-origin mutations should remain explicitly excluded",
    "Mutation-origin eligibility should separate user edits",
    "preview drag should begin one explicit non-zero coalescing sequence",
    "Wheel zoom events should coalesce only while their monotonic gap stays within policy",
    "bounded preview gesture should propagate its kind and token",
    "Preset loads should be classified, normalised and committed as undoable broad replacements",
    "Imported presets should retain an explicit import replacement origin and enter PH-05 broad history",
    "Accepted project dialogs should use an explicit user-control broad replacement boundary",
    "Mismatched replacement kind and origin should be rejected transactionally",
]:
    if marker not in core_tests:
        print(f"PH-03 adapter/fingerprint test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "src/Core/ProjectState.cpp",
    "renderFingerprintCanonicalVersion",
    "render-state.canonical",
]:
    if marker not in cmake_text and marker not in visual_tool:
        print(f"PH-03 integration marker missing: {marker}")
        sys.exit(1)
for field in ["brightness", "contrast", "saturation", "colourOffset"]:
    if re.search(rf"workingPreset_\.{field}\s*=", app_window_text):
        print(f"PH-03 migrated main-window scalar still has a direct write: {field}")
        sys.exit(1)
for marker in [
    "ApplyMainWindowPaletteSelection",
    "ApplyMainWindowPalettePostControls",
    "ApplyProjectPaletteSelection",
    "ApplyProjectParameterMutations",
    "ParameterMutationOrigin::UserControl",
    "ParameterMutationOrigin::ScoutApply",
    "HasParameterInvalidation",
    "ApplyMainWindowCameraMutation",
    "CameraMutationOrigin::CoordinateTriplet",
    "CameraMutationOrigin::Jump",
    "CameraMutationOrigin::ScoutApply",
    "CameraMutationOrigin::ResetView",
    "CameraMutationOrigin::PreviewPan",
    "CameraMutationOrigin::PreviewWheelZoom",
    "ParameterMutationOrigin::UserGesture",
    "ParameterGestureKind::PreviewNavigation",
    "ApplyWorkingPresetReplacement",
    "ProjectPresetReplacementKind::PresetLoad",
    "ProjectPresetReplacementKind::ImportedPreset",
    "ProjectPresetReplacementKind::PaletteDialog",
    "ProjectPresetReplacementKind::EquationDialog",
    "ProjectPresetReplacementKind::SettingsDialog",
    "ProjectPresetReplacementKind::JourneyDialog",
]:
    if marker not in app_window_text and marker not in app_window_header_text:
        print(f"PH-03 main-window mutation coordinator integration missing: {marker}")
        sys.exit(1)
if re.search(r"workingPreset_\.palette\s*=", app_window_text):
    print("PH-03 migrated built-in palette selection still has a direct write.")
    sys.exit(1)
if "id == PaletteCombo && notification == CBN_SELCHANGE)\n            (void)ApplyMainWindowPaletteSelection();" not in app_window_text:
    print("PH-03 palette combo should use the focused palette-selection route.")
    sys.exit(1)
for pattern in [
    r"workingPreset_\.camera\.(?:centreX|centreXLow|centreY|centreYLow|scale)\s*=",
    r"workingPreset_\.camera\s*=\s*candidate\.camera",
    r"workingPreset_\.camera\s*=\s*selected->camera",
    r"workingPreset_\.camera\s*=\s*\{",
]:
    if re.search(pattern, app_window_text):
        print(f"PH-03 migrated deliberate camera route still has a direct write: {pattern}")
        sys.exit(1)
if re.search(r"workingPreset_\.camera\s*=", app_window_text):
    print("A direct working-camera replacement remains outside the coordinator.")
    sys.exit(1)
if re.search(r"workingPreset_\s*=", app_window_text):
    print("A direct whole working-preset replacement remains outside the classified boundary.")
    sys.exit(1)
if "kPreviewNavigationCoalescingWindowMilliseconds = 500U" not in app_window_text:
    print("PH-03 preview navigation coalescing policy should remain an explicit 500 ms inactivity boundary.")
    sys.exit(1)
for marker in [
    "Preset editedPreset = workingPreset_;",
    "LoadSelectedPreset({ParameterMutationOrigin::Import",
    "ProjectPresetReplacementKind::PaletteDialog",
    "ProjectPresetReplacementKind::EquationDialog",
    "ProjectPresetReplacementKind::SettingsDialog",
    "ProjectPresetReplacementKind::JourneyDialog",
]:
    if marker not in app_window_text:
        print(f"PH-03 classified broad replacement route missing: {marker}")
        sys.exit(1)
for marker in [
    "Implemented PH-03 foundation",
    "PH-03 mutation-path audit",
    "main-window palette/post scalar",
    "deliberate main-window camera",
    "built-in palette selection",
    "mutation-origin",
    "future-history eligibility",
    "preview pan/zoom gesture coalescing",
    "classified whole-preset replacements",
    "PH-05 atomic history integration",
    "VAL-016",
    "mw-render-state-v1",
]:
    if marker not in state_doc:
        print(f"PH-03 state architecture evidence boundary missing: {marker}")
        sys.exit(1)
print("PH-03 parameter adapter, canonical fingerprint, mutation coordinator, gesture-coalescing and classified replacement checks passed.")

# PH-04 bounded camera/palette undo and redo.
project_history_header = (root / "src/Core/ProjectHistory.h").read_text(encoding="utf-8")
project_history_source = (root / "src/Core/ProjectHistory.cpp").read_text(encoding="utf-8")
undo_doc = (root / "docs/features/UNDO-REDO-PLAN.md").read_text(encoding="utf-8")
for marker in [
    "ParameterChangeOperation",
    "ProjectHistoryEntry",
    "ProjectHistoryLimits",
    "RecordParameterMutation",
    "ProjectHistoryApplyResult",
    "CanUndo",
    "CanRedo",
    "UndoLabel",
    "RedoLabel",
    "EstimatedBytes",
]:
    if marker not in project_history_header:
        print(f"PH-04 history contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "ParameterMutationOrigin::UndoRedo",
    "TruncateRedoBranch",
    "EnforceBounds",
    "SameTargets",
    "maximumEstimatedBytes",
    "cameraMetadata",
]:
    if marker not in project_history_source:
        print(f"PH-04 history implementation marker missing: {marker}")
        sys.exit(1)
if "src/Core/ProjectHistory.cpp" not in cmake_text:
    print("PH-04 history implementation is not linked into MandelbrotCore.")
    sys.exit(1)
for marker in [
    "TestProjectHistory",
    "Matching gesture kind, token and targets should coalesce",
    "Undo should restore exact full state",
    "A new edit after undo must truncate the redo branch",
    "Runtime and background origins must not enter project history",
    "One palette slider gesture should coalesce",
    "Registered rotation changes should round-trip",
    "entry and estimated-memory bounds",
]:
    if marker not in core_tests:
        print(f"PH-04 history test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "ProjectHistory projectHistory_",
    "UndoProjectEdit",
    "RedoProjectEdit",
    "UpdateHistoryCommands",
    "RefreshAfterProjectHistory",
    "ApplyMainWindowRotationMutation",
    "ParameterGestureKind::PaletteControl",
    "UndoButton",
    "RedoButton",
    "FVIRTKEY | FCONTROL",
]:
    if marker not in app_window_text and marker not in app_window_header_text:
        print(f"PH-04 Win32 integration marker missing: {marker}")
        sys.exit(1)
for marker in [
    "retain PH-04 gesture coalescing",
    "runtime-only",
    "Ctrl+Z",
    "Ctrl+Y",
    "500 ms",
    "redo branch",
    "custom palette-stop",
    "VAL-019",
    "VAL-020",
    "VAL-022",
    "PH-05",
]:
    if marker not in undo_doc:
        print(f"PH-04 undo/redo evidence boundary missing: {marker}")
        sys.exit(1)
print("PH-04 bounded scalar history, coalescing, commands and bounds checks passed.")

# PH-05 complete project undo and redo.
for marker in [
    "ProjectHistoryEntryKind",
    "PresetReplacementOperation",
    "RecordPresetReplacement",
    "requiresFullRender",
]:
    if marker not in project_history_header:
        print(f"PH-05 history contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "FullProjectInvalidationMask",
    "DefaultReplacementLabel",
    "ProjectPresetReplacementKind::DirectProjectEdit",
    "represented != after",
    "EstimatePresetBytes",
]:
    if marker not in project_history_source:
        print(f"PH-05 history implementation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "round-trip as one atomic PH-05 entry",
    "mixed palette/post action should be one complete reversible PH-05 entry",
    "Palette stop insert/remove/reorder",
    "multi-field equation edit",
    "Journey row insertion and reorder",
    "Preset application should be one labelled reversible project action",
    "Applying one Scout result should create exactly one labelled history entry",
    "structural snapshot larger than the configured bound should be rejected",
    "Runtime structural replacements must remain excluded from history",
]:
    if marker not in core_tests:
        print(f"PH-05 history test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "projectHistory_.RecordPresetReplacement",
    "ProjectPresetReplacementKind::DirectProjectEdit",
    "ProjectPresetReplacementKind::PresetLoad,\n                        false",
]:
    if marker not in app_window_text and marker not in app_window_header_text and marker not in project_history_source:
        print(f"PH-05 Win32 integration marker missing: {marker}")
        sys.exit(1)
for marker in [
    "PH-05 complete at the documented automated native scope",
    "runtime-only",
    "atomic structural snapshot",
    "cross-dialog",
    "VAL-021",
    "VAL-036",
    "completing the automated native PH-05 replacement matrix",
]:
    if marker not in undo_doc:
        print(f"PH-05 undo/redo evidence boundary missing: {marker}")
        sys.exit(1)
print("PH-05 structural history, broad transactions, Scout apply and evidence-boundary checks passed.")


# PH-06 deterministic general animation evaluator.
general_animation_header = (root / "src/Core/GeneralAnimation.h").read_text(encoding="utf-8")
general_animation_source = (root / "src/Core/GeneralAnimation.cpp").read_text(encoding="utf-8")
animation_plan = (root / "docs/features/ANIMATION-TRACKS-PLAN.md").read_text(encoding="utf-8")
for marker in [
    "AnimationTimeline",
    "AnimationTrack",
    "AnimationKeyframe",
    "CompensatedAnimationValue",
    "AnimationTargetDescriptor",
    "EvaluateGeneralAnimation",
    "AnimationClockBank",
    "enum class AnimationClockDomain",
    "Preview,",
    "Wallpaper,",
    "Export,",
]:
    if marker not in general_animation_header:
        print(f"PH-06 timeline/evaluator contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "kMaximumTracks = 256U",
    "kMaximumKeyframes = 4096U",
    "kMaximumDurationSeconds = 604800.0",
    "camera.centre-x",
    "equation.parameter-power",
    "std::fma",
    "std::log",
    "std::remainder",
    "Duplicate enabled Replace targets are not allowed",
    "Disabled unknown target was retained and will not be evaluated",
    "left->id < right->id",
    "Animation base snapshot must already satisfy",
    "ParameterInvalidation::EquationPrecision",
]:
    if marker not in general_animation_source:
        print(f"PH-06 deterministic evaluator marker missing: {marker}")
        sys.exit(1)
if "src/Core/GeneralAnimation.cpp" not in cmake_text:
    print("PH-06 evaluator implementation is not linked into MandelbrotCore.")
    sys.exit(1)
for marker in [
    "TestGeneralAnimationEvaluator",
    "Same snapshot, timeline, time and seed should yield identical frame-local state",
    "General animation evaluation must not mutate the immutable base snapshot",
    "Camera scale should use logarithmic interpolation",
    "Deep camera interpolation should retain a compensated low component",
    "Duplicate enabled Replace targets should be rejected",
    "Per-frame animation evaluation must not enter project undo history",
    "Preview, wallpaper and export clocks must remain independent",
]:
    if marker not in core_tests:
        print(f"PH-06 evaluator test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "PH-06 deterministic evaluator implementation complete",
    "runtime-only",
    "compensated high/low values",
    "Clamp / Loop / PingPong",
    "disabled unknown future target",
    "VAL-023–VAL-025",
    "DEC-014",
    "Native MSVC compilation",
]:
    if marker not in animation_plan:
        print(f"PH-06 animation evidence boundary missing: {marker}")
        sys.exit(1)
print("PH-06 deterministic timeline, interpolation, clock-separation and evidence-boundary checks passed.")

# PH-07 basic animation editor and Journey adapter.
general_animation_dialog = (root / "src/App/GeneralAnimationEditorDialog.cpp").read_text(encoding="utf-8")
for marker in [
    "ReadGeneralAnimationTargetValue",
    "ConvertJourneyToGeneralAnimation",
    "ConvertGeneralAnimationToJourney",
    "AddGeneralAnimationKeyframeFromPreset",
]:
    if marker not in general_animation_header:
        print(f"PH-07 editor/adapter contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "candidate.loopMode = AnimationLoopMode::Clamp",
    "Journey conversion supports only enabled camera centre and scale tracks",
    "Journey text cannot preserve compensated low camera components",
    "contains an invalid numeric value",
    "The selected track already has a keyframe at this time",
]:
    if marker not in general_animation_source:
        print(f"PH-07 Journey/authoring implementation marker missing: {marker}")
        sys.exit(1)
for marker in [
    "TestGeneralAnimationEditorAndJourneyAdapter",
    "Supported Journey and camera tracks should round-trip without timing or value loss",
    "Journey conversion must reject malformed rows instead of silently dropping them",
    "Tracks-to-Journey must explicitly refuse unsupported enabled targets without dropping them",
    "Add Current Value should read the authoritative project snapshot through the target registry",
]:
    if marker not in core_tests:
        print(f"PH-07 editor/adapter test marker missing: {marker}")
        sys.exit(1)
if "src/App/GeneralAnimationEditorDialog.cpp" not in cmake_text:
    print("PH-07 animation editor is not linked into the Win32 application target.")
    sys.exit(1)
for marker in [
    "Animation Timeline",
    "Add Current Value",
    "Journey to Tracks",
    "Tracks to Journey",
    "AnimationClockDomain::Preview",
    "ScrubberTrack",
    "PlayPauseButton",
    "ValidateGeneralAnimationTimeline",
]:
    if marker not in general_animation_dialog:
        print(f"PH-07 Win32 editor marker missing: {marker}")
        sys.exit(1)
for marker in [
    "GeneralAnimationEditorDialog::Show",
    "EvaluateGeneralAnimation(workingPreset_, generalAnimationPreviewTimeline_",
    "Preset renderPreset = dialogPreviewPreset_.value_or(workingPreset_)",
    "ProjectPresetReplacementKind::JourneyDialog",
    "previewGestureCoalescer_.Reset()",
]:
    if marker not in app_window_text:
        print(f"PH-07 AppWindow integration marker missing: {marker}")
        sys.exit(1)
for marker in [
    "PH-07 implementation complete",
    "runtime-only",
    "DEC-014",
    "VAL-026",
    "Native MSVC compilation",
]:
    if marker not in animation_plan:
        print(f"PH-07 animation evidence boundary missing: {marker}")
        sys.exit(1)
print("PH-07 editor, authoritative-value authoring, Journey conversion and preview-clock checks passed.")

# PH-08 deterministic PNG frame-sequence export.
frame_export_header = (root / "src/Core/FrameSequenceExport.h").read_text(encoding="utf-8")
frame_export_source = (root / "src/Core/FrameSequenceExport.cpp").read_text(encoding="utf-8")
frame_export_dialog = (root / "src/App/FrameSequenceExportDialog.cpp").read_text(encoding="utf-8")
high_res_export_dialog = (root / "src/App/HighResRenderDialog.cpp").read_text(encoding="utf-8")
image_codec_header = (root / "src/WindowsIntegration/ImageCodec.h").read_text(encoding="utf-8")
image_codec_source = (root / "src/WindowsIntegration/ImageCodec.cpp").read_text(encoding="utf-8")
offline_export_plan = (root / "docs/features/OFFLINE-EXPORT-PLAN.md").read_text(encoding="utf-8")
for marker in [
    "FrameRate",
    "FrameSequenceExportSettings",
    "FrameSequenceExportJob",
    "FrameSequenceManifest",
    "FrameTimeForIndex",
    "BuildFrameSequenceExportJob",
    "RunFrameSequenceExport",
]:
    if marker not in frame_export_header:
        print(f"PH-08 frame-export contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "kMaximumFrameCount = 1000000U",
    "std::hexfloat",
    "CanonicalTimeline",
    "Sha256Hex",
    ".mw-frame-sequence",
    "WriteTextAtomically",
    "SaveReceipt",
    "ValidateGeneralAnimationTimeline",
    "EvaluateGeneralAnimation",
    "An untracked final frame already exists",
    "The existing frame manifest does not match",
    "scaleQualityToResolution",
]:
    if marker not in frame_export_source:
        print(f"PH-08 deterministic job/manifest marker missing: {marker}")
        sys.exit(1)
if "src/Core/FrameSequenceExport.cpp" not in cmake_text:
    print("PH-08 frame export implementation is not linked into MandelbrotCore.")
    sys.exit(1)
if "src/App/FrameSequenceExportDialog.cpp" not in cmake_text:
    print("PH-08 frame export dialog is not linked into the Win32 application target.")
    sys.exit(1)
for marker in [
    "WicRowEncoder",
    "ValidateImageDimensionsWithWic",
]:
    if marker not in image_codec_header or marker not in image_codec_source:
        print(f"PH-08 reusable WIC boundary marker missing: {marker}")
        sys.exit(1)
if "CoInitializeEx" in high_res_export_dialog and "#include <objbase.h>" not in high_res_export_dialog:
    print("PH-08 high-resolution render thread uses COM APIs without the explicit objbase.h declaration header.")
    sys.exit(1)
for marker in [
    "Deterministic PNG Frame Sequence",
    "Resume matching manifest",
    "Include aligned end frame",
    "RunFrameSequenceExport",
    "RenderStillImageTiled",
    "scaleQualityToResolution = false",
    "ValidateImageDimensionsWithWic",
    "cancelRequested",
    "JoinWorker",
    "kExportProgressMessage",
]:
    if marker not in frame_export_dialog:
        print(f"PH-08 Win32 export marker missing: {marker}")
        sys.exit(1)
for marker in [
    "Export Frames...",
    "OpenFrameSequenceExportDialog",
    "PausePresentation",
    "Paused for deterministic frame-sequence export",
    "generalAnimationTimeline_",
]:
    if marker not in app_window_text and marker not in app_window_header_text:
        print(f"PH-08 AppWindow integration marker missing: {marker}")
        sys.exit(1)
for marker in [
    "TestFrameSequenceExport",
    "Frame time should be derived directly from index and rational rate",
    "Cancellation should stop new frames while preserving two verified outputs",
    "Resume should verify two frames and render only the missing frame",
    "Resume must compare typed manifest settings as well as its job fingerprint",
    "A manifest entry missing fileSizeBytes should be rejected safely",
    "Selected production export frame should repeat exactly",
]:
    if marker not in core_tests:
        print(f"PH-08 frame-export test marker missing: {marker}")
        sys.exit(1)
for marker in [
    "PH-08 implementation boundary",
    "immutable normalised",
    "directly from `start + index",
    "temporary/backup promotion",
    "fingerprint and matching typed manifest fields",
    "VAL-027–VAL-030",
    "bounded native success/resume/refusal/cancellation evidence",
]:
    if marker not in offline_export_plan:
        print(f"PH-08 offline-export evidence boundary missing: {marker}")
        sys.exit(1)
if "int schemaVersion{12}" not in (root / "src/Core/Models.h").read_text(encoding="utf-8"):
    print("PH-08/PH-12 settings schema boundary is missing.")
    sys.exit(1)
print("PH-08 immutable jobs, exact timing, verified promotion, resume, cancellation and evidence-boundary checks passed.")

# PH-09 external FFmpeg H.264/MP4 encoding.
video_export_header = (root / "src/Core/ExternalVideoExport.h").read_text(encoding="utf-8")
video_export_source = (root / "src/Core/ExternalVideoExport.cpp").read_text(encoding="utf-8")
video_export_dialog = (root / "src/App/VideoExportDialog.cpp").read_text(encoding="utf-8")
external_process = (root / "src/WindowsIntegration/ExternalProcess.cpp").read_text(encoding="utf-8")
external_video_fixture = (root / "src/Tools/ExternalVideoFixtureTool.cpp").read_text(encoding="utf-8")
for marker in [
    "ExternalEncoderCapabilities",
    "ExternalVideoExportJob",
    "ParseFfmpegCapabilities",
    "BuildWindowsCommandLine",
    "LoadVerifiedFrameSequence",
    "BuildExternalVideoExportJob",
    "RunExternalVideoExport",
]:
    if marker not in video_export_header:
        print(f"PH-09 video-export contract marker missing: {marker}")
        sys.exit(1)
for marker in [
    "kExternalVideoEncoderId",
    "libx264",
    "yuv420p",
    "+faststart",
    "-progress",
    "pipe:1",
    "The final MP4 already exists",
    "Frame sequence verification was cancelled",
    "capability.libx264=true",
    "CleanupTrackedSequence",
    "even source width and height",
]:
    if marker not in video_export_source:
        print(f"PH-09 fixed encoder/safety marker missing: {marker}")
        sys.exit(1)
for forbidden in ["cmd.exe", "powershell.exe", "ShellExecuteW(executable", "system(", "_wsystem("]:
    if forbidden in video_export_source or forbidden in external_process:
        print(f"PH-09 forbidden shell invocation marker found: {forbidden}")
        sys.exit(1)
for marker in [
    "CreateProcessW",
    "CREATE_NO_WINDOW",
    "TerminateProcess",
    "stdoutReader",
    "stderrReader",
    "kMaximumCapturedBytes",
]:
    if marker not in external_process:
        print(f"PH-09 owned-process marker missing: {marker}")
        sys.exit(1)
for marker in [
    "External FFmpeg H.264 / MP4 Export",
    "Detect PATH",
    "encoder=libx264",
    "muxer=mp4",
    "RunOwnedProcess",
    "Delete verified PNGs after success",
    "Source PNG frames and encoder logs were preserved",
    "id == SequenceEdit && notification == EN_CHANGE",
]:
    if marker not in video_export_dialog:
        print(f"PH-09 Win32 video-export marker missing: {marker}")
        sys.exit(1)
for marker in [
    "Encode MP4...",
    "OpenVideoExportDialog",
    "Paused for external FFmpeg video encoding",
]:
    if marker not in app_window_text and marker not in app_window_header_text:
        print(f"PH-09 AppWindow integration marker missing: {marker}")
        sys.exit(1)
for marker in [
    "TestExternalVideoExport",
    "fixed FFmpeg command line must not introduce a command shell",
    "Encoder failure must preserve verified source frames",
    "Encoder cancellation should remove only the owned temporary MP4",
    "Optional cleanup should occur only after verified success",
    "H.264 yuv420p video jobs should reject odd source dimensions before FFmpeg starts",
]:
    if marker not in core_tests:
        print(f"PH-09 video-export regression marker missing: {marker}")
        sys.exit(1)
if "src/Core/ExternalVideoExport.cpp" not in cmake_text or \
        "src/App/VideoExportDialog.cpp" not in cmake_text or \
        "src/WindowsIntegration/ExternalProcess.cpp" not in cmake_text:
    print("PH-09 sources are not fully linked into the core and Win32 targets.")
    sys.exit(1)
for marker in [
    "ProbeFfmpeg",
    "RunOwnedProcess",
    "RenderStillImageTiled",
    "WicRowEncoder",
    "LoadVerifiedFrameSequence",
    "RunExternalVideoExport",
    '"ffmpegPathPersisted", false',
    '"sourceFramesPreserved"',
    '"temporaryOutputAbsent"',
    "RunOwnedCancellationProbe",
    '"ownedProcessCancellation"',
    '"cancellationElapsedMilliseconds"',
    "RunExpectedEncodeFailure",
    '"sequence-directory-unavailable-after-preflight"',
    '"realEncodeFailureContained"',
    '"failureSourceFramesPreserved"',
    "BuildLinkedCancellationSequence",
    "RunIntegratedExportCancellation",
    "kIntegratedCancellationFrameCount = 120U",
    '"integratedExportCancellation"',
    '"integratedCancellationSourceFramesPreserved"',
]:
    if marker not in external_video_fixture:
        print(f"PH-09 real-FFmpeg fixture marker missing: {marker}")
        sys.exit(1)
for marker in [
    "MandelbrotExternalVideoFixture",
    "src/Tools/ExternalVideoFixtureTool.cpp",
    "src/WindowsIntegration/ExternalProcess.cpp",
    "src/WindowsIntegration/ImageCodec.cpp",
    "windowscodecs ole32",
]:
    if marker not in cmake_text:
        print(f"PH-09 real-FFmpeg fixture build marker missing: {marker}")
        sys.exit(1)
if 'add_test(NAME MandelbrotExternalVideoFixture' in cmake_text:
    print("PH-09 real-FFmpeg fixture must remain opt-in because no FFmpeg binary is bundled.")
    sys.exit(1)
if '{"ffmpegExecutable", Utf8Path(ffmpeg)}' in external_video_fixture or \
        "WinGet\\\\Packages" in external_video_fixture:
    print("PH-09 fixture must not persist or hard-code a local FFmpeg path.")
    sys.exit(1)
if "int schemaVersion{12}" not in (root / "src/Core/Models.h").read_text(encoding="utf-8"):
    print("PH-09/PH-12 settings schema boundary is missing.")
    sys.exit(1)
ph09_document_markers = {
    "project_docs/DECISIONS_AND_CHANGE_HISTORY.md": [
        "DEC-009 | Accepted for PH-09",
        "DEC-027 | Accepted for PH-09",
        "user-supplied external executable",
    ],
    "project_docs/IMPLEMENTATION_PLAN.md": [
        "PH-09 — External FFmpeg Encoding",
        "Implementation complete",
        "real FFmpeg 8.1.1 capability/encode/decode, owned-process cancellation, integrated export cancellation and encoder-failure containment fixture passed",
    ],
    "docs/features/OFFLINE-EXPORT-PLAN.md": [
        "Accepted boundary",
        "does not bundle, download or remember",
        "decodes one video frame",
        "manifest/receipts",
        "p9-v7/report.json",
    ],
    "project_docs/TRACEABILITY.md": [
        "REQ-018 | Current",
        "PH-09 | Implemented / partly verified",
        "Current PH-09 evidence",
        "real FFmpeg 8.1.1 success/owned-process cancellation/integrated export cancellation/failure fixture",
    ],
    "project_docs/SECURITY_PRIVACY_AND_RISK.md": [
        "PH-09 external encoder boundary",
        "exact executable is launched directly",
        "capped at 1 MiB each",
        "real FFmpeg 8.1.1 success-path evidence",
    ],
    "project_docs/DATA_AND_PERSISTENCE.md": [
        "PH-09 video export data boundary",
        "The durable formats are now settings schema 12 and preset schema 3",
        "not persisted",
    ],
}
for relative_path, markers in ph09_document_markers.items():
    document_text = (root / relative_path).read_text(encoding="utf-8")
    for marker in markers:
        if marker not in document_text:
            print(f"PH-09 documentation/evidence boundary missing in {relative_path}: {marker}")
            sys.exit(1)
for relative_path in [
    "project_docs/VALIDATION_AND_EVIDENCE.md",
    "project_docs/DELIVERY_REPORT.md",
    "project_docs/PROJECT_INDEX.md",
    "docs/roadmaps/INTEGRATED-CREATIVE-ROADMAP.md",
]:
    document_text = (root / relative_path).read_text(encoding="utf-8")
    for marker in ["BR-20260909-01", "artifacts/p9-v7"]:
        if marker not in document_text:
            print(f"PH-09 real-FFmpeg delivery evidence missing in {relative_path}: {marker}")
            sys.exit(1)
print("PH-09 external capability checking, fixed-vector invocation, verified promotion, cancellation, logs and cleanup checks passed.")

# Proposed PH-12–PH-15 deep-zoom governance integration. These checks prove
# canonical structure only; they deliberately do not claim implementation.
deep_upgrade_pack = root / "Mandelbrot-Deep-Zoom-Upgrade-Pack-1.13.1-v2-COMBINED.md"
if not deep_upgrade_pack.is_file():
    print("Deep-zoom supporting design pack is missing.")
    sys.exit(1)
proposed_deep_document_markers = {
    "project_docs/PROJECT_FOUNDATION.md": [
        "REQ-021:",
        "REQ-040:",
        "AC-021:",
        "AC-040:",
        "Literal “infinite” or “unlimited” zoom claims",
    ],
    "project_docs/IMPLEMENTATION_PLAN.md": [
        "PH-12 — Baseline Repair and Exact Camera Foundation",
        "PH-13 — Central Precision Planner and Orbit Service",
        "PH-14 — Perturbation Validity, Rebase and Multi-Reference",
        "PH-15 — Deep Export and Authoring Integration Hardening",
        "do not change the active phase",
    ],
    "project_docs/TRACEABILITY.md": [
        "REQ-021 | Deep camera",
        "VAL-037 | Baseline structural-gate reconciliation",
        "VAL-060 | Deep still/frame/FFmpeg release matrix",
        "PH-15 | Proposed",
    ],
    "project_docs/DECISIONS_AND_CHANGE_HISTORY.md": [
        "DEC-028 | Accepted by delegated user direction",
        "DEC-029 | Accepted; package selected and source-integrated",
        "DEC-036 | Accepted by user direction",
        "DEC-037 | Proposed",
        "DEC-038 | Accepted by user direction",
    ],
    "project_docs/SECURITY_PRIVACY_AND_RISK.md": [
        "RISK-013 | Long decimal coordinates",
        "RISK-030 | Product copy claims infinite support",
    ],
    "project_docs/UI_WORKFLOWS_AND_ROUTES.md": [
        "ROUTE-015 | Edit/Copy exact coordinates",
        "ROUTE-018 | Deep still / frame / video export",
        "governance mappings only",
    ],
    "docs/architecture/PROJECT-STATE-AND-PARAMETERS.md": [
        "Proposed PH-12 exact camera authority",
        "Exact camera is the only mutable project camera",
    ],
    "docs/architecture/RENDERING-AND-EXPORT-CONTRACTS.md": [
        "Proposed PH-13–PH-15 deep-render contract",
        "One platform-neutral deterministic `PrecisionPlanner`",
        "A frame with unresolved pixels beyond policy is invalid",
    ],
    "project_docs/VALIDATION_AND_EVIDENCE.md": [
        "Deep-zoom pack integration and baseline reconciliation",
        "VAL-037 structural evidence only",
    ],
}
for relative_path, markers in proposed_deep_document_markers.items():
    document_text = (root / relative_path).read_text(encoding="utf-8")
    for marker in markers:
        if marker not in document_text:
            print(f"Proposed deep-zoom governance marker missing in {relative_path}: {marker}")
            sys.exit(1)
print("Proposed PH-12–PH-15 deep-zoom governance and evidence-boundary checks passed.")

# 2026-09-07 modeless-editor camera retention and preview precision recovery.
for marker in [
    "MergePaletteEditorCandidate(workingPreset_, editedPreset)",
    "MergeEquationEditorCandidate(workingPreset_, editedPreset)",
    "PreviewCanRenderCamera",
    "RestartPreviewRenderer",
]:
    if marker not in app_window_text:
        print(f"Modeless-editor/preview recovery marker missing: {marker}")
        sys.exit(1)
for marker in [
    "AuxiliaryWindowSession",
    "auxiliaryWindowOpen_",
    "OpensAuxiliaryWindow",
    "EnableWindow(app_.window_, FALSE)",
    "competing editor",
    "MW_TEST_EDITOR_EXCLUSIVITY_ONLY",
]:
    if marker not in app_window_text and marker not in app_window_header_text and marker not in windows_interaction_fixture_text:
        print(f"Exclusive editor-session marker missing: {marker}")
        sys.exit(1)
for marker in [
    "TestModelessEditorCandidateMerges",
    "A palette-editor change must preserve camera, exact camera",
    "The central legacy GPU policy should begin an automatic preview in float32.",
]:
    if marker not in core_tests:
        print(f"Modeless-editor/automatic-precision regression marker missing: {marker}")
        sys.exit(1)
d3d_fixture_text = (root / "src/Tools/D3D11WarpFixtureTool.cpp").read_text(encoding="utf-8")
opengl_renderer_text = (root / "src/Rendering/OpenGLRenderer.cpp").read_text(encoding="utf-8")
opengl_fixture_text = (root / "src/Tools/OpenGLFixtureTool.cpp").read_text(encoding="utf-8")
for marker in ["VerifyAutomaticPreviewPrecisionSwitches", '"GPU float32"',
               '"Split high/low float"', '"GPU perturbation / arbitrary-precision reference"']:
    if marker not in d3d_fixture_text:
        print(f"Automatic preview precision fixture marker missing: {marker}")
        sys.exit(1)
for marker in ["expD(double value)", "double smoothIteration"]:
    if marker not in opengl_renderer_text:
        print(f"OpenGL native-float64 compatibility marker missing: {marker}")
        sys.exit(1)
if "including native float64" not in opengl_fixture_text:
    print("OpenGL native-float64 production fixture marker missing.")
    sys.exit(1)
print("Modeless-editor camera retention and automatic preview precision recovery checks passed.")

# PH-10 deterministic production Scout thumbnail fixture.
visual_fixture_tool_text = (root / "src/Tools/VisualFixtureTool.cpp").read_text(encoding="utf-8")
for marker in [
    '#include "Core/FractalScout.h"',
    "RunScoutThumbnailCheck",
    'record.id = "fractal-scout-thumbnails"',
    "FRACTAL-SCOUT-CANDIDATE/V2-EXACT-CAMERA",
    "RunFractalScout(request",
    "WriteComparisonArtifacts(candidateDirectory",
    "--no-scout-thumbnail-check",
    "distinct, non-uniform, exactly repeatable thumbnails and identities",
]:
    if marker not in visual_fixture_tool_text:
        print(f"PH-10 Scout thumbnail fixture marker missing: {marker}")
        sys.exit(1)
scout_status_text = (root / "docs/features/FRACTAL-SCOUT-STATUS.md").read_text(encoding="utf-8")
visual_regression_text = (root / "docs/testing/VISUAL-REGRESSION.md").read_text(encoding="utf-8")
for marker in ["PH-10 complete at the documented bounded/native scope", "native dialog isolation and Apply replay"]:
    if marker not in scout_status_text:
        print(f"PH-10 Scout status boundary missing: {marker}")
        sys.exit(1)
for marker in ["`fractal-scout-thumbnails`", "RunFractalScout", "Candidate generation remains separate from baseline approval"]:
    if marker not in visual_regression_text:
        print(f"PH-10 Scout visual evidence boundary missing: {marker}")
        sys.exit(1)
print("PH-10 deterministic production Scout thumbnail and native interaction checks passed.")
