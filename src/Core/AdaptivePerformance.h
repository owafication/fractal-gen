#pragma once

#include "Core/Models.h"

#include <cstdint>
#include <string>
#include <vector>

namespace mw {

struct AdaptivePerformanceSample {
    double framesPerSecond{0.0};
    double targetFramesPerSecond{30.0};
    double processCpuPercent{0.0};
    double processWorkingSetMb{0.0};
    bool rendererActive{false};
    bool framesPerSecondMeaningful{false};
};

struct AdaptivePauseDecision {
    bool pauseNow{false};
    bool resumeNow{false};
    bool paused{false};
    std::string reason;
};

class AdaptivePerformanceController {
public:
    AdaptivePauseDecision Update(const AdaptivePerformanceSettings& settings,
                                    const AdaptivePerformanceSample& sample,
                                    double elapsedSeconds);
    void Reset();
    [[nodiscard]] bool IsPaused() const noexcept { return paused_; }
    [[nodiscard]] const std::string& PauseReason() const noexcept { return pauseReason_; }

private:
    double lowFpsSeconds_{0.0};
    double highCpuSeconds_{0.0};
    double highMemorySeconds_{0.0};
    double stableSeconds_{0.0};
    double activeRendererSeconds_{0.0};
    bool paused_{false};
    std::string pauseReason_;
};

struct VisualFrameDescriptor {
    CameraState camera;
    double colourOffset{0.0};
    int pixelWidth{1};
    int pixelHeight{1};
    std::uint64_t contentRevision{0};
};

class VisibleChangeDetector {
public:
    bool ShouldRender(const std::vector<VisualFrameDescriptor>& current,
                      const AdaptivePerformanceSettings& settings,
                      bool forceRender = false);
    void Reset();
    [[nodiscard]] bool IsVisuallyIdle() const noexcept { return visuallyIdle_; }
    [[nodiscard]] std::uint64_t SkippedFrameCount() const noexcept { return skippedFrameCount_; }

private:
    std::vector<VisualFrameDescriptor> lastRendered_;
    bool visuallyIdle_{false};
    std::uint64_t skippedFrameCount_{0};
};

std::uint64_t ComputeVisualRevision(const Preset& preset,
                                    int maximumIterations,
                                    double renderScale,
                                    int antiAliasingLevel,
                                    const PrecisionSettings& precision);

} // namespace mw
