#pragma once

#include "Core/AdaptivePerformance.h"
#include "Core/Animation.h"
#include "Core/Models.h"
#include "Core/SettingsStore.h"
#include "Rendering/GpuRenderer.h"
#include "App/QuickControllerWindow.h"
#include "WindowsIntegration/SystemStateMonitor.h"
#include "WindowsIntegration/TrayIcon.h"
#include "WindowsIntegration/WallpaperController.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

namespace mw {

class AppWindow {
public:
    AppWindow();
    ~AppWindow();

#ifdef _WIN32
    bool Create(HINSTANCE instance, int showCommand, bool startHidden, std::string& error);
    int RunMessageLoop();
#endif

private:
#ifdef _WIN32
    enum ControlId : int {
        MainTab = 2001,
        NavigationPreviewButton,
        NavigationWallpaperButton,
        NavigationDiagnosticsButton,
        NavigationSettingsButton,
        NavigationPaletteButton,
        NavigationControllerButton,
        NavigationEquationButton,
        PresetLibraryButton,
        CoordinatesEdit,
        PreviewContextLabel,
        WallpaperContextLabel,
        PresetCombo,
        PaletteCombo,
        PaletteEditorButton,
        EquationEditorButton,
        AnimationCombo,
        PerformanceCombo,
        MonitorModeCombo,
        MonitorCombo,
        MonitorAssignmentCombo,
        PresetNameEdit,
        CentreXEdit,
        CentreYEdit,
        ScaleEdit,
        IterationsTrack,
        IterationsEdit,
        FpsTrack,
        FpsEdit,
        RenderScaleTrack,
        RenderScaleEdit,
        ZoomSpeedTrack,
        ColourSpeedTrack,
        ColourCycleButton,
        BrightnessTrack,
        BrightnessEdit,
        ContrastTrack,
        ContrastEdit,
        SaturationTrack,
        SaturationEdit,
        ColourOffsetTrack,
        ColourOffsetEdit,
        SetWallpaperButton,
        SetStaticWallpaperButton,
        AddSlideshowButton,
        ManageSlideshowButton,
        PauseButton,
        StopButton,
        ResetViewButton,
        SaveNewButton,
        SaveChangesButton,
        RestoreBuiltInsButton,
        OpenSettingsButton,
        ConfigurePrecisionButton,
        DeletePresetButton,
        ImportPresetButton,
        ExportPresetButton,
        AssignMonitorButton,
        StartupCheck,
        FullscreenCheck,
        BatteryCheck,
        RemoteCheck,
        ReducedMotionCheck,
        StatusLabel,
        FpsLabel,
        ProfileLabel,
        MonitorInfoLabel,
        PrecisionLabel,
        OpenLogsButton,
        CopyDiagnosticsButton,
        ClearLogsButton,
        OpenControllerButton,
        RenderHighResButton,
    };

    static constexpr UINT TimerId = 1;
    static constexpr UINT TaskbarCreatedMessageFallback = WM_APP + 70;

    static LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK PreviewProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandlePreviewMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

    bool RegisterClasses(std::string& error);
    void CreateControls();
    void LayoutControls(int width, int height);
    void PopulateControls();
    void PopulatePresetCombo();
    void OpenPresetLibrary(bool applyLoadedPresetToDesktop = false);
    void PopulateMonitorControls();
    void ShowSelectedTab();
    void SelectPage(int page);
    void UpdateMonitorAssignmentControls();
    void LoadMonitorAssignmentSelection();
    void SyncNumericEditsFromTracks();
    void SyncTrackFromNumericEdit(int controlId);
    void UpdateCoordinatesEdit();
    bool ApplyCoordinatesEdit(bool showError);
    void LoadSelectedPreset();
    void ApplyControlsToWorkingPreset();
    void ApplyPerformanceProfile();
    bool SaveSettings(std::string* errorOut = nullptr);
    void UpdateStatus();
    void RenderTick();
    void PollSystemState();
    void SetWallpaper();
    void StartSavedStaticWallpaper();
    void SetStaticWallpaper();
    void AddPreviewToSlideshow();
    void ManageSlideshow();
    Preset CurrentPreviewSnapshot();
    std::filesystem::path StaticStorageDirectory() const;
    void OpenPaletteEditor();
    void OpenEquationEditor();
    void TogglePause();
    void ToggleColourCycling();
    void StopWallpaper();
    void SelectRelativePreset(int direction);
    void SaveAsNewPreset(const std::string& requestedName = {});
    void SaveChangesToPreset();
    void RestoreBuiltInPresets();
    void OpenSettings();
    void DeleteSelectedPreset();
    void ImportPreset();
    void ExportPreset();
    void AssignPresetToMonitor();
    void ToggleStartup();
    void OpenLogFolder();
    void CopyDiagnostics();
    void CopyCoordinates();
    void ClearLogs();
    void ShowRendererError(const std::string& detail);
    void ShowWindowAndActivate();
    void ExitApplication();
    void ToggleZoomMotion();
    void TogglePreviewZoomMotion();
    void TogglePreviewColourCycling();
    void ToggleDesktopZoomMotion();
    void ToggleDesktopColourCycling();
    void ApplyPreviewAsSlideshowWallpaper();
    void JumpToCoordinates();
    void OpenQuickController();
    void OpenHighResRenderDialog();
    void UpdateQuickController();

