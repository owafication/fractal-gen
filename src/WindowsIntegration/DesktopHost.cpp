#include "WindowsIntegration/DesktopHost.h"

#include "Infrastructure/Logger.h"

#ifdef _WIN32
#include <windowsx.h>
#endif

namespace mw {

#ifdef _WIN32
namespace {
constexpr UINT kSpawnWorkerMessage = 0x052C;

BOOL CALLBACK FindWorkerCallback(HWND topLevel, LPARAM data) {
    auto* result = reinterpret_cast<HWND*>(data);
    const HWND shellView = FindWindowExW(topLevel, nullptr, L"SHELLDLL_DefView", nullptr);
    if (shellView) {
        const HWND worker = FindWindowExW(nullptr, topLevel, L"WorkerW", nullptr);
        if (worker) {
            *result = worker;
            return FALSE;
        }
        *result = topLevel;
        return FALSE;
    }
    return TRUE;
}
} // namespace

HWND DesktopHost::FindHost(std::string& error) {
    HWND progman = FindWindowW(L"Progman", nullptr);
    if (!progman) {
        error = "The Progman desktop window was not found.";
        return nullptr;
    }

    DWORD_PTR ignored = 0;
    SendMessageTimeoutW(progman, kSpawnWorkerMessage, 0, 0, SMTO_NORMAL, 1000, &ignored);
    HWND host = nullptr;
    EnumWindows(FindWorkerCallback, reinterpret_cast<LPARAM>(&host));
    if (!host) {
        host = progman;
        LogWarning("WorkerW was not found; falling back to Progman.");
    }
    hostWindow_ = host;
    return hostWindow_;
}

bool DesktopHost::Attach(HWND wallpaperWindow, std::string& error) {
    if (!IsWindow(hostWindow_)) {
        if (!FindHost(error)) return false;
    }

    SetLastError(ERROR_SUCCESS);
    const HWND previousParent = SetParent(wallpaperWindow, hostWindow_);
    if (!previousParent && GetLastError() != ERROR_SUCCESS) {
        error = "SetParent failed while attaching the wallpaper window.";
        return false;
    }

    LONG_PTR style = GetWindowLongPtrW(wallpaperWindow, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_POPUP);
    style |= WS_CHILD | WS_VISIBLE;
    SetWindowLongPtrW(wallpaperWindow, GWL_STYLE, style);

    LONG_PTR extendedStyle = GetWindowLongPtrW(wallpaperWindow, GWL_EXSTYLE);
    extendedStyle |= WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW;
    extendedStyle &= ~WS_EX_APPWINDOW;
    SetWindowLongPtrW(wallpaperWindow, GWL_EXSTYLE, extendedStyle);

    SetWindowPos(wallpaperWindow, HWND_BOTTOM, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    LogInfo("Wallpaper window attached to desktop host.");
    return true;
}

bool DesktopHost::IsAttachmentValid(HWND wallpaperWindow) const {
    return IsWindow(hostWindow_) && IsWindow(wallpaperWindow) && GetParent(wallpaperWindow) == hostWindow_;
}

RECT DesktopHost::MapDesktopRectToHost(const RECT& desktopRect) const {
    RECT mapped = desktopRect;
    if (!IsWindow(hostWindow_)) return mapped;
    POINT points[2]{{desktopRect.left, desktopRect.top}, {desktopRect.right, desktopRect.bottom}};
    SetLastError(ERROR_SUCCESS);
    if (MapWindowPoints(HWND_DESKTOP, hostWindow_, points, 2) == 0 && GetLastError() != ERROR_SUCCESS) {
        return mapped;
    }
    mapped.left = points[0].x;
    mapped.top = points[0].y;
    mapped.right = points[1].x;
    mapped.bottom = points[1].y;
    return mapped;
}

void DesktopHost::Detach(HWND wallpaperWindow) {
    if (!IsWindow(wallpaperWindow)) return;
    ShowWindow(wallpaperWindow, SW_HIDE);
    SetParent(wallpaperWindow, nullptr);
    LONG_PTR style = GetWindowLongPtrW(wallpaperWindow, GWL_STYLE);
    style &= ~WS_CHILD;
    style |= WS_POPUP;
    SetWindowLongPtrW(wallpaperWindow, GWL_STYLE, style);
}
#endif

} // namespace mw
