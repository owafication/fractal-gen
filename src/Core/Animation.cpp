#include "Core/Animation.h"
#include "Core/MandelbrotMath.h"
#include "Core/DeepZoom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>


namespace {

std::string TrimCopy(const std::string& value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}


} // namespace

namespace mw {


std::vector<AnimationController::JourneyPoint> AnimationController::ParseJourneyScript(const std::string& script) {
    std::vector<JourneyPoint> points;
    std::string normalised = script.substr(0, 32768U);
    for (char& ch : normalised) {
        if (ch == ';') ch = '\n';
    }
    std::stringstream lines(normalised);
    std::string line;
    while (points.size() < 128U && std::getline(lines, line)) {
        line = TrimCopy(line);
        if (line.empty() || line.rfind("#", 0) == 0 || line.rfind("//", 0) == 0) continue;
        for (char& ch : line) {
            if (ch == '|') ch = ',';
        }
        std::stringstream row(line);
        std::string part;
        std::vector<double> values;
        while (std::getline(row, part, ',')) {
            part = TrimCopy(part);
            if (part.empty()) continue;
            try { values.push_back(std::stod(part)); } catch (...) { values.clear(); break; }
        }
        if (values.size() < 4U || !std::all_of(values.begin(), values.end(), [](double value) { return std::isfinite(value); })) continue;
        JourneyPoint point;
        point.camera = {
            std::clamp(values[0], -4.0, 4.0),
            std::clamp(values[1], -4.0, 4.0),
            std::clamp(values[2], 1.0e-32, 4.0),
        };
        point.transitionSeconds = std::clamp(values[3], 1.0, 3600.0);
        point.holdSeconds = values.size() >= 5U ? std::clamp(values[4], 0.0, 3600.0) : 2.0;
        points.push_back(point);
    }
    return points;
}

AnimationController::AnimationController() {
    SetPreset(BuiltInPresets().front(), false);
}

void AnimationController::SetPreset(const Preset& preset, bool reducedMotion) {
    preset_ = preset;
    ValidateAndNormalise(preset_);
    reducedMotion_ = reducedMotion;
    if (preset_.animationMode == AnimationMode::ContinuousZoom) {
        const auto target = FindInterestingFractalTarget(
            CameraCentreX(preset_.camera), CameraCentreY(preset_.camera), preset_.camera.scale,
            preset_.maximumIterations, preset_.equation);
        preset_.camera.centreX = target.first;
        preset_.camera.centreY = target.second;
        preset_.camera.centreXLow = 0.0;
        preset_.camera.centreYLow = 0.0;
    }

    resetCamera_ = preset_.camera;
    current_.camera = preset_.camera;
    current_.colourOffset = preset_.colourOffset;
    journeyIndex_ = 0;
    segmentTime_ = 0.0;
    journeyPhase_ = JourneyPhase::Transition;
    zoomDirection_ = -1.0;

    const std::vector<JourneyPoint> candidates{
        {{-0.743643887037151, 0.131825904205330, 0.0040}, 16.0, 2.0},
        {{0.285000000000000, 0.010000000000000, 0.0280}, 14.0, 2.0},
        {{-1.250660000000000, 0.020120000000000, 0.0090}, 15.0, 2.0},
        {{-0.160000000000000, 1.040500000000000, 0.0180}, 15.0, 2.0},
        {{-0.776540000000000, -0.136640000000000, 0.0060}, 15.0, 2.0},
        {{-0.101096363845620, 0.956286510809140, 0.0065}, 15.0, 2.0},
        {{-0.088000000000000, 0.654000000000000, 0.0120}, 14.0, 2.0},
        {{-1.768778833000000, 0.001738996000000, 0.0035}, 16.0, 2.0},
        {{-0.745300000000000, 0.112700000000000, 0.0030}, 15.0, 2.0},
        {{-0.122560000000000, 0.744860000000000, 0.0060}, 15.0, 2.0},
        {{0.001643721971153, -0.822467633298876, 0.0040}, 16.0, 2.0},
        {{-1.940000000000000, 0.000000000000000, 0.0150}, 14.0, 2.0},
    };
    journey_.clear();

    if (preset_.animationMode == AnimationMode::AutomaticJourney) {
        const auto scriptedPoints = ParseJourneyScript(preset_.automaticJourneyWaypoints);
        if (!scriptedPoints.empty()) {
            // A custom script is an exact ordered route. Do not redirect its
            // coordinates, inject fallback destinations, or add zoom-out legs.
            journey_ = scriptedPoints;
        } else {
            const double precisionFloor = std::max(1.0e-32, preset_.camera.scale / preset_.maximumZoom);
            auto addGeneratedTarget = [&](JourneyPoint point) {
                const auto target = FindInterestingFractalTarget(
                    point.camera.centreX, point.camera.centreY, point.camera.scale,
                    preset_.maximumIterations, preset_.equation);
                point.camera.centreX = target.first;
                point.camera.centreY = target.second;
                point.camera.scale = std::max(point.camera.scale, precisionFloor);
                if (!IsInterestingFractalTarget(point.camera.centreX, point.camera.centreY,
                                                 preset_.maximumIterations, preset_.equation) ||
                    !IsBoundaryRichFractalTarget(point.camera.centreX, point.camera.centreY,
                                                  point.camera.scale, preset_.maximumIterations,
                                                  preset_.equation)) return;
                const bool duplicate = std::any_of(journey_.begin(), journey_.end(), [&](const JourneyPoint& existing) {
                    const double dx = CameraCentreX(existing.camera) - CameraCentreX(point.camera);
                    const double dy = CameraCentreY(existing.camera) - CameraCentreY(point.camera);
                    return std::hypot(dx, dy) < 0.012;
                });
                if (!duplicate) journey_.push_back(point);
            };
            if (preset_.camera.scale < 0.75) {
                addGeneratedTarget({preset_.camera, 14.0, 2.0});
            }
            for (const auto& point : candidates) addGeneratedTarget(point);
            if (journey_.size() < 6) {
                constexpr std::array<std::pair<double, double>, 8> fallbackSeeds{{
                    {-0.75, 0.10}, {-1.25, 0.00}, {-0.20, 0.70}, {0.25, 0.00},
                    {-0.50, -0.55}, {-1.70, 0.00}, {-0.10, -0.85}, {-0.90, 0.30},
                }};
                for (std::size_t i = 0; i < fallbackSeeds.size() && journey_.size() < 8; ++i) {
                    const double scale = 0.012 + static_cast<double>(i % 3) * 0.006;
                    addGeneratedTarget({{fallbackSeeds[i].first, fallbackSeeds[i].second, scale}, 14.0, 1.5});
                }
            }
        }
        if (journey_.empty()) journey_.push_back({preset_.camera, 14.0, 2.0});
        current_.camera = preset_.camera;
        resetCamera_ = preset_.camera;
        journeyLegStartCamera_ = current_.camera;
    }
}

void AnimationController::SetManualCamera(const CameraState& camera) {
    current_.camera = camera;
    current_.camera.scale = std::clamp(current_.camera.scale, 1.0e-32, 4.0);
    NormaliseCamera(current_.camera);
}

void AnimationController::Pan(double normalisedDeltaX, double normalisedDeltaY, double aspectRatio) {
    const double horizontalSpan = current_.camera.scale * aspectRatio * 2.0;
    const double verticalSpan = current_.camera.scale * 2.0;
    OffsetCamera(current_.camera, -normalisedDeltaX * horizontalSpan, normalisedDeltaY * verticalSpan);
    preset_.animationMode = AnimationMode::ManualView;
}

void AnimationController::ZoomAt(double normalisedX, double normalisedY, double wheelSteps, double aspectRatio) {
    const double factor = std::pow(0.82, wheelSteps);
    const double oldScale = current_.camera.scale;
    const double newScale = std::clamp(oldScale * factor, 1.0e-32, 4.0);
    OffsetCamera(current_.camera,
                 normalisedX * (oldScale - newScale) * aspectRatio,
                 normalisedY * (oldScale - newScale));
    current_.camera.scale = newScale;
    preset_.animationMode = AnimationMode::ManualView;
}

void AnimationController::Reset() {
    current_.camera = resetCamera_;
    current_.colourOffset = preset_.colourOffset;
    segmentTime_ = 0.0;
    journeyIndex_ = 0;
    journeyPhase_ = JourneyPhase::Transition;
    journeyLegStartCamera_ = resetCamera_;
    zoomDirection_ = -1.0;
}

double AnimationController::Smoothstep(double value) {
    value = std::clamp(value, 0.0, 1.0);
    return value * value * (3.0 - 2.0 * value);
}

CameraState AnimationController::Interpolate(const CameraState& from, const CameraState& to, double progress) {
    const double eased = Smoothstep(progress);
    const double logFrom = std::log(std::max(from.scale, 1.0e-32));
    const double logTo = std::log(std::max(to.scale, 1.0e-32));
    CameraState result{
        CameraCentreX(from) + (CameraCentreX(to) - CameraCentreX(from)) * eased,
        CameraCentreY(from) + (CameraCentreY(to) - CameraCentreY(from)) * eased,
        std::exp(logFrom + (logTo - logFrom) * eased),
    };
    NormaliseCamera(result);
    return result;
}

AnimationFrame AnimationController::Update(double deltaSeconds) {
    deltaSeconds = std::clamp(deltaSeconds, 0.0, 0.25);
    const double motionFactor = reducedMotion_ ? 0.25 : 1.0;
    const double colourFactor = reducedMotion_ ? 0.2 : 1.0;
    if (colourCyclingEnabled_) {
        current_.colourOffset += preset_.colourCycleSpeed * colourFactor * deltaSeconds;
    }

    if (!motionEnabled_) return current_;

    switch (preset_.animationMode) {
    case AnimationMode::ContinuousZoom: {
        const double minimumScale = std::max(1.0e-32, preset_.startingScale / preset_.maximumZoom);
        current_.camera.scale *= std::exp(zoomDirection_ * preset_.zoomSpeed * motionFactor * deltaSeconds);
        if (current_.camera.scale <= minimumScale) {
            if (preset_.zoomRestartBehaviour == ZoomRestartBehaviour::PingPong) {
                current_.camera.scale = minimumScale;
                zoomDirection_ = 1.0;
            } else {
                current_.camera.scale = preset_.startingScale;
            }
        } else if (zoomDirection_ > 0.0 && current_.camera.scale >= preset_.startingScale) {
            current_.camera.scale = preset_.startingScale;
            zoomDirection_ = -1.0;
        }
        break;
    }
    case AnimationMode::AutomaticJourney: {
        if (journey_.empty()) break;
        const JourneyPoint& target = journey_[journeyIndex_];
        segmentTime_ += deltaSeconds * motionFactor;

        if (journeyPhase_ == JourneyPhase::Transition) {
            const double duration = std::max(1.0, target.transitionSeconds);
            current_.camera = Interpolate(journeyLegStartCamera_, target.camera, segmentTime_ / duration);
            if (segmentTime_ >= duration) {
                current_.camera = target.camera;
                journeyPhase_ = JourneyPhase::Hold;
                segmentTime_ = 0.0;
            }
        } else {
            current_.camera = target.camera;
            if (segmentTime_ >= target.holdSeconds) {
                journeyLegStartCamera_ = target.camera;
                journeyIndex_ = (journeyIndex_ + 1U) % journey_.size();
                journeyPhase_ = JourneyPhase::Transition;
                segmentTime_ = 0.0;
            }
        }
        break;
    }
    case AnimationMode::StaticAnimatedColour:
    case AnimationMode::ManualView:
        break;
    }
    return current_;
}

} // namespace mw
