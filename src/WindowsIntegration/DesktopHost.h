#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>

namespace mw {

class DesktopHost {
public:
#ifdef _WIN32
    HWND FindHost(std::string& error);
    bool Attach(HWND wallpaperWindow, std::string& error);
    bool IsAttachmentValid(HWND wallpaperWindow) const;
    RECT MapDesktopRectToHost(const RECT& desktopRect) const;
    void Detach(HWND wallpaperWindow);
    [[nodiscard]] HWND HostWindow() const noexcept { return hostWindow_; }
#endif

private:
#ifdef _WIN32
    HWND hostWindow_{nullptr};
#endif
};

} // namespace mw
