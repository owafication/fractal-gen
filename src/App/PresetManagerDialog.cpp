#include "App/PresetManagerDialog.h"
#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#endif

#include <algorithm>
#include <iterator>
#include <string>

namespace mw {

#ifdef _WIN32
namespace {

constexpr wchar_t kPresetManagerClass[] = L"MandelbrotPresetManagerDialog";

enum Id : int {
    PresetList = 7601,
    SaveAsNameEdit,
    LoadButton,
    SaveNewButton,
    UpdateButton,
    DeleteButton,
    RestoreButton,
    ImportButton,
    ExportButton,
    CloseButton,
};

struct DialogState {
    HWND owner{nullptr};
    HWND window{nullptr};
    HINSTANCE instance{nullptr};
    const std::vector<Preset>* presets{nullptr};
    HFONT font{nullptr};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    int currentIndex{0};
    int selectedIndex{0};
    std::string saveAsName;
    PresetManagerAction action{PresetManagerAction::Cancelled};
    bool done{false};
};

HWND Add(HWND parent, HINSTANCE instance, const wchar_t* cls, const wchar_t* text,
         DWORD style, int id, int x, int y, int width, int height, HFONT font,
         DWORD exStyle = 0) {
    HWND control = CreateWindowExW(exStyle, cls, text, WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style),
                                   x, y, width, height, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                                   instance, nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
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

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), result.data(), size, nullptr, nullptr) != size) return {};
    return result;
}

std::string ReadSaveAsName(HWND window) {
    HWND edit = GetDlgItem(window, SaveAsNameEdit);
    const int length = GetWindowTextLengthW(edit);
    std::wstring value(static_cast<std::size_t>(std::max(0, length)) + 1U, L'\0');
    if (length > 0) GetWindowTextW(edit, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(std::max(0, length)));
    std::string name = WideToUtf8(value);
    const auto first = name.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = name.find_last_not_of(" \t\r\n");
    return name.substr(first, last - first + 1U);
}

