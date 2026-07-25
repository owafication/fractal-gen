#include "Core/AdaptivePerformance.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace mw {
namespace {

constexpr double kRendererWarmupSeconds = 3.0;

void AccumulateCondition(bool active, double elapsedSeconds, double& accumulator) {
    if (active) accumulator += std::max(0.0, elapsedSeconds);
    else accumulator = 0.0;
}

double CircularDistance(double a, double b) {
    double difference = std::fmod(std::abs(a - b), 1.0);
    if (difference < 0.0) difference += 1.0;
    return std::min(difference, 1.0 - difference);
}

bool HasVisibleDifference(const VisualFrameDescriptor& previous,
                          const VisualFrameDescriptor& current,
                          const AdaptivePerformanceSettings& settings) {
    if (previous.contentRevision != current.contentRevision ||
        previous.pixelWidth != current.pixelWidth || previous.pixelHeight != current.pixelHeight) {
        return true;
    }

    const double previousScale = std::max(previous.camera.scale, 1.0e-300);
    const double currentScale = std::max(current.camera.scale, 1.0e-300);
    const double referenceScale = std::min(previousScale, currentScale);
    const double height = static_cast<double>(std::max(1, current.pixelHeight));
    const double previousX = previous.camera.centreX + previous.camera.centreXLow;
    const double previousY = previous.camera.centreY + previous.camera.centreYLow;
    const double currentX = current.camera.centreX + current.camera.centreXLow;
    const double currentY = current.camera.centreY + current.camera.centreYLow;

    // Camera scale is the complex-plane half-height. Translating by 2*scale
    // therefore moves a point through the full viewport height.
    const double panPixelsX = std::abs(currentX - previousX) * height / (2.0 * referenceScale);
    const double panPixelsY = std::abs(currentY - previousY) * height / (2.0 * referenceScale);
    const double zoomPixels = std::abs(std::log(currentScale / previousScale)) * height * 0.5;
    const double cameraPixels = std::max({panPixelsX, panPixelsY, zoomPixels});
    if (!std::isfinite(cameraPixels) || cameraPixels >= settings.minimumVisiblePixelChange) return true;

    return CircularDistance(previous.colourOffset, current.colourOffset) >= settings.minimumVisibleColourChange;
}

void Mix(std::uint64_t& hash, std::uint64_t value) {
    constexpr std::uint64_t prime = 1099511628211ULL;
    hash ^= value;
    hash *= prime;
}

void MixDouble(std::uint64_t& hash, double value) {
    Mix(hash, std::bit_cast<std::uint64_t>(value));
}

void MixFloat(std::uint64_t& hash, float value) {
    Mix(hash, static_cast<std::uint64_t>(std::bit_cast<std::uint32_t>(value)));
}

void MixCoefficient(std::uint64_t& hash, const ComplexCoefficient& value) {
    MixDouble(hash, value.real);
    MixDouble(hash, value.imaginary);
}

void MixColour(std::uint64_t& hash, const Colour& colour) {
    MixFloat(hash, colour.r);
    MixFloat(hash, colour.g);
    MixFloat(hash, colour.b);
    MixFloat(hash, colour.a);
}

} // namespace

