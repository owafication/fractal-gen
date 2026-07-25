#include "App/SlideshowDialog.h"
#include "App/DialogSupport.h"

#ifdef _WIN32
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

#include <algorithm>
#include <filesystem>
#include <cwctype>
#include <iterator>
#include <string>
#include <vector>

namespace mw {

#ifdef _WIN32
namespace {

constexpr wchar_t kSlideshowClass[] = L"MandelbrotStaticSlideshowDialog";
constexpr std::size_t kMaximumImages = 512;

enum Id : int {
    FolderEdit = 7101,
    BrowseFolderButton,
    OpenFolderButton,
    ImageList,
    AddImagesButton,
    ScanFolderButton,
    RemoveButton,
    MoveUpButton,
    MoveDownButton,
    SetCurrentButton,
    ClearButton,
    EnableCycleCheck,
    IntervalEdit,
    OrderCombo,
    SummaryLabel,
    SaveButton,
    StartSelectedButton,
    CancelButton,
};

struct DialogState {
    HWND owner{nullptr};
    HWND window{nullptr};
    HINSTANCE instance{nullptr};
    StaticWallpaperSettings* destination{nullptr};
    StaticWallpaperSettings working;
    HFONT font{nullptr};
    ResponsiveDialogLayout layout;
    UINT dpi{96};
    bool done{false};
    SlideshowDialogResult result{SlideshowDialogResult::Cancelled};
};

HWND Add(HWND parent, HINSTANCE instance, const wchar_t* cls, const wchar_t* text, DWORD style,
         int id, int x, int y, int width, int height, HFONT font, DWORD exStyle = 0) {
    HWND control = CreateWindowExW(exStyle, cls, text, WS_CHILD | WS_VISIBLE | AccessibleControlStyle(cls, style),
                                   x, y, width, height, parent,
                                   reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), instance, nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return control;
}

std::wstring ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0);
    if (count <= 0) return {};
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                             static_cast<int>(text.size()), result.data(), count);
    return written == count ? result : std::wstring{};
}

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int count = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                          static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (count <= 0) return {};
    std::string result(static_cast<std::size_t>(count), '\0');
    const int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                             static_cast<int>(text.size()), result.data(), count, nullptr, nullptr);
    return written == count ? result : std::string{};
}

std::wstring ReadText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) return {};
    std::wstring value(static_cast<std::size_t>(length) + 1U, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return value;
}

void SetCheck(HWND window, int id, bool checked) {
    SendMessageW(GetDlgItem(window, id), BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
}

bool Checked(HWND window, int id) {
    return SendMessageW(GetDlgItem(window, id), BM_GETCHECK, 0, 0) == BST_CHECKED;
}

int Selection(const DialogState& state) {
    return static_cast<int>(SendMessageW(GetDlgItem(state.window, ImageList), LB_GETCURSEL, 0, 0));
}

bool IsBmpPath(const std::filesystem::path& path) {
    std::wstring extension = path.extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](wchar_t ch) { return static_cast<wchar_t>(std::towlower(ch)); });
    return extension == L".bmp";
}

void UpdateSummary(DialogState& state) {
    const std::size_t count = state.working.imagePaths.size();
    std::wstring text = std::to_wstring(count) + (count == 1U ? L" image" : L" images");
    if (count > 0U) {
        text += L"  •  selected image " + std::to_wstring(state.working.currentIndex + 1);
    }
    SetWindowTextW(GetDlgItem(state.window, SummaryLabel), text.c_str());
    EnableWindow(GetDlgItem(state.window, StartSelectedButton), count > 0U ? TRUE : FALSE);
}

