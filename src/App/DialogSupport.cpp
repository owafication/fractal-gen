#include "App/DialogSupport.h"

#include "Infrastructure/Paths.h"

#ifdef _WIN32
#include <commctrl.h>
#include <windowsx.h>
#endif

#include <algorithm>
#include <array>
#include <cmath>
#include <cwchar>
#include <iterator>
#include <filesystem>
#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

namespace mw {

#ifdef _WIN32
namespace {

UINT SafeWindowDpi(HWND window) noexcept {
    using GetDpiForWindowFunction = UINT(WINAPI*)(HWND);
    static const auto getDpiForWindow = reinterpret_cast<GetDpiForWindowFunction>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    if (getDpiForWindow && window) {
        const UINT dpi = getDpiForWindow(window);
        if (dpi >= 48U && dpi <= 768U) return dpi;
    }
    HDC dc = GetDC(window);
    const int dpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : 96;
    if (dc) ReleaseDC(window, dc);
    return static_cast<UINT>(std::clamp(dpi, 48, 768));
}

int ClampScrollPosition(int requested, int maximum, int page) noexcept {
    const int last = std::max(0, maximum - std::max(0, page - 1));
    return std::clamp(requested, 0, last);
}

struct StoredDialogPlacement {
    int left{0};
    int top{0};
    int width{0};
    int height{0};
    UINT dpi{96};
};

std::string PlacementKey(const wchar_t* key) {
    if (!key) return {};
    std::string result;
    for (const wchar_t* cursor = key; *cursor != L'\0'; ++cursor) {
        if (*cursor < 32 || *cursor > 126 || *cursor == L'|') return {};
        result.push_back(static_cast<char>(*cursor));
    }
    return result;
}

class DialogPlacementStore {
public:
    static DialogPlacementStore& Instance() {
        static DialogPlacementStore store;
        return store;
    }

    bool Find(const std::string& key, StoredDialogPlacement& placement) {
        std::scoped_lock lock(mutex_);
        LoadLocked();
        const auto found = placements_.find(key);
        if (found == placements_.end()) return false;
        placement = found->second;
        return true;
    }

    void Save(const std::string& key, const StoredDialogPlacement& placement) {
        if (key.empty()) return;
        std::scoped_lock lock(mutex_);
        LoadLocked();
        placements_[key] = placement;
        WriteLocked();
    }

private:
    std::filesystem::path Path() const {
        return Paths::SettingsPath().parent_path() / L"dialog-layout.txt";
    }

    void LoadLocked() {
        if (loaded_) return;
        loaded_ = true;
        std::ifstream input{Path()};
        std::string line;
        while (std::getline(input, line)) {
            std::stringstream stream(line);
            std::string key;
            std::string part;
            if (!std::getline(stream, key, '|') || key.empty()) continue;
            int values[5]{};
            bool valid = true;
            for (int& value : values) {
                if (!std::getline(stream, part, '|')) { valid = false; break; }
                try {
                    std::size_t consumed = 0;
                    value = std::stoi(part, &consumed);
                    if (consumed != part.size()) valid = false;
                } catch (...) { valid = false; }
                if (!valid) break;
            }
            if (!valid || values[2] < 240 || values[2] > 10000 ||
                values[3] < 160 || values[3] > 10000 || values[4] < 48 || values[4] > 768) continue;
            placements_[key] = {values[0], values[1], values[2], values[3], static_cast<UINT>(values[4])};
        }
    }

    void WriteLocked() {
        const auto path = Path();
        std::error_code error;
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) return;
        const auto temporary = path.wstring() + L".tmp";
        {
            std::ofstream output{std::filesystem::path(temporary), std::ios::trunc};
            if (!output) return;
            for (const auto& [key, placement] : placements_) {
                output << key << '|' << placement.left << '|' << placement.top << '|'
                       << placement.width << '|' << placement.height << '|' << placement.dpi << '\n';
            }
            if (!output) return;
        }
        std::filesystem::rename(std::filesystem::path(temporary), path, error);
        if (error) {
            error.clear();
            std::filesystem::remove(path, error);
            error.clear();
            std::filesystem::rename(std::filesystem::path(temporary), path, error);
        }
        if (error) {
            error.clear();
            std::filesystem::remove(std::filesystem::path(temporary), error);
        }
    }

