#include "App/AppWindow.h"

#include "App/resource.h"
#include "App/DialogSupport.h"
#include "App/PaletteEditorDialog.h"
#include "App/EquationEditorDialog.h"
#include "App/SettingsDialog.h"
#include "App/SlideshowDialog.h"
#include "App/PrecisionDialog.h"
#include "App/PresetManagerDialog.h"
#include "App/HighResRenderDialog.h"
#include "Core/DeepZoom.h"
#include "Infrastructure/Logger.h"
#include "Infrastructure/Paths.h"
#include "WindowsIntegration/DisplayManager.h"
#include "WindowsIntegration/StartupManager.h"

#ifdef _WIN32
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <windowsx.h>
#endif

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string_view>

namespace mw {

#ifdef _WIN32
namespace {
constexpr wchar_t kAppClass[] = L"MandelbrotLiveWallpaperControl";
constexpr wchar_t kPreviewClass[] = L"MandelbrotLiveWallpaperPreview";

HWND MakeControl(DWORD exStyle, const wchar_t* className, const wchar_t* text, DWORD style,
                 int id, HWND parent, HINSTANCE instance) {
    return CreateWindowExW(exStyle, className, text, AccessibleControlStyle(className, style) | WS_CHILD | WS_VISIBLE,
                           0, 0, 10, 10, parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
}

void AddComboItem(HWND combo, const std::wstring& text) {
    SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
}

void SetCheck(HWND control, bool checked) {
    SendMessageW(control, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool IsChecked(HWND control) {
    return SendMessageW(control, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

std::wstring OpenFileDialog(HWND owner, bool save) {
    wchar_t fileName[MAX_PATH]{};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"Mandelbrot preset (*.json)\0*.json\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    dialog.lpstrDefExt = L"json";
    const BOOL selected = save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    return selected ? std::wstring(fileName) : std::wstring{};
}

std::string Slugify(const std::string& name) {
    std::string slug;
    for (const unsigned char ch : name) {
        if ((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')) slug.push_back(static_cast<char>(ch));
        else if (ch >= 'A' && ch <= 'Z') slug.push_back(static_cast<char>(ch - 'A' + 'a'));
        else if (!slug.empty() && slug.back() != '-') slug.push_back('-');
    }
    while (!slug.empty() && slug.back() == '-') slug.pop_back();
    if (slug.empty()) slug = "custom-preset";
    return slug;
}

struct TextPromptState {
    HWND owner{nullptr};
    HWND window{nullptr};
    HWND edit{nullptr};
    HFONT font{nullptr};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    std::wstring instructions;
    std::wstring value;
    HINSTANCE instance{nullptr};
    bool accepted{false};
    bool done{false};
};

LRESULT CALLBACK TextPromptProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<TextPromptState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<TextPromptState*>(create->lpCreateParams);
        state->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(window, message, wParam, lParam);
    switch (message) {
    case WM_CREATE: {
        state->font = CreateResponsiveDialogFont(state->dpi);
        HWND label = CreateWindowExW(0, WC_STATICW, state->instructions.c_str(), WS_CHILD|WS_VISIBLE,
                                     14, 14, 452, 52, window, nullptr, state->instance, nullptr);
        state->edit = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, state->value.c_str(),
                                      WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,
                                      14, 72, 452, 26, window, reinterpret_cast<HMENU>(1), state->instance, nullptr);
        HWND ok = CreateWindowExW(0, WC_BUTTONW, L"OK", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,
                                  272, 112, 92, 30, window, reinterpret_cast<HMENU>(IDOK), state->instance, nullptr);
        HWND cancel = CreateWindowExW(0, WC_BUTTONW, L"Cancel", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_PUSHBUTTON,
                                      374, 112, 92, 30, window, reinterpret_cast<HMENU>(IDCANCEL), state->instance, nullptr);
        for (HWND child : {label, state->edit, ok, cancel}) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
        SendMessageW(window, DM_SETDEFID, IDOK, 0);
        state->layout.Initialise(window, state->dpi, state->font, 400, 170);
        state->layout.Focus(state->edit);
        return 0;
    }
    case WM_GETMINMAXINFO:
        state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;
    case WM_SIZE:
        state->layout.OnSize();
        return 0;
    case WM_VSCROLL:
    case WM_HSCROLL:
        if (lParam == 0 && state->layout.OnScroll(message, wParam)) return 0;
        break;
    case WM_MOUSEWHEEL:
        if (state->layout.OnMouseWheel(wParam)) return 0;
        break;
    case WM_DPICHANGED: {
        const UINT newDpi = HIWORD(wParam);
        HFONT newFont = CreateResponsiveDialogFont(newDpi);
        if (!newFont) newFont = state->font;
        const RECT suggested = *reinterpret_cast<const RECT*>(lParam);
        state->layout.OnDpiChanged(newDpi, suggested, newFont);
        if (newFont != state->font && state->font) DeleteObject(state->font);
        state->font = newFont;
        state->dpi = newDpi;
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            wchar_t buffer[1024]{};
            GetWindowTextW(state->edit, buffer, static_cast<int>(std::size(buffer)));
            state->value.assign(buffer);
            state->accepted = true;
            DestroyWindow(window);
            return 0;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            DestroyWindow(window);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        RememberDialogPlacement(window, L"MandelbrotTextPrompt", state->dpi);
        state->layout.Shutdown();
        if (state->font) DeleteObject(state->font);
        state->font = nullptr;
        state->done = true;
        EnableWindow(state->owner, TRUE);
        SetForegroundWindow(state->owner);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

bool PromptForText(HWND owner, HINSTANCE instance, const std::wstring& title, const std::wstring& instructions,
                   const std::wstring& initialValue, std::wstring& value) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = TextPromptProcedure;
        windowClass.hInstance = instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        windowClass.lpszClassName = L"MandelbrotTextPrompt";
        RegisterClassExW(&windowClass);
        registered = true;
    }
    TextPromptState state;
    state.owner = owner;
    state.instance = instance;
    state.instructions = instructions;
    state.value = initialValue;
    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 490, 185, state.dpi, L"MandelbrotTextPrompt");
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, L"MandelbrotTextPrompt", title.c_str(),
                                  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_POPUP | WS_VISIBLE |
                                  WS_VSCROLL | WS_HSCROLL,
                                  dialogRect.left, dialogRect.top,
                                  dialogRect.right - dialogRect.left, dialogRect.bottom - dialogRect.top,
                                  owner, nullptr, instance, &state);
    if (!window) return false;
    EnableWindow(owner, FALSE);
    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, IDCANCEL, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    if (state.accepted) value = state.value;
    return state.accepted;
}

bool ParseCoordinateTriplet(std::wstring text, double& x, double& y, double& scale) {
    for (wchar_t& ch : text) {
        if (ch == L';' || ch == L'|') ch = L',';
    }
    std::wstringstream stream(text);
    std::wstring part;
    double values[3]{};
    int count = 0;
    while (std::getline(stream, part, L',')) {
        if (part.find_first_not_of(L" \t\r\n") == std::wstring::npos) continue;
        try { values[count++] = std::stod(part); } catch (...) { return false; }
        if (count >= 3) break;
    }
    if (count != 3) return false;
    x = values[0]; y = values[1]; scale = values[2];
    return true;
}

bool CopyUnicodeText(HWND owner, const std::wstring& text) {
    if (!OpenClipboard(owner)) return false;
    EmptyClipboard();
    const std::size_t bytes = (text.size() + 1U) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory) {
        CloseClipboard();
        return false;
    }
    void* target = GlobalLock(memory);
    if (!target) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    memcpy(target, text.c_str(), bytes);
    GlobalUnlock(memory);
    if (!SetClipboardData(CF_UNICODETEXT, memory)) {
        GlobalFree(memory);
        CloseClipboard();
        return false;
    }
    CloseClipboard();
    return true; // Clipboard owns memory after SetClipboardData succeeds.
}

} // namespace

AppWindow::AppWindow()
    : settingsStore_(Paths::SettingsPath()), builtInPresets_(BuiltInPresets()) {}

AppWindow::~AppWindow() {
    if (uiFont_) DeleteObject(uiFont_);
    if (icon_) DestroyIcon(icon_);
}

bool AppWindow::Create(HINSTANCE instance, int showCommand, bool startHidden, std::string& error) {
    instance_ = instance;
    const auto loaded = settingsStore_.Load();
    settings_ = loaded.settings;
    if (settings_.staticWallpaper.storageDirectory.empty()) {
        settings_.staticWallpaper.storageDirectory = ToUtf8(Paths::StaticRenderDirectory().wstring());
    }
    if (!loaded.warning.empty()) LogWarning(loaded.warning);

    INITCOMMONCONTROLSEX common{};
    common.dwSize = sizeof(common);
    common.dwICC = ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&common);

    icon_ = static_cast<HICON>(LoadImageW(instance_, MAKEINTRESOURCEW(IDI_APP_ICON), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE));
    if (!icon_) icon_ = LoadIconW(nullptr, IDI_APPLICATION);
    if (!RegisterClasses(error)) return false;
    taskbarCreatedMessage_ = RegisterWindowMessageW(L"TaskbarCreated");

    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int availableWidth = std::max(900, static_cast<int>(workArea.right - workArea.left - 32));
    const int availableHeight = std::max(640, static_cast<int>(workArea.bottom - workArea.top - 32));
    const int initialWidth = std::min(1420, availableWidth);
    const int initialHeight = std::min(900, availableHeight);
    const int initialX = workArea.left + std::max(0, (static_cast<int>(workArea.right - workArea.left) - initialWidth) / 2);
    const int initialY = workArea.top + std::max(0, (static_cast<int>(workArea.bottom - workArea.top) - initialHeight) / 2);
    window_ = CreateWindowExW(0, kAppClass, L"Mandelbrot Live Wallpaper", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              initialX, initialY, initialWidth, initialHeight, nullptr, nullptr, instance_, this);
    if (!window_) {
        error = "The application window could not be created.";
        return false;
    }

    std::string sessionError;
    if (!systemStateMonitor_.Register(window_, sessionError)) LogWarning(sessionError);
    trayIcon_.Create(window_, icon_, L"Mandelbrot Live Wallpaper");
    SetTimer(window_, TimerId, 10, nullptr);
    lastFrameTime_ = std::chrono::steady_clock::now();
    lastSystemPoll_ = lastFrameTime_;
    lastAdaptiveSample_ = lastFrameTime_;

    if (!startHidden) {
        ShowWindow(window_, showCommand);
        UpdateWindow(window_);
    }
    if (settings_.general.startWallpaperOnLaunch || settings_.lastWallpaperRunning) {
        if (settings_.staticWallpaper.enabled && !settings_.staticWallpaper.imagePaths.empty()) StartSavedStaticWallpaper();
        else SetWallpaper();
    }
    return true;
}

int AppWindow::RunMessageLoop() {
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (quickController_.ProcessDialogMessage(message)) continue;
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

bool AppWindow::RegisterClasses(std::string& error) {
    WNDCLASSEXW mainClass{};
    mainClass.cbSize = sizeof(mainClass);
    mainClass.style = CS_HREDRAW | CS_VREDRAW;
    mainClass.lpfnWndProc = WindowProcedure;
    mainClass.hInstance = instance_;
    mainClass.hIcon = icon_;
    mainClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    mainClass.lpszClassName = kAppClass;
    mainClass.hIconSm = icon_;
    if (!RegisterClassExW(&mainClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        error = "The main window class could not be registered.";
        return false;
    }

    WNDCLASSEXW previewClass{};
    previewClass.cbSize = sizeof(previewClass);
    previewClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    previewClass.lpfnWndProc = PreviewProcedure;
    previewClass.hInstance = instance_;
    previewClass.hCursor = LoadCursorW(nullptr, IDC_CROSS);
    previewClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    previewClass.lpszClassName = kPreviewClass;
    if (!RegisterClassExW(&previewClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        error = "The preview window class could not be registered.";
        return false;
    }
    return true;
}

void AppWindow::CreateControls() {
    uiFont_ = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                          OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                          DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    previewWindow_ = CreateWindowExW(WS_EX_CLIENTEDGE, kPreviewClass, L"", WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                     0, 0, 100, 100, window_, nullptr, instance_, this);
    // The old horizontal tab bar is retained as a hidden compatibility control.
    // Navigation is now a compact vertical rail beside the preview.
    mainTab_ = MakeControl(0, L"STATIC", L"", 0, MainTab, window_, instance_);
    navigationPreviewButton_ = MakeControl(0, L"BUTTON", L"Preview\n◉", BS_MULTILINE | BS_PUSHBUTTON | WS_TABSTOP, NavigationPreviewButton, window_, instance_);
    navigationWallpaperButton_ = MakeControl(0, L"BUTTON", L"Desktop\n▣", BS_MULTILINE | BS_PUSHBUTTON | WS_TABSTOP, NavigationWallpaperButton, window_, instance_);
    navigationDiagnosticsButton_ = MakeControl(0, L"BUTTON", L"Status\nⓘ", BS_MULTILINE | BS_PUSHBUTTON | WS_TABSTOP, NavigationDiagnosticsButton, window_, instance_);
    navigationSettingsButton_ = MakeControl(0, L"BUTTON", L"Settings\n⚙", BS_MULTILINE | BS_PUSHBUTTON | WS_TABSTOP, NavigationSettingsButton, window_, instance_);
    navigationPaletteButton_ = MakeControl(0, L"BUTTON", L"Palette\n◈", BS_MULTILINE | BS_PUSHBUTTON | WS_TABSTOP, NavigationPaletteButton, window_, instance_);
    navigationControllerButton_ = MakeControl(0, L"BUTTON", L"Quick\n▶", BS_MULTILINE | BS_PUSHBUTTON | WS_TABSTOP, NavigationControllerButton, window_, instance_);
    navigationEquationButton_ = MakeControl(0, L"BUTTON", L"Equation\n∑", BS_MULTILINE | BS_PUSHBUTTON | WS_TABSTOP, NavigationEquationButton, window_, instance_);

    auto staticLabel = [&](const wchar_t* text, int id = 0) {
        return MakeControl(0, L"STATIC", text, SS_LEFT, id, window_, instance_);
    };
    previewContextLabel_ = staticLabel(L"PREVIEW CONTROLS", PreviewContextLabel);
    wallpaperContextLabel_ = staticLabel(L"DESKTOP WALLPAPER", WallpaperContextLabel);

    presetLibraryButton_ = MakeControl(0, L"BUTTON", L"Preset Library...", BS_DEFPUSHBUTTON | WS_TABSTOP, PresetLibraryButton, window_, instance_);
    presetCombo_ = MakeControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, PresetCombo, window_, instance_);
    staticLabel(L"Preset name");
    presetNameEdit_ = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, PresetNameEdit, window_, instance_);
    staticLabel(L"Centre X");
    centreXEdit_ = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, CentreXEdit, window_, instance_);
    staticLabel(L"Centre Y");
    centreYEdit_ = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, CentreYEdit, window_, instance_);
    staticLabel(L"View scale");
    scaleEdit_ = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, ScaleEdit, window_, instance_);
    staticLabel(L"Coordinates  X, Y, Scale");
    coordinatesEdit_ = MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | WS_TABSTOP, CoordinatesEdit, window_, instance_);
    staticLabel(L"Palette");
    paletteCombo_ = MakeControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, PaletteCombo, window_, instance_);
    MakeControl(0, L"BUTTON", L"Edit Palette...", BS_PUSHBUTTON | WS_TABSTOP, PaletteEditorButton, window_, instance_);
    staticLabel(L"Animation");
    animationCombo_ = MakeControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, AnimationCombo, window_, instance_);
    staticLabel(L"Equation");
    MakeControl(0, L"BUTTON", L"Edit Equation...", BS_PUSHBUTTON | WS_TABSTOP, EquationEditorButton, window_, instance_);

    auto makeNumericEdit = [&](int id) {
        return MakeControl(WS_EX_CLIENTEDGE, L"EDIT", L"", ES_AUTOHSCROLL | ES_CENTER | WS_TABSTOP, id, window_, instance_);
    };
    staticLabel(L"Iterations");
    iterationsTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, IterationsTrack, window_, instance_);
    iterationsEdit_ = makeNumericEdit(IterationsEdit);
    staticLabel(L"Frame-rate limit");
    fpsTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, FpsTrack, window_, instance_);
    fpsEdit_ = makeNumericEdit(FpsEdit);
    staticLabel(L"Render scale");
    renderScaleTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, RenderScaleTrack, window_, instance_);
    renderScaleEdit_ = makeNumericEdit(RenderScaleEdit);
    staticLabel(L"Zoom speed");
    zoomSpeedTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, ZoomSpeedTrack, window_, instance_);
    staticLabel(L"Colour speed");
    colourSpeedTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, ColourSpeedTrack, window_, instance_);
    colourCycleButton_ = MakeControl(0, L"BUTTON", L"Pause Colours", BS_PUSHBUTTON | WS_TABSTOP, ColourCycleButton, window_, instance_);

    staticLabel(L"Brightness");
    brightnessTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, BrightnessTrack, window_, instance_);
    brightnessEdit_ = makeNumericEdit(BrightnessEdit);
    staticLabel(L"Contrast");
    contrastTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, ContrastTrack, window_, instance_);
    contrastEdit_ = makeNumericEdit(ContrastEdit);
    staticLabel(L"Saturation");
    saturationTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, SaturationTrack, window_, instance_);
    saturationEdit_ = makeNumericEdit(SaturationEdit);
    staticLabel(L"Colour offset");
    colourOffsetTrack_ = MakeControl(0, TRACKBAR_CLASSW, L"", TBS_AUTOTICKS | WS_TABSTOP, ColourOffsetTrack, window_, instance_);
    colourOffsetEdit_ = makeNumericEdit(ColourOffsetEdit);

    staticLabel(L"Performance profile");
    performanceCombo_ = MakeControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, PerformanceCombo, window_, instance_);
    staticLabel(L"Monitor mode");
    monitorModeCombo_ = MakeControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, MonitorModeCombo, window_, instance_);
    staticLabel(L"Target monitor");
    monitorCombo_ = MakeControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, MonitorCombo, window_, instance_);
    staticLabel(L"Assigned preset");
    monitorAssignmentCombo_ = MakeControl(0, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, MonitorAssignmentCombo, window_, instance_);

    MakeControl(0, L"BUTTON", L"Apply Preview as Live Wallpaper", BS_DEFPUSHBUTTON | WS_TABSTOP, SetWallpaperButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Capture Preview as Static", BS_PUSHBUTTON | WS_TABSTOP, SetStaticWallpaperButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Add Preview to Slideshow", BS_PUSHBUTTON | WS_TABSTOP, AddSlideshowButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Manage Slideshow...", BS_PUSHBUTTON | WS_TABSTOP, ManageSlideshowButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Open Quick Controller", BS_PUSHBUTTON | WS_TABSTOP, OpenControllerButton, window_, instance_);
    pauseButton_ = MakeControl(0, L"BUTTON", L"Pause", BS_PUSHBUTTON | WS_TABSTOP, PauseButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Stop Wallpaper", BS_PUSHBUTTON | WS_TABSTOP, StopButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Reset View", BS_PUSHBUTTON | WS_TABSTOP, ResetViewButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Render Hi-Res and Save...", BS_PUSHBUTTON | WS_TABSTOP, RenderHighResButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Save as New", BS_PUSHBUTTON | WS_TABSTOP, SaveNewButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Save Changes", BS_PUSHBUTTON | WS_TABSTOP, SaveChangesButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Restore Built-ins", BS_PUSHBUTTON | WS_TABSTOP, RestoreBuiltInsButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"More Settings...", BS_PUSHBUTTON | WS_TABSTOP, OpenSettingsButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Configure Precision...", BS_PUSHBUTTON | WS_TABSTOP, ConfigurePrecisionButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Delete Custom", BS_PUSHBUTTON | WS_TABSTOP, DeletePresetButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Import", BS_PUSHBUTTON | WS_TABSTOP, ImportPresetButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Export", BS_PUSHBUTTON | WS_TABSTOP, ExportPresetButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Apply Assignment", BS_PUSHBUTTON | WS_TABSTOP, AssignMonitorButton, window_, instance_);

    MakeControl(0, L"BUTTON", L"Start with Windows", BS_AUTOCHECKBOX | WS_TABSTOP, StartupCheck, window_, instance_);
    MakeControl(0, L"BUTTON", L"Pause for full-screen apps", BS_AUTOCHECKBOX | WS_TABSTOP, FullscreenCheck, window_, instance_);
    MakeControl(0, L"BUTTON", L"Pause on battery", BS_AUTOCHECKBOX | WS_TABSTOP, BatteryCheck, window_, instance_);
    MakeControl(0, L"BUTTON", L"Pause in Remote Desktop", BS_AUTOCHECKBOX | WS_TABSTOP, RemoteCheck, window_, instance_);
    MakeControl(0, L"BUTTON", L"Reduced Motion", BS_AUTOCHECKBOX | WS_TABSTOP, ReducedMotionCheck, window_, instance_);

    statusLabel_ = MakeControl(0, L"STATIC", L"Status: stopped", SS_LEFT, StatusLabel, window_, instance_);
    fpsLabel_ = MakeControl(0, L"STATIC", L"FPS: 0", SS_LEFT, FpsLabel, window_, instance_);
    profileLabel_ = MakeControl(0, L"STATIC", L"", SS_LEFT, ProfileLabel, window_, instance_);
    monitorInfoLabel_ = MakeControl(0, L"STATIC", L"", SS_LEFT, MonitorInfoLabel, window_, instance_);
    precisionLabel_ = MakeControl(0, L"STATIC", L"Deep zoom precision: Automatic", SS_LEFT, PrecisionLabel, window_, instance_);

    MakeControl(0, L"BUTTON", L"Open Log Folder", BS_PUSHBUTTON | WS_TABSTOP, OpenLogsButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Copy Diagnostics", BS_PUSHBUTTON | WS_TABSTOP, CopyDiagnosticsButton, window_, instance_);
    MakeControl(0, L"BUTTON", L"Clear Logs", BS_PUSHBUTTON | WS_TABSTOP, ClearLogsButton, window_, instance_);

    for (HWND child = GetWindow(window_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont_), TRUE);
    }

    SendMessageW(iterationsTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(32, 4096));
    SendMessageW(fpsTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(5, 240));
    SendMessageW(renderScaleTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(25, 100));
    SendMessageW(zoomSpeedTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 200));
    SendMessageW(colourSpeedTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 25));
    SendMessageW(brightnessTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(10, 250));
    SendMessageW(contrastTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(10, 300));
    SendMessageW(saturationTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 200));
    SendMessageW(colourOffsetTrack_, TBM_SETRANGE, TRUE, MAKELPARAM(0, 200));

    std::string rendererError;
    if (!previewRenderer_.Initialise(previewWindow_, rendererError)) {
        previewRendererError_ = rendererError;
        ShowRendererError(rendererError);
        InvalidateRect(previewWindow_, nullptr, TRUE);
    } else {
        previewRendererError_.clear();
    }
    PopulateControls();
    ShowSelectedTab();
}

