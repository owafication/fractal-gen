#include "App/SettingsDialog.h"
#include "App/DialogSupport.h"
#include "App/PrecisionDialog.h"
#include "App/AdaptivePerformanceDialog.h"
#include "WindowsIntegration/DisplayManager.h"

#ifdef _WIN32
#include <commctrl.h>
#include <commdlg.h>
#endif

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace mw {

#ifdef _WIN32
namespace {

constexpr wchar_t kSettingsClass[] = L"MandelbrotLiveWallpaperSettingsDialog";

enum Id : int {
    MaximumZoomEdit = 5001,
    MaximumIterationsEdit,
    PerformanceProfileCombo,
    FrameRateEdit,
    RenderScaleEdit,
    ZoomRestartCombo,
    AntiAliasingCombo,
    ResumeDelayEdit,
    SmoothCheck,
    InteriorButton,
    BackgroundButton,
    PrecisionButton,
    AdaptiveButton,
    StartWindowsCheck,
    StartWallpaperCheck,
    MinimiseTrayCheck,
    FullscreenCheck,
    BatteryPauseCheck,
    RemotePauseCheck,
    DesktopHiddenCheck,
    LockedCheck,
    ReduceBatteryCheck,
    ReducedMotionCheck,
    MonitorModeCombo,
    MonitorCombo,
    MonitorPresetCombo,
    ApplyAssignmentButton,
    JourneyWaypointsEdit,
    OkButton,
    CancelButton,
};

struct DialogState {
    HWND owner{nullptr};
    HWND window{nullptr};
    HINSTANCE instance{nullptr};
    AppSettings* settings{nullptr};
    Preset* preset{nullptr};
    const std::vector<Preset>* presets{nullptr};
    std::vector<DisplayInfo> displays;
    HFONT font{nullptr};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    bool accepted{false};
    bool done{false};
    Colour interior;
    Colour background;
    PrecisionSettings precision;
    AdaptivePerformanceSettings adaptive;
};

HWND Add(HWND parent, HINSTANCE instance, const wchar_t* cls, const wchar_t* text,
         DWORD style, int id, int x, int y, int width, int height, HFONT font,
         DWORD exStyle = 0) {
    const bool isEdit = cls && std::wcscmp(cls, WC_EDITW) == 0;
    HWND control = CreateWindowExW(exStyle | (isEdit ? WS_EX_CLIENTEDGE : 0), cls, text,
                                   WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style),
                                   x, y, width, height, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   instance, nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

void SetCheck(HWND window, int id, bool checked) {
    SendMessageW(GetDlgItem(window, id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool Checked(HWND window, int id) {
    return SendMessageW(GetDlgItem(window, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                            result.data(), size, nullptr, nullptr) != size) return {};
    return result;
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), result.data(), size) != size) return {};
    return result;
}

COLORREF ToColorRef(const Colour& colour) {
    return RGB(static_cast<BYTE>(std::lround(std::clamp(colour.r, 0.0F, 1.0F) * 255.0F)),
               static_cast<BYTE>(std::lround(std::clamp(colour.g, 0.0F, 1.0F) * 255.0F)),
               static_cast<BYTE>(std::lround(std::clamp(colour.b, 0.0F, 1.0F) * 255.0F)));
}

Colour FromColorRef(COLORREF colour) {
    return {GetRValue(colour) / 255.0F, GetGValue(colour) / 255.0F,
            GetBValue(colour) / 255.0F, 1.0F};
}

void PickColour(DialogState& state, bool interior) {
    static COLORREF customColours[16]{};
    CHOOSECOLORW chooser{};
    chooser.lStructSize = sizeof(chooser);
    chooser.hwndOwner = state.window;
    chooser.rgbResult = ToColorRef(interior ? state.interior : state.background);
    chooser.lpCustColors = customColours;
    chooser.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColorW(&chooser)) {
        if (interior) state.interior = FromColorRef(chooser.rgbResult);
        else state.background = FromColorRef(chooser.rgbResult);
    }
}

std::wstring ReadText(HWND window, int id) {
    const int length = GetWindowTextLengthW(GetDlgItem(window, id));
    if (length <= 0) return {};
    std::wstring value(static_cast<std::size_t>(length + 1), L'\0');
    GetWindowTextW(GetDlgItem(window, id), value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

void SetText(HWND window, int id, const std::wstring& value) {
    SetWindowTextW(GetDlgItem(window, id), value.c_str());
}

int ReadInt(HWND window, int id, int fallback) {
    try { return std::stoi(ReadText(window, id)); } catch (...) { return fallback; }
}

double ReadDouble(HWND window, int id, double fallback) {
    try { return std::stod(ReadText(window, id)); } catch (...) { return fallback; }
}

void UpdateMonitorControlState(DialogState& state) {
    const int mode = static_cast<int>(SendMessageW(GetDlgItem(state.window, MonitorModeCombo), CB_GETCURSEL, 0, 0));
    const bool independent = mode == static_cast<int>(MonitorMode::Independent);
    EnableWindow(GetDlgItem(state.window, MonitorCombo), independent);
    EnableWindow(GetDlgItem(state.window, MonitorPresetCombo), independent);
    EnableWindow(GetDlgItem(state.window, ApplyAssignmentButton), independent);
}

void PopulateMonitorAssignment(DialogState& state) {
    HWND monitor = GetDlgItem(state.window, MonitorCombo);
    HWND preset = GetDlgItem(state.window, MonitorPresetCombo);
    const int monitorIndex = static_cast<int>(SendMessageW(monitor, CB_GETCURSEL, 0, 0));
    if (monitorIndex < 0 || monitorIndex >= static_cast<int>(state.displays.size())) return;
    const std::string key = WideToUtf8(state.displays[static_cast<std::size_t>(monitorIndex)].deviceName);
    const auto found = state.settings->monitorPresetAssignments.find(key);
    const std::string assignedId = found == state.settings->monitorPresetAssignments.end()
        ? state.settings->selectedPresetId : found->second;
    int selected = 0;
    for (std::size_t index = 0; index < state.presets->size(); ++index) {
        if ((*state.presets)[index].id == assignedId) selected = static_cast<int>(index);
    }
    SendMessageW(preset, CB_SETCURSEL, selected, 0);
}

void ApplyMonitorAssignment(DialogState& state) {
    const int monitorIndex = static_cast<int>(SendMessageW(GetDlgItem(state.window, MonitorCombo), CB_GETCURSEL, 0, 0));
    const int presetIndex = static_cast<int>(SendMessageW(GetDlgItem(state.window, MonitorPresetCombo), CB_GETCURSEL, 0, 0));
    if (monitorIndex < 0 || monitorIndex >= static_cast<int>(state.displays.size()) ||
        presetIndex < 0 || presetIndex >= static_cast<int>(state.presets->size())) return;
    const std::string key = WideToUtf8(state.displays[static_cast<std::size_t>(monitorIndex)].deviceName);
    state.settings->monitorPresetAssignments[key] = (*state.presets)[static_cast<std::size_t>(presetIndex)].id;
}

void ApplyProfileDefaults(DialogState& state) {
    const int selected = static_cast<int>(SendMessageW(GetDlgItem(state.window, PerformanceProfileCombo), CB_GETCURSEL, 0, 0));
    if (selected < 0 || selected >= static_cast<int>(PerformanceProfile::Custom)) return;
    const PerformanceSettings defaults = SettingsForProfile(static_cast<PerformanceProfile>(selected));
    SetText(state.window, FrameRateEdit, std::to_wstring(defaults.maximumFrameRate));
    SetText(state.window, RenderScaleEdit, std::to_wstring(static_cast<int>(std::lround(defaults.renderScale * 100.0))));
    SetText(state.window, MaximumIterationsEdit, std::to_wstring(defaults.maximumIterations));
    SendMessageW(GetDlgItem(state.window, AntiAliasingCombo), CB_SETCURSEL, defaults.antiAliasingLevel - 1, 0);
    SetCheck(state.window, BatteryPauseCheck, defaults.pauseOnBattery);
}

void Save(DialogState& state) {
    state.preset->maximumZoom = ReadDouble(state.window, MaximumZoomEdit, state.preset->maximumZoom);
    state.preset->maximumIterations = ReadInt(state.window, MaximumIterationsEdit, state.preset->maximumIterations);
    state.settings->performance.maximumIterations = state.preset->maximumIterations;
    state.settings->performance.maximumFrameRate = ReadInt(state.window, FrameRateEdit, state.settings->performance.maximumFrameRate);
    state.settings->performance.renderScale = ReadDouble(state.window, RenderScaleEdit,
                                                          state.settings->performance.renderScale * 100.0) / 100.0;

    const int profile = static_cast<int>(SendMessageW(GetDlgItem(state.window, PerformanceProfileCombo), CB_GETCURSEL, 0, 0));
    state.settings->performance.profile = profile >= 0 ? static_cast<PerformanceProfile>(profile) : PerformanceProfile::Custom;
    const int restart = static_cast<int>(SendMessageW(GetDlgItem(state.window, ZoomRestartCombo), CB_GETCURSEL, 0, 0));
    state.preset->zoomRestartBehaviour = restart == 1 ? ZoomRestartBehaviour::PingPong : ZoomRestartBehaviour::Restart;
    const int aa = static_cast<int>(SendMessageW(GetDlgItem(state.window, AntiAliasingCombo), CB_GETCURSEL, 0, 0));
    state.settings->performance.antiAliasingLevel = std::clamp(aa + 1, 1, 4);
    state.settings->performance.resumeDelayMs = ReadInt(state.window, ResumeDelayEdit, state.settings->performance.resumeDelayMs);

    state.preset->smoothColouring = Checked(state.window, SmoothCheck);
    state.preset->interiorColour = state.interior;
    state.preset->backgroundColour = state.background;
    state.settings->performance.precision = state.precision;
    state.settings->performance.adaptive = state.adaptive;

    state.settings->general.startWithWindows = Checked(state.window, StartWindowsCheck);
    state.settings->general.startWallpaperOnLaunch = Checked(state.window, StartWallpaperCheck);
    state.settings->general.minimiseToTray = Checked(state.window, MinimiseTrayCheck);
    state.settings->general.reducedMotion = Checked(state.window, ReducedMotionCheck);
    state.settings->performance.pauseWhenFullscreen = Checked(state.window, FullscreenCheck);
    state.settings->performance.pauseOnBattery = Checked(state.window, BatteryPauseCheck);
    state.settings->performance.pauseDuringRemoteDesktop = Checked(state.window, RemotePauseCheck);
    state.settings->performance.pauseWhenDesktopHidden = Checked(state.window, DesktopHiddenCheck);
    state.settings->performance.pauseWhenLocked = Checked(state.window, LockedCheck);
    state.settings->performance.reduceQualityOnBattery = Checked(state.window, ReduceBatteryCheck);

    const int monitorMode = static_cast<int>(SendMessageW(GetDlgItem(state.window, MonitorModeCombo), CB_GETCURSEL, 0, 0));
    state.settings->monitorMode = monitorMode >= 0 ? static_cast<MonitorMode>(monitorMode) : MonitorMode::Mirror;
    ApplyMonitorAssignment(state);

    const std::wstring waypoints = ReadText(state.window, JourneyWaypointsEdit);
    state.preset->automaticJourneyWaypoints = WideToUtf8(waypoints);

    ValidateAndNormalise(*state.preset);
    ValidateAndNormalise(*state.settings);
}

LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<DialogState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<DialogState*>(create->lpCreateParams);
        state->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(window, message, wParam, lParam);

    if (message == WM_CREATE) {
        state->font = CreateResponsiveDialogFont(state->dpi);
        constexpr int leftX = 14;
        constexpr int centreX = 326;
        constexpr int rightX = 638;
        constexpr int groupWidth = 300;
        Add(window, state->instance, WC_BUTTONW, L"Graphics", BS_GROUPBOX, 0,
            leftX, 12, groupWidth, 610, state->font);
        Add(window, state->instance, WC_BUTTONW, L"System behaviour", BS_GROUPBOX, 0,
            centreX, 12, groupWidth, 610, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Monitors and journey", BS_GROUPBOX, 0,
            rightX, 12, groupWidth, 610, state->font);

        int y = 42;
        auto labelEdit = [&](const wchar_t* label, int id, const std::wstring& value) {
            Add(window, state->instance, WC_STATICW, label, SS_LEFT, 0, leftX + 12, y + 4, 125, 22, state->font);
            Add(window, state->instance, WC_EDITW, value.c_str(), ES_AUTOHSCROLL | WS_TABSTOP,
                id, leftX + 142, y, 140, 26, state->font);
            y += 36;
        };
        std::wostringstream zoom;
        zoom << std::setprecision(12) << state->preset->maximumZoom;
        labelEdit(L"Maximum zoom", MaximumZoomEdit, zoom.str());
        labelEdit(L"Iterations", MaximumIterationsEdit, std::to_wstring(state->preset->maximumIterations));

        Add(window, state->instance, WC_STATICW, L"Performance", SS_LEFT, 0, leftX + 12, y + 4, 125, 22, state->font);
        HWND profile = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                           PerformanceProfileCombo, leftX + 142, y, 140, 150, state->font);
        for (const wchar_t* item : {L"Battery Saver", L"Balanced", L"High Quality", L"Custom"})
            SendMessageW(profile, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        SendMessageW(profile, CB_SETCURSEL, static_cast<WPARAM>(state->settings->performance.profile), 0);
        y += 36;
        labelEdit(L"Frame-rate limit", FrameRateEdit, std::to_wstring(state->settings->performance.maximumFrameRate));
        labelEdit(L"Render scale %", RenderScaleEdit,
                  std::to_wstring(static_cast<int>(std::lround(state->settings->performance.renderScale * 100.0))));

        Add(window, state->instance, WC_STATICW, L"At max zoom", SS_LEFT, 0, leftX + 12, y + 4, 125, 22, state->font);
        HWND restart = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                           ZoomRestartCombo, leftX + 142, y, 140, 150, state->font);
        SendMessageW(restart, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Restart"));
        SendMessageW(restart, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Ping-pong"));
        SendMessageW(restart, CB_SETCURSEL,
                     state->preset->zoomRestartBehaviour == ZoomRestartBehaviour::PingPong ? 1 : 0, 0);
        y += 36;

        Add(window, state->instance, WC_STATICW, L"Anti-aliasing", SS_LEFT, 0, leftX + 12, y + 4, 125, 22, state->font);
        HWND aa = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                      AntiAliasingCombo, leftX + 142, y, 140, 150, state->font);
        for (const wchar_t* item : {L"1x", L"2x", L"3x", L"4x"})
            SendMessageW(aa, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        SendMessageW(aa, CB_SETCURSEL, state->settings->performance.antiAliasingLevel - 1, 0);
        y += 40;
        Add(window, state->instance, WC_BUTTONW, L"Configure precision...", BS_PUSHBUTTON | WS_TABSTOP,
            PrecisionButton, leftX + 12, y, 270, 30, state->font); y += 38;
        Add(window, state->instance, WC_BUTTONW, L"Adaptive resource protection...", BS_PUSHBUTTON | WS_TABSTOP,
            AdaptiveButton, leftX + 12, y, 270, 30, state->font); y += 38;
        labelEdit(L"Resume delay ms", ResumeDelayEdit, std::to_wstring(state->settings->performance.resumeDelayMs));
        Add(window, state->instance, WC_BUTTONW, L"Smooth colouring", BS_AUTOCHECKBOX | WS_TABSTOP,
            SmoothCheck, leftX + 12, y, 270, 24, state->font);
        SetCheck(window, SmoothCheck, state->preset->smoothColouring); y += 32;
        Add(window, state->instance, WC_BUTTONW, L"Interior colour...", BS_PUSHBUTTON | WS_TABSTOP,
            InteriorButton, leftX + 12, y, 132, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Background colour...", BS_PUSHBUTTON | WS_TABSTOP,
            BackgroundButton, leftX + 150, y, 132, 30, state->font);

        int systemY = 42;
        auto check = [&](int id, const wchar_t* label, bool value) {
            Add(window, state->instance, WC_BUTTONW, label, BS_AUTOCHECKBOX | WS_TABSTOP,
                id, centreX + 12, systemY, 276, 25, state->font);
            SetCheck(window, id, value);
            systemY += 34;
        };
        check(StartWindowsCheck, L"Start with Windows", state->settings->general.startWithWindows);
        check(StartWallpaperCheck, L"Start wallpaper when app launches", state->settings->general.startWallpaperOnLaunch);
        check(MinimiseTrayCheck, L"Close control window to tray", state->settings->general.minimiseToTray);
        check(FullscreenCheck, L"Pause for full-screen applications", state->settings->performance.pauseWhenFullscreen);
        check(BatteryPauseCheck, L"Pause on battery", state->settings->performance.pauseOnBattery);
        check(RemotePauseCheck, L"Pause in Remote Desktop", state->settings->performance.pauseDuringRemoteDesktop);
        check(DesktopHiddenCheck, L"Pause when desktop is not visible", state->settings->performance.pauseWhenDesktopHidden);
        check(LockedCheck, L"Pause when device is locked", state->settings->performance.pauseWhenLocked);
        check(ReduceBatteryCheck, L"Reduce quality on battery", state->settings->performance.reduceQualityOnBattery);
        check(ReducedMotionCheck, L"Reduced Motion", state->settings->general.reducedMotion);
        Add(window, state->instance, WC_STATICW,
            L"Live/static/slideshow actions are available from the preview hover menu and the movable Quick Controller.",
            SS_LEFT, 0, centreX + 12, systemY + 8, 276, 74, state->font);

        int monitorY = 42;
        Add(window, state->instance, WC_STATICW, L"Wallpaper mode", SS_LEFT, 0,
            rightX + 12, monitorY + 4, 118, 22, state->font);
        HWND monitorMode = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                               MonitorModeCombo, rightX + 136, monitorY, 150, 150, state->font);
        for (const wchar_t* item : {L"Mirror", L"Span", L"Independent"})
            SendMessageW(monitorMode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        SendMessageW(monitorMode, CB_SETCURSEL, static_cast<WPARAM>(state->settings->monitorMode), 0);
        monitorY += 40;

        Add(window, state->instance, WC_STATICW, L"Monitor", SS_LEFT, 0,
            rightX + 12, monitorY + 4, 118, 22, state->font);
        HWND monitorCombo = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                                MonitorCombo, rightX + 136, monitorY, 150, 190, state->font);
        for (const auto& display : state->displays) {
            std::wostringstream description;
            description << display.friendlyName << L" (" << (display.bounds.right - display.bounds.left)
                        << L"x" << (display.bounds.bottom - display.bounds.top) << L")";
            SendMessageW(monitorCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(description.str().c_str()));
        }
        if (!state->displays.empty()) SendMessageW(monitorCombo, CB_SETCURSEL, 0, 0);
        monitorY += 40;

        Add(window, state->instance, WC_STATICW, L"Assigned preset", SS_LEFT, 0,
            rightX + 12, monitorY + 4, 118, 22, state->font);
        HWND monitorPreset = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                                 MonitorPresetCombo, rightX + 136, monitorY, 150, 190, state->font);
        for (const auto& preset : *state->presets) {
            const std::wstring name = Utf8ToWide(preset.name);
            SendMessageW(monitorPreset, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
        }
        monitorY += 40;
        Add(window, state->instance, WC_BUTTONW, L"Apply monitor assignment", BS_PUSHBUTTON | WS_TABSTOP,
            ApplyAssignmentButton, rightX + 12, monitorY, 274, 30, state->font);
        monitorY += 44;
        PopulateMonitorAssignment(*state);
        UpdateMonitorControlState(*state);

        Add(window, state->instance, WC_STATICW,
            L"Ordered Automatic Journey (optional)\r\nX,Y,Scale,TransitionSeconds,HoldSeconds — one destination per line",
            SS_LEFT, 0, rightX + 12, monitorY, 276, 62, state->font);
        monitorY += 64;
        const std::wstring waypointText = Utf8ToWide(state->preset->automaticJourneyWaypoints);
        Add(window, state->instance, WC_EDITW, waypointText.c_str(),
            ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL | WS_TABSTOP,
            JourneyWaypointsEdit, rightX + 12, monitorY, 274, 190, state->font, WS_EX_CLIENTEDGE);

        Add(window, state->instance, WC_BUTTONW, L"&OK", BS_DEFPUSHBUTTON | WS_TABSTOP,
            OkButton, 720, 635, 102, 32, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&Cancel", BS_PUSHBUTTON | WS_TABSTOP,
            CancelButton, 832, 635, 102, 32, state->font);
        SendMessageW(window, DM_SETDEFID, OkButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 680, 500);
        state->layout.Focus(GetDlgItem(window, MaximumZoomEdit));
        return 0;
    }

    if (message == WM_GETMINMAXINFO) {
        state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;
    }
    if (message == WM_SIZE) { state->layout.OnSize(); return 0; }
    if ((message == WM_VSCROLL || message == WM_HSCROLL) && lParam == 0) {
        if (state->layout.OnScroll(message, wParam)) return 0;
    }
    if (message == WM_MOUSEWHEEL && state->layout.OnMouseWheel(wParam)) return 0;
    if (message == WM_DPICHANGED) {
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

    if (message == WM_COMMAND) {
        const int id = LOWORD(wParam);
        if (id == InteriorButton) PickColour(*state, true);
        else if (id == BackgroundButton) PickColour(*state, false);
        else if (id == PrecisionButton) PrecisionDialog::Show(state->window, state->instance, state->precision);
        else if (id == AdaptiveButton) AdaptivePerformanceDialog::Show(state->window, state->instance, state->adaptive);
        else if (id == PerformanceProfileCombo && HIWORD(wParam) == CBN_SELCHANGE) ApplyProfileDefaults(*state);
        else if (id == MonitorModeCombo && HIWORD(wParam) == CBN_SELCHANGE) UpdateMonitorControlState(*state);
        else if (id == MonitorCombo && HIWORD(wParam) == CBN_SELCHANGE) PopulateMonitorAssignment(*state);
        else if (id == ApplyAssignmentButton) ApplyMonitorAssignment(*state);
        else if (id == OkButton) {
            Save(*state);
            state->accepted = true;
            DestroyWindow(window);
        } else if (id == CancelButton) {
            DestroyWindow(window);
        }
        return 0;
    }
    if (message == WM_CLOSE) {
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_DESTROY) {
        RememberDialogPlacement(window, kSettingsClass, state->dpi);
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

} // namespace

bool SettingsDialog::Show(HWND owner, HINSTANCE instance, AppSettings& settings, Preset& preset,
                          const std::vector<Preset>& presets) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kSettingsClass;
    RegisterClassExW(&windowClass);

    DialogState state;
    state.owner = owner;
    state.instance = instance;
    state.settings = &settings;
    state.preset = &preset;
    state.presets = &presets;
    state.displays = DisplayManager::Enumerate();
    state.interior = preset.interiorColour;
    state.background = preset.backgroundColour;
    state.precision = settings.performance.precision;
    state.adaptive = settings.performance.adaptive;

    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 970, 720, state.dpi, kSettingsClass);
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kSettingsClass, L"Mandelbrot Settings",
                                  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX |
                                  WS_POPUP | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
                                  dialogRect.left, dialogRect.top,
                                  dialogRect.right - dialogRect.left, dialogRect.bottom - dialogRect.top,
                                  owner, nullptr, instance, &state);
    if (!window) return false;
    EnableWindow(owner, FALSE);

    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, CancelButton, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return state.accepted;
}
#endif

} // namespace mw