void RefreshList(DialogState& state, int requestedSelection = -1) {
    HWND list = GetDlgItem(state.window, ImageList);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);
    int widest = 0;
    HDC dc = GetDC(list);
    HFONT oldFont = dc ? static_cast<HFONT>(SelectObject(dc, state.font)) : nullptr;
    for (std::size_t index = 0; index < state.working.imagePaths.size(); ++index) {
        const std::filesystem::path path(ToWide(state.working.imagePaths[index]));
        std::error_code existsError;
        const bool exists = std::filesystem::is_regular_file(path, existsError);
        std::wstring text = index == static_cast<std::size_t>(state.working.currentIndex) ? L"▶  " : L"    ";
        if (!exists) text += L"[missing] ";
        text += path.filename().empty() ? path.wstring() : path.filename().wstring();
        text += L"    —    " + path.wstring();
        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(text.c_str()));
        if (dc) {
            SIZE size{};
            if (GetTextExtentPoint32W(dc, text.c_str(), static_cast<int>(text.size()), &size)) {
                widest = std::max(widest, static_cast<int>(size.cx) + 28);
            }
        }
    }
    if (dc) {
        if (oldFont) SelectObject(dc, oldFont);
        ReleaseDC(list, dc);
    }
    SendMessageW(list, LB_SETHORIZONTALEXTENT, static_cast<WPARAM>(widest), 0);
    if (!state.working.imagePaths.empty()) {
        const int maximum = static_cast<int>(state.working.imagePaths.size()) - 1;
        const int selection = std::clamp(requestedSelection < 0 ? state.working.currentIndex : requestedSelection, 0, maximum);
        SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(selection), 0);
    }
    UpdateSummary(state);
}

bool AddPath(DialogState& state, const std::filesystem::path& path) {
    if (state.working.imagePaths.size() >= kMaximumImages || !IsBmpPath(path)) return false;
    const std::string utf8 = ToUtf8(path.wstring());
    if (utf8.empty()) return false;
    if (std::find(state.working.imagePaths.begin(), state.working.imagePaths.end(), utf8) !=
        state.working.imagePaths.end()) return false;
    state.working.imagePaths.push_back(utf8);
    return true;
}

std::vector<std::filesystem::path> SelectBmpFiles(HWND owner) {
    std::vector<wchar_t> buffer(32768, L'\0');
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = L"Mandelbrot static images (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = buffer.data();
    dialog.nMaxFile = static_cast<DWORD>(buffer.size());
    dialog.Flags = OFN_EXPLORER | OFN_ALLOWMULTISELECT | OFN_FILEMUSTEXIST |
                   OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&dialog)) return {};

    std::vector<std::filesystem::path> result;
    const std::wstring first(buffer.data());
    const wchar_t* cursor = buffer.data() + first.size() + 1U;
    if (*cursor == L'\0') {
        result.emplace_back(first);
        return result;
    }
    const std::filesystem::path directory(first);
    while (*cursor != L'\0') {
        const std::wstring name(cursor);
        result.push_back(directory / name);
        cursor += name.size() + 1U;
    }
    return result;
}

std::filesystem::path BrowseFolder(HWND owner) {
    BROWSEINFOW browse{};
    browse.hwndOwner = owner;
    browse.lpszTitle = L"Choose the folder used for new static Mandelbrot captures";
    browse.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_EDITBOX;
    PIDLIST_ABSOLUTE item = SHBrowseForFolderW(&browse);
    if (!item) return {};
    wchar_t path[MAX_PATH]{};
    const bool converted = SHGetPathFromIDListW(item, path) == TRUE;
    CoTaskMemFree(item);
    return converted ? std::filesystem::path(path) : std::filesystem::path{};
}

std::filesystem::path StorageFolder(const DialogState& state) {
    return std::filesystem::path(ReadText(GetDlgItem(state.window, FolderEdit)));
}

void AddFiles(DialogState& state) {
    const auto selected = SelectBmpFiles(state.window);
    int added = 0;
    for (const auto& path : selected) {
        if (AddPath(state, path)) ++added;
    }
    RefreshList(state, static_cast<int>(state.working.imagePaths.size()) - 1);
    if (!selected.empty() && added == 0) {
        MessageBoxW(state.window, L"No new BMP files were added. The files may already be listed or the 512-image safety limit may have been reached.",
                    L"Static Slideshow", MB_OK | MB_ICONINFORMATION);
    }
}