void AppWindow::LayoutControls(int width, int height) {
    const int margin = 10;
    const int gap = 8;
    const int navigationWidth = 76;
    const int panelWidth = std::clamp(width / 3, 360, 470);
    const int previewWidth = std::max(360, width - panelWidth - navigationWidth - margin * 2 - gap * 2);
    const int contentHeight = std::max(520, height - margin * 2);

    MoveWindow(previewWindow_, margin, margin, previewWidth, contentHeight, TRUE);
    const int navigationX = margin + previewWidth + gap;
    const int panelX = navigationX + navigationWidth + gap;
    MoveWindow(mainTab_, 0, 0, 0, 0, FALSE);
    MoveWindow(navigationPreviewButton_, navigationX, margin, navigationWidth, 66, TRUE);
    MoveWindow(navigationWallpaperButton_, navigationX, margin + 72, navigationWidth, 66, TRUE);
    MoveWindow(navigationDiagnosticsButton_, navigationX, margin + 144, navigationWidth, 66, TRUE);
    MoveWindow(navigationSettingsButton_, navigationX, margin + 216, navigationWidth, 66, TRUE);
    MoveWindow(navigationPaletteButton_, navigationX, margin + 288, navigationWidth, 66, TRUE);
    MoveWindow(navigationControllerButton_, navigationX, margin + 360, navigationWidth, 66, TRUE);
    MoveWindow(navigationEquationButton_, navigationX, margin + 432, navigationWidth, 66, TRUE);

    RECT page{panelX, margin, panelX + panelWidth, margin + contentHeight};
    const int x = page.left;
    int y = page.top;
    const int full = std::max(280, static_cast<int>(page.right - page.left));
    const int labelWidth = 122;
    const int controlX = x + labelWidth + 8;
    const int controlWidth = full - labelWidth - 8;
    const int editWidth = 68;
    const bool compact = contentHeight < 760;
    const int rowHeight = compact ? 23 : 26;
    const int rowGap = compact ? 4 : 7;
    const int sliderHeight = compact ? 24 : 28;

    auto findPrevStatic = [&](HWND control) -> HWND {
        HWND current = GetWindow(control, GW_HWNDPREV);
        while (current && current != previewWindow_) {
            wchar_t cls[32]{};
            GetClassNameW(current, cls, 32);
            if (wcscmp(cls, L"Static") == 0) return current;
            current = GetWindow(current, GW_HWNDPREV);
        }
        return nullptr;
    };
    auto pairVisible = [&](HWND control, bool visible) {
        ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
        if (HWND label = findPrevStatic(control)) ShowWindow(label, visible ? SW_SHOW : SW_HIDE);
    };
    auto showId = [&](int id, bool visible) {
        if (HWND control = GetDlgItem(window_, id)) ShowWindow(control, visible ? SW_SHOW : SW_HIDE);
    };

    for (HWND control : {presetCombo_, presetNameEdit_, centreXEdit_, centreYEdit_, scaleEdit_, coordinatesEdit_, paletteCombo_,
                         animationCombo_, iterationsTrack_, fpsTrack_, renderScaleTrack_, zoomSpeedTrack_,
                         colourSpeedTrack_, brightnessTrack_, contrastTrack_, saturationTrack_, colourOffsetTrack_,
                         performanceCombo_, monitorModeCombo_, monitorCombo_, monitorAssignmentCombo_}) {
        pairVisible(control, false);
    }
    for (int id : {PreviewContextLabel, WallpaperContextLabel, PresetLibraryButton,
                   PaletteEditorButton, EquationEditorButton, IterationsEdit, FpsEdit, RenderScaleEdit,
                   ColourCycleButton, BrightnessEdit, ContrastEdit, SaturationEdit, ColourOffsetEdit,
                   SetWallpaperButton, SetStaticWallpaperButton, AddSlideshowButton, ManageSlideshowButton,
                   PauseButton, StopButton, ResetViewButton, SaveNewButton, SaveChangesButton,
                   RestoreBuiltInsButton, OpenSettingsButton, ConfigurePrecisionButton, DeletePresetButton,
                   ImportPresetButton, ExportPresetButton, AssignMonitorButton, StartupCheck, FullscreenCheck,
                   BatteryCheck, RemoteCheck, ReducedMotionCheck, StatusLabel, FpsLabel, ProfileLabel,
                   MonitorInfoLabel, PrecisionLabel, OpenLogsButton, CopyDiagnosticsButton, ClearLogsButton,
                   OpenControllerButton, RenderHighResButton}) {
        showId(id, false);
    }

    auto heading = [&](HWND control) {
        ShowWindow(control, SW_SHOW);
        MoveWindow(control, x, y, full, 26, TRUE);
        y += 34;
    };
    auto pair = [&](HWND control, int h = 0) {
        if (h <= 0) h = rowHeight;
        pairVisible(control, true);
        if (HWND label = findPrevStatic(control)) MoveWindow(label, x, y + 4, labelWidth, 20, TRUE);
        wchar_t className[32]{};
        GetClassNameW(control, className, static_cast<int>(std::size(className)));
        const int windowHeight = wcscmp(className, WC_COMBOBOXW) == 0 ? h + 150 : h;
        MoveWindow(control, controlX, y, controlWidth, windowHeight, TRUE);
        y += h + rowGap;
    };
    auto sliderEdit = [&](HWND track, HWND edit, int h = 0) {
        if (h <= 0) h = sliderHeight;
        pairVisible(track, true);
        ShowWindow(edit, SW_SHOW);
        if (HWND label = findPrevStatic(track)) MoveWindow(label, x, y + 4, labelWidth, 20, TRUE);
        MoveWindow(track, controlX, y, std::max(90, controlWidth - editWidth - 6), h, TRUE);
        MoveWindow(edit, controlX + controlWidth - editWidth, y + 1, editWidth, 24, TRUE);
        y += h + rowGap;
    };
    auto button = [&](int id, int bx, int by, int bw, int bh = 30) {
        if (HWND control = GetDlgItem(window_, id)) {
            ShowWindow(control, SW_SHOW);
            MoveWindow(control, bx, by, bw, bh, TRUE);
        }
    };

    if (selectedTab_ == 0) {
        heading(previewContextLabel_);
        button(PresetLibraryButton, x, y, full, 32);
        y += 40;
        pair(coordinatesEdit_);

        pair(paletteCombo_);
        pair(animationCombo_);
        sliderEdit(iterationsTrack_, iterationsEdit_);
        pair(zoomSpeedTrack_, sliderHeight);

        const int colourY = y;
        pair(colourSpeedTrack_, sliderHeight);
        const int cycleWidth = 108;
        MoveWindow(colourSpeedTrack_, controlX, colourY, controlWidth - cycleWidth - 6, sliderHeight, TRUE);
        button(ColourCycleButton, controlX + controlWidth - cycleWidth, colourY, cycleWidth, sliderHeight);
        sliderEdit(brightnessTrack_, brightnessEdit_);
        sliderEdit(contrastTrack_, contrastEdit_);
        sliderEdit(saturationTrack_, saturationEdit_);
        sliderEdit(colourOffsetTrack_, colourOffsetEdit_);

        const int baseY = std::min(y + 8, static_cast<int>(page.bottom - 76));
        button(ResetViewButton, x, baseY, full, 32);
        button(RenderHighResButton, x, baseY + 38, full, 32);
    } else if (selectedTab_ == 1) {
        heading(wallpaperContextLabel_);
        for (HWND label : {statusLabel_, fpsLabel_, monitorInfoLabel_}) ShowWindow(label, SW_SHOW);
        MoveWindow(statusLabel_, x, y, full, 42, TRUE); y += 48;
        MoveWindow(fpsLabel_, x, y, full, 42, TRUE); y += 48;
        MoveWindow(monitorInfoLabel_, x, y, full, 72, TRUE); y += 80;
        button(ManageSlideshowButton, x, y, full, 34); y += 42;
        ShowWindow(wallpaperContextLabel_, SW_SHOW);
        SetControlText(wallpaperContextLabel_,
            "Desktop actions are available from the preview hover menu and Quick Controller. Graphics, monitor and pause behaviour are configured in Settings.");
        MoveWindow(wallpaperContextLabel_, x, page.bottom - 82, full, 72, TRUE);
    } else {
        for (HWND label : {statusLabel_, fpsLabel_, profileLabel_, monitorInfoLabel_, precisionLabel_}) ShowWindow(label, SW_SHOW);
        MoveWindow(statusLabel_, x, y, full, 36, TRUE); y += 42;
        MoveWindow(fpsLabel_, x, y, full, 42, TRUE); y += 48;
        MoveWindow(profileLabel_, x, y, full, 52, TRUE); y += 58;
        MoveWindow(monitorInfoLabel_, x, y, full, 72, TRUE); y += 78;
        MoveWindow(precisionLabel_, x, y, full, 92, TRUE); y += 100;
        const int bw = (full - 12) / 3;
        button(OpenLogsButton, x, y, bw);
        button(CopyDiagnosticsButton, x + bw + 6, y, bw);
        button(ClearLogsButton, x + (bw + 6) * 2, y, bw);
        y += 40;
    }


    SetWindowTextW(navigationPreviewButton_, selectedTab_ == 0 ? L"PREVIEW\n◉" : L"Preview\n◉");
    SetWindowTextW(navigationWallpaperButton_, selectedTab_ == 1 ? L"DESKTOP\n▣" : L"Desktop\n▣");
    SetWindowTextW(navigationDiagnosticsButton_, selectedTab_ == 2 ? L"STATUS\nⓘ" : L"Status\nⓘ");
    SendMessageW(navigationPreviewButton_, BM_SETSTATE, selectedTab_ == 0 ? TRUE : FALSE, 0);
    SendMessageW(navigationWallpaperButton_, BM_SETSTATE, selectedTab_ == 1 ? TRUE : FALSE, 0);
    SendMessageW(navigationDiagnosticsButton_, BM_SETSTATE, selectedTab_ == 2 ? TRUE : FALSE, 0);
}

