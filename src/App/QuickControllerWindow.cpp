#include "App/QuickControllerWindow.h"
#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

namespace mw {

#ifdef _WIN32
namespace {
constexpr wchar_t kQuickControllerClass[] = L"MandelbrotQuickController";

HWND Add(HWND parent, HINSTANCE instance, const wchar_t* cls, const wchar_t* text,
         DWORD style, int id, int x, int y, int width, int height, HFONT font) {
    HWND control = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style),
                                   x, y, width, height, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   instance, nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}
}

bool QuickControllerWindow::Create(HINSTANCE instance, HWND commandTarget, HICON icon) {
    if (window_) return true;
    instance_ = instance;
    commandTarget_ = commandTarget;
    icon_ = icon;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance_;
    windowClass.hIcon = icon_;
    windowClass.hIconSm = icon_;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kQuickControllerClass;
    RegisterClassExW(&windowClass);

    dpi_ = DialogDpi(commandTarget_);
    const RECT dialogRect = ResponsiveDialogRect(commandTarget_, 610, 410, dpi_, kQuickControllerClass);
    window_ = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT, kQuickControllerClass,
                              L"Mandelbrot Quick Controller",
                              WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME |
                              WS_MAXIMIZEBOX | WS_VSCROLL | WS_HSCROLL,
                              dialogRect.left, dialogRect.top,
                              dialogRect.right - dialogRect.left, dialogRect.bottom - dialogRect.top,
                              nullptr, nullptr, instance_, this);
    return window_ != nullptr;
}

void QuickControllerWindow::CreateControls() {
    font_ = CreateResponsiveDialogFont(dpi_);
    statusLabel_ = Add(window_, instance_, WC_STATICW, L"Wallpaper stopped", SS_LEFT,
                       0, 14, 12, 568, 24, font_);
    coordinatesLabel_ = Add(window_, instance_, WC_STATICW, L"Coordinates unavailable", SS_LEFT,
                            0, 14, 40, 568, 42, font_);
    resourcesLabel_ = Add(window_, instance_, WC_STATICW, L"Resources unavailable", SS_LEFT,
                          0, 14, 84, 568, 24, font_);

    constexpr int left = 14;
    constexpr int top = 120;
    constexpr int gap = 8;
    constexpr int buttonWidth = 136;
    constexpr int buttonHeight = 32;
    constexpr int rowStep = 40;
    const int column2 = left + buttonWidth + gap;
    const int column3 = column2 + buttonWidth + gap;
    const int column4 = column3 + buttonWidth + gap;

    applyLiveButton_ = Add(window_, instance_, WC_BUTTONW, L"Apply Settings Live", BS_DEFPUSHBUTTON | WS_TABSTOP,
                           QuickControllerCommands::ApplySettingsLive, left, top, buttonWidth, buttonHeight, font_);
    staticDesktopButton_ = Add(window_, instance_, WC_BUTTONW, L"Static Desktop", BS_PUSHBUTTON | WS_TABSTOP,
                               QuickControllerCommands::StaticDesktop, column2, top, buttonWidth, buttonHeight, font_);
    slideshowDesktopButton_ = Add(window_, instance_, WC_BUTTONW, L"Slideshow Desktop", BS_PUSHBUTTON | WS_TABSTOP,
                                  QuickControllerCommands::SlideshowDesktop, column3, top, buttonWidth, buttonHeight, font_);
    jumpCoordinatesButton_ = Add(window_, instance_, WC_BUTTONW, L"Jump to Coordinates...", BS_PUSHBUTTON | WS_TABSTOP,
                                 QuickControllerCommands::JumpToCoordinates, column4, top, buttonWidth, buttonHeight, font_);

    previewZoomButton_ = Add(window_, instance_, WC_BUTTONW, L"Start Preview Zoom", BS_PUSHBUTTON | WS_TABSTOP,
                             QuickControllerCommands::TogglePreviewZoom, left, top + rowStep, buttonWidth, buttonHeight, font_);
    previewColourButton_ = Add(window_, instance_, WC_BUTTONW, L"Start Preview Colours", BS_PUSHBUTTON | WS_TABSTOP,
                               QuickControllerCommands::TogglePreviewColours, column2, top + rowStep, buttonWidth, buttonHeight, font_);
    desktopZoomButton_ = Add(window_, instance_, WC_BUTTONW, L"Start Desktop Zoom", BS_PUSHBUTTON | WS_TABSTOP,
                             QuickControllerCommands::ToggleDesktopZoom, column3, top + rowStep, buttonWidth, buttonHeight, font_);
    desktopColourButton_ = Add(window_, instance_, WC_BUTTONW, L"Start Desktop Colours", BS_PUSHBUTTON | WS_TABSTOP,
                               QuickControllerCommands::ToggleDesktopColours, column4, top + rowStep, buttonWidth, buttonHeight, font_);

    saveImageButton_ = Add(window_, instance_, WC_BUTTONW, L"Save Image", BS_PUSHBUTTON | WS_TABSTOP,
                           QuickControllerCommands::SaveImage, left, top + rowStep * 2, buttonWidth, buttonHeight, font_);
    renderHighResButton_ = Add(window_, instance_, WC_BUTTONW, L"Render Hi-Res...", BS_PUSHBUTTON | WS_TABSTOP,
                               QuickControllerCommands::RenderHighRes, column2, top + rowStep * 2, buttonWidth, buttonHeight, font_);
    copyCoordinatesButton_ = Add(window_, instance_, WC_BUTTONW, L"Copy Coordinates", BS_PUSHBUTTON | WS_TABSTOP,
                                 QuickControllerCommands::CopyCoordinates, column3, top + rowStep * 2, buttonWidth, buttonHeight, font_);
    loadPresetButton_ = Add(window_, instance_, WC_BUTTONW, L"Load Preset...", BS_PUSHBUTTON | WS_TABSTOP,
                            QuickControllerCommands::LoadPreset, column4, top + rowStep * 2, buttonWidth, buttonHeight, font_);

    constexpr int wideButton = (buttonWidth * 4 + gap * 3 - gap) / 2;
    editButton_ = Add(window_, instance_, WC_BUTTONW, L"Open Editor", BS_PUSHBUTTON | WS_TABSTOP,
                      QuickControllerCommands::Edit, left, top + rowStep * 3, wideButton, buttonHeight, font_);
    exitButton_ = Add(window_, instance_, WC_BUTTONW, L"Exit App", BS_PUSHBUTTON | WS_TABSTOP,
                      QuickControllerCommands::ExitApp, left + wideButton + gap, top + rowStep * 3,
                      wideButton, buttonHeight, font_);

    SendMessageW(window_, DM_SETDEFID, QuickControllerCommands::ApplySettingsLive, 0);
    layout_.Initialise(window_, dpi_, font_, 590, 380);
    layout_.Focus(applyLiveButton_);
}