    bool loaded_{false};
    std::map<std::string, StoredDialogPlacement> placements_;
    std::mutex mutex_;
};

RECT ClampPlacementToWorkArea(RECT rect) {
    HMONITOR monitor = MonitorFromRect(&rect, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) return rect;
    const int width = std::min(static_cast<int>(rect.right - rect.left),
                               static_cast<int>(info.rcWork.right - info.rcWork.left));
    const int height = std::min(static_cast<int>(rect.bottom - rect.top),
                                static_cast<int>(info.rcWork.bottom - info.rcWork.top));
    const int left = std::clamp(static_cast<int>(rect.left), static_cast<int>(info.rcWork.left),
                                std::max(static_cast<int>(info.rcWork.left), static_cast<int>(info.rcWork.right) - width));
    const int top = std::clamp(static_cast<int>(rect.top), static_cast<int>(info.rcWork.top),
                               std::max(static_cast<int>(info.rcWork.top), static_cast<int>(info.rcWork.bottom) - height));
    return {left, top, left + width, top + height};
}

} // namespace

UINT DialogDpi(HWND owner) noexcept {
    return SafeWindowDpi(owner);
}

int ScaleDialogMetric(int value, UINT dpi) noexcept {
    return MulDiv(value, static_cast<int>(dpi), 96);
}

DWORD AccessibleControlStyle(const wchar_t* className, DWORD style) noexcept {
    if (!className) return style;
    if (_wcsicmp(className, L"Edit") == 0 ||
        _wcsicmp(className, L"ComboBox") == 0 ||
        _wcsicmp(className, L"ListBox") == 0 ||
        _wcsicmp(className, L"SysListView32") == 0 ||
        _wcsicmp(className, L"msctls_trackbar32") == 0) {
        return style | WS_TABSTOP;
    }
    if (_wcsicmp(className, L"Button") == 0 && (style & BS_TYPEMASK) != BS_GROUPBOX) {
        return style | WS_TABSTOP;
    }
    return style;
}

HFONT CreateResponsiveDialogFont(UINT dpi) {
    NONCLIENTMETRICSW metrics{};
    metrics.cbSize = static_cast<UINT>(sizeof(metrics));
    if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, static_cast<UINT>(sizeof(metrics)), &metrics, 0)) {
        const UINT systemDpi = SafeWindowDpi(GetDesktopWindow());
        if (systemDpi > 0U) {
            metrics.lfMessageFont.lfHeight = MulDiv(
                metrics.lfMessageFont.lfHeight, static_cast<int>(dpi), static_cast<int>(systemDpi));
            metrics.lfMessageFont.lfWidth = MulDiv(
                metrics.lfMessageFont.lfWidth, static_cast<int>(dpi), static_cast<int>(systemDpi));
        }
        metrics.lfMessageFont.lfQuality = CLEARTYPE_QUALITY;
        if (HFONT font = CreateFontIndirectW(&metrics.lfMessageFont)) return font;
    }
    return CreateFontW(-ScaleDialogMetric(16, dpi), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
}

