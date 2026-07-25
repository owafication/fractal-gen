#include "WindowsIntegration/SystemStateMonitor.h"

#include "Infrastructure/Logger.h"

#include <algorithm>
#include <iterator>

#ifdef _WIN32
#include <dwmapi.h>
#include <wtsapi32.h>
#include <psapi.h>
#endif

namespace mw {

#ifdef _WIN32
bool SystemStateMonitor::Register(HWND messageWindow, std::string& error) {
    messageWindow_ = messageWindow;
    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    processorCount_ = static_cast<unsigned>(std::max<DWORD>(1UL, systemInfo.dwNumberOfProcessors));
    resourceSampleInitialised_ = false;
    if (!WTSRegisterSessionNotification(messageWindow_, NOTIFY_FOR_THIS_SESSION)) {
        error = "Session notifications could not be registered.";
        return false;
    }
    return true;
}

void SystemStateMonitor::Unregister() {
    if (messageWindow_) {
        WTSUnRegisterSessionNotification(messageWindow_);
        messageWindow_ = nullptr;
    }
}

void SystemStateMonitor::HandleMessage(UINT message, WPARAM wParam, LPARAM) {
    if (message == WM_WTSSESSION_CHANGE) {
        if (wParam == WTS_SESSION_LOCK) {
            state_.sessionLocked = true;
            LogInfo("Windows session locked.");
        } else if (wParam == WTS_SESSION_UNLOCK) {
            state_.sessionLocked = false;
            LogInfo("Windows session unlocked.");
        }
    } else if (message == WM_POWERBROADCAST) {
        if (wParam == PBT_APMSUSPEND) {
            state_.suspended = true;
            LogInfo("System suspend detected.");
        } else if (wParam == PBT_APMRESUMEAUTOMATIC || wParam == PBT_APMRESUMESUSPEND) {
            state_.suspended = false;
            LogInfo("System resume detected.");
        }
    }
}

bool SystemStateMonitor::IsForegroundWindowFullscreen() {
    const HWND foreground = GetForegroundWindow();
    if (!foreground || foreground == GetShellWindow() || IsIconic(foreground)) return false;
    wchar_t className[128]{};
    GetClassNameW(foreground, className, static_cast<int>(std::size(className)));
    if (wcscmp(className, L"Progman") == 0 || wcscmp(className, L"WorkerW") == 0 || wcscmp(className, L"Shell_TrayWnd") == 0) return false;

    RECT windowRect{};
    if (FAILED(DwmGetWindowAttribute(foreground, DWMWA_EXTENDED_FRAME_BOUNDS, &windowRect, sizeof(windowRect)))) {
        if (!GetWindowRect(foreground, &windowRect)) return false;
    }
    const HMONITOR monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) return false;
    constexpr int tolerance = 2;
    return windowRect.left <= info.rcMonitor.left + tolerance &&
           windowRect.top <= info.rcMonitor.top + tolerance &&
           windowRect.right >= info.rcMonitor.right - tolerance &&
           windowRect.bottom >= info.rcMonitor.bottom - tolerance;
}

bool SystemStateMonitor::IsDesktopLikelyVisible() {
    const HWND foreground = GetForegroundWindow();
    if (!foreground || foreground == GetShellWindow() || IsIconic(foreground)) return true;
    wchar_t className[128]{};
    GetClassNameW(foreground, className, static_cast<int>(std::size(className)));
    if (wcscmp(className, L"Progman") == 0 || wcscmp(className, L"WorkerW") == 0 || wcscmp(className, L"Shell_TrayWnd") == 0) return true;

    RECT windowRect{};
    if (FAILED(DwmGetWindowAttribute(foreground, DWMWA_EXTENDED_FRAME_BOUNDS, &windowRect, sizeof(windowRect)))) {
        if (!GetWindowRect(foreground, &windowRect)) return true;
    }
    const HMONITOR monitor = MonitorFromWindow(foreground, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) return true;
    const long windowWidth = std::max(0L, windowRect.right - windowRect.left);
    const long windowHeight = std::max(0L, windowRect.bottom - windowRect.top);
    const long workWidth = std::max(1L, info.rcWork.right - info.rcWork.left);
    const long workHeight = std::max(1L, info.rcWork.bottom - info.rcWork.top);
    const double coverage = static_cast<double>(windowWidth) * windowHeight / (static_cast<double>(workWidth) * workHeight);
    const bool coversWorkArea = windowRect.left <= info.rcWork.left + 4 && windowRect.top <= info.rcWork.top + 4 &&
                                windowRect.right >= info.rcWork.right - 4 && windowRect.bottom >= info.rcWork.bottom - 4;
    return !(coversWorkArea && coverage >= 0.95);
}
#endif

SystemState SystemStateMonitor::Poll() {
#ifdef _WIN32
    SYSTEM_POWER_STATUS power{};
    if (GetSystemPowerStatus(&power)) state_.onBattery = power.ACLineStatus == 0;
    state_.remoteDesktop = GetSystemMetrics(SM_REMOTESESSION) != 0;
    state_.fullscreenApplication = IsForegroundWindowFullscreen();
    state_.desktopVisible = IsDesktopLikelyVisible();

    FILETIME creation{}, exit{}, kernel{}, user{};
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        ULARGE_INTEGER kernelValue{};
        kernelValue.LowPart = kernel.dwLowDateTime;
        kernelValue.HighPart = kernel.dwHighDateTime;
        ULARGE_INTEGER userValue{};
        userValue.LowPart = user.dwLowDateTime;
        userValue.HighPart = user.dwHighDateTime;
        const std::uint64_t processTime = kernelValue.QuadPart + userValue.QuadPart;
        const auto now = std::chrono::steady_clock::now();
        if (resourceSampleInitialised_) {
            const double wallSeconds = std::chrono::duration<double>(now - previousResourceSample_).count();
            const std::uint64_t processDelta = processTime >= previousProcessTime100ns_
                ? processTime - previousProcessTime100ns_ : 0;
            if (wallSeconds > 0.0) {
                const double processSeconds = static_cast<double>(processDelta) / 10000000.0;
                state_.processCpuPercent = std::clamp(
                    processSeconds / (wallSeconds * static_cast<double>(processorCount_)) * 100.0, 0.0, 100.0);
            }
        }
        previousProcessTime100ns_ = processTime;
        previousResourceSample_ = now;
        resourceSampleInitialised_ = true;
    }

    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = static_cast<DWORD>(sizeof(memory));
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), static_cast<DWORD>(sizeof(memory)))) {
        state_.processWorkingSetBytes = static_cast<std::uint64_t>(memory.WorkingSetSize);
    }
#endif
    return state_;
}

PauseDecision SystemStateMonitor::Evaluate(const PerformanceSettings& settings) const {
    if (state_.suspended) return {true, "System is suspended"};
    if (settings.pauseWhenLocked && state_.sessionLocked) return {true, "Windows session is locked"};
    if (settings.pauseDuringRemoteDesktop && state_.remoteDesktop) return {true, "Remote Desktop session is active"};
    if (settings.pauseOnBattery && state_.onBattery) return {true, "Running on battery"};
    if (settings.pauseWhenFullscreen && state_.fullscreenApplication) return {true, "A full-screen application is active"};
    if (settings.pauseWhenDesktopHidden && !state_.desktopVisible) return {true, "Desktop is not visible"};
    return {};
}

} // namespace mw
