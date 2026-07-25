#include "App/PaletteEditorDialog.h"
#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#include <commdlg.h>
#endif

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace mw {

#ifdef _WIN32
namespace {

constexpr wchar_t kPaletteEditorClass[] = L"MandelbrotPaletteEditorDialog";

enum Id : int {
    SavedPaletteLabel = 6000,
    SavedPaletteCombo,
    LoadSavedButton,
    DeleteSavedButton,
    PaletteNameLabel,
    PaletteNameEdit,
    SaveSavedButton,
    InfoLabel,
    ColourList,
    AddColourButton,
    EditColourButton,
    RemoveColourButton,
    MoveUpButton,
    MoveDownButton,
    BuiltInButton,
    OkButton,
    CancelButton,
};

struct EditorState {
    HWND owner{nullptr};
    HWND window{nullptr};
    HINSTANCE instance{nullptr};
    Preset* preset{nullptr};
    std::vector<PalettePreset>* destinationPalettes{nullptr};
    std::vector<PalettePreset> savedPalettes;
    std::size_t builtInPaletteCount{0};
    HFONT font{nullptr};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    std::vector<Colour> colours;
    bool useBuiltIn{false};
    bool accepted{false};
    bool paletteLibraryChanged{false};
    bool done{false};
};

HWND Add(HWND parent, HINSTANCE instance, const wchar_t* cls, const wchar_t* text, DWORD style,
         int id, int x, int y, int width, int height, HFONT font, DWORD exStyle = 0) {
    HWND control = CreateWindowExW(exStyle, cls, text, WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style),
                                   x, y, width, height, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
    SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

std::wstring ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                        result.data(), count);
    return result;
}

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                        result.data(), count, nullptr, nullptr);
    return result;
}

std::wstring ReadText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) return {};
    std::wstring text(static_cast<std::size_t>(length) + 1U, L'\0');
    GetWindowTextW(control, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));
    return text;
}

COLORREF ToColorRef(const Colour& colour) {
    return RGB(static_cast<BYTE>(std::lround(std::clamp(colour.r, 0.0F, 1.0F) * 255.0F)),
               static_cast<BYTE>(std::lround(std::clamp(colour.g, 0.0F, 1.0F) * 255.0F)),
               static_cast<BYTE>(std::lround(std::clamp(colour.b, 0.0F, 1.0F) * 255.0F)));
}

Colour FromColorRef(COLORREF colour) {
    return {GetRValue(colour) / 255.0F, GetGValue(colour) / 255.0F, GetBValue(colour) / 255.0F, 1.0F};
}

std::wstring ColourText(std::size_t index, const Colour& colour) {
    std::wostringstream text;
    text << (index + 1U) << L".  #" << std::uppercase << std::hex << std::setfill(L'0')
         << std::setw(2) << static_cast<int>(std::lround(std::clamp(colour.r, 0.0F, 1.0F) * 255.0F))
         << std::setw(2) << static_cast<int>(std::lround(std::clamp(colour.g, 0.0F, 1.0F) * 255.0F))
         << std::setw(2) << static_cast<int>(std::lround(std::clamp(colour.b, 0.0F, 1.0F) * 255.0F));
    return text.str();
}

void RefreshSavedPaletteCombo(EditorState& state, int selection = -1) {
    HWND combo = GetDlgItem(state.window, SavedPaletteCombo);
    SendMessageW(combo, CB_RESETCONTENT, 0, 0);
    for (std::size_t index = 0; index < state.savedPalettes.size(); ++index) {
        const auto& palette = state.savedPalettes[index];
        const std::wstring prefix = index < state.builtInPaletteCount ? L"Built-in — " : L"Saved — ";
        const auto name = prefix + ToWide(palette.name);
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name.c_str()));
    }
    if (selection >= 0 && selection < static_cast<int>(state.savedPalettes.size())) {
        SendMessageW(combo, CB_SETCURSEL, selection, 0);
        SetWindowTextW(GetDlgItem(state.window, PaletteNameEdit),
                       ToWide(state.savedPalettes[static_cast<std::size_t>(selection)].name).c_str());
    } else {
        SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
    }
}