RECT ResponsiveDialogRect(HWND owner, int widthAt96Dpi, int heightAt96Dpi, UINT dpi,
                          const wchar_t* placementKey) {
    int desiredWidth = std::max(320, ScaleDialogMetric(widthAt96Dpi, dpi));
    int desiredHeight = std::max(240, ScaleDialogMetric(heightAt96Dpi, dpi));
    StoredDialogPlacement stored;
    const std::string key = PlacementKey(placementKey);
    const bool hasStoredPlacement = !key.empty() && DialogPlacementStore::Instance().Find(key, stored);
    if (hasStoredPlacement && stored.dpi > 0U) {
        desiredWidth = std::max(320, MulDiv(stored.width, static_cast<int>(dpi), static_cast<int>(stored.dpi)));
        desiredHeight = std::max(240, MulDiv(stored.height, static_cast<int>(dpi), static_cast<int>(stored.dpi)));
    }

    HMONITOR monitor = MonitorFromWindow(owner, MONITOR_DEFAULTTONEAREST);
    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (!GetMonitorInfoW(monitor, &monitorInfo)) {
        monitorInfo.rcWork = {0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
    }
    const RECT work = monitorInfo.rcWork;
    const int margin = ScaleDialogMetric(12, dpi);
    const int availableWidth = std::max(320, static_cast<int>(work.right - work.left) - margin * 2);
    const int availableHeight = std::max(240, static_cast<int>(work.bottom - work.top) - margin * 2);
    const int width = std::min(desiredWidth, availableWidth);
    const int height = std::min(desiredHeight, availableHeight);

    RECT ownerRect = work;
    if (owner && IsWindow(owner)) GetWindowRect(owner, &ownerRect);
    int x = hasStoredPlacement ? stored.left
        : static_cast<int>(ownerRect.left) + (static_cast<int>(ownerRect.right - ownerRect.left) - width) / 2;
    int y = hasStoredPlacement ? stored.top
        : static_cast<int>(ownerRect.top) + (static_cast<int>(ownerRect.bottom - ownerRect.top) - height) / 2;
    x = std::clamp(x, static_cast<int>(work.left) + margin,
                   std::max(static_cast<int>(work.left) + margin, static_cast<int>(work.right) - width - margin));
    y = std::clamp(y, static_cast<int>(work.top) + margin,
                   std::max(static_cast<int>(work.top) + margin, static_cast<int>(work.bottom) - height - margin));
    return {x, y, x + width, y + height};
}

void RememberDialogPlacement(HWND window, const wchar_t* placementKey, UINT dpi) {
    if (!window || !placementKey || IsIconic(window)) return;
    RECT rect{};
    if (!GetWindowRect(window, &rect)) return;
    rect = ClampPlacementToWorkArea(rect);
    const int width = static_cast<int>(rect.right - rect.left);
    const int height = static_cast<int>(rect.bottom - rect.top);
    if (width < 240 || height < 160) return;
    const std::string key = PlacementKey(placementKey);
    DialogPlacementStore::Instance().Save(key, {static_cast<int>(rect.left), static_cast<int>(rect.top),
                                                width, height, std::clamp(dpi, 48U, 768U)});
}

void ResponsiveDialogLayout::Initialise(HWND window, UINT dpi, HFONT font,
                                        int minimumWidthAt96Dpi, int minimumHeightAt96Dpi) {
    window_ = window;
    dpi_ = std::clamp(dpi, 48U, 768U);
    minimumWidth_ = ScaleDialogMetric(minimumWidthAt96Dpi, dpi_);
    minimumHeight_ = ScaleDialogMetric(minimumHeightAt96Dpi, dpi_);
    CaptureAndScaleChildren(font);
    ApplyLayout();
}

void ResponsiveDialogLayout::Shutdown() noexcept {
    children_.clear();
    window_ = nullptr;
}

void ResponsiveDialogLayout::CaptureAndScaleChildren(HFONT font) {
    children_.clear();
    if (!window_) return;

    RECT client{};
    GetClientRect(window_, &client);
    baselineWidth_ = std::max(1, static_cast<int>(client.right - client.left));
    baselineHeight_ = std::max(1, static_cast<int>(client.bottom - client.top));

    int maximumRight = 0;
    int maximumBottom = 0;
    for (HWND child = GetWindow(window_, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
        RECT rect{};
        GetWindowRect(child, &rect);
        POINT topLeft{rect.left, rect.top};
        POINT bottomRight{rect.right, rect.bottom};
        ScreenToClient(window_, &topLeft);
        ScreenToClient(window_, &bottomRight);
        rect = {
            ScaleDialogMetric(topLeft.x, dpi_),
            ScaleDialogMetric(topLeft.y, dpi_),
            ScaleDialogMetric(bottomRight.x, dpi_),
            ScaleDialogMetric(bottomRight.y, dpi_),
        };
        wchar_t className[64]{};
        GetClassNameW(child, className, static_cast<int>(std::size(className)));
        const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(child, GWL_STYLE));
        bool stretchWidth = true;
        bool stretchHeight = false;
        if (_wcsicmp(className, L"ListBox") == 0) {
            stretchHeight = true;
        } else if (_wcsicmp(className, L"Edit") == 0) {
            stretchHeight = (style & ES_MULTILINE) != 0;
        } else if (_wcsicmp(className, L"Button") == 0) {
            const DWORD type = style & BS_TYPEMASK;
            if (type == BS_GROUPBOX) stretchHeight = true;
            if (type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON) stretchWidth = false;
        } else if (_wcsicmp(className, L"Static") == 0) {
            stretchHeight = (rect.bottom - rect.top) >= ScaleDialogMetric(40, dpi_);
        } else if (_wcsicmp(className, L"ComboBox") == 0 ||
                   _wcsicmp(className, L"msctls_trackbar32") == 0) {
            stretchHeight = false;
        }
        children_.push_back({child, rect, stretchWidth, stretchHeight});
        maximumRight = std::max(maximumRight, static_cast<int>(rect.right));
        maximumBottom = std::max(maximumBottom, static_cast<int>(rect.bottom));
        if (font) SendMessageW(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    }

    const int margin = ScaleDialogMetric(18, dpi_);
    baselineWidth_ = std::max(baselineWidth_, maximumRight + margin);
    baselineHeight_ = std::max(baselineHeight_, maximumBottom + margin);
}

void ResponsiveDialogLayout::OnSize() {
    ApplyLayout();
}

void ResponsiveDialogLayout::SetScrollPosition(int bar, int position) {
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    GetScrollInfo(window_, bar, &info);
    info.nPos = ClampScrollPosition(position, info.nMax + 1, static_cast<int>(info.nPage));
    info.fMask = SIF_POS;
    SetScrollInfo(window_, bar, &info, TRUE);
    if (bar == SB_HORZ) horizontalOffset_ = info.nPos;
    else verticalOffset_ = info.nPos;
}

bool ResponsiveDialogLayout::OnScroll(UINT message, WPARAM wParam) {
    if (!window_) return false;
    const int bar = message == WM_HSCROLL ? SB_HORZ : SB_VERT;
    SCROLLINFO info{};
    info.cbSize = sizeof(info);
    info.fMask = SIF_ALL;
    if (!GetScrollInfo(window_, bar, &info)) return false;

    int position = info.nPos;
    const int line = std::max(8, ScaleDialogMetric(24, dpi_));
    const int page = std::max(line, static_cast<int>(info.nPage) - line);
    switch (LOWORD(wParam)) {
    case SB_LINELEFT: position -= line; break;
    case SB_LINERIGHT: position += line; break;
    case SB_PAGELEFT: position -= page; break;
    case SB_PAGERIGHT: position += page; break;
    case SB_THUMBTRACK:
    case SB_THUMBPOSITION: position = info.nTrackPos; break;
    case SB_LEFT: position = 0; break;
    case SB_RIGHT: position = info.nMax; break;
    default: return false;
    }
    SetScrollPosition(bar, position);
    ApplyLayout();
    return true;
}

bool ResponsiveDialogLayout::OnMouseWheel(WPARAM wParam) {
    if (!window_) return false;
    const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    if (delta == 0) return false;
    const int lines = std::max(1, std::abs(delta) / WHEEL_DELTA) * 3;
    const int line = std::max(8, ScaleDialogMetric(24, dpi_));
    SetScrollPosition(SB_VERT, verticalOffset_ - (delta > 0 ? lines * line : -lines * line));
    ApplyLayout();
    return true;
}

void ResponsiveDialogLayout::OnDpiChanged(UINT newDpi, const RECT& suggestedRect, HFONT newFont) {
    if (!window_ || newDpi == 0U || newDpi == dpi_) return;
    const UINT oldDpi = dpi_;
    dpi_ = std::clamp(newDpi, 48U, 768U);
    minimumWidth_ = MulDiv(minimumWidth_, static_cast<int>(dpi_), static_cast<int>(oldDpi));
    minimumHeight_ = MulDiv(minimumHeight_, static_cast<int>(dpi_), static_cast<int>(oldDpi));
    baselineWidth_ = MulDiv(baselineWidth_, static_cast<int>(dpi_), static_cast<int>(oldDpi));
    baselineHeight_ = MulDiv(baselineHeight_, static_cast<int>(dpi_), static_cast<int>(oldDpi));
    horizontalOffset_ = MulDiv(horizontalOffset_, static_cast<int>(dpi_), static_cast<int>(oldDpi));
    verticalOffset_ = MulDiv(verticalOffset_, static_cast<int>(dpi_), static_cast<int>(oldDpi));
    for (auto& child : children_) {
        child.rect.left = MulDiv(child.rect.left, static_cast<int>(dpi_), static_cast<int>(oldDpi));
        child.rect.top = MulDiv(child.rect.top, static_cast<int>(dpi_), static_cast<int>(oldDpi));
        child.rect.right = MulDiv(child.rect.right, static_cast<int>(dpi_), static_cast<int>(oldDpi));
        child.rect.bottom = MulDiv(child.rect.bottom, static_cast<int>(dpi_), static_cast<int>(oldDpi));
        if (newFont) SendMessageW(child.window, WM_SETFONT, reinterpret_cast<WPARAM>(newFont), TRUE);
    }
    SetWindowPos(window_, nullptr, suggestedRect.left, suggestedRect.top,
                 suggestedRect.right - suggestedRect.left, suggestedRect.bottom - suggestedRect.top,
                 SWP_NOACTIVATE | SWP_NOZORDER);
    ApplyLayout();
}

void ResponsiveDialogLayout::ApplyMinimumTrackSize(MINMAXINFO& info) const noexcept {
    info.ptMinTrackSize.x = std::max<LONG>(info.ptMinTrackSize.x, static_cast<LONG>(minimumWidth_));
    info.ptMinTrackSize.y = std::max<LONG>(info.ptMinTrackSize.y, static_cast<LONG>(minimumHeight_));
}

void ResponsiveDialogLayout::Focus(HWND control) const noexcept {
    if (!window_ || !control) return;
    PostMessageW(window_, WM_NEXTDLGCTL, reinterpret_cast<WPARAM>(control), TRUE);
}

void ResponsiveDialogLayout::EnsureFocusedControlVisible() {
    if (!window_) return;
    HWND focused = GetFocus();
    if (!focused || (focused != window_ && !IsChild(window_, focused))) return;
    while (focused && GetParent(focused) != window_) focused = GetParent(focused);
    if (!focused) return;

    RECT rect{};
    GetWindowRect(focused, &rect);
    POINT topLeft{rect.left, rect.top};
    POINT bottomRight{rect.right, rect.bottom};
    ScreenToClient(window_, &topLeft);
    ScreenToClient(window_, &bottomRight);
    RECT client{};
    GetClientRect(window_, &client);
    const int margin = ScaleDialogMetric(8, dpi_);

    const int clientRight = static_cast<int>(client.right);
    const int clientBottom = static_cast<int>(client.bottom);
    int nextHorizontal = horizontalOffset_;
    int nextVertical = verticalOffset_;
    if (topLeft.x < margin) nextHorizontal += topLeft.x - margin;
    else if (bottomRight.x > clientRight - margin) nextHorizontal += bottomRight.x - clientRight + margin;
    if (topLeft.y < margin) nextVertical += topLeft.y - margin;
    else if (bottomRight.y > clientBottom - margin) nextVertical += bottomRight.y - clientBottom + margin;

    if (nextHorizontal != horizontalOffset_) SetScrollPosition(SB_HORZ, nextHorizontal);
    if (nextVertical != verticalOffset_) SetScrollPosition(SB_VERT, nextVertical);
    ApplyLayout();
}

bool DialogTooltipManager::Initialise(HWND owner, UINT dpi, HFONT font) {
    Shutdown();
    dpi_ = std::clamp(dpi, 48U, 768U);
    tooltip_ = CreateWindowExW(WS_EX_TOPMOST, TOOLTIPS_CLASSW, nullptr,
                               WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               owner, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!tooltip_) return false;
    SendMessageW(tooltip_, TTM_SETMAXTIPWIDTH, 0, ScaleDialogMetric(440, dpi_));
    SendMessageW(tooltip_, TTM_SETDELAYTIME, TTDT_AUTOPOP, 15000);
    if (font) SendMessageW(tooltip_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
    return true;
}

void DialogTooltipManager::Add(HWND control, const wchar_t* text) {
    if (!tooltip_ || !control || !text || *text == L'\0') return;
    texts_.emplace_back(text);
    TTTOOLINFOW tool{};
    tool.cbSize = static_cast<UINT>(sizeof(tool));
    tool.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    tool.hwnd = GetParent(control);
    tool.uId = reinterpret_cast<UINT_PTR>(control);
    tool.lpszText = texts_.back().data();
    SendMessageW(tooltip_, TTM_ADDTOOLW, 0, reinterpret_cast<LPARAM>(&tool));
}

void DialogTooltipManager::SetFont(UINT dpi, HFONT font) {
    dpi_ = std::clamp(dpi, 48U, 768U);
    if (!tooltip_) return;
    SendMessageW(tooltip_, TTM_SETMAXTIPWIDTH, 0, ScaleDialogMetric(440, dpi_));
    if (font) SendMessageW(tooltip_, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

void DialogTooltipManager::Shutdown() noexcept {
    if (tooltip_) DestroyWindow(tooltip_);
    tooltip_ = nullptr;
    texts_.clear();
}

bool ProcessKeyboardDialogMessage(HWND dialog, MSG& message,
                                  ResponsiveDialogLayout* layout) {
    if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN) {
        HWND focused = GetFocus();
        if (focused && (focused == dialog || IsChild(dialog, focused))) {
            wchar_t className[32]{};
            GetClassNameW(focused, className, static_cast<int>(std::size(className)));
            const DWORD style = static_cast<DWORD>(GetWindowLongPtrW(focused, GWL_STYLE));
            const bool multilineEdit = _wcsicmp(className, L"Edit") == 0 && (style & ES_MULTILINE) != 0;
            const bool openCombo = _wcsicmp(className, L"ComboBox") == 0 &&
                                   SendMessageW(focused, CB_GETDROPPEDSTATE, 0, 0) != 0;
            const DWORD buttonType = style & BS_TYPEMASK;
            if (_wcsicmp(className, L"Button") == 0 &&
                (buttonType == BS_PUSHBUTTON || buttonType == BS_DEFPUSHBUTTON) &&
                IsWindowEnabled(focused)) {
                SendMessageW(focused, BM_CLICK, 0, 0);
                return true;
            }
            if (!multilineEdit && !openCombo) {
                HWND defaultButton = nullptr;
                for (HWND child = GetWindow(dialog, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
                    wchar_t childClass[32]{};
                    GetClassNameW(child, childClass, static_cast<int>(std::size(childClass)));
                    const DWORD childStyle = static_cast<DWORD>(GetWindowLongPtrW(child, GWL_STYLE));
                    if (_wcsicmp(childClass, L"Button") == 0 &&
                        (childStyle & BS_TYPEMASK) == BS_DEFPUSHBUTTON && IsWindowEnabled(child)) {
                        defaultButton = child;
                        break;
                    }
                }
                if (defaultButton) {
                    SendMessageW(defaultButton, BM_CLICK, 0, 0);
                    return true;
                }
            }
        }
    }
    const bool handled = IsDialogMessageW(dialog, &message) != FALSE;
    if (handled && layout) layout->EnsureFocusedControlVisible();
    return handled;
}

bool ProcessModalDialogMessage(HWND dialog, int cancelCommandId, MSG& message,
                               ResponsiveDialogLayout* layout) {
    if (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE) {
        PostMessageW(dialog, WM_COMMAND, MAKEWPARAM(cancelCommandId, BN_CLICKED), 0);
        return true;
    }
    return ProcessKeyboardDialogMessage(dialog, message, layout);
}

void ResponsiveDialogLayout::UpdateScrollBars(int clientWidth, int clientHeight,
                                               int contentWidth, int contentHeight) {
    SCROLLINFO horizontal{};
    horizontal.cbSize = sizeof(horizontal);
    horizontal.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    horizontal.nMin = 0;
    horizontal.nMax = std::max(0, contentWidth - 1);
    horizontal.nPage = static_cast<UINT>(std::max(0, clientWidth));
    horizontalOffset_ = ClampScrollPosition(horizontalOffset_, contentWidth, clientWidth);
    horizontal.nPos = horizontalOffset_;
    SetScrollInfo(window_, SB_HORZ, &horizontal, TRUE);

    SCROLLINFO vertical{};
    vertical.cbSize = sizeof(vertical);
    vertical.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    vertical.nMin = 0;
    vertical.nMax = std::max(0, contentHeight - 1);
    vertical.nPage = static_cast<UINT>(std::max(0, clientHeight));
    verticalOffset_ = ClampScrollPosition(verticalOffset_, contentHeight, clientHeight);
    vertical.nPos = verticalOffset_;
    SetScrollInfo(window_, SB_VERT, &vertical, TRUE);
}

void ResponsiveDialogLayout::ApplyLayout() {
    if (!window_ || children_.empty()) return;
    RECT client{};
    GetClientRect(window_, &client);
    const int clientWidth = std::max(1, static_cast<int>(client.right - client.left));
    const int clientHeight = std::max(1, static_cast<int>(client.bottom - client.top));
    const int contentWidth = std::max(clientWidth, baselineWidth_);
    const int contentHeight = std::max(clientHeight, baselineHeight_);
    const double scaleX = static_cast<double>(contentWidth) / static_cast<double>(baselineWidth_);
    const double scaleY = static_cast<double>(contentHeight) / static_cast<double>(baselineHeight_);

    UpdateScrollBars(clientWidth, clientHeight, contentWidth, contentHeight);

    for (const auto& child : children_) {
        const int left = static_cast<int>(std::lround(child.rect.left * scaleX)) - horizontalOffset_;
        const int top = static_cast<int>(std::lround(child.rect.top * scaleY)) - verticalOffset_;
        const int baseWidth = static_cast<int>(child.rect.right - child.rect.left);
        const int baseHeight = static_cast<int>(child.rect.bottom - child.rect.top);
        const int width = child.stretchWidth
            ? std::max(1, static_cast<int>(std::lround(baseWidth * scaleX)))
            : std::max(1, baseWidth);
        const int height = child.stretchHeight
            ? std::max(1, static_cast<int>(std::lround(baseHeight * scaleY)))
            : std::max(1, baseHeight);
        MoveWindow(child.window, left, top, width, height, TRUE);
    }
}

#endif

} // namespace mw