    Preset* FindPresetMutable(const std::string& id);
    const Preset* FindPreset(const std::string& id) const;
    std::vector<Preset> AllPresets() const;
    int SelectedComboIndex(HWND combo) const;
    static std::wstring ToWide(const std::string& text);
    static std::string ToUtf8(const std::wstring& text);
    static std::string ReadControlText(HWND control);
    static void SetControlText(HWND control, const std::string& text);
    static std::string FormatDouble(double value, int precision = 15);
    static bool ReadTextFile(const std::wstring& path, std::string& text, std::string& error);
    static bool WriteTextFile(const std::wstring& path, const std::string& text, std::string& error);

    HINSTANCE instance_{nullptr};
    HWND window_{nullptr};
    HWND previewWindow_{nullptr};
    HWND mainTab_{nullptr};
    HWND navigationPreviewButton_{nullptr};
    HWND navigationWallpaperButton_{nullptr};
    HWND navigationDiagnosticsButton_{nullptr};
    HWND navigationSettingsButton_{nullptr};
    HWND navigationPaletteButton_{nullptr};
    HWND navigationControllerButton_{nullptr};
    HWND navigationEquationButton_{nullptr};
    HWND presetLibraryButton_{nullptr};
    HWND coordinatesEdit_{nullptr};
    HWND previewContextLabel_{nullptr};
    HWND wallpaperContextLabel_{nullptr};
    HWND presetCombo_{nullptr};
    HWND paletteCombo_{nullptr};
    HWND animationCombo_{nullptr};
    HWND performanceCombo_{nullptr};
    HWND monitorModeCombo_{nullptr};
    HWND monitorCombo_{nullptr};
    HWND monitorAssignmentCombo_{nullptr};
    HWND presetNameEdit_{nullptr};
    HWND centreXEdit_{nullptr};
    HWND centreYEdit_{nullptr};
    HWND scaleEdit_{nullptr};
    HWND iterationsTrack_{nullptr};
    HWND iterationsEdit_{nullptr};
    HWND fpsTrack_{nullptr};
    HWND fpsEdit_{nullptr};
    HWND renderScaleTrack_{nullptr};
    HWND renderScaleEdit_{nullptr};
    HWND zoomSpeedTrack_{nullptr};
    HWND colourSpeedTrack_{nullptr};
    HWND brightnessTrack_{nullptr};
    HWND brightnessEdit_{nullptr};
    HWND contrastTrack_{nullptr};
    HWND contrastEdit_{nullptr};
    HWND saturationTrack_{nullptr};
    HWND saturationEdit_{nullptr};
    HWND colourOffsetTrack_{nullptr};
    HWND colourOffsetEdit_{nullptr};
    HWND statusLabel_{nullptr};
    HWND fpsLabel_{nullptr};
    HWND profileLabel_{nullptr};
    HWND monitorInfoLabel_{nullptr};
    HWND precisionLabel_{nullptr};
    HWND pauseButton_{nullptr};
    HWND colourCycleButton_{nullptr};


    HICON icon_{nullptr};
    HFONT uiFont_{nullptr};
    TrayIcon trayIcon_;
    QuickControllerWindow quickController_;
    UINT taskbarCreatedMessage_{0};

    bool draggingPreview_{false};
    POINT dragStart_{};
    bool exitRequested_{false};
    std::string previewRendererError_;
    bool userPaused_{false};
    bool autoPaused_{false};
    bool adaptivePaused_{false};
    bool previewChangesPending_{false};
    // Every process starts with a still preview and still desktop runtime.
    // Preset animation modes remain intact and can be started explicitly from
    // the Quick Controller without mutating the saved preset.
    bool zoomMotionEnabled_{false};
    bool previewColourCyclingEnabled_{false};
    bool desktopZoomMotionEnabled_{false};
    bool desktopColourCyclingEnabled_{false};
    int selectedTab_{0};
    std::chrono::steady_clock::time_point lastFrameTime_{};
    std::chrono::steady_clock::time_point lastSystemPoll_{};
    std::chrono::steady_clock::time_point autoResumeEligibleAt_{};
    std::chrono::steady_clock::time_point lastAdaptiveSample_{};

    SettingsStore settingsStore_;
    AppSettings settings_;
    std::vector<Preset> builtInPresets_;
    Preset workingPreset_;
    GpuRenderer previewRenderer_;
    AnimationController previewAnimation_;
    AnimationFrame lastPreviewFrame_;
    WallpaperController wallpaperController_;
    SystemStateMonitor systemStateMonitor_;
    AdaptivePerformanceController adaptivePerformanceController_;
    VisibleChangeDetector previewChangeDetector_;
    bool previewForceRender_{true};
#endif
};

} // namespace mw
