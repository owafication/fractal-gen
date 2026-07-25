#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <deque>
#include <string>
#include <vector>

namespace mw {

#ifdef _WIN32

UINT DialogDpi(HWND owner) noexcept;
int ScaleDialogMetric(int value, UINT dpi) noexcept;
HFONT CreateResponsiveDialogFont(UINT dpi);
DWORD AccessibleControlStyle(const wchar_t* className, DWORD style) noexcept;
RECT ResponsiveDialogRect(HWND owner, int widthAt96Dpi, int heightAt96Dpi, UINT dpi,
                          const wchar_t* placementKey = nullptr);
void RememberDialogPlacement(HWND window, const wchar_t* placementKey, UINT dpi);

class ResponsiveDialogLayout {
public:
    void Initialise(HWND window, UINT dpi, HFONT font,
                    int minimumWidthAt96Dpi, int minimumHeightAt96Dpi);
    void Shutdown() noexcept;

    void OnSize();
    bool OnScroll(UINT message, WPARAM wParam);
    bool OnMouseWheel(WPARAM wParam);
    void OnDpiChanged(UINT newDpi, const RECT& suggestedRect, HFONT newFont);
    void ApplyMinimumTrackSize(MINMAXINFO& info) const noexcept;
    void Focus(HWND control) const noexcept;
    void EnsureFocusedControlVisible();

    [[nodiscard]] UINT Dpi() const noexcept { return dpi_; }

private:
    struct ChildPlacement {
        HWND window{nullptr};
        RECT rect{};
        bool stretchWidth{true};
        bool stretchHeight{false};
    };

    void CaptureAndScaleChildren(HFONT font);
    void ApplyLayout();
    void UpdateScrollBars(int clientWidth, int clientHeight, int contentWidth, int contentHeight);
    void SetScrollPosition(int bar, int position);

    HWND window_{nullptr};
    UINT dpi_{96};
    int minimumWidth_{0};
    int minimumHeight_{0};
    int baselineWidth_{1};
    int baselineHeight_{1};
    int horizontalOffset_{0};
    int verticalOffset_{0};
    std::vector<ChildPlacement> children_;
};

class DialogTooltipManager {
public:
    bool Initialise(HWND owner, UINT dpi, HFONT font);
    void Add(HWND control, const wchar_t* text);
    void SetFont(UINT dpi, HFONT font);
    void Shutdown() noexcept;

private:
    HWND tooltip_{nullptr};
    UINT dpi_{96};
    std::deque<std::wstring> texts_;
};

bool ProcessKeyboardDialogMessage(HWND dialog, MSG& message,
                                  ResponsiveDialogLayout* layout = nullptr);
bool ProcessModalDialogMessage(HWND dialog, int cancelCommandId, MSG& message,
                               ResponsiveDialogLayout* layout = nullptr);

#endif

} // namespace mw
