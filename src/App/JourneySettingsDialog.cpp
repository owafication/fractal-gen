#include "App/JourneySettingsDialog.h"
#include "App/DialogSupport.h"
#include "Core/Animation.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <string>
#include <utility>

namespace mw {
#ifdef _WIN32
namespace {
constexpr wchar_t kJourneySettingsClass[] = L"MandelbrotJourneySettingsDialog";
enum Id : int { WaypointsEdit = 9701, ApplyButton, OkButton, CancelButton };

struct State {
    HWND owner{};
    HWND window{};
    HINSTANCE instance{};
    Preset* preset{};
    Preset originalPreset;
    HFONT font{};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    std::function<void()> onChanged;
    bool accepted{false};
    bool done{false};
};

std::wstring Wide(const std::string& value) {
    if (value.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), result.data(), count);
    return result;
}

std::string Utf8(const std::wstring& value) {
    if (value.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                                          static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                        static_cast<int>(value.size()), result.data(), count, nullptr, nullptr);
    return result;
}

std::wstring Read(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) return {};
    std::wstring value(static_cast<std::size_t>(length) + 1U, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

bool Apply(State& state, bool showError) {
    const std::string script = Utf8(Read(GetDlgItem(state.window, WaypointsEdit)));
    if (script.size() > 32768U) {
        if (showError) MessageBoxW(state.window, L"Journey text is limited to 32,768 UTF-8 bytes.",
                                   L"Journey Settings", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (!script.empty() && !AnimationController::HasValidJourneyScriptTargets(script)) {
        if (showError) MessageBoxW(state.window,
            L"No valid waypoints were found. Use one row per destination:\r\nX,Y,Scale,TransitionSeconds,HoldSeconds",
            L"Journey Settings", MB_OK | MB_ICONWARNING);
        return false;
    }
    state.preset->automaticJourneyWaypoints = script;
    ValidateAndNormalise(*state.preset);
    if (state.onChanged) state.onChanged();
    return true;
}

LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<State*>(create->lpCreateParams);
        state->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(window, message, wParam, lParam);
    if (message == WM_CREATE) {
        state->font = CreateResponsiveDialogFont(state->dpi);
        auto add = [&](const wchar_t* cls, const wchar_t* text, DWORD style, int id,
                       int x, int y, int width, int height, DWORD exStyle = 0) {
            HWND control = CreateWindowExW(exStyle, cls, text,
                WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style), x, y, width, height,
                window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), state->instance, nullptr);
            if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(state->font), TRUE);
            return control;
        };
        add(WC_STATICW,
            L"Structured route: transition from the current view to each coordinate, hold, then continue to the next row. The final row loops back to the first.",
            SS_LEFT, 0, 14, 14, 650, 52);
        add(WC_STATICW, L"X,Y,Scale,TransitionSeconds,HoldSeconds", SS_LEFT, 0, 14, 70, 650, 22);
        add(WC_EDITW, Wide(state->preset->automaticJourneyWaypoints).c_str(),
            ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
            WaypointsEdit, 14, 96, 650, 330, WS_EX_CLIENTEDGE);
        SendMessageW(GetDlgItem(window, WaypointsEdit), EM_SETLIMITTEXT, 32768, 0);
        add(WC_BUTTONW, L"&Apply", BS_PUSHBUTTON | WS_TABSTOP, ApplyButton, 326, 442, 104, 32);
        add(WC_BUTTONW, L"&OK", BS_DEFPUSHBUTTON | WS_TABSTOP, OkButton, 438, 442, 104, 32);
        add(WC_BUTTONW, L"&Cancel", BS_PUSHBUTTON | WS_TABSTOP, CancelButton, 550, 442, 114, 32);
        SendMessageW(window, DM_SETDEFID, OkButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 520, 380);
        state->layout.Focus(GetDlgItem(window, WaypointsEdit));
        return 0;
    }
    if (message == WM_GETMINMAXINFO) { state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam)); return 0; }
    if (message == WM_SIZE) { state->layout.OnSize(); return 0; }
    if ((message == WM_VSCROLL || message == WM_HSCROLL) && lParam == 0 && state->layout.OnScroll(message, wParam)) return 0;
    if (message == WM_MOUSEWHEEL && state->layout.OnMouseWheel(wParam)) return 0;
    if (message == WM_COMMAND) {
        const int id = LOWORD(wParam);
        if (id == ApplyButton) {
            (void)Apply(*state, true);
        } else if (id == OkButton) {
            if (Apply(*state, true)) { state->accepted = true; DestroyWindow(window); }
        } else if (id == CancelButton) DestroyWindow(window);
        return 0;
    }
    if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
    if (message == WM_DESTROY) {
        if (!state->accepted) {
            *state->preset = state->originalPreset;
            if (state->onChanged) state->onChanged();
        }
        RememberDialogPlacement(window, kJourneySettingsClass, state->dpi);
        state->layout.Shutdown();
        if (state->font) DeleteObject(state->font);
        state->font = nullptr;
        state->done = true;
        if (IsWindow(state->owner)) {
            EnableWindow(state->owner, TRUE);
            SetForegroundWindow(state->owner);
        }
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

bool JourneySettingsDialog::Show(HWND owner, HINSTANCE instance, Preset& preset,
                                 std::function<void()> onChanged) {
    WNDCLASSEXW cls{};
    cls.cbSize = sizeof(cls);
    cls.lpfnWndProc = Procedure;
    cls.hInstance = instance;
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cls.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    cls.lpszClassName = kJourneySettingsClass;
    RegisterClassExW(&cls);

    State state;
    state.owner = owner;
    state.instance = instance;
    state.preset = &preset;
    state.originalPreset = preset;
    state.onChanged = std::move(onChanged);
    state.dpi = DialogDpi(owner);
    const RECT rect = ResponsiveDialogRect(owner, 700, 530, state.dpi, kJourneySettingsClass);
    HWND window = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_CONTROLPARENT, kJourneySettingsClass, L"Journey Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX |
        WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
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
