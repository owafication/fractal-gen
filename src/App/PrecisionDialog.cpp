#include "App/PrecisionDialog.h"
#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <algorithm>
#include <array>
#include <cwchar>

namespace mw {
#ifdef _WIN32
namespace {

constexpr wchar_t kClassName[] = L"MandelbrotPrecisionDialog";

enum Id : int {
    ModeCombo = 8001,
    Float64Check,
    SplitCheck,
    PerturbCheck,
    ArbitraryCheck,
    FallbackCheck,
    BitsCombo,
    ModeDescription,
    OkButton,
    CancelButton,
};

struct State {
    HWND owner{};
    HWND window{};
    HINSTANCE instance{};
    PrecisionSettings value;
    HFONT font{};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    bool accepted{};
    bool done{};
};

const wchar_t* DescriptionForMode(PrecisionMode mode) {
    switch (mode) {
    case PrecisionMode::Automatic:
        return L"Automatic chooses the fastest enabled strategy that can represent the current zoom. "
               L"It begins with float32, then moves through native float64, split coordinates and perturbation as needed.";
    case PrecisionMode::Float32:
        return L"GPU float32 is the fastest and most compatible mode. It is suitable for normal views, but blockiness and coordinate collapse appear at deep zoom levels.";
    case PrecisionMode::Float64:
        return L"Native GPU float64 uses double-precision shader arithmetic. It extends useful zoom depth substantially, but many consumer OpenGL drivers do not expose the required capability or run it slowly.";
    case PrecisionMode::SplitFloat:
        return L"Split high/low float stores each camera coordinate as two 32-bit values. It works on more GPUs than native float64 and improves panning precision, but it is not unlimited.";
    case PrecisionMode::Perturbation:
        return L"Perturbation computes one double-precision reference orbit on the CPU and lets the GPU calculate nearby pixel deltas. It is efficient for deep analytic Mandelbrot-style zooms.";
    case PrecisionMode::ArbitraryPrecisionPerturbation:
        return L"Arbitrary-reference perturbation computes the reference orbit at 128, 256 or 512 bits on the CPU, then renders nearby deltas on the GPU. It reaches the deepest supported zooms but has the highest setup cost.";
    }
    return L"";
}

void UpdateDescription(State& state) {
    const int index = static_cast<int>(SendMessageW(GetDlgItem(state.window, ModeCombo), CB_GETCURSEL, 0, 0));
    const auto mode = static_cast<PrecisionMode>(std::clamp(index, 0, 5));
    SetWindowTextW(GetDlgItem(state.window, ModeDescription), DescriptionForMode(mode));
}

HWND Add(State& state, const wchar_t* className, const wchar_t* text, DWORD style, int id,
         int x, int y, int width, int height) {
    const bool edit = className && std::wcscmp(className, WC_EDITW) == 0;
    HWND control = CreateWindowExW(
        edit ? WS_EX_CLIENTEDGE : 0,
        className,
        text,
        WS_CHILD | WS_VISIBLE | AccessibleControlStyle(className, style),
        x,
        y,
        width,
        height,
        state.window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        state.instance,
        nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
    return control;
}

void SetCheck(HWND window, int id, bool checked) {
    SendMessageW(GetDlgItem(window, id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool Checked(HWND window, int id) {
    return SendMessageW(GetDlgItem(window, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void Save(State& state) {
    const int modeIndex = static_cast<int>(SendMessageW(GetDlgItem(state.window, ModeCombo), CB_GETCURSEL, 0, 0));
    state.value.mode = static_cast<PrecisionMode>(std::clamp(modeIndex, 0, 5));
    state.value.allowFloat64 = Checked(state.window, Float64Check);
    state.value.allowSplitFloat = Checked(state.window, SplitCheck);
    state.value.allowPerturbation = Checked(state.window, PerturbCheck);
    state.value.allowArbitraryPrecision = Checked(state.window, ArbitraryCheck);
    state.value.automaticFallback = Checked(state.window, FallbackCheck);

    const int bitsIndex = static_cast<int>(SendMessageW(GetDlgItem(state.window, BitsCombo), CB_GETCURSEL, 0, 0));
    state.value.arbitraryPrecisionBits = bitsIndex == 0 ? 128 : (bitsIndex == 2 ? 512 : 256);
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
        int y = 18;
        Add(*state, WC_STATICW, L"Precision strategy", SS_LEFT, 0, 18, y + 4, 180, 22);
        HWND mode = Add(*state, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                        ModeCombo, 205, y, 330, 180);
        constexpr std::array<const wchar_t*, 6> modes{
            L"Automatic",
            L"GPU float32",
            L"Native GPU float64",
            L"Split high/low float",
            L"Perturbation - double CPU reference",
            L"Perturbation - arbitrary CPU reference",
        };
        for (const wchar_t* item : modes) {
            SendMessageW(mode, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        }
        SendMessageW(mode, CB_SETCURSEL, static_cast<WPARAM>(state->value.mode), 0);
        y += 48;

        Add(*state, WC_STATICW, L"", SS_LEFT, ModeDescription, 18, y, 612, 66);
        UpdateDescription(*state);
        y += 76;

        Add(*state, WC_STATICW, L"Automatic mode candidates", SS_LEFT, 0, 18, y, 612, 24);
        y += 30;
        const auto addCheck = [&](int id, const wchar_t* text, bool value) {
            Add(*state, WC_BUTTONW, text, BS_AUTOCHECKBOX | WS_TABSTOP, id, 28, y, 602, 24);
            SetCheck(window, id, value);
            y += 30;
        };
        addCheck(Float64Check, L"Allow native GPU double precision when supported", state->value.allowFloat64);
        addCheck(SplitCheck, L"Allow split high/low coordinates", state->value.allowSplitFloat);
        addCheck(PerturbCheck, L"Allow perturbation with a double CPU reference orbit", state->value.allowPerturbation);
        addCheck(ArbitraryCheck, L"Allow arbitrary-precision CPU reference orbit + GPU perturbation",
                 state->value.allowArbitraryPrecision);
        addCheck(FallbackCheck, L"Fall back safely when the selected strategy is unavailable",
                 state->value.automaticFallback);

        Add(*state, WC_STATICW, L"Arbitrary reference precision", SS_LEFT, 0, 18, y + 4, 210, 22);
        HWND bits = Add(*state, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                        BitsCombo, 235, y, 300, 120);
        constexpr std::array<const wchar_t*, 3> bitOptions{L"128 bits", L"256 bits", L"512 bits"};
        for (const wchar_t* item : bitOptions) {
            SendMessageW(bits, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
        }
        SendMessageW(bits, CB_SETCURSEL,
                     state->value.arbitraryPrecisionBits <= 128 ? 0
                     : (state->value.arbitraryPrecisionBits >= 512 ? 2 : 1),
                     0);
        y += 48;

        Add(*state, WC_STATICW,
            L"Perturbation and split-float deep zoom are limited to compatible quadratic parameter maps. "
            L"Higher powers, Julia, rational, Newton, conjugate, absolute, trigonometric, exponential and logarithmic operations use native float32/float64 or report an error when fallback is disabled.",
            SS_LEFT, 0, 18, y, 612, 64);
        y += 74;

        Add(*state, WC_BUTTONW, L"&OK", BS_DEFPUSHBUTTON | WS_TABSTOP, OkButton, 410, y, 105, 32);
        Add(*state, WC_BUTTONW, L"&Cancel", BS_PUSHBUTTON | WS_TABSTOP, CancelButton, 525, y, 105, 32);
        SendMessageW(window, DM_SETDEFID, OkButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 520, 400);
        state->layout.Focus(GetDlgItem(window, ModeCombo));
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
        if (id == ModeCombo && HIWORD(wParam) == CBN_SELCHANGE) {
            UpdateDescription(*state);
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

bool PrecisionDialog::Show(HWND owner, HINSTANCE instance, PrecisionSettings& settings) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kClassName;
    RegisterClassExW(&windowClass);

    State state;
    state.owner = owner;
    state.instance = instance;
    state.value = settings;

    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 670, 570, state.dpi, kClassName);
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kClassName, L"Deep Zoom Precision",
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
    if (state.accepted) settings = state.value;
    return state.accepted;
}
#endif
} // namespace mw