void RefreshList(EditorState& state, int selection = -1) {
    HWND list = GetDlgItem(state.window, ColourList);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    for (std::size_t index = 0; index < state.colours.size(); ++index) {
        const auto text = ColourText(index, state.colours[index]);
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
    }
    if (!state.colours.empty()) {
        const int safeSelection = std::clamp(selection < 0 ? 0 : selection, 0,
                                             static_cast<int>(state.colours.size()) - 1);
        SendMessageW(list, LB_SETCURSEL, safeSelection, 0);
    }
    const std::wstring title = state.useBuiltIn
        ? L"Built-in palette preview (add or edit a colour to make it custom)"
        : L"Custom palette: " + std::to_wstring(state.colours.size()) + L" colour stops";
    SetWindowTextW(GetDlgItem(state.window, InfoLabel), title.c_str());
    InvalidateRect(list, nullptr, TRUE);
}

bool ChooseColour(EditorState& state, Colour& colour) {
    static COLORREF customColours[16]{};
    CHOOSECOLORW chooser{};
    chooser.lStructSize = sizeof(chooser);
    chooser.hwndOwner = state.window;
    chooser.rgbResult = ToColorRef(colour);
    chooser.lpCustColors = customColours;
    chooser.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (!ChooseColorW(&chooser)) return false;
    colour = FromColorRef(chooser.rgbResult);
    return true;
}

int Selection(const EditorState& state) {
    return static_cast<int>(SendMessageW(GetDlgItem(state.window, ColourList), LB_GETCURSEL, 0, 0));
}

int SavedSelection(const EditorState& state) {
    return static_cast<int>(SendMessageW(GetDlgItem(state.window, SavedPaletteCombo), CB_GETCURSEL, 0, 0));
}

void AddColour(EditorState& state) {
    if (state.colours.size() >= 4096) {
        MessageBoxW(state.window, L"Custom palettes are limited to 4096 colour stops for safe storage and rendering.",
                    L"Palette Limit", MB_OK | MB_ICONINFORMATION);
        return;
    }
    Colour colour = state.colours.empty() ? Colour{1.0F, 1.0F, 1.0F, 1.0F} : state.colours.back();
    if (!ChooseColour(state, colour)) return;
    state.useBuiltIn = false;
    const int selected = Selection(state);
    const auto insertion = selected < 0 ? state.colours.end() : state.colours.begin() + selected + 1;
    const int newIndex = selected < 0 ? static_cast<int>(state.colours.size()) : selected + 1;
    state.colours.insert(insertion, colour);
    RefreshList(state, newIndex);
}

void EditColour(EditorState& state) {
    const int selected = Selection(state);
    if (selected < 0 || selected >= static_cast<int>(state.colours.size())) return;
    Colour colour = state.colours[static_cast<std::size_t>(selected)];
    if (!ChooseColour(state, colour)) return;
    state.useBuiltIn = false;
    state.colours[static_cast<std::size_t>(selected)] = colour;
    RefreshList(state, selected);
}

void RemoveColour(EditorState& state) {
    const int selected = Selection(state);
    if (selected < 0 || selected >= static_cast<int>(state.colours.size())) return;
    if (state.colours.size() <= 2) {
        MessageBoxW(state.window, L"A custom palette needs at least two colours. Use Built-in Palette to remove the custom palette.",
                    L"Palette", MB_OK | MB_ICONINFORMATION);
        return;
    }
    state.useBuiltIn = false;
    state.colours.erase(state.colours.begin() + selected);
    RefreshList(state, std::min(selected, static_cast<int>(state.colours.size()) - 1));
}

void Move(EditorState& state, int direction) {
    const int selected = Selection(state);
    const int target = selected + direction;
    if (selected < 0 || target < 0 || target >= static_cast<int>(state.colours.size())) return;
    state.useBuiltIn = false;
    std::swap(state.colours[static_cast<std::size_t>(selected)], state.colours[static_cast<std::size_t>(target)]);
    RefreshList(state, target);
}

void SelectSavedPalette(EditorState& state) {
    const int selected = SavedSelection(state);
    if (selected < 0 || selected >= static_cast<int>(state.savedPalettes.size())) return;
    const auto& palette = state.savedPalettes[static_cast<std::size_t>(selected)];
    state.colours = palette.colours;
    state.useBuiltIn = false;
    SetWindowTextW(GetDlgItem(state.window, PaletteNameEdit), ToWide(palette.name).c_str());
    RefreshList(state);
}