void AppWindow::PopulateControls() {
    PopulatePresetCombo();
    for (const wchar_t* item : {L"Classic Spectrum", L"Deep Ocean", L"Fire", L"Purple Neon", L"Green Matrix", L"Gold", L"Ice", L"Greyscale", L"Pastel", L"High Contrast"}) AddComboItem(paletteCombo_, item);
    for (const wchar_t* item : {L"Automatic Journey", L"Continuous Zoom", L"Static Animated Colour", L"Manual View"}) AddComboItem(animationCombo_, item);
    for (const wchar_t* item : {L"Battery Saver", L"Balanced", L"High Quality", L"Custom"}) AddComboItem(performanceCombo_, item);
    for (const wchar_t* item : {L"Mirror", L"Span", L"Independent"}) AddComboItem(monitorModeCombo_, item);
    SendMessageW(performanceCombo_, CB_SETCURSEL, static_cast<WPARAM>(settings_.performance.profile), 0);
    SendMessageW(monitorModeCombo_, CB_SETCURSEL, static_cast<WPARAM>(settings_.monitorMode), 0);
    settings_.general.startWithWindows = StartupManager::IsEnabled();
    SetCheck(GetDlgItem(window_, StartupCheck), settings_.general.startWithWindows);
    SetCheck(GetDlgItem(window_, FullscreenCheck), settings_.performance.pauseWhenFullscreen);
    SetCheck(GetDlgItem(window_, BatteryCheck), settings_.performance.pauseOnBattery);
    SetCheck(GetDlgItem(window_, RemoteCheck), settings_.performance.pauseDuringRemoteDesktop);
    SetCheck(GetDlgItem(window_, ReducedMotionCheck), settings_.general.reducedMotion);
    PopulateMonitorControls();
    LoadSelectedPreset();
}

void AppWindow::PopulatePresetCombo() {
    SendMessageW(presetCombo_, CB_RESETCONTENT, 0, 0);
    const auto presets = AllPresets();
    int selected = 0;
    for (std::size_t i = 0; i < presets.size(); ++i) {
        AddComboItem(presetCombo_, ToWide(presets[i].name));
        if (presets[i].id == settings_.selectedPresetId) selected = static_cast<int>(i);
    }
    SendMessageW(presetCombo_, CB_SETCURSEL, selected, 0);

    SendMessageW(monitorAssignmentCombo_, CB_RESETCONTENT, 0, 0);
    for (const auto& preset : presets) AddComboItem(monitorAssignmentCombo_, ToWide(preset.name));
    SendMessageW(monitorAssignmentCombo_, CB_SETCURSEL, selected, 0);
}

void AppWindow::PopulateMonitorControls() {
    const int previousSelection = SelectedComboIndex(monitorCombo_);
    SendMessageW(monitorCombo_, CB_RESETCONTENT, 0, 0);
    const auto displays = DisplayManager::Enumerate();
    for (const auto& display : displays) {
        std::wostringstream text;
        text << display.friendlyName << L" (" << (display.bounds.right - display.bounds.left) << L"x" << (display.bounds.bottom - display.bounds.top)
             << L", " << display.dpiX << L" DPI" << (display.primary ? L", primary" : L"") << L")";
        AddComboItem(monitorCombo_, text.str());
    }
    if (!displays.empty()) {
        SendMessageW(monitorCombo_, CB_SETCURSEL,
                     std::clamp(previousSelection, 0, static_cast<int>(displays.size()) - 1), 0);
    }
    std::ostringstream info;
    info << "Detected monitors: " << displays.size() << ". Mode: " << ToString(settings_.monitorMode)
         << ". Span uses one continuous virtual-desktop canvas; Independent uses the assignments below.";
    SetControlText(monitorInfoLabel_, info.str());
    LoadMonitorAssignmentSelection();
    UpdateMonitorAssignmentControls();
}

void AppWindow::ShowSelectedTab() {
    RECT client{};
    GetClientRect(window_, &client);
    LayoutControls(client.right - client.left, client.bottom - client.top);
}

void AppWindow::SelectPage(int page) {
    selectedTab_ = std::clamp(page, 0, 2);
    ShowSelectedTab();
}

void AppWindow::UpdateMonitorAssignmentControls() {
    const bool independent = settings_.monitorMode == MonitorMode::Independent;
    EnableWindow(monitorCombo_, independent);
    EnableWindow(monitorAssignmentCombo_, independent);
    EnableWindow(GetDlgItem(window_, AssignMonitorButton), independent);
}

void AppWindow::LoadMonitorAssignmentSelection() {
    const auto displays = DisplayManager::Enumerate();
    const auto presets = AllPresets();
    const int monitorIndex = SelectedComboIndex(monitorCombo_);
    if (monitorIndex < 0 || monitorIndex >= static_cast<int>(displays.size())) return;
    const std::string key = ToUtf8(displays[static_cast<std::size_t>(monitorIndex)].deviceName);
    std::string presetId = settings_.selectedPresetId;
    const auto assignment = settings_.monitorPresetAssignments.find(key);
    if (assignment != settings_.monitorPresetAssignments.end()) presetId = assignment->second;
    int selection = 0;
    for (std::size_t i = 0; i < presets.size(); ++i) {
        if (presets[i].id == presetId) {
            selection = static_cast<int>(i);
            break;
        }
    }
    SendMessageW(monitorAssignmentCombo_, CB_SETCURSEL, selection, 0);
}

void AppWindow::SyncNumericEditsFromTracks() {
    SetControlText(iterationsEdit_, std::to_string(static_cast<int>(SendMessageW(iterationsTrack_, TBM_GETPOS, 0, 0))));
    SetControlText(fpsEdit_, std::to_string(static_cast<int>(SendMessageW(fpsTrack_, TBM_GETPOS, 0, 0))));
    SetControlText(renderScaleEdit_, std::to_string(static_cast<int>(SendMessageW(renderScaleTrack_, TBM_GETPOS, 0, 0))) + "%");
    SetControlText(brightnessEdit_, FormatDouble(static_cast<double>(SendMessageW(brightnessTrack_, TBM_GETPOS, 0, 0)) / 100.0, 2));
    SetControlText(contrastEdit_, FormatDouble(static_cast<double>(SendMessageW(contrastTrack_, TBM_GETPOS, 0, 0)) / 100.0, 2));
    SetControlText(saturationEdit_, FormatDouble(static_cast<double>(SendMessageW(saturationTrack_, TBM_GETPOS, 0, 0)) / 100.0, 2));
    SetControlText(colourOffsetEdit_, FormatDouble((static_cast<double>(SendMessageW(colourOffsetTrack_, TBM_GETPOS, 0, 0)) - 100.0) / 100.0, 2));
}

void AppWindow::SyncTrackFromNumericEdit(int controlId) {
    auto parse = [&](HWND edit, double fallback) {
        std::string text = ReadControlText(edit);
        text.erase(std::remove(text.begin(), text.end(), '%'), text.end());
        try { return std::stod(text); } catch (...) { return fallback; }
    };
    if (controlId == IterationsEdit) {
        const int value = std::clamp(static_cast<int>(std::lround(parse(iterationsEdit_, workingPreset_.maximumIterations))), 32, 4096);
        SendMessageW(iterationsTrack_, TBM_SETPOS, TRUE, value);
    } else if (controlId == FpsEdit) {
        const int value = std::clamp(static_cast<int>(std::lround(parse(fpsEdit_, settings_.performance.maximumFrameRate))), 5, 240);
        SendMessageW(fpsTrack_, TBM_SETPOS, TRUE, value);
    } else if (controlId == RenderScaleEdit) {
        const int value = std::clamp(static_cast<int>(std::lround(parse(renderScaleEdit_, settings_.performance.renderScale * 100.0))), 25, 100);
        SendMessageW(renderScaleTrack_, TBM_SETPOS, TRUE, value);
    } else if (controlId == BrightnessEdit) {
        SendMessageW(brightnessTrack_, TBM_SETPOS, TRUE, std::clamp(static_cast<int>(std::lround(parse(brightnessEdit_, workingPreset_.brightness) * 100.0)), 10, 250));
    } else if (controlId == ContrastEdit) {
        SendMessageW(contrastTrack_, TBM_SETPOS, TRUE, std::clamp(static_cast<int>(std::lround(parse(contrastEdit_, workingPreset_.contrast) * 100.0)), 10, 300));
    } else if (controlId == SaturationEdit) {
        SendMessageW(saturationTrack_, TBM_SETPOS, TRUE, std::clamp(static_cast<int>(std::lround(parse(saturationEdit_, workingPreset_.saturation) * 100.0)), 0, 200));
    } else if (controlId == ColourOffsetEdit) {
        SendMessageW(colourOffsetTrack_, TBM_SETPOS, TRUE, std::clamp(static_cast<int>(std::lround((parse(colourOffsetEdit_, workingPreset_.colourOffset) + 1.0) * 100.0)), 0, 200));
    }
    SyncNumericEditsFromTracks();
}

std::vector<Preset> AppWindow::AllPresets() const {
    auto result = builtInPresets_;
    result.insert(result.end(), settings_.customPresets.begin(), settings_.customPresets.end());
    if (!workingPreset_.id.empty()) {
        const auto current = std::find_if(result.begin(), result.end(), [&](const Preset& preset) { return preset.id == workingPreset_.id; });
        if (current != result.end()) *current = workingPreset_;
    }
    return result;
}

const Preset* AppWindow::FindPreset(const std::string& id) const {
    const auto builtIn = std::find_if(builtInPresets_.begin(), builtInPresets_.end(), [&](const Preset& p) { return p.id == id; });
    if (builtIn != builtInPresets_.end()) return &*builtIn;
    const auto custom = std::find_if(settings_.customPresets.begin(), settings_.customPresets.end(), [&](const Preset& p) { return p.id == id; });
    return custom == settings_.customPresets.end() ? nullptr : &*custom;
}

Preset* AppWindow::FindPresetMutable(const std::string& id) {
    const auto custom = std::find_if(settings_.customPresets.begin(), settings_.customPresets.end(), [&](const Preset& p) { return p.id == id; });
    return custom == settings_.customPresets.end() ? nullptr : &*custom;
}

void AppWindow::UpdateCoordinatesEdit() {
    if (!coordinatesEdit_) return;
    SetControlText(coordinatesEdit_,
        FormatDouble(CameraCentreX(workingPreset_.camera)) + ", " +
        FormatDouble(CameraCentreY(workingPreset_.camera)) + ", " +
        FormatDouble(workingPreset_.camera.scale, 12));
}

bool AppWindow::ApplyCoordinatesEdit(bool showError) {
    if (!coordinatesEdit_) return true;
    const std::wstring value = ToWide(ReadControlText(coordinatesEdit_));
    double x = 0.0;
    double y = 0.0;
    double scale = 0.0;
    if (!ParseCoordinateTriplet(value, x, y, scale) || !std::isfinite(x) || !std::isfinite(y) ||
        !std::isfinite(scale) || scale <= 0.0) {
        if (showError) {
            MessageBoxW(window_, L"Use the format centreX, centreY, scale with a positive scale.",
                        L"Coordinates", MB_OK | MB_ICONERROR);
        }
        UpdateCoordinatesEdit();
        return false;
    }
    workingPreset_.camera = {x, y, std::clamp(scale, 1.0e-32, 4.0)};
    SetControlText(centreXEdit_, FormatDouble(x));
    SetControlText(centreYEdit_, FormatDouble(y));
    SetControlText(scaleEdit_, FormatDouble(workingPreset_.camera.scale));
    return true;
}