AdaptivePauseDecision AdaptivePerformanceController::Update(const AdaptivePerformanceSettings& settings,
                                                               const AdaptivePerformanceSample& sample,
                                                               double elapsedSeconds) {
    AdaptivePauseDecision decision;
    elapsedSeconds = std::clamp(elapsedSeconds, 0.0, 5.0);
    if (!settings.enabled) {
        const bool wasPaused = paused_;
        Reset();
        decision.resumeNow = wasPaused;
        return decision;
    }

    if (sample.rendererActive) activeRendererSeconds_ += elapsedSeconds;
    else activeRendererSeconds_ = 0.0;

    const double effectiveMinimumFps = std::min(
        settings.minimumFramesPerSecond, std::max(1.0, sample.targetFramesPerSecond * 0.8));
    const bool lowFps = settings.pauseOnLowFps && sample.rendererActive &&
                        sample.framesPerSecondMeaningful && activeRendererSeconds_ >= kRendererWarmupSeconds &&
                        sample.framesPerSecond > 0.0 && sample.framesPerSecond < effectiveMinimumFps;
    const bool highCpu = settings.pauseOnHighCpu &&
                         sample.processCpuPercent >= settings.maximumProcessCpuPercent;
    const bool highMemory = settings.pauseOnHighMemory &&
                            sample.processWorkingSetMb >= static_cast<double>(settings.maximumWorkingSetMb);

    if (paused_) {
        // FPS is intentionally ignored while paused because no frames are being
        // produced. CPU and memory must remain below their limits for the full
        // stable period before a bounded resume probe is allowed.
        const bool stableResources = !highCpu && !highMemory;
        if (stableResources) stableSeconds_ += elapsedSeconds;
        else stableSeconds_ = 0.0;
        if (stableSeconds_ * 1000.0 >= static_cast<double>(settings.resumeStableMs)) {
            paused_ = false;
            pauseReason_.clear();
            lowFpsSeconds_ = 0.0;
            highCpuSeconds_ = 0.0;
            highMemorySeconds_ = 0.0;
            stableSeconds_ = 0.0;
            activeRendererSeconds_ = 0.0;
            decision.resumeNow = true;
        }
        decision.paused = paused_;
        decision.reason = pauseReason_;
        return decision;
    }

    AccumulateCondition(lowFps, elapsedSeconds, lowFpsSeconds_);
    AccumulateCondition(highCpu, elapsedSeconds, highCpuSeconds_);
    AccumulateCondition(highMemory, elapsedSeconds, highMemorySeconds_);

    if (highCpuSeconds_ * 1000.0 >= static_cast<double>(settings.highCpuSustainMs)) {
        paused_ = true;
        pauseReason_ = "Adaptive pause: process CPU usage stayed above " +
                       std::to_string(static_cast<int>(std::lround(settings.maximumProcessCpuPercent))) + "%";
    } else if (highMemorySeconds_ * 1000.0 >= static_cast<double>(settings.highMemorySustainMs)) {
        paused_ = true;
        pauseReason_ = "Adaptive pause: process memory stayed above " +
                       std::to_string(settings.maximumWorkingSetMb) + " MB";
    } else if (lowFpsSeconds_ * 1000.0 >= static_cast<double>(settings.lowFpsSustainMs)) {
        paused_ = true;
        pauseReason_ = "Adaptive pause: rendering stayed below " +
                       std::to_string(static_cast<int>(std::lround(effectiveMinimumFps))) + " FPS";
    }

    if (paused_) {
        stableSeconds_ = 0.0;
        decision.pauseNow = true;
        decision.paused = true;
        decision.reason = pauseReason_;
    }
    return decision;
}

void AdaptivePerformanceController::Reset() {
    lowFpsSeconds_ = 0.0;
    highCpuSeconds_ = 0.0;
    highMemorySeconds_ = 0.0;
    stableSeconds_ = 0.0;
    activeRendererSeconds_ = 0.0;
    paused_ = false;
    pauseReason_.clear();
}

bool VisibleChangeDetector::ShouldRender(const std::vector<VisualFrameDescriptor>& current,
                                         const AdaptivePerformanceSettings& settings,
                                         bool forceRender) {
    if (forceRender || !settings.stopWhenVisuallyUnchanged || current.empty() ||
        lastRendered_.size() != current.size()) {
        lastRendered_ = current;
        visuallyIdle_ = false;
        return true;
    }

    for (std::size_t index = 0; index < current.size(); ++index) {
        if (HasVisibleDifference(lastRendered_[index], current[index], settings)) {
            lastRendered_ = current;
            visuallyIdle_ = false;
            return true;
        }
    }

    visuallyIdle_ = true;
    ++skippedFrameCount_;
    return false;
}

void VisibleChangeDetector::Reset() {
    lastRendered_.clear();
    visuallyIdle_ = false;
    skippedFrameCount_ = 0;
}