std::string NewPaletteId(const EditorState& state) {
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string candidate = "palette-" + std::to_string(milliseconds);
    int suffix = 2;
    const auto exists = [&](const std::string& id) {
        return std::any_of(state.savedPalettes.begin(), state.savedPalettes.end(),
                           [&](const PalettePreset& preset) { return preset.id == id; });
    };
    while (exists(candidate)) candidate = "palette-" + std::to_string(milliseconds) + "-" + std::to_string(suffix++);
    return candidate;
}

std::vector<PalettePreset> CustomPaletteLibrary(const EditorState& state) {
    if (state.builtInPaletteCount >= state.savedPalettes.size()) return {};
    return std::vector<PalettePreset>(
        state.savedPalettes.begin() + static_cast<std::ptrdiff_t>(state.builtInPaletteCount),
        state.savedPalettes.end());
}

void SavePalettePreset(EditorState& state) {
    if (state.colours.size() < 2) {
        MessageBoxW(state.window, L"A saved palette needs at least two colours.", L"Palette", MB_OK | MB_ICONWARNING);
        return;
    }
    const std::wstring nameWide = ReadText(GetDlgItem(state.window, PaletteNameEdit));
    const std::string name = ToUtf8(nameWide);
    if (name.empty() || name.size() > 120) {
        MessageBoxW(state.window, L"Enter a palette name containing 1 to 120 characters.", L"Palette Name", MB_OK | MB_ICONWARNING);
        return;
    }

    int selected = SavedSelection(state);
    if (selected < 0) {
        const auto sameName = std::find_if(state.savedPalettes.begin(), state.savedPalettes.end(),
            [&](const PalettePreset& preset) { return preset.name == name; });
        if (sameName != state.savedPalettes.end()) {
            selected = static_cast<int>(std::distance(state.savedPalettes.begin(), sameName));
        }
    }

    const auto duplicateName = std::find_if(state.savedPalettes.begin(), state.savedPalettes.end(),
        [&](const PalettePreset& preset) { return preset.name == name; });
    if (duplicateName != state.savedPalettes.end() &&
        static_cast<int>(std::distance(state.savedPalettes.begin(), duplicateName)) != selected) {
        MessageBoxW(state.window, L"A saved palette already uses that name. Choose a different name.",
                    L"Palette Name", MB_OK | MB_ICONWARNING);
        return;
    }

    if (selected >= static_cast<int>(state.builtInPaletteCount) &&
        selected < static_cast<int>(state.savedPalettes.size())) {
        auto& saved = state.savedPalettes[static_cast<std::size_t>(selected)];
        saved.name = name;
        saved.colours = state.colours;
        ValidateAndNormalise(saved);
    } else {
        const std::size_t customCount = state.savedPalettes.size() - state.builtInPaletteCount;
        if (customCount >= 256U) {
            MessageBoxW(state.window, L"A maximum of 256 user-saved palette presets is supported.", L"Palette Limit", MB_OK | MB_ICONINFORMATION);
            return;
        }
        PalettePreset saved;
        saved.id = NewPaletteId(state);
        saved.name = name;
        saved.colours = state.colours;
        ValidateAndNormalise(saved);
        state.savedPalettes.push_back(std::move(saved));
        selected = static_cast<int>(state.savedPalettes.size()) - 1;
    }
    state.useBuiltIn = false;
    state.paletteLibraryChanged = true;
    RefreshSavedPaletteCombo(state, selected);
}