void AppWindow::OpenPresetLibrary(bool applyLoadedPresetToDesktop) {
    const auto presets = AllPresets();
    int selectedIndex = std::max(0, SelectedComboIndex(presetCombo_));
    std::string saveAsName = workingPreset_.name.empty() ? std::string("Custom Preset") : workingPreset_.name;
    if (const Preset* selected = FindPreset(workingPreset_.id); selected && selected->name == saveAsName) {
        saveAsName += " Copy";
    }
    const PresetManagerAction action = PresetManagerDialog::Show(
        window_, instance_, presets, selectedIndex, selectedIndex, saveAsName);
    if (action == PresetManagerAction::Cancelled) return;
    selectedIndex = std::clamp(selectedIndex, 0, std::max(0, static_cast<int>(presets.size()) - 1));

    if (action == PresetManagerAction::Load) {
        SendMessageW(presetCombo_, CB_SETCURSEL, selectedIndex, 0);
        LoadSelectedPreset();
        if (applyLoadedPresetToDesktop) SetWallpaper();
    } else if (action == PresetManagerAction::SaveNew) {
        SaveAsNewPreset(saveAsName);
    } else if (action == PresetManagerAction::Update) {
        if (selectedIndex >= static_cast<int>(presets.size()) || presets[static_cast<std::size_t>(selectedIndex)].builtIn) {
            MessageBoxW(window_, L"Built-in presets are read-only. Save the preview as a new preset instead.",
                        L"Preset Library", MB_OK | MB_ICONINFORMATION);
            return;
        }
        ApplyControlsToWorkingPreset();
        Preset updated = workingPreset_;
        updated.id = presets[static_cast<std::size_t>(selectedIndex)].id;
        updated.name = presets[static_cast<std::size_t>(selectedIndex)].name;
        updated.builtIn = false;
        if (Preset* stored = FindPresetMutable(updated.id)) *stored = updated;
        settings_.selectedPresetId = updated.id;
        PopulatePresetCombo();
        LoadSelectedPreset();
        SaveSettings();
    } else if (action == PresetManagerAction::Delete) {
        SendMessageW(presetCombo_, CB_SETCURSEL, selectedIndex, 0);
        LoadSelectedPreset();
        DeleteSelectedPreset();
    } else if (action == PresetManagerAction::RestoreBuiltIns) {
        RestoreBuiltInPresets();
    } else if (action == PresetManagerAction::Import) {
        ImportPreset();
    } else if (action == PresetManagerAction::Export) {
        SendMessageW(presetCombo_, CB_SETCURSEL, selectedIndex, 0);
        LoadSelectedPreset();
        ExportPreset();
    }
}

void AppWindow::LoadSelectedPreset() {
    const auto presets = AllPresets();
    const int index = SelectedComboIndex(presetCombo_);
    if (index < 0 || index >= static_cast<int>(presets.size())) return;
    workingPreset_ = presets[static_cast<std::size_t>(index)];
    settings_.selectedPresetId = workingPreset_.id;
    SetControlText(presetNameEdit_, workingPreset_.name);
    SetControlText(centreXEdit_, FormatDouble(CameraCentreX(workingPreset_.camera)));
    SetControlText(centreYEdit_, FormatDouble(CameraCentreY(workingPreset_.camera)));
    SetControlText(scaleEdit_, FormatDouble(workingPreset_.camera.scale));
    UpdateCoordinatesEdit();
    SetWindowTextW(presetLibraryButton_, ToWide("Preset: " + workingPreset_.name + "...").c_str());
    SendMessageW(paletteCombo_, CB_SETCURSEL, static_cast<WPARAM>(workingPreset_.palette), 0);
    SendMessageW(animationCombo_, CB_SETCURSEL, static_cast<WPARAM>(workingPreset_.animationMode), 0);
    SendMessageW(iterationsTrack_, TBM_SETPOS, TRUE, workingPreset_.maximumIterations);
    SendMessageW(fpsTrack_, TBM_SETPOS, TRUE, settings_.performance.maximumFrameRate);
    SendMessageW(renderScaleTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(settings_.performance.renderScale * 100.0)));
    SendMessageW(zoomSpeedTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(workingPreset_.zoomSpeed * 100.0)));
    SendMessageW(colourSpeedTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(std::abs(workingPreset_.colourCycleSpeed) * 100.0)));
    SendMessageW(brightnessTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(workingPreset_.brightness * 100.0)));
    SendMessageW(contrastTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(workingPreset_.contrast * 100.0)));
    SendMessageW(saturationTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(workingPreset_.saturation * 100.0)));
    SendMessageW(colourOffsetTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround((workingPreset_.colourOffset + 1.0) * 100.0)));
    SyncNumericEditsFromTracks();
    previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
    previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    lastPreviewFrame_ = {previewAnimation_.Camera(), workingPreset_.colourOffset};
    const std::wstring paletteButtonText = workingPreset_.customPaletteColours.empty()
        ? L"Edit Palette..."
        : L"Custom (" + std::to_wstring(workingPreset_.customPaletteColours.size()) + L")";
    SetWindowTextW(GetDlgItem(window_, PaletteEditorButton), paletteButtonText.c_str());
    SetWindowTextW(GetDlgItem(window_, EquationEditorButton), ToWide(EquationSummary(workingPreset_.equation)).c_str());
    previewChangesPending_ = wallpaperController_.IsRunning();
    LoadMonitorAssignmentSelection();
    UpdateStatus();
}

void AppWindow::ApplyControlsToWorkingPreset() {
    ApplyCoordinatesEdit(false);
    workingPreset_.name = ReadControlText(presetNameEdit_);
    try { workingPreset_.camera.centreX = std::stod(ReadControlText(centreXEdit_)); workingPreset_.camera.centreXLow = 0.0; } catch (...) {}
    try { workingPreset_.camera.centreY = std::stod(ReadControlText(centreYEdit_)); workingPreset_.camera.centreYLow = 0.0; } catch (...) {}
    try { workingPreset_.camera.scale = std::stod(ReadControlText(scaleEdit_)); } catch (...) {}
    workingPreset_.startingScale = workingPreset_.camera.scale;
    const int palette = SelectedComboIndex(paletteCombo_);
    const int animation = SelectedComboIndex(animationCombo_);
    if (palette >= 0 && palette <= static_cast<int>(Palette::HighContrast)) {
        const auto selectedPalette = static_cast<Palette>(palette);
        if (selectedPalette != workingPreset_.palette) workingPreset_.customPaletteColours.clear();
        workingPreset_.palette = selectedPalette;
    }
    if (animation >= 0 && animation <= static_cast<int>(AnimationMode::ManualView)) workingPreset_.animationMode = static_cast<AnimationMode>(animation);
    workingPreset_.maximumIterations = static_cast<int>(SendMessageW(iterationsTrack_, TBM_GETPOS, 0, 0));
    workingPreset_.zoomSpeed = static_cast<double>(SendMessageW(zoomSpeedTrack_, TBM_GETPOS, 0, 0)) / 100.0;
    workingPreset_.colourCycleSpeed = static_cast<double>(SendMessageW(colourSpeedTrack_, TBM_GETPOS, 0, 0)) / 100.0;
    workingPreset_.brightness = static_cast<double>(SendMessageW(brightnessTrack_, TBM_GETPOS, 0, 0)) / 100.0;
    workingPreset_.contrast = static_cast<double>(SendMessageW(contrastTrack_, TBM_GETPOS, 0, 0)) / 100.0;
    workingPreset_.saturation = static_cast<double>(SendMessageW(saturationTrack_, TBM_GETPOS, 0, 0)) / 100.0;
    workingPreset_.colourOffset = (static_cast<double>(SendMessageW(colourOffsetTrack_, TBM_GETPOS, 0, 0)) - 100.0) / 100.0;
    settings_.performance.maximumFrameRate = static_cast<int>(SendMessageW(fpsTrack_, TBM_GETPOS, 0, 0));
    settings_.performance.renderScale = static_cast<double>(SendMessageW(renderScaleTrack_, TBM_GETPOS, 0, 0)) / 100.0;
    settings_.performance.maximumIterations = workingPreset_.maximumIterations;
    settings_.performance.profile = static_cast<PerformanceProfile>(std::max(0, SelectedComboIndex(performanceCombo_)));
    settings_.monitorMode = static_cast<MonitorMode>(std::max(0, SelectedComboIndex(monitorModeCombo_)));
    settings_.performance.pauseWhenFullscreen = IsChecked(GetDlgItem(window_, FullscreenCheck));
    settings_.performance.pauseOnBattery = IsChecked(GetDlgItem(window_, BatteryCheck));
    settings_.performance.pauseDuringRemoteDesktop = IsChecked(GetDlgItem(window_, RemoteCheck));
    settings_.general.reducedMotion = IsChecked(GetDlgItem(window_, ReducedMotionCheck));
    ValidateAndNormalise(workingPreset_);
    ValidateAndNormalise(settings_);
    previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
    previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    SyncNumericEditsFromTracks();
    UpdateCoordinatesEdit();
    previewChangesPending_ = true;
    UpdateMonitorAssignmentControls();
}

void AppWindow::ApplyPerformanceProfile() {
    const int index = SelectedComboIndex(performanceCombo_);
    if (index < 0 || index > static_cast<int>(PerformanceProfile::Custom)) return;
    const auto profile = static_cast<PerformanceProfile>(index);
    if (profile != PerformanceProfile::Custom) {
        auto defaults = SettingsForProfile(profile);
        defaults.precision = settings_.performance.precision;
        defaults.adaptive = settings_.performance.adaptive;
        settings_.performance = defaults;
        SendMessageW(fpsTrack_, TBM_SETPOS, TRUE, defaults.maximumFrameRate);
        SendMessageW(renderScaleTrack_, TBM_SETPOS, TRUE, static_cast<LPARAM>(std::lround(defaults.renderScale * 100.0)));
        SendMessageW(iterationsTrack_, TBM_SETPOS, TRUE, defaults.maximumIterations);
        SetCheck(GetDlgItem(window_, BatteryCheck), defaults.pauseOnBattery);
        SyncNumericEditsFromTracks();
    }
    ApplyControlsToWorkingPreset();
}

bool AppWindow::SaveSettings(std::string* errorOut) {
    settings_.lastWallpaperRunning = wallpaperController_.IsRunning();
    if (wallpaperController_.UsingUserStatic()) {
        settings_.staticWallpaper.currentIndex = wallpaperController_.CurrentStaticImageIndex();
    }
    std::string error;
    if (!settingsStore_.Save(settings_, error)) {
        LogError("Settings save failed: " + error);
        if (errorOut) *errorOut = error;
        return false;
    }
    if (errorOut) errorOut->clear();
    return true;
}

void AppWindow::SetWallpaper() {
    ApplyControlsToWorkingPreset();
    std::string error;
    AppSettings runtimeSettings = settings_;
    runtimeSettings.general.colourCyclingEnabled = desktopColourCyclingEnabled_;
    if (!wallpaperController_.Start(instance_, runtimeSettings, AllPresets(), error)) {
        MessageBoxW(window_, L"The wallpaper could not be attached behind the desktop icons. Restart Windows Explorer or reopen the application.",
                    L"Desktop Attachment Failure", MB_ICONERROR | MB_OK);
        LogError("Wallpaper start failed: " + error);
        return;
    }
    userPaused_ = false;
    autoPaused_ = false;
    adaptivePaused_ = false;
    adaptivePerformanceController_.Reset();
    previewForceRender_ = true;
    settings_.staticWallpaper.enabled = false;
    settings_.lastWallpaperRunning = true;
    previewChangesPending_ = false;
    wallpaperController_.SetMotionEnabled(desktopZoomMotionEnabled_);
    wallpaperController_.SetColourCyclingEnabled(desktopColourCyclingEnabled_);
    SaveSettings();
    UpdateStatus();
}

void AppWindow::StartSavedStaticWallpaper() {
    std::string error;
    if (!wallpaperController_.StartStaticGallery(instance_, settings_, AllPresets(), error)) {
        settings_.staticWallpaper.enabled = false;
        LogWarning("Saved static wallpaper start failed: " + error);
        SetWallpaper();
        return;
    }
    userPaused_ = false;
    autoPaused_ = false;
    adaptivePaused_ = false;
    adaptivePerformanceController_.Reset();
    previewForceRender_ = true;
    settings_.lastWallpaperRunning = true;
    previewChangesPending_ = false;
    SaveSettings();
    UpdateStatus();
}

void AppWindow::SetStaticWallpaper() {
    if (settings_.staticWallpaper.imagePaths.size() >= 512U) {
        MessageBoxW(window_, L"The slideshow list already contains the 512-image safety maximum. Remove an image before capturing another.",
                    L"Static Slideshow", MB_OK | MB_ICONINFORMATION);
        return;
    }
    const Preset snapshot = CurrentPreviewSnapshot();
    std::string savedPath;
    std::string error;
    if (!wallpaperController_.CaptureAndUseStatic(instance_, settings_, AllPresets(), snapshot,
                                                   StaticStorageDirectory(), savedPath, error)) {
        MessageBoxW(window_, ToWide("The current render could not be captured as a static wallpaper. " + error).c_str(),
                    L"Static Wallpaper", MB_OK | MB_ICONERROR);
        LogError("Static wallpaper capture failed: " + error);
        return;
    }
    if (std::find(settings_.staticWallpaper.imagePaths.begin(), settings_.staticWallpaper.imagePaths.end(), savedPath) ==
        settings_.staticWallpaper.imagePaths.end()) {
        settings_.staticWallpaper.imagePaths.push_back(savedPath);
    }
    settings_.staticWallpaper.currentIndex = static_cast<int>(settings_.staticWallpaper.imagePaths.size()) - 1;
    settings_.staticWallpaper.enabled = true;
    settings_.lastWallpaperRunning = true;
    userPaused_ = false;
    autoPaused_ = false;
    adaptivePaused_ = false;
    adaptivePerformanceController_.Reset();
    previewForceRender_ = true;
    wallpaperController_.UpdateConfiguration(settings_, AllPresets());
    previewChangesPending_ = false;
    SaveSettings();
    UpdateStatus();
}

Preset AppWindow::CurrentPreviewSnapshot() {
    const bool replacingStatic = wallpaperController_.UsingUserStatic();
    AnimationFrame currentFrame = lastPreviewFrame_;
    ApplyControlsToWorkingPreset();
    if (replacingStatic || currentFrame.camera.scale <= 0.0) {
        currentFrame = {workingPreset_.camera, workingPreset_.colourOffset};
    }
    Preset snapshot = workingPreset_;
    snapshot.camera = currentFrame.camera;
    snapshot.startingScale = currentFrame.camera.scale;
    snapshot.colourOffset = currentFrame.colourOffset;
    ValidateAndNormalise(snapshot);
    return snapshot;
}