void QuickControllerWindow::Show() {
    if (!window_) return;
    ShowWindow(window_, SW_SHOWNORMAL);
    SetForegroundWindow(window_);
}

void QuickControllerWindow::Hide() {
    if (window_) ShowWindow(window_, SW_HIDE);
}

void QuickControllerWindow::Update(const std::wstring& status, const std::wstring& coordinates,
                                   const std::wstring& resources,
                                   bool previewZoomMotionEnabled, bool previewColourCyclingEnabled,
                                   bool desktopZoomMotionEnabled, bool desktopColourCyclingEnabled) {
    if (!window_) return;
    SetWindowTextW(statusLabel_, status.c_str());
    SetWindowTextW(coordinatesLabel_, coordinates.c_str());
    SetWindowTextW(resourcesLabel_, resources.c_str());
    SetWindowTextW(previewZoomButton_, previewZoomMotionEnabled ? L"Stop Preview Zoom" : L"Start Preview Zoom");
    SetWindowTextW(previewColourButton_, previewColourCyclingEnabled ? L"Stop Preview Colours" : L"Start Preview Colours");
    SetWindowTextW(desktopZoomButton_, desktopZoomMotionEnabled ? L"Stop Desktop Zoom" : L"Start Desktop Zoom");
    SetWindowTextW(desktopColourButton_, desktopColourCyclingEnabled ? L"Stop Desktop Colours" : L"Start Desktop Colours");
}

bool QuickControllerWindow::IsVisible() const noexcept {
    return window_ && IsWindowVisible(window_);
}

bool QuickControllerWindow::ProcessDialogMessage(MSG& message) {
    if (!window_ || !IsWindowVisible(window_)) return false;
    if (message.hwnd != window_ && !IsChild(window_, message.hwnd)) return false;
    if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) {
        Hide();
        return true;
    }
    return ProcessKeyboardDialogMessage(window_, message, &layout_);
}

void QuickControllerWindow::Destroy() {
    if (window_) DestroyWindow(window_);
    window_ = nullptr;
}

LRESULT CALLBACK QuickControllerWindow::Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<QuickControllerWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<QuickControllerWindow*>(create->lpCreateParams);
        self->window_ = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->HandleMessage(message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
}

LRESULT QuickControllerWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateControls();
        return 0;
    case WM_GETMINMAXINFO:
        layout_.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;
    case WM_SIZE:
        layout_.OnSize();
        return 0;
    case WM_VSCROLL:
    case WM_HSCROLL:
        if (lParam == 0 && layout_.OnScroll(message, wParam)) return 0;
        break;
    case WM_MOUSEWHEEL:
        if (layout_.OnMouseWheel(wParam)) return 0;
        break;
    case WM_DPICHANGED: {
        const UINT newDpi = HIWORD(wParam);
        HFONT newFont = CreateResponsiveDialogFont(newDpi);
        if (!newFont) newFont = font_;
        const RECT suggested = *reinterpret_cast<const RECT*>(lParam);
        layout_.OnDpiChanged(newDpi, suggested, newFont);
        if (newFont != font_ && font_) DeleteObject(font_);
        font_ = newFont;
        dpi_ = newDpi;
        return 0;
    }
    case WM_COMMAND:
        if (commandTarget_) PostMessageW(commandTarget_, WM_COMMAND, MAKEWPARAM(LOWORD(wParam), 0), 0);
        return 0;
    case WM_CLOSE:
        Hide();
        return 0;
    case WM_DESTROY:
        RememberDialogPlacement(window_, kQuickControllerClass, dpi_);
        layout_.Shutdown();
        if (font_) DeleteObject(font_);
        font_ = nullptr;
        window_ = nullptr;
        return 0;
    default:
        break;
    }
    return DefWindowProcW(window_, message, wParam, lParam);
}
#endif

} // namespace mw
