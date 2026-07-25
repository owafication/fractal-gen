#pragma once

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#include <string>

namespace mw {

class TrayIcon {
public:
#ifdef _WIN32
    static constexpr UINT MessageId = WM_APP + 20;
    bool Create(HWND owner, HICON icon, const std::wstring& tooltip);
    void Remove();
    void ShowMenu(POINT screenPoint, bool paused, bool startupEnabled);
#endif

private:
#ifdef _WIN32
    NOTIFYICONDATAW data_{};
    bool created_{false};
#endif
};

namespace TrayCommands {
constexpr unsigned Open = 41001;
constexpr unsigned Pause = 41002;
constexpr unsigned Resume = 41003;
constexpr unsigned NextPreset = 41004;
constexpr unsigned PreviousPreset = 41005;
constexpr unsigned Stop = 41006;
constexpr unsigned Startup = 41007;
constexpr unsigned Exit = 41008;
constexpr unsigned Controller = 41009;
}

} // namespace mw