std::uint64_t ComputeVisualRevision(const Preset& preset,
                                    int maximumIterations,
                                    double renderScale,
                                    int antiAliasingLevel,
                                    const PrecisionSettings& precision) {
    std::uint64_t hash = 1469598103934665603ULL;
    Mix(hash, static_cast<std::uint64_t>(preset.palette));
    Mix(hash, static_cast<std::uint64_t>(maximumIterations));
    MixDouble(hash, renderScale);
    Mix(hash, static_cast<std::uint64_t>(antiAliasingLevel));
    Mix(hash, static_cast<std::uint64_t>(precision.mode));
    Mix(hash, precision.allowFloat64 ? 1ULL : 0ULL);
    Mix(hash, precision.allowSplitFloat ? 1ULL : 0ULL);
    Mix(hash, precision.allowPerturbation ? 1ULL : 0ULL);
    Mix(hash, precision.allowArbitraryPrecision ? 1ULL : 0ULL);
    Mix(hash, precision.automaticFallback ? 1ULL : 0ULL);
    Mix(hash, static_cast<std::uint64_t>(precision.arbitraryPrecisionBits));
    MixCoefficient(hash, preset.equation.quadratic);
    MixCoefficient(hash, preset.equation.linear);
    MixCoefficient(hash, preset.equation.parameter);
    MixCoefficient(hash, preset.equation.constant);
    MixCoefficient(hash, preset.equation.iterationTerm);
    MixCoefficient(hash, preset.equation.reciprocalCoefficient);
    Mix(hash, static_cast<std::uint64_t>(preset.equation.power));
    Mix(hash, static_cast<std::uint64_t>(preset.equation.parameterPower));
    Mix(hash, static_cast<std::uint64_t>(preset.equation.reciprocalPower));
    Mix(hash, preset.equation.absoluteReal ? 1ULL : 0ULL);
    Mix(hash, preset.equation.absoluteImaginary ? 1ULL : 0ULL);
    Mix(hash, preset.equation.conjugate ? 1ULL : 0ULL);
    Mix(hash, preset.equation.swapRealImaginary ? 1ULL : 0ULL);
    Mix(hash, static_cast<std::uint64_t>(preset.equation.unaryTransform));
    Mix(hash, static_cast<std::uint64_t>(preset.equation.initialZMode));
    MixCoefficient(hash, preset.equation.initialZ);
    Mix(hash, preset.equation.juliaMode ? 1ULL : 0ULL);
    MixCoefficient(hash, preset.equation.juliaParameter);
    MixDouble(hash, preset.equation.bailoutRadius);
    Mix(hash, preset.equation.newtonMode ? 1ULL : 0ULL);
    Mix(hash, static_cast<std::uint64_t>(preset.equation.newtonDegree));
    MixCoefficient(hash, preset.equation.newtonTarget);
    MixCoefficient(hash, preset.equation.newtonRelaxation);
    MixDouble(hash, preset.equation.convergenceTolerance);
    Mix(hash, static_cast<std::uint64_t>(preset.equation.colouringMethod));
    Mix(hash, static_cast<std::uint64_t>(preset.equation.orbitTrap));
    MixCoefficient(hash, preset.equation.orbitTrapPoint);
    MixDouble(hash, preset.equation.orbitTrapRadius);
    MixDouble(hash, preset.equation.glowStrength);
    MixDouble(hash, preset.equation.depthStrength);
    Mix(hash, preset.equation.animateCoefficients ? 1ULL : 0ULL);
    MixDouble(hash, preset.equation.coefficientAnimationSpeed);
    MixDouble(hash, preset.equation.coefficientAnimationAmplitude);
    MixDouble(hash, preset.brightness);
    MixDouble(hash, preset.contrast);
    MixDouble(hash, preset.saturation);
    MixColour(hash, preset.interiorColour);
    MixColour(hash, preset.backgroundColour);
    Mix(hash, preset.smoothColouring ? 1ULL : 0ULL);
    Mix(hash, static_cast<std::uint64_t>(preset.customPaletteColours.size()));
    for (const auto& colour : preset.customPaletteColours) MixColour(hash, colour);
    return hash;
}

} // namespace mw
