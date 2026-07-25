#include "App/AdaptivePerformanceDialog.h"
#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <algorithm>
#include <cwchar>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>

namespace mw {

#ifdef _WIN32
namespace {

constexpr wchar_t kClassName[] = L"MandelbrotAdaptivePerformanceDialog";

enum Id : int {
    EnabledCheck = 7001,
    LowFpsCheck,
    MinimumFpsEdit,
    LowFpsSecondsEdit,
    HighCpuCheck,
    MaximumCpuEdit,
    HighCpuSecondsEdit,
    HighMemoryCheck,
    MaximumMemoryEdit,
    HighMemorySecondsEdit,
    ResumeSecondsEdit,
    InvisibleCheck,
    PixelThresholdEdit,
    ColourThresholdEdit,
    DefaultsButton,
    OkButton,
    CancelButton,
};

struct DialogState {
    HWND owner{nullptr};
    HWND window{nullptr};
    HINSTANCE instance{nullptr};
    AdaptivePerformanceSettings* destination{nullptr};
    AdaptivePerformanceSettings working;
    HFONT font{nullptr};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    bool accepted{false};
    bool done{false};
};

HWND Add(HWND parent, HINSTANCE instance, const wchar_t* cls, const wchar_t* text, DWORD style, int id,
         int x, int y, int width, int height, HFONT font, DWORD exStyle = 0) {
    const bool isEdit = cls != nullptr && std::wcscmp(cls, WC_EDITW) == 0;
    HWND control = CreateWindowExW(exStyle | (isEdit ? WS_EX_CLIENTEDGE : 0), cls, text,
                                   WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style), x, y, width, height,
                                   parent, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

void SetCheck(HWND window, int id, bool checked) {
    SendMessageW(GetDlgItem(window, id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool Checked(HWND window, int id) {
    return SendMessageW(GetDlgItem(window, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

std::wstring Number(double value, int precision = 3) {
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    std::wstring result = stream.str();
    while (result.size() > 1 && result.back() == L'0') result.pop_back();
    if (!result.empty() && result.back() == L'.') result.pop_back();
    return result;
}

double ReadDouble(HWND window, int id, double fallback) {
    wchar_t text[64]{};
    GetWindowTextW(GetDlgItem(window, id), text, static_cast<int>(std::size(text)));
    try { return std::stod(text); } catch (...) { return fallback; }
}

int ReadInt(HWND window, int id, int fallback) {
    wchar_t text[64]{};
    GetWindowTextW(GetDlgItem(window, id), text, static_cast<int>(std::size(text)));
    try { return std::stoi(text); } catch (...) { return fallback; }
}

void Populate(HWND window, const AdaptivePerformanceSettings& settings) {
    SetCheck(window, EnabledCheck, settings.enabled);
    SetCheck(window, LowFpsCheck, settings.pauseOnLowFps);
    SetWindowTextW(GetDlgItem(window, MinimumFpsEdit), Number(settings.minimumFramesPerSecond, 1).c_str());
    SetWindowTextW(GetDlgItem(window, LowFpsSecondsEdit), Number(settings.lowFpsSustainMs / 1000.0, 1).c_str());
    SetCheck(window, HighCpuCheck, settings.pauseOnHighCpu);
    SetWindowTextW(GetDlgItem(window, MaximumCpuEdit), Number(settings.maximumProcessCpuPercent, 1).c_str());
    SetWindowTextW(GetDlgItem(window, HighCpuSecondsEdit), Number(settings.highCpuSustainMs / 1000.0, 1).c_str());
    SetCheck(window, HighMemoryCheck, settings.pauseOnHighMemory);
    SetWindowTextW(GetDlgItem(window, MaximumMemoryEdit), std::to_wstring(settings.maximumWorkingSetMb).c_str());
    SetWindowTextW(GetDlgItem(window, HighMemorySecondsEdit), Number(settings.highMemorySustainMs / 1000.0, 1).c_str());
    SetWindowTextW(GetDlgItem(window, ResumeSecondsEdit), Number(settings.resumeStableMs / 1000.0, 1).c_str());
    SetCheck(window, InvisibleCheck, settings.stopWhenVisuallyUnchanged);
    SetWindowTextW(GetDlgItem(window, PixelThresholdEdit), Number(settings.minimumVisiblePixelChange, 3).c_str());
    SetWindowTextW(GetDlgItem(window, ColourThresholdEdit), Number(settings.minimumVisibleColourChange, 5).c_str());
}

void Save(DialogState& state) {
    auto& settings = state.working;
    settings.enabled = Checked(state.window, EnabledCheck);
    settings.pauseOnLowFps = Checked(state.window, LowFpsCheck);
    settings.minimumFramesPerSecond = ReadDouble(state.window, MinimumFpsEdit, settings.minimumFramesPerSecond);
    settings.lowFpsSustainMs = static_cast<int>(ReadDouble(state.window, LowFpsSecondsEdit,
        settings.lowFpsSustainMs / 1000.0) * 1000.0);
    settings.pauseOnHighCpu = Checked(state.window, HighCpuCheck);
    settings.maximumProcessCpuPercent = ReadDouble(state.window, MaximumCpuEdit, settings.maximumProcessCpuPercent);
    settings.highCpuSustainMs = static_cast<int>(ReadDouble(state.window, HighCpuSecondsEdit,
        settings.highCpuSustainMs / 1000.0) * 1000.0);
    settings.pauseOnHighMemory = Checked(state.window, HighMemoryCheck);
    settings.maximumWorkingSetMb = ReadInt(state.window, MaximumMemoryEdit, settings.maximumWorkingSetMb);
    settings.highMemorySustainMs = static_cast<int>(ReadDouble(state.window, HighMemorySecondsEdit,
        settings.highMemorySustainMs / 1000.0) * 1000.0);
    settings.resumeStableMs = static_cast<int>(ReadDouble(state.window, ResumeSecondsEdit,
        settings.resumeStableMs / 1000.0) * 1000.0);
    settings.stopWhenVisuallyUnchanged = Checked(state.window, InvisibleCheck);
    settings.minimumVisiblePixelChange = ReadDouble(state.window, PixelThresholdEdit,
        settings.minimumVisiblePixelChange);
    settings.minimumVisibleColourChange = ReadDouble(state.window, ColourThresholdEdit,
        settings.minimumVisibleColourChange);

    AppSettings validation;
    validation.performance.adaptive = settings;
    ValidateAndNormalise(validation);
    settings = validation.performance.adaptive;
    *state.destination = settings;
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
        Add(window, state->instance, WC_STATICW,
            L"Adaptive protection pauses both live wallpaper and preview after sustained overload. "
            L"Low FPS is the portable signal for GPU saturation; CPU and working-set limits protect the rest of the process.",
            SS_LEFT, 0, 18, 14, 704, 54, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Enable adaptive resource protection",
            BS_AUTOCHECKBOX | WS_TABSTOP, EnabledCheck, 18, 72, 340, 25, state->font);

        Add(window, state->instance, WC_BUTTONW, L"Sustained overload pause", BS_GROUPBOX, 0,
            18, 108, 704, 265, state->font);
        SetCheck(window, EnabledCheck, state->working.enabled);

        Add(window, state->instance, WC_BUTTONW, L"Pause when FPS remains below",
            BS_AUTOCHECKBOX | WS_TABSTOP, LowFpsCheck, 34, 139, 260, 25, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, MinimumFpsEdit,
            304, 136, 80, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"FPS for", SS_LEFT, 0, 395, 140, 55, 22, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, LowFpsSecondsEdit,
            452, 136, 80, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"seconds", SS_LEFT, 0, 540, 140, 80, 22, state->font);

        Add(window, state->instance, WC_BUTTONW, L"Pause when process CPU exceeds",
            BS_AUTOCHECKBOX | WS_TABSTOP, HighCpuCheck, 34, 181, 270, 25, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, MaximumCpuEdit,
            304, 178, 80, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"% for", SS_LEFT, 0, 395, 182, 55, 22, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, HighCpuSecondsEdit,
            452, 178, 80, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"seconds", SS_LEFT, 0, 540, 182, 80, 22, state->font);

        Add(window, state->instance, WC_BUTTONW, L"Pause when working memory exceeds",
            BS_AUTOCHECKBOX | WS_TABSTOP, HighMemoryCheck, 34, 223, 270, 25, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP, MaximumMemoryEdit,
            304, 220, 80, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"MB for", SS_LEFT, 0, 395, 224, 55, 22, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, HighMemorySecondsEdit,
            452, 220, 80, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"seconds", SS_LEFT, 0, 540, 224, 80, 22, state->font);

        Add(window, state->instance, WC_STATICW, L"Resume after CPU and memory are stable for",
            SS_LEFT, 0, 34, 269, 320, 22, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, ResumeSecondsEdit,
            360, 266, 80, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"seconds", SS_LEFT, 0, 450, 270, 80, 22, state->font);
        Add(window, state->instance, WC_STATICW,
            L"A resume is a bounded probe. If the same limit is exceeded again, the app pauses again instead of entering a restart loop.",
            SS_LEFT, 0, 34, 307, 660, 46, state->font);

        Add(window, state->instance, WC_BUTTONW, L"Stop invisible equation work", BS_GROUPBOX, 0,
            18, 388, 704, 156, state->font);
        Add(window, state->instance, WC_BUTTONW,
            L"Skip equation rendering until camera or colour movement becomes visible",
            BS_AUTOCHECKBOX | WS_TABSTOP, InvisibleCheck, 34, 419, 630, 25, state->font);
        Add(window, state->instance, WC_STATICW, L"Minimum camera movement", SS_LEFT, 0,
            34, 461, 220, 22, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, PixelThresholdEdit,
            260, 458, 90, 26, state->font);
        Add(window, state->instance, WC_STATICW, L"screen pixels", SS_LEFT, 0,
            360, 462, 120, 22, state->font);
        Add(window, state->instance, WC_STATICW, L"Minimum colour-offset change", SS_LEFT, 0,
            34, 501, 220, 22, state->font);
        Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP, ColourThresholdEdit,
            260, 498, 90, 26, state->font);
        Add(window, state->instance, WC_STATICW,
            L"The renderer compares against the last rendered frame, so sub-threshold movement accumulates and is rendered once visible.",
            SS_LEFT, 0, 360, 493, 340, 42, state->font);

        Add(window, state->instance, WC_BUTTONW, L"Restore safe defaults", BS_PUSHBUTTON | WS_TABSTOP,
            DefaultsButton, 18, 562, 170, 32, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&OK", BS_DEFPUSHBUTTON | WS_TABSTOP,
            OkButton, 498, 562, 105, 32, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&Cancel", BS_PUSHBUTTON | WS_TABSTOP,
            CancelButton, 617, 562, 105, 32, state->font);
        Populate(window, state->working);
        SendMessageW(window, DM_SETDEFID, OkButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 560, 420);
        state->layout.Focus(GetDlgItem(window, EnabledCheck));
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
        if (id == DefaultsButton) {
            state->working = AdaptivePerformanceSettings{};
            Populate(window, state->working);
        } else if (id == OkButton) {
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
        RememberDialogPlacement(window, kClassName, state->dpi);
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

bool AdaptivePerformanceDialog::Show(HWND owner, HINSTANCE instance, AdaptivePerformanceSettings& settings) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kClassName;
    RegisterClassExW(&windowClass);

    DialogState state;
    state.owner = owner;
    state.instance = instance;
    state.destination = &settings;
    state.working = settings;

    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 760, 650, state.dpi, kClassName);
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kClassName, L"Adaptive Resource Protection",
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
