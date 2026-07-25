#include "WindowsIntegration/TrayIcon.h"

#ifdef _WIN32
#include <strsafe.h>
#endif

namespace mw {

#ifdef _WIN32
bool TrayIcon::Create(HWND owner, HICON icon, const std::wstring& tooltip) {
    data_ = {};
    data_.cbSize = sizeof(data_);
    data_.hWnd = owner;
    data_.uID = 1;
    data_.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    data_.uCallbackMessage = MessageId;
    data_.hIcon = icon;
    StringCchCopyW(data_.szTip, std::size(data_.szTip), tooltip.c_str());
    created_ = Shell_NotifyIconW(NIM_ADD, &data_) == TRUE;
    if (created_) {
        data_.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &data_);
    }
    return created_;
}

void TrayIcon::Remove() {
    if (created_) Shell_NotifyIconW(NIM_DELETE, &data_);
    created_ = false;
}

void TrayIcon::ShowMenu(POINT screenPoint, bool paused, bool startupEnabled) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, TrayCommands::Open, L"Open Mandelbrot Live Wallpaper");
    AppendMenuW(menu, MF_STRING, TrayCommands::Controller, L"Open Quick Controller");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (paused ? MF_GRAYED : 0), TrayCommands::Pause, L"Pause");
    AppendMenuW(menu, MF_STRING | (!paused ? MF_GRAYED : 0), TrayCommands::Resume, L"Resume");
    AppendMenuW(menu, MF_STRING, TrayCommands::NextPreset, L"Next Preset");
    AppendMenuW(menu, MF_STRING, TrayCommands::PreviousPreset, L"Previous Preset");
    AppendMenuW(menu, MF_STRING, TrayCommands::Stop, L"Stop Wallpaper");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING | (startupEnabled ? MF_CHECKED : 0), TrayCommands::Startup, L"Start with Windows");
    AppendMenuW(menu, MF_STRING, TrayCommands::Exit, L"Exit");
    SetForegroundWindow(data_.hWnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, screenPoint.x, screenPoint.y, 0, data_.hWnd, nullptr);
    DestroyMenu(menu);
}
#endif

} // namespace mw
