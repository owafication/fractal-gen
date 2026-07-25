#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <chrono>
#include <cstdint>
#include <string>

namespace mw {

struct SystemState {
    bool onBattery{false};
    bool fullscreenApplication{false};
    bool remoteDesktop{false};
    bool sessionLocked{false};
    bool suspended{false};
    bool desktopVisible{true};
    double processCpuPercent{0.0};
    std::uint64_t processWorkingSetBytes{0};
};

struct PauseDecision {
    bool shouldPause{false};
    std::string reason;
};

class SystemStateMonitor {
public:
#ifdef _WIN32
    bool Register(HWND messageWindow, std::string& error);
    void Unregister();
    void HandleMessage(UINT message, WPARAM wParam, LPARAM lParam);
#endif
    SystemState Poll();
    PauseDecision Evaluate(const PerformanceSettings& settings) const;
    [[nodiscard]] const SystemState& State() const noexcept { return state_; }

private:
#ifdef _WIN32
    static bool IsForegroundWindowFullscreen();
    static bool IsDesktopLikelyVisible();
    HWND messageWindow_{nullptr};
    std::uint64_t previousProcessTime100ns_{0};
    std::chrono::steady_clock::time_point previousResourceSample_{};
    unsigned processorCount_{1};
    bool resourceSampleInitialised_{false};
#endif
    SystemState state_;
};

} // namespace mw