std::filesystem::path AppWindow::StaticStorageDirectory() const {
    if (!settings_.staticWallpaper.storageDirectory.empty()) {
        const std::wstring configured = ToWide(settings_.staticWallpaper.storageDirectory);
        if (!configured.empty()) return std::filesystem::path(configured);
    }
    return Paths::StaticRenderDirectory();
}

void AppWindow::AddPreviewToSlideshow() {
    if (settings_.staticWallpaper.imagePaths.size() >= 512U) {
        MessageBoxW(window_, L"The slideshow list already contains the 512-image safety maximum. Remove an image before capturing another.",
                    L"Static Slideshow", MB_OK | MB_ICONINFORMATION);
        return;
    }
    const bool wasRunning = wallpaperController_.IsRunning();
    const bool wasStatic = wallpaperController_.UsingUserStatic();
    const bool wasUserPaused = userPaused_;
    const Preset snapshot = CurrentPreviewSnapshot();

    std::string savedPath;
    std::string error;
    if (!wallpaperController_.CaptureAndUseStatic(instance_, settings_, AllPresets(), snapshot,
                                                   StaticStorageDirectory(), savedPath, error)) {
        MessageBoxW(window_, ToWide("The preview could not be saved to the slideshow. " + error).c_str(),
                    L"Static Slideshow", MB_OK | MB_ICONERROR);
        LogError("Slideshow capture failed: " + error);
        return;
    }
    if (std::find(settings_.staticWallpaper.imagePaths.begin(), settings_.staticWallpaper.imagePaths.end(), savedPath) ==
        settings_.staticWallpaper.imagePaths.end()) {
        settings_.staticWallpaper.imagePaths.push_back(savedPath);
    }
    settings_.staticWallpaper.currentIndex = static_cast<int>(settings_.staticWallpaper.imagePaths.size()) - 1;

    if (wasStatic) {
        settings_.staticWallpaper.enabled = true;
        wallpaperController_.UpdateConfiguration(settings_, AllPresets());
        if (wasUserPaused) {
            std::string pauseError;
            wallpaperController_.PauseAndReleaseGpu("Paused by user", pauseError);
        }
    } else if (wasRunning) {
        settings_.staticWallpaper.enabled = false;
        std::string restoreError;
        AppSettings runtimeSettings = settings_;
        runtimeSettings.general.colourCyclingEnabled = desktopColourCyclingEnabled_;
        if (!wallpaperController_.Start(instance_, runtimeSettings, AllPresets(), restoreError)) {
            LogError("Live wallpaper restore after slideshow capture failed: " + restoreError);
            settings_.staticWallpaper.enabled = true;
            std::string staticError;
            wallpaperController_.StartStaticGallery(instance_, settings_, AllPresets(), staticError);
        } else {
            wallpaperController_.SetMotionEnabled(desktopZoomMotionEnabled_);
            wallpaperController_.SetColourCyclingEnabled(desktopColourCyclingEnabled_);
            if (wasUserPaused) {
                std::string pauseError;
                wallpaperController_.PauseAndReleaseGpu("Paused by user", pauseError);
            }
        }
    } else {
        wallpaperController_.Stop();
        settings_.staticWallpaper.enabled = false;
    }
    settings_.lastWallpaperRunning = wallpaperController_.IsRunning();
    SaveSettings();
    UpdateStatus();
    MessageBoxW(window_, L"The current preview was saved and added to the static slideshow list.",
                L"Static Slideshow", MB_OK | MB_ICONINFORMATION);
}

void AppWindow::ManageSlideshow() {
    if (settings_.staticWallpaper.storageDirectory.empty()) {
        settings_.staticWallpaper.storageDirectory = ToUtf8(Paths::StaticRenderDirectory().wstring());
    }
    const bool wasStatic = wallpaperController_.UsingUserStatic();
    const SlideshowDialogResult result = SlideshowDialog::Show(window_, instance_, settings_.staticWallpaper);
    if (result == SlideshowDialogResult::Cancelled) return;
    ValidateAndNormalise(settings_);
    wallpaperController_.UpdateConfiguration(settings_, AllPresets());

    if (result == SlideshowDialogResult::StartSelected || wasStatic) {
        if (settings_.staticWallpaper.imagePaths.empty()) {
            StopWallpaper();
            return;
        }
        settings_.staticWallpaper.enabled = true;
        StartSavedStaticWallpaper();
    } else {
        SaveSettings();
        UpdateStatus();
    }
}

void AppWindow::OpenPaletteEditor() {
    ApplyControlsToWorkingPreset();
    if (!PaletteEditorDialog::Show(window_, instance_, workingPreset_, settings_.customPalettePresets)) return;
    if (!workingPreset_.builtIn) {
        if (Preset* stored = FindPresetMutable(workingPreset_.id)) *stored = workingPreset_;
    }
    previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
    previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    previewChangesPending_ = true;
    const std::wstring text = workingPreset_.customPaletteColours.empty()
        ? L"Edit Palette..."
        : L"Custom (" + std::to_wstring(workingPreset_.customPaletteColours.size()) + L")";
    SetWindowTextW(GetDlgItem(window_, PaletteEditorButton), text.c_str());
    SaveSettings();
}

void AppWindow::OpenEquationEditor() {
    ApplyControlsToWorkingPreset();
    const bool accepted = EquationEditorDialog::Show(
        window_, instance_, workingPreset_, settings_.customEquationPresets);
    SaveSettings();
    if (!accepted) return;
    if (!workingPreset_.builtIn) {
        if (Preset* stored = FindPresetMutable(workingPreset_.id)) *stored = workingPreset_;
    }
    previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
    previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    previewChangesPending_ = true;
    SetWindowTextW(GetDlgItem(window_, EquationEditorButton),
                   ToWide(EquationSummary(workingPreset_.equation)).c_str());
    previewForceRender_ = true;
    SaveSettings();
}

void AppWindow::TogglePause() {
    if (!wallpaperController_.IsRunning()) return;
    if (wallpaperController_.IsPaused()) {
        if (adaptivePaused_) {
            MessageBoxW(window_, L"Rendering is paused by adaptive resource protection. It will resume after CPU and memory remain stable for the configured cooldown.",
                        L"Adaptive Pause", MB_OK | MB_ICONINFORMATION);
            return;
        }
        autoPaused_ = false;
        wallpaperController_.Resume();
        userPaused_ = wallpaperController_.IsPaused();
        if (userPaused_ && !wallpaperController_.LastRendererError().empty()) {
            MessageBoxW(window_, ToWide("The live renderer could not resume. The captured paused frame remains active. " +
                                       wallpaperController_.LastRendererError()).c_str(),
                        L"Resume Wallpaper", MB_OK | MB_ICONERROR);
        }
        previewForceRender_ = true;
    } else {
        userPaused_ = true;
        std::string pauseError;
        if (!wallpaperController_.PauseAndReleaseGpu("Paused by user", pauseError) && !pauseError.empty()) {
            LogWarning("Paused without releasing the wallpaper GPU: " + pauseError);
        }
    }
    UpdateStatus();
}

void AppWindow::ToggleColourCycling() {
    const bool enabled = !previewColourCyclingEnabled_;
    previewColourCyclingEnabled_ = enabled;
    desktopColourCyclingEnabled_ = enabled;
    settings_.general.colourCyclingEnabled = enabled;
    previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    wallpaperController_.SetColourCyclingEnabled(desktopColourCyclingEnabled_);
    previewForceRender_ = true;
    SaveSettings();
    UpdateStatus();
}

void AppWindow::TogglePreviewColourCycling() {
    previewColourCyclingEnabled_ = !previewColourCyclingEnabled_;
    previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    previewForceRender_ = true;
    UpdateStatus();
}

void AppWindow::StopWallpaper() {
    if (wallpaperController_.UsingUserStatic()) {
        settings_.staticWallpaper.currentIndex = wallpaperController_.CurrentStaticImageIndex();
    }
    wallpaperController_.Stop();
    settings_.staticWallpaper.enabled = false;
    settings_.lastWallpaperRunning = false;
    userPaused_ = false;
    autoPaused_ = false;
    adaptivePaused_ = false;
    adaptivePerformanceController_.Reset();
    SaveSettings();
    UpdateStatus();
}

void AppWindow::PollSystemState() {
    const auto state = systemStateMonitor_.Poll();
    const auto systemDecision = systemStateMonitor_.Evaluate(settings_.performance);
    const bool reducedBatteryQuality = state.onBattery && settings_.performance.reduceQualityOnBattery;
    wallpaperController_.SetBatteryQualityReduction(reducedBatteryQuality);

    const auto now = std::chrono::steady_clock::now();
    const double adaptiveElapsed = std::clamp(
        std::chrono::duration<double>(now - lastAdaptiveSample_).count(), 0.0, 5.0);
    lastAdaptiveSample_ = now;

    AdaptivePerformanceSample sample;
    sample.targetFramesPerSecond = static_cast<double>(
        state.onBattery && settings_.performance.reduceQualityOnBattery
            ? std::min(settings_.performance.maximumFrameRate, 15)
            : settings_.performance.maximumFrameRate);
    sample.processCpuPercent = state.processCpuPercent;
    sample.processWorkingSetMb = static_cast<double>(state.processWorkingSetBytes) / (1024.0 * 1024.0);

    std::vector<double> activeFps;
    const bool previewActive = previewRenderer_.IsReady() && IsWindowVisible(window_) && !IsIconic(window_) &&
                               !previewChangeDetector_.IsVisuallyIdle();
    if (previewActive && previewRenderer_.FramesPerSecond() > 0.0) {
        activeFps.push_back(previewRenderer_.FramesPerSecond());
    }
    const bool wallpaperActive = wallpaperController_.IsRunning() && !wallpaperController_.IsPaused() &&
                                 !wallpaperController_.UsingUserStatic() &&
                                 !wallpaperController_.UsingStaticFallback() &&
                                 !wallpaperController_.IsVisuallyIdle();
    if (wallpaperActive && wallpaperController_.FramesPerSecond() > 0.0) {
        activeFps.push_back(wallpaperController_.FramesPerSecond());
    }
    sample.rendererActive = previewActive || wallpaperActive;
    sample.framesPerSecondMeaningful = !activeFps.empty();
    if (!activeFps.empty()) sample.framesPerSecond = *std::min_element(activeFps.begin(), activeFps.end());

    const auto adaptiveDecision = adaptivePerformanceController_.Update(
        settings_.performance.adaptive, sample, adaptiveElapsed);
    adaptivePaused_ = adaptiveDecision.paused;

    const bool shouldAutoPause = systemDecision.shouldPause || adaptivePaused_;
    const std::string autoPauseReason = systemDecision.shouldPause ? systemDecision.reason : adaptiveDecision.reason;
    if (shouldAutoPause && wallpaperController_.IsRunning() && !userPaused_) {
        if (!autoPaused_ || wallpaperController_.PauseReason() != autoPauseReason) {
            autoPaused_ = true;
            wallpaperController_.Pause(autoPauseReason);
            autoResumeEligibleAt_ = now + std::chrono::milliseconds(settings_.performance.resumeDelayMs);
        }
    } else if (!shouldAutoPause && autoPaused_ && !userPaused_ && now >= autoResumeEligibleAt_) {
        autoPaused_ = false;
        wallpaperController_.Resume();
        previewForceRender_ = true;
    }
}

void AppWindow::RenderTick() {
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<double>(now - lastSystemPoll_).count() >= 0.5) {
        PollSystemState();
        lastSystemPoll_ = now;
    }

    int frameLimit = settings_.performance.maximumFrameRate;
    const auto state = systemStateMonitor_.State();
    if (state.onBattery && settings_.performance.reduceQualityOnBattery) frameLimit = std::min(frameLimit, 15);
    const double minimumFrameSeconds = 1.0 / std::max(5, frameLimit);
    const double elapsed = std::chrono::duration<double>(now - lastFrameTime_).count();
    if (elapsed < minimumFrameSeconds) return;
    lastFrameTime_ = now;

    wallpaperController_.Tick(elapsed);
    if (adaptivePaused_) {
        UpdateStatus();
        return;
    }
    if (!IsWindowVisible(window_) || IsIconic(window_)) {
        // The Quick Controller may remain open while the main editor is hidden.
        // Do not spend GPU time rendering an invisible preview in that state.
        UpdateStatus();
        return;
    }

    const auto frame = previewAnimation_.Update(elapsed);
    lastPreviewFrame_ = frame;
    RECT client{};
    GetClientRect(previewWindow_, &client);
    RenderRegion region;
    region.pixels = client;
    region.camera = frame.camera;
    region.palette = workingPreset_.palette;
    region.customPaletteColours = workingPreset_.customPaletteColours;
    region.equation = workingPreset_.equation;
    region.maximumIterations = (state.onBattery && settings_.performance.reduceQualityOnBattery)
        ? std::max(64, static_cast<int>(std::lround(workingPreset_.maximumIterations * 0.6)))
        : workingPreset_.maximumIterations;
    region.colourOffset = frame.colourOffset;
    region.brightness = workingPreset_.brightness;
    region.contrast = workingPreset_.contrast;
    region.saturation = workingPreset_.saturation;
    region.interiorColour = workingPreset_.interiorColour;
    region.backgroundColour = workingPreset_.backgroundColour;
    region.smoothColouring = workingPreset_.smoothColouring;

    RenderOptions options;
    options.renderScale = (state.onBattery && settings_.performance.reduceQualityOnBattery)
        ? settings_.performance.renderScale * 0.66 : settings_.performance.renderScale;
    options.antiAliasingLevel = (state.onBattery && settings_.performance.reduceQualityOnBattery)
        ? 1 : settings_.performance.antiAliasingLevel;
    options.precision = settings_.performance.precision;

    VisualFrameDescriptor descriptor;
    descriptor.camera = region.camera;
    descriptor.colourOffset = region.colourOffset;
    descriptor.pixelWidth = static_cast<int>(std::max<LONG>(1L, client.right - client.left));
    descriptor.pixelHeight = static_cast<int>(std::max<LONG>(1L, client.bottom - client.top));
    descriptor.contentRevision = ComputeVisualRevision(workingPreset_, region.maximumIterations,
                                                       options.renderScale, options.antiAliasingLevel,
                                                       options.precision);
    const bool shouldRenderPreview = previewChangeDetector_.ShouldRender(
        {descriptor}, settings_.performance.adaptive,
        previewForceRender_ || workingPreset_.equation.animateCoefficients);
    previewForceRender_ = false;

    std::string error;
    if (previewRenderer_.IsReady()) {
        if (shouldRenderPreview && !previewRenderer_.Render({region}, options, error)) {
            previewRendererError_ = error;
            LogError("Preview renderer error: " + error);
            previewRenderer_.Shutdown();
            InvalidateRect(previewWindow_, nullptr, TRUE);
        }
    } else {
        // Keep the explicit failure message visible if GPU initialisation failed.
        InvalidateRect(previewWindow_, nullptr, FALSE);
    }
    UpdateStatus();
    UpdateQuickController();
}