void Finish(DialogState& state, PresetManagerAction action) {
    const int selection = static_cast<int>(SendMessageW(GetDlgItem(state.window, PresetList), LB_GETCURSEL, 0, 0));
    if (selection >= 0) state.selectedIndex = selection;
    state.saveAsName = ReadSaveAsName(state.window);
    state.action = action;
    DestroyWindow(state.window);
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

    switch (message) {
    case WM_CREATE: {
        state->font = CreateResponsiveDialogFont(state->dpi);
        Add(window, state->instance, WC_STATICW,
            L"Load and maintain wallpaper presets. Built-in presets can be loaded or exported but cannot be overwritten or deleted.",
            SS_LEFT, 0, 16, 14, 566, 42, state->font);
        HWND list = Add(window, state->instance, WC_LISTBOXW, L"",
                        LBS_NOTIFY | WS_TABSTOP | WS_VSCROLL,
                        PresetList, 16, 62, 360, 384, state->font, WS_EX_CLIENTEDGE);
        SetWindowTextW(list, L"Available presets");
        for (const auto& preset : *state->presets) {
            std::wstring name = Utf8ToWide(preset.name);
            if (preset.builtIn) name += L"  [built-in]";
            SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
        }
        SendMessageW(list, LB_SETCURSEL,
                     std::clamp(state->currentIndex, 0, std::max(0, static_cast<int>(state->presets->size()) - 1)), 0);

        constexpr int x = 394;
        constexpr int w = 228;
        Add(window, state->instance, WC_STATICW, L"Name for Save Preview as New",
            SS_LEFT, 0, x, 62, w, 22, state->font);
        HWND saveAsName = Add(window, state->instance, WC_EDITW, Utf8ToWide(state->saveAsName).c_str(),
                              ES_AUTOHSCROLL | WS_TABSTOP, SaveAsNameEdit, x, 86, w, 28,
                              state->font, WS_EX_CLIENTEDGE);
        SendMessageW(saveAsName, EM_SETLIMITTEXT, 120, 0);
        int y = 126;
        auto button = [&](int id, const wchar_t* text) {
            Add(window, state->instance, WC_BUTTONW, text, BS_PUSHBUTTON | WS_TABSTOP,
                id, x, y, w, 32, state->font);
            y += 40;
        };
        button(LoadButton, L"&Load Selected");
        button(SaveNewButton, L"Save Preview as New");
        button(UpdateButton, L"Update Selected");
        button(DeleteButton, L"Delete Selected Custom");
        button(RestoreButton, L"Restore Built-ins");
        button(ImportButton, L"Import JSON...");
        button(ExportButton, L"Export Selected...");
        button(CloseButton, L"&Close");
        SendMessageW(window, DM_SETDEFID, LoadButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 620, 430);
        state->layout.Focus(GetDlgItem(window, PresetList));
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
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id == PresetList && HIWORD(wParam) == LBN_DBLCLK) Finish(*state, PresetManagerAction::Load);
        else if (id == LoadButton) Finish(*state, PresetManagerAction::Load);
        else if (id == SaveNewButton) {
            const std::string name = ReadSaveAsName(window);
            if (name.empty()) {
                MessageBoxW(window, L"Enter a name for the new preset.",
                            L"Save Preview as New", MB_OK | MB_ICONWARNING);
                state->layout.Focus(GetDlgItem(window, SaveAsNameEdit));
                return 0;
            }
            if (name.size() > 120U) {
                MessageBoxW(window, L"The preset name is too long. Use no more than 120 UTF-8 bytes.",
                            L"Save Preview as New", MB_OK | MB_ICONWARNING);
                state->layout.Focus(GetDlgItem(window, SaveAsNameEdit));
                return 0;
            }
            Finish(*state, PresetManagerAction::SaveNew);
        }
        else if (id == UpdateButton) Finish(*state, PresetManagerAction::Update);
        else if (id == DeleteButton) Finish(*state, PresetManagerAction::Delete);
        else if (id == RestoreButton) Finish(*state, PresetManagerAction::RestoreBuiltIns);
        else if (id == ImportButton) Finish(*state, PresetManagerAction::Import);
        else if (id == ExportButton) Finish(*state, PresetManagerAction::Export);
        else if (id == CloseButton) Finish(*state, PresetManagerAction::Cancelled);
        return 0;
    }
    case WM_CLOSE:
        Finish(*state, PresetManagerAction::Cancelled);
        return 0;
    case WM_DESTROY:
        RememberDialogPlacement(window, kPresetManagerClass, state->dpi);
        state->layout.Shutdown();
        if (state->font) DeleteObject(state->font);
        state->font = nullptr;
        state->done = true;
        EnableWindow(state->owner, TRUE);
        SetForegroundWindow(state->owner);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

PresetManagerAction PresetManagerDialog::Show(HWND owner, HINSTANCE instance,
                                               const std::vector<Preset>& presets,
                                               int currentIndex,
                                               int& selectedIndex,
                                               std::string& saveAsName) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kPresetManagerClass;
    RegisterClassExW(&windowClass);

    DialogState state;
    state.owner = owner;
    state.instance = instance;
    state.presets = &presets;
    state.currentIndex = currentIndex;
    state.selectedIndex = currentIndex;
    state.saveAsName = saveAsName;

    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 660, 520, state.dpi, kPresetManagerClass);
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kPresetManagerClass, L"Preset Library",
                                  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX |
                                  WS_POPUP | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
                                  dialogRect.left, dialogRect.top,
                                  dialogRect.right - dialogRect.left, dialogRect.bottom - dialogRect.top,
                                  owner, nullptr, instance, &state);
    if (!window) return PresetManagerAction::Cancelled;
    EnableWindow(owner, FALSE);

    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, CloseButton, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    selectedIndex = state.selectedIndex;
    saveAsName = state.saveAsName;
    return state.action;
}
#endif

} // namespace mw
