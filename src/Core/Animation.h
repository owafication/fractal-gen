#pragma once

#include "Core/Models.h"

#include <cstddef>
#include <string>
#include <vector>

namespace mw {

struct AnimationFrame {
    CameraState camera;
    double colourOffset{0.0};
};

class AnimationController {
public:
    AnimationController();

    void SetPreset(const Preset& preset, bool reducedMotion);
    void SetManualCamera(const CameraState& camera);
    void Pan(double normalisedDeltaX, double normalisedDeltaY, double aspectRatio);
    void ZoomAt(double normalisedX, double normalisedY, double wheelSteps, double aspectRatio);
    void Reset();
    void SetColourCyclingEnabled(bool enabled) noexcept { colourCyclingEnabled_ = enabled; }
    [[nodiscard]] bool ColourCyclingEnabled() const noexcept { return colourCyclingEnabled_; }
    void SetMotionEnabled(bool enabled) noexcept { motionEnabled_ = enabled; }
    [[nodiscard]] bool MotionEnabled() const noexcept { return motionEnabled_; }
    AnimationFrame Update(double deltaSeconds);
    [[nodiscard]] const CameraState& Camera() const noexcept { return current_.camera; }
    [[nodiscard]] std::size_t JourneyTargetCount() const noexcept { return journey_.size(); }

private:
    struct JourneyPoint {
        CameraState camera;
        double transitionSeconds;
        double holdSeconds;
    };

    enum class JourneyPhase { Transition, Hold };

    static std::vector<JourneyPoint> ParseJourneyScript(const std::string& script);
    static double Smoothstep(double value);
    static CameraState Interpolate(const CameraState& from, const CameraState& to, double progress);

    Preset preset_;
    AnimationFrame current_;
    CameraState resetCamera_;
    bool reducedMotion_{false};
    bool colourCyclingEnabled_{true};
    bool motionEnabled_{true};
    std::vector<JourneyPoint> journey_;
    std::size_t journeyIndex_{0};
    double segmentTime_{0.0};
    JourneyPhase journeyPhase_{JourneyPhase::Transition};
    CameraState journeyLegStartCamera_;
    double zoomDirection_{-1.0};
};

} // namespace mw