void AppWindow::UpdateStatus() {
    std::ostringstream status;
    if (!wallpaperController_.IsRunning()) status << "Status: stopped";
    else if (wallpaperController_.IsPaused()) {
        status << "Status: paused — " << wallpaperController_.PauseReason();
        if (wallpaperController_.UsingPausedSnapshot()) status << " (captured frame, GPU released)";
    }
    else if (wallpaperController_.UsingUserStatic()) status << "Status: running — saved static image (GPU idle)";
    else if (wallpaperController_.UsingStaticFallback()) status << "Status: running — static CPU fallback (GPU unavailable)";
    else status << "Status: running";
    if (adaptivePaused_) status << " | preview paused by resource protection";
    else if (previewChangeDetector_.IsVisuallyIdle()) status << " | preview equation idle until a visible change";
    if (previewChangesPending_) status << " | preview changes not yet applied to wallpaper";
    SetControlText(statusLabel_, status.str());

    std::ostringstream fps;
    const bool previewFrozen = !IsWindowVisible(window_) || IsIconic(window_);
    const double previewFps = (previewFrozen || adaptivePaused_ || previewChangeDetector_.IsVisuallyIdle())
        ? 0.0 : previewRenderer_.FramesPerSecond();
    const auto systemState = systemStateMonitor_.State();
    fps << std::fixed << std::setprecision(1) << "Preview FPS: " << previewFps
        << " | Wallpaper FPS: " << wallpaperController_.FramesPerSecond()
        << " | CPU: " << systemState.processCpuPercent << "%"
        << " | Memory: " << static_cast<double>(systemState.processWorkingSetBytes) / (1024.0 * 1024.0) << " MB";
    SetControlText(fpsLabel_, fps.str());

    std::ostringstream profile;
    profile << "Profile: " << ToString(settings_.performance.profile) << " | " << settings_.performance.maximumFrameRate << " FPS | "
            << static_cast<int>(std::lround(settings_.performance.renderScale * 100.0)) << "% scale | "
            << settings_.performance.maximumIterations << " iterations";
    SetControlText(profileLabel_, profile.str());
    std::ostringstream precision;
    precision << "Deep zoom: requested " << PrecisionModeDisplayName(settings_.performance.precision.mode)
              << " | preview " << previewRenderer_.PrecisionDescription()
              << " | wallpaper " << wallpaperController_.PrecisionDescription();
    SetControlText(precisionLabel_, precision.str());
    SetWindowTextW(pauseButton_, wallpaperController_.IsPaused() ? L"Resume" : L"Pause");
    SetWindowTextW(colourCycleButton_, previewColourCyclingEnabled_ ? L"Pause Colours" : L"Play Colours");
    UpdateQuickController();
}

void AppWindow::OpenHighResRenderDialog() {
    const Preset snapshot = CurrentPreviewSnapshot();
    HighResRenderDialog::Show(window_, instance_, snapshot, settings_.performance);
}

void AppWindow::OpenQuickController() {
    if (!quickController_.Create(instance_, window_, icon_)) {
        MessageBoxW(window_, L"The Quick Controller could not be opened.",
                    L"Quick Controller", MB_OK | MB_ICONERROR);
        return;
    }
    quickController_.Show();
    UpdateQuickController();
}

void AppWindow::UpdateQuickController() {
    if (!quickController_.IsVisible()) return;
    const AnimationFrame frame = lastPreviewFrame_.camera.scale > 0.0
        ? lastPreviewFrame_ : AnimationFrame{workingPreset_.camera, workingPreset_.colourOffset};
    std::wostringstream coordinates;
    coordinates << std::setprecision(12) << L"X " << CameraCentreX(frame.camera)
                << L"   Y " << CameraCentreY(frame.camera)
                << L"\r\nScale " << frame.camera.scale;
    std::wostringstream resources;
    const auto resourceState = systemStateMonitor_.State();
    resources << std::fixed << std::setprecision(1)
              << L"Preview " << previewRenderer_.FramesPerSecond() << L" FPS | Desktop "
              << wallpaperController_.FramesPerSecond() << L" FPS | CPU "
              << resourceState.processCpuPercent << L"% | Memory "
              << static_cast<double>(resourceState.processWorkingSetBytes) / (1024.0 * 1024.0) << L" MB";
    std::wstring status = L"Wallpaper stopped";
    if (wallpaperController_.IsPaused()) {
        status = L"Wallpaper paused — " + ToWide(wallpaperController_.PauseReason());
        if (wallpaperController_.UsingPausedSnapshot()) status += L" — GPU released";
    }
    else if (wallpaperController_.UsingUserStatic()) status = L"Static wallpaper active — GPU idle";
    else if (wallpaperController_.IsRunning()) status = L"Live wallpaper running";
    quickController_.Update(status, coordinates.str(), resources.str(),
                            zoomMotionEnabled_, previewColourCyclingEnabled_,
                            desktopZoomMotionEnabled_, desktopColourCyclingEnabled_);
}

void AppWindow::ToggleZoomMotion() {
    zoomMotionEnabled_ = !zoomMotionEnabled_;
    desktopZoomMotionEnabled_ = zoomMotionEnabled_;
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    wallpaperController_.SetMotionEnabled(desktopZoomMotionEnabled_);
    previewForceRender_ = true;
    UpdateQuickController();
}

void AppWindow::TogglePreviewZoomMotion() {
    zoomMotionEnabled_ = !zoomMotionEnabled_;
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    previewForceRender_ = true;
    UpdateQuickController();
}

void AppWindow::ToggleDesktopZoomMotion() {
    desktopZoomMotionEnabled_ = !desktopZoomMotionEnabled_;
    wallpaperController_.SetMotionEnabled(desktopZoomMotionEnabled_);
    UpdateQuickController();
}

void AppWindow::ToggleDesktopColourCycling() {
    desktopColourCyclingEnabled_ = !desktopColourCyclingEnabled_;
    wallpaperController_.SetColourCyclingEnabled(desktopColourCyclingEnabled_);
    UpdateQuickController();
}

void AppWindow::ApplyPreviewAsSlideshowWallpaper() {
    if (settings_.staticWallpaper.imagePaths.empty()) {
        AddPreviewToSlideshow();
    }
    if (!settings_.staticWallpaper.imagePaths.empty()) {
        settings_.staticWallpaper.enabled = true;
        settings_.staticWallpaper.cycleEnabled = true;
        StartSavedStaticWallpaper();
    }
}

void AppWindow::JumpToCoordinates() {
    std::wstring input = ToWide(FormatDouble(CameraCentreX(workingPreset_.camera)) + "," +
                                FormatDouble(CameraCentreY(workingPreset_.camera)) + "," +
                                FormatDouble(workingPreset_.camera.scale, 12));
    if (!PromptForText(window_, instance_, L"Jump to Coordinates",
                       L"Enter coordinates as centreX,centreY,scale", input, input)) return;
    double x = 0.0; double y = 0.0; double scale = 0.0;
    if (!ParseCoordinateTriplet(input, x, y, scale)) {
        MessageBoxW(window_, L"Use the format centreX,centreY,scale", L"Jump to Coordinates", MB_OK | MB_ICONERROR);
        return;
    }
    workingPreset_.camera = {x, y, scale};
    workingPreset_.animationMode = AnimationMode::ManualView;
    previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
    previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
    previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
    SendMessageW(animationCombo_, CB_SETCURSEL, static_cast<WPARAM>(AnimationMode::ManualView), 0);
    SetControlText(centreXEdit_, FormatDouble(x));
    SetControlText(centreYEdit_, FormatDouble(y));
    SetControlText(scaleEdit_, FormatDouble(scale));
    UpdateCoordinatesEdit();
    previewChangesPending_ = true;
    previewForceRender_ = true;
}

void AppWindow::SelectRelativePreset(int direction) {
    const int count = static_cast<int>(SendMessageW(presetCombo_, CB_GETCOUNT, 0, 0));
    if (count <= 0) return;
    int index = SelectedComboIndex(presetCombo_);
    index = (index + direction + count) % count;
    SendMessageW(presetCombo_, CB_SETCURSEL, index, 0);
    LoadSelectedPreset();
    if (wallpaperController_.IsRunning()) {
        wallpaperController_.SelectPreset(settings_.selectedPresetId);
        previewChangesPending_ = false;
    }
    SaveSettings();
}

void AppWindow::SaveAsNewPreset(const std::string& requestedName) {
    ApplyControlsToWorkingPreset();
    if (settings_.customPresets.size() >= 256U) {
        MessageBoxW(window_, L"The custom preset library already contains the 256-entry safety maximum.",
                    L"Save Preset as New", MB_OK | MB_ICONINFORMATION);
        return;
    }
    std::string name = requestedName;
    if (name.empty()) {
        std::string suggestedName = workingPreset_.name.empty() ? std::string("Custom Preset") : workingPreset_.name;
        if (const Preset* stored = FindPreset(workingPreset_.id); stored && stored->name == suggestedName) {
            suggestedName += " Copy";
        }
        std::wstring promptedName = ToWide(suggestedName);
        if (!PromptForText(window_, instance_, L"Save Preset as New",
                           L"Enter a name for the new preset.", promptedName, promptedName)) return;
        name = ToUtf8(promptedName);
    }
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) name = "Custom Preset";
    else {
        const auto last = name.find_last_not_of(" \t\r\n");
        name = name.substr(first, last - first + 1U);
    }
    if (name.size() > 120U) {
        MessageBoxW(window_, L"The preset name is too long. Use no more than 120 UTF-8 bytes.",
                    L"Save Preset as New", MB_OK | MB_ICONWARNING);
        return;
    }
    workingPreset_.name = name;
    SetControlText(presetNameEdit_, workingPreset_.name);

    const std::string previousSelectedPreset = settings_.selectedPresetId;
    Preset custom = workingPreset_;
    custom.name = workingPreset_.name;
    custom.builtIn = false;
    const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    custom.id = Slugify(custom.name) + "-" + std::to_string(stamp);
    if (custom.id.empty()) custom.id = std::string("custom-preset-") + std::to_string(stamp);
    ValidateAndNormalise(custom);
    settings_.customPresets.push_back(custom);
    settings_.selectedPresetId = custom.id;

    std::string saveError;
    if (!SaveSettings(&saveError)) {
        settings_.customPresets.pop_back();
        settings_.selectedPresetId = previousSelectedPreset;
        PopulatePresetCombo();
        LoadSelectedPreset();
        MessageBoxW(window_, ToWide("The preset could not be saved. " + saveError).c_str(),
                    L"Save Preset as New", MB_OK | MB_ICONERROR);
        return;
    }
    PopulatePresetCombo();
    LoadSelectedPreset();
    MessageBoxW(window_, L"Preset saved successfully and selected.",
                L"Save Preset as New", MB_OK | MB_ICONINFORMATION);
}


