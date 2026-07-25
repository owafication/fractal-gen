#include "WindowsIntegration/DisplayManager.h"

#ifdef _WIN32
#include <shellscalingapi.h>
#endif

#include <algorithm>

namespace mw {

#ifdef _WIN32
namespace {

using GetDpiForMonitorFn = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);

BOOL CALLBACK MonitorCallback(HMONITOR monitor, HDC, LPRECT, LPARAM data) {
    auto* displays = reinterpret_cast<std::vector<DisplayInfo>*>(data);
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) return TRUE;

    DisplayInfo display;
    display.handle = monitor;
    display.deviceName = info.szDevice;
    display.friendlyName = info.szDevice;
    display.bounds = info.rcMonitor;
    display.workArea = info.rcWork;
    display.primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

    HMODULE shcore = LoadLibraryW(L"Shcore.dll");
    if (shcore) {
        const auto getDpi = reinterpret_cast<GetDpiForMonitorFn>(GetProcAddress(shcore, "GetDpiForMonitor"));
        if (getDpi) {
            UINT x = 96;
            UINT y = 96;
            if (SUCCEEDED(getDpi(monitor, 0, &x, &y))) {
                display.dpiX = x;
                display.dpiY = y;
            }
        }
        FreeLibrary(shcore);
    }

    DISPLAY_DEVICEW device{};
    device.cb = sizeof(device);
    if (EnumDisplayDevicesW(info.szDevice, 0, &device, 0) && device.DeviceString[0]) {
        display.friendlyName = device.DeviceString;
    }
    displays->push_back(std::move(display));
    return TRUE;
}

} // namespace
#endif

std::vector<DisplayInfo> DisplayManager::Enumerate() {
    std::vector<DisplayInfo> displays;
#ifdef _WIN32
    EnumDisplayMonitors(nullptr, nullptr, MonitorCallback, reinterpret_cast<LPARAM>(&displays));
    std::stable_sort(displays.begin(), displays.end(), [](const DisplayInfo& a, const DisplayInfo& b) {
        if (a.primary != b.primary) return a.primary;
        if (a.bounds.top != b.bounds.top) return a.bounds.top < b.bounds.top;
        return a.bounds.left < b.bounds.left;
    });
#endif
    return displays;
}

#ifdef _WIN32
RECT DisplayManager::VirtualDesktopBounds(const std::vector<DisplayInfo>& displays) {
    if (displays.empty()) {
        return {GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN),
                GetSystemMetrics(SM_XVIRTUALSCREEN) + GetSystemMetrics(SM_CXVIRTUALSCREEN),
                GetSystemMetrics(SM_YVIRTUALSCREEN) + GetSystemMetrics(SM_CYVIRTUALSCREEN)};
    }
    RECT result = displays.front().bounds;
    for (const auto& display : displays) {
        result.left = std::min(result.left, display.bounds.left);
        result.top = std::min(result.top, display.bounds.top);
        result.right = std::max(result.right, display.bounds.right);
        result.bottom = std::max(result.bottom, display.bounds.bottom);
    }
    return result;
}
#endif

} // namespace mw
