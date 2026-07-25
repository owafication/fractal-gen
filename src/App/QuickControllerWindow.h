#pragma once

#ifdef _WIN32
#include <windows.h>
#include "App/DialogSupport.h"
#endif

#include <string>

namespace mw {

namespace QuickControllerCommands {
constexpr unsigned ApplySettingsLive = 43002;
constexpr unsigned SaveImage = 43003;
constexpr unsigned LoadPreset = 43004;
constexpr unsigned Edit = 43005;
constexpr unsigned StaticDesktop = 43006;
constexpr unsigned TogglePreviewZoom = 43007;
constexpr unsigned TogglePreviewColours = 43008;
constexpr unsigned CopyCoordinates = 43009;
constexpr unsigned ExitApp = 43010;
constexpr unsigned JumpToCoordinates = 43012;
constexpr unsigned ToggleDesktopZoom = 43013;
constexpr unsigned ToggleDesktopColours = 43014;
constexpr unsigned SlideshowDesktop = 43015;
constexpr unsigned RenderHighRes = 43016;
}

class QuickControllerWindow {
public:
#ifdef _WIN32
    bool Create(HINSTANCE instance, HWND commandTarget, HICON icon);
    void Show();
    void Hide();
    void Update(const std::wstring& status, const std::wstring& coordinates,
                const std::wstring& resources,
                bool previewZoomMotionEnabled, bool previewColourCyclingEnabled,
                bool desktopZoomMotionEnabled, bool desktopColourCyclingEnabled);
    [[nodiscard]] bool IsVisible() const noexcept;
    void Destroy();
    bool ProcessDialogMessage(MSG& message);
#endif

private:
#ifdef _WIN32
    static LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void CreateControls();

    HINSTANCE instance_{nullptr};
    HWND commandTarget_{nullptr};
    HWND window_{nullptr};
    HWND statusLabel_{nullptr};
    HWND coordinatesLabel_{nullptr};
    HWND resourcesLabel_{nullptr};
    HWND applyLiveButton_{nullptr};
    HWND staticDesktopButton_{nullptr};
    HWND slideshowDesktopButton_{nullptr};
    HWND previewZoomButton_{nullptr};
    HWND previewColourButton_{nullptr};
    HWND jumpCoordinatesButton_{nullptr};
    HWND desktopZoomButton_{nullptr};
    HWND desktopColourButton_{nullptr};
    HWND saveImageButton_{nullptr};
    HWND renderHighResButton_{nullptr};
    HWND copyCoordinatesButton_{nullptr};
    HWND loadPresetButton_{nullptr};
    HWND editButton_{nullptr};
    HWND exitButton_{nullptr};
    HFONT font_{nullptr};
    ResponsiveDialogLayout layout_;
    UINT dpi_{96};
    HICON icon_{nullptr};
#endif
};

} // namespace mw