void DeletePalettePreset(EditorState& state) {
    const int selected = SavedSelection(state);
    if (selected < 0 || selected >= static_cast<int>(state.savedPalettes.size())) return;
    if (selected < static_cast<int>(state.builtInPaletteCount)) {
        MessageBoxW(state.window, L"Built-in palette presets are read-only. Load it, enter a new name, and use Save / Update to create an editable copy.",
                    L"Built-in Palette", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (MessageBoxW(state.window, L"Delete the selected saved palette preset?", L"Delete Palette",
                    MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    state.savedPalettes.erase(state.savedPalettes.begin() + selected);
    state.paletteLibraryChanged = true;
    SetWindowTextW(GetDlgItem(state.window, PaletteNameEdit), L"");
    RefreshSavedPaletteCombo(state);
}

void DrawColourItem(const EditorState& state, const DRAWITEMSTRUCT& draw) {
    if (draw.itemID == static_cast<UINT>(-1) || draw.itemID >= state.colours.size()) return;
    const bool selected = (draw.itemState & ODS_SELECTED) != 0;
    const COLORREF background = GetSysColor(selected ? COLOR_HIGHLIGHT : COLOR_WINDOW);
    const COLORREF foreground = GetSysColor(selected ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
    HBRUSH backgroundBrush = CreateSolidBrush(background);
    FillRect(draw.hDC, &draw.rcItem, backgroundBrush);
    DeleteObject(backgroundBrush);

    RECT swatch{draw.rcItem.left + 8, draw.rcItem.top + 4, draw.rcItem.left + 34, draw.rcItem.bottom - 4};
    HBRUSH colourBrush = CreateSolidBrush(ToColorRef(state.colours[draw.itemID]));
    FillRect(draw.hDC, &swatch, colourBrush);
    DeleteObject(colourBrush);
    FrameRect(draw.hDC, &swatch, reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));

    const auto text = ColourText(draw.itemID, state.colours[draw.itemID]);
    RECT textRect = draw.rcItem;
    textRect.left += 44;
    SetBkMode(draw.hDC, TRANSPARENT);
    SetTextColor(draw.hDC, foreground);
    DrawTextW(draw.hDC, text.c_str(), static_cast<int>(text.size()), &textRect,
              DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
    if ((draw.itemState & ODS_FOCUS) != 0) DrawFocusRect(draw.hDC, &draw.rcItem);
}

LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<EditorState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<EditorState*>(create->lpCreateParams);
        state->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(window, message, wParam, lParam);

    if (message == WM_CREATE) {
        state->font = CreateResponsiveDialogFont(state->dpi);
        Add(window, state->instance, WC_STATICW, L"Saved palette", SS_LEFT, SavedPaletteLabel, 18, 17, 104, 24, state->font);
        Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
            SavedPaletteCombo, 126, 12, 290, 220, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Load", BS_PUSHBUTTON | WS_TABSTOP, LoadSavedButton, 426, 12, 80, 28, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Delete", BS_PUSHBUTTON | WS_TABSTOP, DeleteSavedButton, 514, 12, 80, 28, state->font);
        Add(window, state->instance, WC_STATICW, L"Preset name", SS_LEFT, PaletteNameLabel, 18, 53, 104, 24, state->font);
        HWND nameEdit = Add(window, state->instance, WC_EDITW, L"", ES_AUTOHSCROLL | WS_TABSTOP,
                            PaletteNameEdit, 126, 48, 290, 28, state->font, WS_EX_CLIENTEDGE);
        SendMessageW(nameEdit, EM_SETLIMITTEXT, 120, 0);
        Add(window, state->instance, WC_BUTTONW, L"Save / Update", BS_PUSHBUTTON | WS_TABSTOP,
            SaveSavedButton, 426, 48, 168, 28, state->font);
        Add(window, state->instance, WC_STATICW, L"", SS_LEFT, InfoLabel, 18, 88, 650, 24, state->font);
        HWND list = Add(window, state->instance, WC_LISTBOXW, L"",
            LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP,
            ColourList, 18, 116, 430, 350, state->font, WS_EX_CLIENTEDGE);
        SetWindowTextW(list, L"Palette colour stops");
        SendMessageW(list, LB_SETITEMHEIGHT, 0, ScaleDialogMetric(28, state->dpi));
        Add(window, state->instance, WC_BUTTONW, L"Add Colour...", BS_PUSHBUTTON | WS_TABSTOP, AddColourButton, 464, 116, 150, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Edit Colour...", BS_PUSHBUTTON | WS_TABSTOP, EditColourButton, 464, 154, 150, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Remove", BS_PUSHBUTTON | WS_TABSTOP, RemoveColourButton, 464, 192, 150, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Move Up", BS_PUSHBUTTON | WS_TABSTOP, MoveUpButton, 464, 246, 150, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Move Down", BS_PUSHBUTTON | WS_TABSTOP, MoveDownButton, 464, 284, 150, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Use Built-in Palette", BS_PUSHBUTTON | WS_TABSTOP, BuiltInButton, 464, 338, 150, 42, state->font);
        Add(window, state->instance, WC_STATICW,
            L"Colour stops interpolate in list order and wrap smoothly back to the first colour.",
            SS_LEFT, 0, 464, 392, 190, 64, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&OK", BS_DEFPUSHBUTTON | WS_TABSTOP, OkButton, 422, 484, 105, 32, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&Cancel", BS_PUSHBUTTON | WS_TABSTOP, CancelButton, 537, 484, 105, 32, state->font);
        RefreshSavedPaletteCombo(*state);
        RefreshList(*state);
        SendMessageW(window, DM_SETDEFID, OkButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 520, 400);
        state->layout.Focus(GetDlgItem(window, SavedPaletteCombo));
        return 0;
    }
    if (message == WM_DRAWITEM) {
        const auto* draw = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
        if (draw && draw->CtlID == ColourList) {
            DrawColourItem(*state, *draw);
            return TRUE;
        }
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
        SendMessageW(GetDlgItem(window, ColourList), LB_SETITEMHEIGHT, 0, ScaleDialogMetric(28, newDpi));
        return 0;
    }

    if (message == WM_COMMAND) {
        const int id = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (id == AddColourButton) AddColour(*state);
        else if (id == EditColourButton || (id == ColourList && notification == LBN_DBLCLK)) EditColour(*state);
        else if (id == RemoveColourButton) RemoveColour(*state);
        else if (id == MoveUpButton) Move(*state, -1);
        else if (id == MoveDownButton) Move(*state, 1);
        else if (id == LoadSavedButton || (id == SavedPaletteCombo && notification == CBN_SELCHANGE)) SelectSavedPalette(*state);
        else if (id == SaveSavedButton) SavePalettePreset(*state);
        else if (id == DeleteSavedButton) DeletePalettePreset(*state);
        else if (id == BuiltInButton) {
            state->useBuiltIn = true;
            state->colours = PalettePreviewColours(state->preset->palette);
            SetWindowTextW(GetDlgItem(state->window, PaletteNameEdit), L"");
            SendMessageW(GetDlgItem(state->window, SavedPaletteCombo), CB_SETCURSEL, static_cast<WPARAM>(-1), 0);
            RefreshList(*state);
        } else if (id == OkButton) {
            if (!state->useBuiltIn && state->colours.size() < 2) {
                MessageBoxW(window, L"Add at least two colours or choose Use Built-in Palette.", L"Palette", MB_OK | MB_ICONWARNING);
                return 0;
            }
            state->preset->customPaletteColours = state->useBuiltIn ? std::vector<Colour>{} : state->colours;
            ValidateAndNormalise(*state->preset);
            *state->destinationPalettes = CustomPaletteLibrary(*state);
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
        RememberDialogPlacement(window, kPaletteEditorClass, state->dpi);
        state->layout.Shutdown();
        if (state->paletteLibraryChanged) *state->destinationPalettes = CustomPaletteLibrary(*state);
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

bool PaletteEditorDialog::Show(HWND owner, HINSTANCE instance, Preset& preset,
                               std::vector<PalettePreset>& savedPalettes) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kPaletteEditorClass;
    RegisterClassExW(&windowClass);

    EditorState state;
    state.owner = owner;
    state.instance = instance;
    state.preset = &preset;
    state.destinationPalettes = &savedPalettes;
    state.savedPalettes = BuiltInPalettePresets();
    state.builtInPaletteCount = state.savedPalettes.size();
    state.savedPalettes.insert(state.savedPalettes.end(), savedPalettes.begin(), savedPalettes.end());
    state.useBuiltIn = preset.customPaletteColours.empty();
    state.colours = state.useBuiltIn ? PalettePreviewColours(preset.palette) : preset.customPaletteColours;

    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 682, 565, state.dpi, kPaletteEditorClass);
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kPaletteEditorClass, L"Custom Palette Editor",
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
    return state.accepted || state.paletteLibraryChanged;
}
#endif

} // namespace mw