void ScanFolder(DialogState& state) {
    const auto folder = StorageFolder(state);
    std::error_code error;
    if (folder.empty() || !std::filesystem::is_directory(folder, error)) {
        MessageBoxW(state.window, L"Choose an existing slideshow folder first.", L"Static Slideshow",
                    MB_OK | MB_ICONWARNING);
        return;
    }
    std::vector<std::filesystem::path> files;
    for (std::filesystem::directory_iterator it(folder, error), end; !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && IsBmpPath(it->path())) files.push_back(it->path());
    }
    if (error) {
        MessageBoxW(state.window, L"The selected folder could not be scanned.", L"Static Slideshow",
                    MB_OK | MB_ICONERROR);
        return;
    }
    std::sort(files.begin(), files.end());
    int added = 0;
    for (const auto& path : files) {
        if (AddPath(state, path)) ++added;
    }
    RefreshList(state, static_cast<int>(state.working.imagePaths.size()) - 1);
    const std::wstring message = L"Added " + std::to_wstring(added) + L" new BMP image(s) from the folder.";
    MessageBoxW(state.window, message.c_str(), L"Static Slideshow", MB_OK | MB_ICONINFORMATION);
}

void RemoveSelected(DialogState& state) {
    const int selected = Selection(state);
    if (selected < 0 || selected >= static_cast<int>(state.working.imagePaths.size())) return;
    state.working.imagePaths.erase(state.working.imagePaths.begin() + selected);
    if (state.working.imagePaths.empty()) {
        state.working.currentIndex = 0;
    } else {
        if (selected < state.working.currentIndex) --state.working.currentIndex;
        else if (selected == state.working.currentIndex) {
            state.working.currentIndex = std::min(selected, static_cast<int>(state.working.imagePaths.size()) - 1);
        }
    }
    RefreshList(state, selected);
}

void MoveSelected(DialogState& state, int direction) {
    const int selected = Selection(state);
    const int target = selected + direction;
    if (selected < 0 || target < 0 || target >= static_cast<int>(state.working.imagePaths.size())) return;
    std::swap(state.working.imagePaths[static_cast<std::size_t>(selected)],
              state.working.imagePaths[static_cast<std::size_t>(target)]);
    if (state.working.currentIndex == selected) state.working.currentIndex = target;
    else if (state.working.currentIndex == target) state.working.currentIndex = selected;
    RefreshList(state, target);
}

void SetCurrent(DialogState& state) {
    const int selected = Selection(state);
    if (selected < 0 || selected >= static_cast<int>(state.working.imagePaths.size())) return;
    state.working.currentIndex = selected;
    RefreshList(state, selected);
}