void AppWindow::SaveChangesToPreset() {
    ApplyControlsToWorkingPreset();
    if (workingPreset_.builtIn) {
        MessageBoxW(window_, L"Built-in presets are read-only. Use Save as New to create an editable copy.",
                    L"Built-in Preset", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (Preset* custom = FindPresetMutable(workingPreset_.id)) {
        const Preset previous = *custom;
        *custom = workingPreset_;
        custom->builtIn = false;
        settings_.selectedPresetId = custom->id;
        std::string saveError;
        if (!SaveSettings(&saveError)) {
            *custom = previous;
            PopulatePresetCombo();
            LoadSelectedPreset();
            MessageBoxW(window_, ToWide("The preset changes could not be saved. " + saveError).c_str(),
                        L"Save Preset Changes", MB_OK | MB_ICONERROR);
            return;
        }
        PopulatePresetCombo();
        LoadSelectedPreset();
        MessageBoxW(window_, L"Preset changes saved successfully.",
                    L"Save Preset Changes", MB_OK | MB_ICONINFORMATION);
    }
}


void AppWindow::RestoreBuiltInPresets() {
    builtInPresets_ = BuiltInPresets();
    if (!FindPreset(settings_.selectedPresetId)) settings_.selectedPresetId = builtInPresets_.front().id;
    PopulatePresetCombo();
    LoadSelectedPreset();
    SaveSettings();
}

void AppWindow::OpenSettings() {
    ApplyControlsToWorkingPreset();
    const bool previousStartup = StartupManager::IsEnabled();
    if (SettingsDialog::Show(window_, instance_, settings_, workingPreset_, AllPresets())) {
        SendMessageW(performanceCombo_, CB_SETCURSEL, static_cast<WPARAM>(settings_.performance.profile), 0);
        SendMessageW(monitorModeCombo_, CB_SETCURSEL, static_cast<WPARAM>(settings_.monitorMode), 0);
        SendMessageW(iterationsTrack_, TBM_SETPOS, TRUE, workingPreset_.maximumIterations);
        SendMessageW(fpsTrack_, TBM_SETPOS, TRUE, settings_.performance.maximumFrameRate);
        SendMessageW(renderScaleTrack_, TBM_SETPOS, TRUE,
                     static_cast<LPARAM>(std::lround(settings_.performance.renderScale * 100.0)));
        SendMessageW(brightnessTrack_, TBM_SETPOS, TRUE,
                     static_cast<LPARAM>(std::lround(workingPreset_.brightness * 100.0)));
        SendMessageW(contrastTrack_, TBM_SETPOS, TRUE,
                     static_cast<LPARAM>(std::lround(workingPreset_.contrast * 100.0)));
        SendMessageW(saturationTrack_, TBM_SETPOS, TRUE,
                     static_cast<LPARAM>(std::lround(workingPreset_.saturation * 100.0)));
        SendMessageW(colourOffsetTrack_, TBM_SETPOS, TRUE,
                     static_cast<LPARAM>(std::lround((workingPreset_.colourOffset + 1.0) * 100.0)));
        SetCheck(GetDlgItem(window_, StartupCheck), settings_.general.startWithWindows);
        SetCheck(GetDlgItem(window_, FullscreenCheck), settings_.performance.pauseWhenFullscreen);
        SetCheck(GetDlgItem(window_, BatteryCheck), settings_.performance.pauseOnBattery);
        SetCheck(GetDlgItem(window_, RemoteCheck), settings_.performance.pauseDuringRemoteDesktop);
        SetCheck(GetDlgItem(window_, ReducedMotionCheck), settings_.general.reducedMotion);

        if (previousStartup != settings_.general.startWithWindows) {
            std::string startupError;
            if (!StartupManager::SetEnabled(settings_.general.startWithWindows,
                                            Paths::ExecutablePath(), startupError)) {
                settings_.general.startWithWindows = previousStartup;
                SetCheck(GetDlgItem(window_, StartupCheck), previousStartup);
                MessageBoxW(window_, ToWide(startupError).c_str(), L"Start with Windows",
                            MB_OK | MB_ICONERROR);
            }
        }

        SyncNumericEditsFromTracks();
        UpdateCoordinatesEdit();
        PopulateMonitorControls();
        previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
        previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
        previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
        wallpaperController_.UpdateConfiguration(settings_, AllPresets());
        previewChangesPending_ = true;
        previewForceRender_ = true;
        SaveSettings();
        UpdateStatus();
    }
}

void AppWindow::DeleteSelectedPreset() {
    if (workingPreset_.builtIn) {
        MessageBoxW(window_, L"Built-in presets cannot be deleted. Use Save as New to create an editable copy.", L"Built-in Preset", MB_OK | MB_ICONINFORMATION);
        return;
    }
    settings_.customPresets.erase(std::remove_if(settings_.customPresets.begin(), settings_.customPresets.end(),
                                                  [&](const Preset& preset) { return preset.id == workingPreset_.id; }),
                                   settings_.customPresets.end());
    settings_.selectedPresetId = builtInPresets_.front().id;
    PopulatePresetCombo();
    LoadSelectedPreset();
    SaveSettings();
}

void AppWindow::ImportPreset() {
    const auto path = OpenFileDialog(window_, false);
    if (path.empty()) return;
    std::string text;
    std::string error;
    if (!ReadTextFile(path, text, error)) {
        MessageBoxW(window_, ToWide(error).c_str(), L"Invalid Preset", MB_OK | MB_ICONERROR);
        return;
    }
    auto preset = SettingsStore::DeserialisePreset(text, error);
    if (!preset) {
        MessageBoxW(window_, L"This preset could not be imported because its format is invalid or unsupported.", L"Invalid Preset", MB_OK | MB_ICONERROR);
        LogWarning("Preset import rejected: " + error);
        return;
    }
    const auto stamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    preset->id = Slugify(preset->name) + "-" + std::to_string(stamp);
    preset->builtIn = false;
    settings_.customPresets.push_back(*preset);
    settings_.selectedPresetId = preset->id;
    PopulatePresetCombo();
    LoadSelectedPreset();
    SaveSettings();
}

void AppWindow::ExportPreset() {
    ApplyControlsToWorkingPreset();
    const auto path = OpenFileDialog(window_, true);
    if (path.empty()) return;
    std::string error;
    if (!WriteTextFile(path, SettingsStore::SerialisePreset(workingPreset_), error)) {
        MessageBoxW(window_, ToWide(error).c_str(), L"Preset Export", MB_OK | MB_ICONERROR);
    }
}

void AppWindow::AssignPresetToMonitor() {
    if (settings_.monitorMode != MonitorMode::Independent) {
        MessageBoxW(window_, L"Monitor assignments are used only in Independent mode. Select Independent under Monitor mode first.",
                    L"Monitor Assignment", MB_OK | MB_ICONINFORMATION);
        return;
    }
    const auto displays = DisplayManager::Enumerate();
    const auto presets = AllPresets();
    const int monitorIndex = SelectedComboIndex(monitorCombo_);
    const int presetIndex = SelectedComboIndex(monitorAssignmentCombo_);
    if (monitorIndex < 0 || monitorIndex >= static_cast<int>(displays.size()) || presetIndex < 0 || presetIndex >= static_cast<int>(presets.size())) return;
    const std::string key = ToUtf8(displays[static_cast<std::size_t>(monitorIndex)].deviceName);
    settings_.monitorPresetAssignments[key] = presets[static_cast<std::size_t>(presetIndex)].id;
    wallpaperController_.UpdateConfiguration(settings_, presets);
    SaveSettings();
    std::wstring message = L"Assigned \"" + ToWide(presets[static_cast<std::size_t>(presetIndex)].name) +
                           L"\" to " + displays[static_cast<std::size_t>(monitorIndex)].friendlyName + L".";
    if (!wallpaperController_.IsRunning() || wallpaperController_.UsingUserStatic()) {
        message += L" The assignment is saved and will appear the next time you apply a live wallpaper.";
    }
    MessageBoxW(window_, message.c_str(), L"Monitor Assignment Applied", MB_OK | MB_ICONINFORMATION);
    PopulateMonitorControls();
}

void AppWindow::ToggleStartup() {
    const bool enable = IsChecked(GetDlgItem(window_, StartupCheck));
    std::string error;
    if (!StartupManager::SetEnabled(enable, Paths::ExecutablePath(), error)) {
        SetCheck(GetDlgItem(window_, StartupCheck), !enable);
        MessageBoxW(window_, ToWide(error).c_str(), L"Start with Windows", MB_OK | MB_ICONERROR);
        return;
    }
    settings_.general.startWithWindows = enable;
    SaveSettings();
}

void AppWindow::OpenLogFolder() {
    std::error_code ec;
    std::filesystem::create_directories(Paths::LogDirectory(), ec);
    ShellExecuteW(window_, L"open", Paths::LogDirectory().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void AppWindow::CopyDiagnostics() {
    std::ostringstream diagnostics;
    diagnostics << Logger::Instance().DiagnosticSummary();
    const std::string previewGpu = previewRenderer_.GraphicsDescription();
    diagnostics << "Preview GPU: " << (previewGpu.empty() ? "unavailable" : previewGpu);
    if (previewGpu.empty() && !previewRendererError_.empty()) diagnostics << " — " << previewRendererError_;
    diagnostics << '\n';
    const std::string wallpaperGpu = wallpaperController_.GraphicsDescription();
    diagnostics << "Wallpaper GPU: " << (wallpaperGpu.empty() ? "unavailable" : wallpaperGpu);
    if (wallpaperGpu.empty() && !wallpaperController_.LastRendererError().empty()) {
        diagnostics << " — " << wallpaperController_.LastRendererError();
    }
    diagnostics << '\n';
    diagnostics << "Wallpaper renderer mode: ";
    if (wallpaperController_.UsingUserStatic()) diagnostics << "saved static image (GPU idle)";
    else if (wallpaperController_.UsingPausedSnapshot()) diagnostics << "captured paused frame (GPU released)";
    else if (wallpaperController_.UsingStaticFallback()) diagnostics << "static CPU fallback";
    else diagnostics << "GPU";
    diagnostics << '\n';
    diagnostics << "Profile: " << ToString(settings_.performance.profile) << '\n';
    const auto resourceState = systemStateMonitor_.State();
    diagnostics << "Process resources: CPU=" << std::fixed << std::setprecision(1)
                << resourceState.processCpuPercent << "% | working-set="
                << static_cast<double>(resourceState.processWorkingSetBytes) / (1024.0 * 1024.0) << " MB\n";
    diagnostics << "Adaptive protection: " << (settings_.performance.adaptive.enabled ? "enabled" : "disabled")
                << " | state=" << (adaptivePaused_ ? "paused" : "active")
                << " | low-FPS=" << (settings_.performance.adaptive.pauseOnLowFps ? "on" : "off")
                << " | high-CPU=" << (settings_.performance.adaptive.pauseOnHighCpu ? "on" : "off")
                << " | high-memory=" << (settings_.performance.adaptive.pauseOnHighMemory ? "on" : "off") << '\n';
    diagnostics << "Invisible-frame suppression: "
                << (settings_.performance.adaptive.stopWhenVisuallyUnchanged ? "enabled" : "disabled")
                << " | preview-idle=" << (previewChangeDetector_.IsVisuallyIdle() ? "yes" : "no")
                << " | wallpaper-idle=" << (wallpaperController_.IsVisuallyIdle() ? "yes" : "no")
                << " | preview-skipped=" << previewChangeDetector_.SkippedFrameCount()
                << " | wallpaper-skipped=" << wallpaperController_.SkippedInvisibleFrames() << '\n';
    diagnostics << "Precision requested: " << PrecisionModeDisplayName(settings_.performance.precision.mode) << '\n';
    diagnostics << "Preview precision active: " << previewRenderer_.PrecisionDescription() << '\n';
    diagnostics << "Wallpaper precision active: " << wallpaperController_.PrecisionDescription() << '\n';
    const auto precisionCaps = previewRenderer_.Capabilities();
    diagnostics << "Precision capabilities: float64=" << (precisionCaps.nativeFloat64 ? "yes" : "no")
                << ", split=yes, perturbation=" << (precisionCaps.perturbation ? "yes" : "no")
                << ", arbitrary-reference=" << (precisionCaps.arbitraryReference ? "yes" : "no") << '\n';
    const bool previewFrozen = !IsWindowVisible(window_) || IsIconic(window_);
    diagnostics << "Preview activity: " << (previewFrozen ? "frozen while main editor is hidden" : "active") << '\n';
    diagnostics << "Preview changes pending: " << (previewChangesPending_ ? "yes" : "no") << '\n';
    diagnostics << "Preview colour cycling: " << (previewColourCyclingEnabled_ ? "playing" : "paused") << '\n';
    diagnostics << "Desktop colour cycling: " << (desktopColourCyclingEnabled_ ? "playing" : "paused") << '\n';
    diagnostics << "Equation: " << EquationSummary(workingPreset_.equation) << '\n';
    diagnostics << "Static slideshow: " << settings_.staticWallpaper.imagePaths.size()
                << " images | cycle: " << (settings_.staticWallpaper.cycleEnabled ? "enabled" : "disabled")
                << " | interval: " << settings_.staticWallpaper.cycleSeconds << " seconds"
                << " | order: " << ToString(settings_.staticWallpaper.order) << '\n';
    diagnostics << "Static capture folder: "
                << (settings_.staticWallpaper.storageDirectory.empty() ? "default" : settings_.staticWallpaper.storageDirectory)
                << '\n';
    if (!settings_.staticWallpaper.imagePaths.empty()) {
        const int selectedStatic = std::clamp(settings_.staticWallpaper.currentIndex, 0,
            static_cast<int>(settings_.staticWallpaper.imagePaths.size()) - 1);
        diagnostics << "Static current image: " << (selectedStatic + 1) << "/"
                    << settings_.staticWallpaper.imagePaths.size() << " | "
                    << settings_.staticWallpaper.imagePaths[static_cast<std::size_t>(selectedStatic)] << '\n';
    }
    diagnostics << "Monitor mode: " << ToString(settings_.monitorMode) << '\n';
    const auto displays = DisplayManager::Enumerate();
    const RECT virtualBounds = DisplayManager::VirtualDesktopBounds(displays);
    diagnostics << "Virtual desktop: left=" << virtualBounds.left << ", top=" << virtualBounds.top
                << ", right=" << virtualBounds.right << ", bottom=" << virtualBounds.bottom << '\n';
    for (const auto& display : displays) {
        const std::string key = ToUtf8(display.deviceName);
        diagnostics << "Monitor " << key << ": " << ToUtf8(display.friendlyName)
                    << " [" << display.bounds.left << ',' << display.bounds.top << " to "
                    << display.bounds.right << ',' << display.bounds.bottom << ']';
        const auto assignment = settings_.monitorPresetAssignments.find(key);
        if (assignment != settings_.monitorPresetAssignments.end()) {
            diagnostics << " | assigned preset=" << assignment->second;
        }
        diagnostics << '\n';
    }
    const std::wstring text = ToWide(diagnostics.str());
    if (!CopyUnicodeText(window_, text)) {
        MessageBoxW(window_, L"Diagnostics could not be copied to the clipboard.",
                    L"Copy Diagnostics", MB_OK | MB_ICONERROR);
    }
}

void AppWindow::CopyCoordinates() {
    const AnimationFrame frame = lastPreviewFrame_.camera.scale > 0.0
        ? lastPreviewFrame_ : AnimationFrame{workingPreset_.camera, workingPreset_.colourOffset};
    std::wostringstream coordinates;
    coordinates << std::setprecision(17)
                << CameraCentreX(frame.camera) << L"," << CameraCentreY(frame.camera)
                << L"," << frame.camera.scale;
    if (!CopyUnicodeText(window_, coordinates.str())) {
        MessageBoxW(window_, L"The coordinates could not be copied to the clipboard.",
                    L"Copy Coordinates", MB_OK | MB_ICONERROR);
        return;
    }
    LogInfo("Copied the current preview coordinates to the clipboard.");
}

void AppWindow::ClearLogs() {
    Logger::Instance().Clear();
}

void AppWindow::ShowRendererError(const std::string& detail) {
    LogError("Renderer error: " + detail);
    MessageBoxW(window_, L"The Mandelbrot renderer could not start. The live wallpaper will remain stopped. Try the Battery Saver profile or update the graphics driver.",
                L"Renderer Error", MB_OK | MB_ICONERROR);
}

void AppWindow::ShowWindowAndActivate() {
    ShowWindow(window_, SW_RESTORE);
    SetForegroundWindow(window_);
}

void AppWindow::ExitApplication() {
    exitRequested_ = true;
    if (wallpaperController_.UsingUserStatic()) {
        settings_.staticWallpaper.currentIndex = wallpaperController_.CurrentStaticImageIndex();
    }
    settings_.lastWallpaperRunning = false;
    wallpaperController_.Stop();
    SaveSettings();
    DestroyWindow(window_);
}

LRESULT AppWindow::HandlePreviewMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_SIZE) {
        previewRenderer_.Resize(LOWORD(lParam), HIWORD(lParam));
        return 0;
    }
    if (message == WM_LBUTTONDOWN) {
        draggingPreview_ = true;
        dragStart_ = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        SetCapture(window);
        return 0;
    }
    if (message == WM_MOUSEMOVE && draggingPreview_) {
        RECT client{};
        GetClientRect(window, &client);
        const POINT current{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        const double dx = static_cast<double>(current.x - dragStart_.x) / std::max(1L, client.right);
        const double dy = static_cast<double>(current.y - dragStart_.y) / std::max(1L, client.bottom);
        previewAnimation_.Pan(dx, dy, static_cast<double>(std::max(1L, client.right)) / std::max(1L, client.bottom));
        workingPreset_.camera = previewAnimation_.Camera();
        previewChangesPending_ = true;
        SetControlText(centreXEdit_, FormatDouble(CameraCentreX(workingPreset_.camera)));
        SetControlText(centreYEdit_, FormatDouble(CameraCentreY(workingPreset_.camera)));
        UpdateCoordinatesEdit();
        dragStart_ = current;
        return 0;
    }
    if (message == WM_LBUTTONUP) {
        draggingPreview_ = false;
        ReleaseCapture();
        return 0;
    }
    if (message == WM_MOUSEWHEEL) {
        RECT client{};
        GetClientRect(window, &client);
        POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ScreenToClient(window, &point);
        const double nx = (static_cast<double>(point.x) / std::max(1L, client.right)) * 2.0 - 1.0;
        const double ny = 1.0 - (static_cast<double>(point.y) / std::max(1L, client.bottom)) * 2.0;
        const double steps = static_cast<double>(GET_WHEEL_DELTA_WPARAM(wParam)) / WHEEL_DELTA;
        previewAnimation_.ZoomAt(nx, ny, steps, static_cast<double>(std::max(1L, client.right)) / std::max(1L, client.bottom));
        workingPreset_.camera = previewAnimation_.Camera();
        previewChangesPending_ = true;
        SetControlText(centreXEdit_, FormatDouble(CameraCentreX(workingPreset_.camera)));
        SetControlText(centreYEdit_, FormatDouble(CameraCentreY(workingPreset_.camera)));
        SetControlText(scaleEdit_, FormatDouble(workingPreset_.camera.scale));
        UpdateCoordinatesEdit();
        return 0;
    }
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        if (!previewRenderer_.IsReady()) {
            RECT client{};
            GetClientRect(window, &client);
            FillRect(dc, &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(225, 225, 225));
            HFONT previous = static_cast<HFONT>(SelectObject(dc, uiFont_));
            const std::wstring messageText = L"GPU preview unavailable. The wallpaper will use its static fallback.\nSee the log for shader details.";
            RECT textRect = client;
            InflateRect(&textRect, -32, -32);
            DrawTextW(dc, messageText.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_WORDBREAK | DT_NOPREFIX);
            SelectObject(dc, previous);
        }
        EndPaint(window, &paint);
        return 0;
    }
    if (message == WM_ERASEBKGND) {
        if (!previewRenderer_.IsReady()) {
            RECT client{};
            GetClientRect(window, &client);
            FillRect(reinterpret_cast<HDC>(wParam), &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
        }
        return 1;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT AppWindow::HandleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == taskbarCreatedMessage_ && taskbarCreatedMessage_ != 0) {
        trayIcon_.Create(window_, icon_, L"Mandelbrot Live Wallpaper");
        return 0;
    }
    switch (message) {
    case WM_CREATE:
        CreateControls();
        return 0;
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED && settings_.general.minimiseToTray) {
            ShowWindow(window_, SW_HIDE);
            return 0;
        }
        LayoutControls(LOWORD(lParam), HIWORD(lParam));
        return 0;
    case WM_GETMINMAXINFO: {
        auto* info = reinterpret_cast<MINMAXINFO*>(lParam);
        info->ptMinTrackSize.x = 900;
        info->ptMinTrackSize.y = 640;
        return 0;
    }
    case WM_NOTIFY: {
        const auto* header = reinterpret_cast<NMHDR*>(lParam);
        if (header && header->idFrom == MainTab && header->code == TCN_SELCHANGE) {
            ShowSelectedTab();
            return 0;
        }
        break;
    }
    case WM_TIMER:
        if (wParam == TimerId) RenderTick();
        return 0;
    case WM_HSCROLL:
        if (reinterpret_cast<HWND>(lParam) == fpsTrack_ ||
            reinterpret_cast<HWND>(lParam) == renderScaleTrack_ ||
            reinterpret_cast<HWND>(lParam) == iterationsTrack_) {
            SendMessageW(performanceCombo_, CB_SETCURSEL,
                         static_cast<WPARAM>(PerformanceProfile::Custom), 0);
        }
        SyncNumericEditsFromTracks();
        ApplyControlsToWorkingPreset();
        return 0;
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (id == NavigationPreviewButton) SelectPage(0);
        else if (id == NavigationWallpaperButton) SelectPage(1);
        else if (id == NavigationDiagnosticsButton) SelectPage(2);
        else if (id == NavigationSettingsButton) OpenSettings();
        else if (id == NavigationPaletteButton) OpenPaletteEditor();
        else if (id == NavigationControllerButton) OpenQuickController();
        else if (id == NavigationEquationButton) OpenEquationEditor();
        else if (id == PresetLibraryButton) OpenPresetLibrary();
        else if (id == CoordinatesEdit && notification == EN_KILLFOCUS) {
            if (ApplyCoordinatesEdit(true)) {
                previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
                previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
                previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
                previewChangesPending_ = true;
                previewForceRender_ = true;
            }
        }
        else if (id == PresetCombo && notification == CBN_SELCHANGE) LoadSelectedPreset();
        else if ((id == PaletteCombo || id == AnimationCombo) && notification == CBN_SELCHANGE) ApplyControlsToWorkingPreset();
        else if (id == MonitorModeCombo && notification == CBN_SELCHANGE) {
            ApplyControlsToWorkingPreset();
            UpdateMonitorAssignmentControls();
            LoadMonitorAssignmentSelection();
            PopulateMonitorControls();
        }
        else if (id == MonitorCombo && notification == CBN_SELCHANGE) LoadMonitorAssignmentSelection();
        else if (id == PerformanceCombo && notification == CBN_SELCHANGE) ApplyPerformanceProfile();
        else if ((id == IterationsEdit || id == FpsEdit || id == RenderScaleEdit ||
                  id == BrightnessEdit || id == ContrastEdit || id == SaturationEdit ||
                  id == ColourOffsetEdit) && notification == EN_KILLFOCUS) {
            SyncTrackFromNumericEdit(id);
            if (id == IterationsEdit || id == FpsEdit || id == RenderScaleEdit) {
                SendMessageW(performanceCombo_, CB_SETCURSEL,
                             static_cast<WPARAM>(PerformanceProfile::Custom), 0);
            }
            ApplyControlsToWorkingPreset();
        }
        else if (id == SetWallpaperButton) SetWallpaper();
        else if (id == SetStaticWallpaperButton) SetStaticWallpaper();
        else if (id == AddSlideshowButton) AddPreviewToSlideshow();
        else if (id == ManageSlideshowButton) ManageSlideshow();
        else if (id == PaletteEditorButton) OpenPaletteEditor();
        else if (id == EquationEditorButton) OpenEquationEditor();
        else if (id == PauseButton) TogglePause();
        else if (id == ColourCycleButton) ToggleColourCycling();
        else if (id == StopButton) StopWallpaper();
        else if (id == ResetViewButton) {
            if (const Preset* selected = FindPreset(settings_.selectedPresetId)) {
                workingPreset_.camera = selected->camera;
                previewAnimation_.SetPreset(workingPreset_, settings_.general.reducedMotion);
                previewAnimation_.SetColourCyclingEnabled(previewColourCyclingEnabled_);
                previewAnimation_.SetMotionEnabled(zoomMotionEnabled_);
                SetControlText(centreXEdit_, FormatDouble(CameraCentreX(workingPreset_.camera)));
                SetControlText(centreYEdit_, FormatDouble(CameraCentreY(workingPreset_.camera)));
                SetControlText(scaleEdit_, FormatDouble(workingPreset_.camera.scale));
                UpdateCoordinatesEdit();
            }
        }
        else if (id == SaveNewButton) SaveAsNewPreset();
        else if (id == SaveChangesButton) SaveChangesToPreset();
        else if (id == RestoreBuiltInsButton) RestoreBuiltInPresets();
        else if (id == OpenSettingsButton) OpenSettings();
        else if (id == ConfigurePrecisionButton) {
            ApplyControlsToWorkingPreset();
            if (PrecisionDialog::Show(window_, instance_, settings_.performance.precision)) {
                previewChangesPending_ = true;
                SaveSettings();
                UpdateStatus();
            }
        }
        else if (id == DeletePresetButton) DeleteSelectedPreset();
        else if (id == ImportPresetButton) ImportPreset();
        else if (id == ExportPresetButton) ExportPreset();
        else if (id == AssignMonitorButton) AssignPresetToMonitor();
        else if (id == StartupCheck) ToggleStartup();
        else if (id == FullscreenCheck || id == BatteryCheck || id == RemoteCheck || id == ReducedMotionCheck) { ApplyControlsToWorkingPreset(); SaveSettings(); }
        else if (id == OpenLogsButton) OpenLogFolder();
        else if (id == CopyDiagnosticsButton) CopyDiagnostics();
        else if (id == ClearLogsButton) ClearLogs();
        else if (id == OpenControllerButton) OpenQuickController();
        else if (id == RenderHighResButton) OpenHighResRenderDialog();
        else if (id == QuickControllerCommands::ApplySettingsLive) SetWallpaper();
        else if (id == QuickControllerCommands::SaveImage) AddPreviewToSlideshow();
        else if (id == QuickControllerCommands::StaticDesktop) SetStaticWallpaper();
        else if (id == QuickControllerCommands::TogglePreviewZoom) TogglePreviewZoomMotion();
        else if (id == QuickControllerCommands::TogglePreviewColours) TogglePreviewColourCycling();
        else if (id == QuickControllerCommands::CopyCoordinates) CopyCoordinates();
        else if (id == QuickControllerCommands::ExitApp) ExitApplication();
        else if (id == QuickControllerCommands::JumpToCoordinates) JumpToCoordinates();
        else if (id == QuickControllerCommands::ToggleDesktopZoom) ToggleDesktopZoomMotion();
        else if (id == QuickControllerCommands::ToggleDesktopColours) ToggleDesktopColourCycling();
        else if (id == QuickControllerCommands::SlideshowDesktop) ApplyPreviewAsSlideshowWallpaper();
        else if (id == QuickControllerCommands::RenderHighRes) OpenHighResRenderDialog();
        else if (id == QuickControllerCommands::LoadPreset) { OpenPresetLibrary(true); }
        else if (id == QuickControllerCommands::Edit) { ShowWindowAndActivate(); SelectPage(0); }
        else if (id == TrayCommands::Open) ShowWindowAndActivate();
        else if (id == TrayCommands::Controller) OpenQuickController();
        else if (id == TrayCommands::Pause) { if (!wallpaperController_.IsPaused()) TogglePause(); }
        else if (id == TrayCommands::Resume) { if (wallpaperController_.IsPaused()) TogglePause(); }
        else if (id == TrayCommands::NextPreset) SelectRelativePreset(1);
        else if (id == TrayCommands::PreviousPreset) SelectRelativePreset(-1);
        else if (id == TrayCommands::Stop) StopWallpaper();
        else if (id == TrayCommands::Startup) { SetCheck(GetDlgItem(window_, StartupCheck), !StartupManager::IsEnabled()); ToggleStartup(); }
        else if (id == TrayCommands::Exit) ExitApplication();
        return 0;
    }
    case TrayIcon::MessageId:
        if (LOWORD(lParam) == WM_LBUTTONDBLCLK || LOWORD(lParam) == NIN_SELECT) ShowWindowAndActivate();
        else if (LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == WM_RBUTTONUP) {
            POINT point{};
            GetCursorPos(&point);
            trayIcon_.ShowMenu(point, wallpaperController_.IsPaused(), StartupManager::IsEnabled());
        }
        return 0;
    case WM_DISPLAYCHANGE:
    case WM_DPICHANGED:
        PopulateMonitorControls();
        wallpaperController_.HandleDisplayChange();
        return 0;
    case WM_WTSSESSION_CHANGE:
    case WM_POWERBROADCAST:
        systemStateMonitor_.HandleMessage(message, wParam, lParam);
        return message == WM_POWERBROADCAST ? TRUE : 0;
    case WM_QUERYENDSESSION:
        if (wallpaperController_.UsingUserStatic()) {
            settings_.staticWallpaper.currentIndex = wallpaperController_.CurrentStaticImageIndex();
        }
        settings_.lastWallpaperRunning = false;
        wallpaperController_.Stop();
        SaveSettings();
        return TRUE;
    case WM_CLOSE:
        if (!exitRequested_ && settings_.general.minimiseToTray) {
            ShowWindow(window_, SW_HIDE);
            return 0;
        }
        ExitApplication();
        return 0;
    case WM_DESTROY:
        KillTimer(window_, TimerId);
        systemStateMonitor_.Unregister();
        trayIcon_.Remove();
        quickController_.Destroy();
        previewRenderer_.Shutdown();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window_, message, wParam, lParam);
}

LRESULT CALLBACK AppWindow::WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<AppWindow*>(create->lpCreateParams);
        self->window_ = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->HandleMessage(message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK AppWindow::PreviewProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<AppWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<AppWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->HandlePreviewMessage(window, message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
}

int AppWindow::SelectedComboIndex(HWND combo) const {
    return static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
}

std::wstring AppWindow::ToWide(const std::string& text) {
    if (text.empty()) return {};

    DWORD flags = MB_ERR_INVALID_CHARS;
    int length = MultiByteToWideChar(
        CP_UTF8, flags, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        // Replace malformed UTF-8 sequences instead of narrowing bytes into wchar_t.
        flags = 0;
        length = MultiByteToWideChar(
            CP_UTF8, flags, text.data(), static_cast<int>(text.size()), nullptr, 0);
    }
    if (length <= 0) return {};

    std::wstring wide(static_cast<std::size_t>(length), L'\0');
    if (MultiByteToWideChar(
            CP_UTF8, flags, text.data(), static_cast<int>(text.size()), wide.data(), length) <= 0) {
        return {};
    }
    return wide;
}

std::string AppWindow::ToUtf8(const std::wstring& text) {
    if (text.empty()) return {};

    DWORD flags = WC_ERR_INVALID_CHARS;
    int length = WideCharToMultiByte(
        CP_UTF8, flags, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        // Replace malformed UTF-16 sequences instead of narrowing wchar_t values into char.
        flags = 0;
        length = WideCharToMultiByte(
            CP_UTF8, flags, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    }
    if (length <= 0) return {};

    std::string utf8(static_cast<std::size_t>(length), '\0');
    if (WideCharToMultiByte(
            CP_UTF8, flags, text.data(), static_cast<int>(text.size()), utf8.data(), length, nullptr, nullptr) <= 0) {
        return {};
    }
    return utf8;
}

std::string AppWindow::ReadControlText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');
    if (length > 0) GetWindowTextW(control, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return ToUtf8(text);
}

void AppWindow::SetControlText(HWND control, const std::string& text) {
    SetWindowTextW(control, ToWide(text).c_str());
}

std::string AppWindow::FormatDouble(double value, int precision) {
    std::ostringstream stream;
    stream << std::setprecision(precision) << value;
    return stream.str();
}

bool AppWindow::ReadTextFile(const std::wstring& path, std::string& text, std::string& error) {
    std::ifstream stream(std::filesystem::path(path), std::ios::binary);
    if (!stream) { error = "The selected file could not be opened."; return false; }
    stream.seekg(0, std::ios::end);
    const auto size = stream.tellg();
    if (size < 0 || size > static_cast<std::streamoff>(256 * 1024)) { error = "Preset files must be smaller than 256 KiB."; return false; }
    stream.seekg(0, std::ios::beg);
    text.assign(static_cast<std::size_t>(size), '\0');
    if (!text.empty()) stream.read(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream && !text.empty()) { error = "The selected file could not be read completely."; return false; }
    return true;
}

bool AppWindow::WriteTextFile(const std::wstring& path, const std::string& text, std::string& error) {
    std::ofstream stream(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!stream) { error = "The preset file could not be created."; return false; }
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream) { error = "The preset file could not be written completely."; return false; }
    return true;
}

#endif

} // namespace mw
