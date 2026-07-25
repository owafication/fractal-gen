#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>
#include <vector>

namespace mw {

struct DisplayInfo {
    std::wstring deviceName;
    std::wstring friendlyName;
#ifdef _WIN32
    RECT bounds{};
    RECT workArea{};
    HMONITOR handle{nullptr};
#endif
    unsigned dpiX{96};
    unsigned dpiY{96};
    bool primary{false};
};

class DisplayManager {
public:
    static std::vector<DisplayInfo> Enumerate();
#ifdef _WIN32
    static RECT VirtualDesktopBounds(const std::vector<DisplayInfo>& displays);
#endif
};

} // namespace mw