bool Save(DialogState& state, SlideshowDialogResult result) {
    const std::wstring folderWide = ReadText(GetDlgItem(state.window, FolderEdit));
    if (folderWide.empty()) {
        MessageBoxW(state.window, L"Choose a folder for new static captures.", L"Static Slideshow",
                    MB_OK | MB_ICONWARNING);
        return false;
    }
    const std::filesystem::path folderPath(folderWide);
    if (!folderPath.is_absolute()) {
        MessageBoxW(state.window, L"Choose an absolute local or network folder path for new captures.",
                    L"Static Slideshow", MB_OK | MB_ICONWARNING);
        return false;
    }
    std::error_code directoryError;
    std::filesystem::create_directories(folderPath, directoryError);
    if (directoryError) {
        MessageBoxW(state.window, L"The selected slideshow folder could not be created or accessed.",
                    L"Static Slideshow", MB_OK | MB_ICONERROR);
        return false;
    }
    const std::string folderUtf8 = ToUtf8(folderWide);
    if (folderUtf8.empty()) {
        MessageBoxW(state.window, L"The slideshow folder contains unsupported characters.",
                    L"Static Slideshow", MB_OK | MB_ICONERROR);
        return false;
    }
    state.working.storageDirectory = folderUtf8;
    state.working.cycleEnabled = Checked(state.window, EnableCycleCheck);
    wchar_t intervalText[32]{};
    GetWindowTextW(GetDlgItem(state.window, IntervalEdit), intervalText, static_cast<int>(std::size(intervalText)));
    try {
        state.working.cycleSeconds = std::stoi(intervalText);
    } catch (...) {
        MessageBoxW(state.window, L"Enter a slideshow interval from 10 to 86400 seconds.",
                    L"Static Slideshow", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (state.working.cycleSeconds < 10 || state.working.cycleSeconds > 86400) {
        MessageBoxW(state.window, L"Enter a slideshow interval from 10 to 86400 seconds.",
                    L"Static Slideshow", MB_OK | MB_ICONWARNING);
        return false;
    }
    const int order = static_cast<int>(SendMessageW(GetDlgItem(state.window, OrderCombo), CB_GETCURSEL, 0, 0));
    state.working.order = order == 1 ? StaticSlideshowOrder::Shuffle : StaticSlideshowOrder::Sequential;
    if (state.working.imagePaths.empty()) {
        state.working.enabled = false;
        state.working.currentIndex = 0;
        if (result == SlideshowDialogResult::StartSelected) {
            MessageBoxW(state.window, L"Add at least one image before starting the slideshow.",
                        L"Static Slideshow", MB_OK | MB_ICONWARNING);
            return false;
        }
    }
    *state.destination = state.working;
    AppSettings validationSettings;
    validationSettings.staticWallpaper = *state.destination;
    ValidateAndNormalise(validationSettings);
    *state.destination = validationSettings.staticWallpaper;
    state.result = result;
    DestroyWindow(state.window);
    return true;
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
            L"STATIC SLIDESHOW  —  Choose where new captures are stored, then build and order the exact image list used by the wallpaper.",
            SS_LEFT, 0, 18, 14, 770, 34, state->font);
        Add(window, state->instance, WC_STATICW, L"Capture folder", SS_LEFT, 0, 18, 55, 110, 24, state->font);
        HWND folder = Add(window, state->instance, WC_EDITW, ToWide(state->working.storageDirectory).c_str(),
                          ES_AUTOHSCROLL | WS_TABSTOP, FolderEdit, 132, 50, 476, 28, state->font, WS_EX_CLIENTEDGE);
        SendMessageW(folder, EM_SETLIMITTEXT, 32767, 0);
        Add(window, state->instance, WC_BUTTONW, L"Browse...", BS_PUSHBUTTON | WS_TABSTOP,
            BrowseFolderButton, 618, 50, 82, 28, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Open", BS_PUSHBUTTON | WS_TABSTOP,
            OpenFolderButton, 708, 50, 76, 28, state->font);

        HWND list = Add(window, state->instance, WC_LISTBOXW, L"",
                        LBS_NOTIFY | LBS_NOINTEGRALHEIGHT | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                        ImageList, 18, 92, 566, 365, state->font, WS_EX_CLIENTEDGE);
        SetWindowTextW(list, L"Static slideshow images");
        SendMessageW(list, LB_SETITEMHEIGHT, 0, ScaleDialogMetric(23, state->dpi));
        Add(window, state->instance, WC_BUTTONW, L"Add Images...", BS_PUSHBUTTON | WS_TABSTOP,
            AddImagesButton, 600, 92, 184, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Add BMPs from Folder", BS_PUSHBUTTON | WS_TABSTOP,
            ScanFolderButton, 600, 130, 184, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Remove", BS_PUSHBUTTON | WS_TABSTOP,
            RemoveButton, 600, 184, 184, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Move Up", BS_PUSHBUTTON | WS_TABSTOP,
            MoveUpButton, 600, 222, 88, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Move Down", BS_PUSHBUTTON | WS_TABSTOP,
            MoveDownButton, 696, 222, 88, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Set as Current", BS_PUSHBUTTON | WS_TABSTOP,
            SetCurrentButton, 600, 276, 184, 30, state->font);
        Add(window, state->instance, WC_BUTTONW, L"Clear List", BS_PUSHBUTTON | WS_TABSTOP,
            ClearButton, 600, 314, 184, 30, state->font);
        Add(window, state->instance, WC_STATICW,
            L"Only local 32-bit BMP files are loaded. Missing or invalid entries are skipped safely at playback time.",
            SS_LEFT, 0, 600, 370, 184, 76, state->font);

        Add(window, state->instance, WC_BUTTONW, L"Enable timed slideshow", BS_AUTOCHECKBOX | WS_TABSTOP,
            EnableCycleCheck, 18, 474, 210, 26, state->font);
        SetCheck(window, EnableCycleCheck, state->working.cycleEnabled);
        Add(window, state->instance, WC_STATICW, L"Interval (seconds)", SS_LEFT, 0, 246, 478, 124, 24, state->font);
        Add(window, state->instance, WC_EDITW, std::to_wstring(state->working.cycleSeconds).c_str(),
            ES_NUMBER | ES_AUTOHSCROLL | WS_TABSTOP, IntervalEdit, 370, 474, 82, 28, state->font, WS_EX_CLIENTEDGE);
        Add(window, state->instance, WC_STATICW, L"Order", SS_LEFT, 0, 474, 478, 54, 24, state->font);
        HWND order = Add(window, state->instance, WC_COMBOBOXW, L"", CBS_DROPDOWNLIST | WS_TABSTOP,
                         OrderCombo, 530, 474, 152, 150, state->font);
        SendMessageW(order, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Sequential"));
        SendMessageW(order, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"Shuffle"));
        SendMessageW(order, CB_SETCURSEL,
                     state->working.order == StaticSlideshowOrder::Shuffle ? 1 : 0, 0);
        Add(window, state->instance, WC_STATICW, L"", SS_LEFT, SummaryLabel, 18, 515, 420, 24, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&Save", BS_DEFPUSHBUTTON | WS_TABSTOP,
            SaveButton, 456, 548, 100, 32, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&Use Selected Now", BS_PUSHBUTTON | WS_TABSTOP,
            StartSelectedButton, 566, 548, 132, 32, state->font);
        Add(window, state->instance, WC_BUTTONW, L"&Cancel", BS_PUSHBUTTON | WS_TABSTOP,
            CancelButton, 708, 548, 76, 32, state->font);
        RefreshList(*state);
        SendMessageW(window, DM_SETDEFID, SaveButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 620, 450);
        state->layout.Focus(GetDlgItem(window, ImageList));
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
        SendMessageW(GetDlgItem(window, ImageList), LB_SETITEMHEIGHT, 0, ScaleDialogMetric(23, newDpi));
        return 0;
    }

    if (message == WM_COMMAND) {
        const int id = LOWORD(wParam);
        const int notification = HIWORD(wParam);
        if (id == BrowseFolderButton) {
            const auto folder = BrowseFolder(window);
            if (!folder.empty()) SetWindowTextW(GetDlgItem(window, FolderEdit), folder.wstring().c_str());
        } else if (id == OpenFolderButton) {
            const auto folder = StorageFolder(*state);
            if (!folder.empty()) ShellExecuteW(window, L"open", folder.wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else if (id == AddImagesButton) AddFiles(*state);
        else if (id == ScanFolderButton) ScanFolder(*state);
        else if (id == RemoveButton) RemoveSelected(*state);
        else if (id == MoveUpButton) MoveSelected(*state, -1);
        else if (id == MoveDownButton) MoveSelected(*state, 1);
        else if (id == SetCurrentButton || (id == ImageList && notification == LBN_DBLCLK)) SetCurrent(*state);
        else if (id == ClearButton) {
            if (state->working.imagePaths.empty() ||
                MessageBoxW(window, L"Remove every image from the slideshow list? The files themselves will not be deleted.",
                            L"Clear Slideshow", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                state->working.imagePaths.clear();
                state->working.currentIndex = 0;
                RefreshList(*state);
            }
        } else if (id == SaveButton) Save(*state, SlideshowDialogResult::Saved);
        else if (id == StartSelectedButton) {
            SetCurrent(*state);
            Save(*state, SlideshowDialogResult::StartSelected);
        } else if (id == CancelButton) DestroyWindow(window);
        return 0;
    }
    if (message == WM_CLOSE) {
        DestroyWindow(window);
        return 0;
    }
    if (message == WM_DESTROY) {
        RememberDialogPlacement(window, kSlideshowClass, state->dpi);
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

SlideshowDialogResult SlideshowDialog::Show(HWND owner, HINSTANCE instance, StaticWallpaperSettings& settings) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kSlideshowClass;
    RegisterClassExW(&windowClass);

    DialogState state;
    state.owner = owner;
    state.instance = instance;
    state.destination = &settings;
    state.working = settings;

    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 820, 630, state.dpi, kSlideshowClass);
    HWND window = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT, kSlideshowClass, L"Static Wallpaper Slideshow",
                                  WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX |
                                  WS_POPUP | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
                                  dialogRect.left, dialogRect.top,
                                  dialogRect.right - dialogRect.left, dialogRect.bottom - dialogRect.top,
                                  owner, nullptr, instance, &state);
    if (!window) return SlideshowDialogResult::Cancelled;
    EnableWindow(owner, FALSE);

    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, CancelButton, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return state.result;
}
#endif

} // namespace mw
