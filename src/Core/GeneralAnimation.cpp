#include "Core/GeneralAnimation.h"

#include "Core/DeepZoom.h"
#include "Core/Precision/ExactCameraAdapter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <utility>

namespace mw {
namespace {

constexpr std::size_t kMaximumTracks = 256U;
constexpr std::size_t kMaximumKeyframes = 4096U;
constexpr std::size_t kMaximumIdLength = 80U;
constexpr std::size_t kMaximumTargetLength = 120U;
constexpr double kMaximumDurationSeconds = 604800.0; // Seven days.

constexpr std::array<AnimationTargetDescriptor, 18> kTargetDescriptors{{
    {AnimationTargetKey::CameraCentreX, "camera.centre-x",
     AnimationValueType::CompensatedReal, ParameterInvalidation::CameraViewport,
     true, true, true},
    {AnimationTargetKey::CameraCentreY, "camera.centre-y",
     AnimationValueType::CompensatedReal, ParameterInvalidation::CameraViewport,
     true, true, true},
    {AnimationTargetKey::CameraScale, "camera.scale",
     AnimationValueType::Real, ParameterInvalidation::CameraViewport,
     true, true, true},
    {AnimationTargetKey::RotationDegrees, "camera.rotation-degrees",
     AnimationValueType::Real, ParameterInvalidation::CameraViewport,
     true, true, true},
    {AnimationTargetKey::PaletteOffset, "palette.offset",
     AnimationValueType::Real, ParameterInvalidation::PaletteColouring,
     true, true, true},
    {AnimationTargetKey::PaletteFrequency, "palette.frequency",
     AnimationValueType::Real, ParameterInvalidation::PaletteColouring,
     true, true, true},
    {AnimationTargetKey::PaletteGamma, "palette.gamma",
     AnimationValueType::Real, ParameterInvalidation::PaletteColouring,
     true, true, true},
    {AnimationTargetKey::Brightness, "post.brightness",
     AnimationValueType::Real, ParameterInvalidation::PostProcessing,
     true, true, true},
    {AnimationTargetKey::Contrast, "post.contrast",
     AnimationValueType::Real, ParameterInvalidation::PostProcessing,
     true, true, true},
    {AnimationTargetKey::Saturation, "post.saturation",
     AnimationValueType::Real, ParameterInvalidation::PostProcessing,
     true, true, true},
    {AnimationTargetKey::StripeStrength, "palette.stripe-strength",
     AnimationValueType::Real, ParameterInvalidation::PaletteColouring,
     true, true, true},
    {AnimationTargetKey::BloomStrength, "post.bloom-strength",
     AnimationValueType::Real, ParameterInvalidation::PostProcessing,
     true, true, true},
    {AnimationTargetKey::EdgeLightingStrength, "post.edge-lighting-strength",
     AnimationValueType::Real, ParameterInvalidation::PaletteColouring,
     true, true, true},
    {AnimationTargetKey::EquationQuadraticReal, "equation.quadratic.real",
     AnimationValueType::Real, ParameterInvalidation::EquationPrecision,
     true, true, true},
    {AnimationTargetKey::EquationQuadraticImaginary, "equation.quadratic.imaginary",
     AnimationValueType::Real, ParameterInvalidation::EquationPrecision,
     true, true, true},
    {AnimationTargetKey::EquationPower, "equation.power",
     AnimationValueType::Integer, ParameterInvalidation::EquationPrecision,
     true, false, false},
    {AnimationTargetKey::EquationParameterPower, "equation.parameter-power",
     AnimationValueType::Integer, ParameterInvalidation::EquationPrecision,
     true, false, false},
    {AnimationTargetKey::EquationBailoutRadius, "equation.bailout-radius",
     AnimationValueType::Real, ParameterInvalidation::EquationPrecision,
     true, true, true},
}};

struct RealBounds {
    double minimum;
    double maximum;
};

struct IntegerBounds {
    std::int64_t minimum;
    std::int64_t maximum;
};

struct DoubleDouble {
    double high;
    double low;
};

void AddIssue(AnimationTimelineValidationResult& result,
              std::string field,
              std::string message) {
    result.valid = false;
    result.issues.push_back({std::move(field), std::move(message)});
}

void AddWarning(AnimationTimelineValidationResult& result,
                std::string field,
                std::string message) {
    result.warnings.push_back({std::move(field), std::move(message)});
}

bool IsStableId(std::string_view id) noexcept {
    if (id.empty() || id.size() > kMaximumIdLength) return false;
    for (const char rawCharacter : id) {
        const auto character = static_cast<unsigned char>(rawCharacter);
        const bool letter = (character >= static_cast<unsigned char>('a') &&
                             character <= static_cast<unsigned char>('z')) ||
                            (character >= static_cast<unsigned char>('A') &&
                             character <= static_cast<unsigned char>('Z'));
        const bool digit = character >= static_cast<unsigned char>('0') &&
                           character <= static_cast<unsigned char>('9');
        const bool punctuation = character == static_cast<unsigned char>('-') ||
                                 character == static_cast<unsigned char>('_') ||
                                 character == static_cast<unsigned char>('.') ||
                                 character == static_cast<unsigned char>(':');
        if (!letter && !digit && !punctuation) return false;
    }
    return true;
}

bool IsStableTargetName(std::string_view name) noexcept {
    if (name.empty() || name.size() > kMaximumTargetLength) return false;
    for (const char rawCharacter : name) {
        const auto character = static_cast<unsigned char>(rawCharacter);
        const bool letter = (character >= static_cast<unsigned char>('a') &&
                             character <= static_cast<unsigned char>('z')) ||
                            (character >= static_cast<unsigned char>('A') &&
                             character <= static_cast<unsigned char>('Z'));
        const bool digit = character >= static_cast<unsigned char>('0') &&
                           character <= static_cast<unsigned char>('9');
        const bool punctuation = character == static_cast<unsigned char>('-') ||
                                 character == static_cast<unsigned char>('_') ||
                                 character == static_cast<unsigned char>('.') ||
                                 character == static_cast<unsigned char>(':');
        if (!letter && !digit && !punctuation) return false;
    }
    return true;
}

const AnimationTargetDescriptor* DescriptorForKey(AnimationTargetKey key) noexcept {
    for (const auto& descriptor : kTargetDescriptors) {
        if (descriptor.key == key) return &descriptor;
    }
    return nullptr;
}

const AnimationTargetDescriptor* DescriptorForName(std::string_view stableName) noexcept {
    for (const auto& descriptor : kTargetDescriptors) {
        if (descriptor.stableName == stableName) return &descriptor;
    }
    return nullptr;
}

std::optional<RealBounds> BoundsForReal(AnimationTargetKey key) noexcept {
    switch (key) {
    case AnimationTargetKey::CameraScale: return RealBounds{1.0e-32, 4.0};
    case AnimationTargetKey::RotationDegrees: return RealBounds{-3600.0, 3600.0};
    case AnimationTargetKey::PaletteOffset: return RealBounds{-1000.0, 1000.0};
    case AnimationTargetKey::PaletteFrequency: return RealBounds{0.05, 256.0};
    case AnimationTargetKey::PaletteGamma: return RealBounds{0.05, 8.0};
    case AnimationTargetKey::Brightness: return RealBounds{0.1, 2.5};
    case AnimationTargetKey::Contrast: return RealBounds{0.1, 3.0};
    case AnimationTargetKey::Saturation: return RealBounds{0.0, 2.0};
    case AnimationTargetKey::StripeStrength: return RealBounds{0.0, 2.0};
    case AnimationTargetKey::BloomStrength: return RealBounds{0.0, 4.0};
    case AnimationTargetKey::EdgeLightingStrength: return RealBounds{0.0, 4.0};
    case AnimationTargetKey::EquationQuadraticReal:
    case AnimationTargetKey::EquationQuadraticImaginary:
        return RealBounds{-8.0, 8.0};
    case AnimationTargetKey::EquationBailoutRadius: return RealBounds{1.01, 1.0e6};
    case AnimationTargetKey::CameraCentreX:
    case AnimationTargetKey::CameraCentreY:
    case AnimationTargetKey::EquationPower:
    case AnimationTargetKey::EquationParameterPower:
        return std::nullopt;
    }
    return std::nullopt;
}

std::optional<IntegerBounds> BoundsForInteger(AnimationTargetKey key) noexcept {
    switch (key) {
    case AnimationTargetKey::EquationPower:
    case AnimationTargetKey::EquationParameterPower:
        return IntegerBounds{1, 12};
    default:
        return std::nullopt;
    }
}

bool SupportsInterpolation(const AnimationTargetDescriptor& descriptor,
                           AnimationInterpolation interpolation) noexcept {
    switch (interpolation) {
    case AnimationInterpolation::Step: return descriptor.supportsStep;
    case AnimationInterpolation::Linear: return descriptor.supportsLinear;
    case AnimationInterpolation::Smoothstep: return descriptor.supportsSmoothstep;
    }
    return false;
}

bool ValidateValue(const AnimationTargetDescriptor& descriptor,
                   const AnimationValue& value,
                   std::string& message) {
    switch (descriptor.valueType) {
    case AnimationValueType::Real: {
        const auto* realValue = std::get_if<double>(&value);
        if (!realValue) {
            message = "Target requires a real keyframe value.";
            return false;
        }
        if (!std::isfinite(*realValue)) {
            message = "Real keyframe values must be finite.";
            return false;
        }
        const auto bounds = BoundsForReal(descriptor.key);
        if (!bounds || *realValue < bounds->minimum || *realValue > bounds->maximum) {
            message = "Real keyframe value is outside the target's supported range.";
            return false;
        }
        return true;
    }
    case AnimationValueType::Integer: {
        const auto* integerValue = std::get_if<std::int64_t>(&value);
        if (!integerValue) {
            message = "Target requires an integer keyframe value.";
            return false;
        }
        const auto bounds = BoundsForInteger(descriptor.key);
        if (!bounds || *integerValue < bounds->minimum || *integerValue > bounds->maximum) {
            message = "Integer keyframe value is outside the target's supported range.";
            return false;
        }
        return true;
    }
    case AnimationValueType::CompensatedReal: {
        const auto* compensatedValue = std::get_if<CompensatedAnimationValue>(&value);
        if (!compensatedValue) {
            message = "Target requires a compensated high/low keyframe value.";
            return false;
        }
        if (!std::isfinite(compensatedValue->high) ||
            !std::isfinite(compensatedValue->low)) {
            message = "Compensated keyframe components must be finite.";
            return false;
        }
        CameraState camera{};
        if (descriptor.key == AnimationTargetKey::CameraCentreX) {
            camera.centreX = compensatedValue->high;
            camera.centreXLow = compensatedValue->low;
        } else {
            camera.centreY = compensatedValue->high;
            camera.centreYLow = compensatedValue->low;
        }
        NormaliseCamera(camera);
        const double normalisedHigh = descriptor.key == AnimationTargetKey::CameraCentreX
                                          ? camera.centreX
                                          : camera.centreY;
        if (normalisedHigh < -4.0 || normalisedHigh > 4.0) {
            message = "Compensated camera value is outside the supported coordinate range.";
            return false;
        }
        return true;
    }
    }
    message = "Unsupported animation value type.";
    return false;
}

DoubleDouble Normalise(DoubleDouble value) noexcept {
    const double sum = value.high + value.low;
    const double virtualLow = sum - value.high;
    const double residual = (value.high - (sum - virtualLow)) + (value.low - virtualLow);
    return {sum, residual};
}

DoubleDouble Add(DoubleDouble left, DoubleDouble right) noexcept {
    const double highSum = left.high + right.high;
    const double virtualRight = highSum - left.high;
    const double highResidual = (left.high - (highSum - virtualRight)) +
                                (right.high - virtualRight);
    return Normalise({highSum, highResidual + left.low + right.low});
}

DoubleDouble Subtract(DoubleDouble left, DoubleDouble right) noexcept {
    return Add(left, {-right.high, -right.low});
}

DoubleDouble Multiply(DoubleDouble value, double factor) noexcept {
    const double product = value.high * factor;
    const double residual = std::fma(value.high, factor, -product) + value.low * factor;
    return Normalise({product, residual});
}

CompensatedAnimationValue InterpolateCompensated(
    const CompensatedAnimationValue& from,
    const CompensatedAnimationValue& to,
    double factor) noexcept {
    const DoubleDouble start{from.high, from.low};
    const DoubleDouble finish{to.high, to.low};
    const DoubleDouble interpolated = Add(start, Multiply(Subtract(finish, start), factor));
    return {interpolated.high, interpolated.low};
}

double Smoothstep(double value) noexcept {
    const double clamped = std::clamp(value, 0.0, 1.0);
    return clamped * clamped * (3.0 - 2.0 * clamped);
}

double ResolveTimelineTime(const AnimationTimeline& timeline, double timeSeconds) noexcept {
    const double duration = timeline.durationSeconds;
    switch (timeline.loopMode) {
    case AnimationLoopMode::Clamp:
        return std::clamp(timeSeconds, 0.0, duration);
    case AnimationLoopMode::Loop: {
        double wrapped = std::fmod(timeSeconds, duration);
        if (wrapped < 0.0) wrapped += duration;
        return wrapped;
    }
    case AnimationLoopMode::PingPong: {
        const double period = duration * 2.0;
        double wrapped = std::fmod(timeSeconds, period);
        if (wrapped < 0.0) wrapped += period;
        return wrapped <= duration ? wrapped : period - wrapped;
    }
    }
    return 0.0;
}

AnimationValue EvaluateTrackValue(const AnimationTargetDescriptor& descriptor,
                                  const AnimationTrack& track,
                                  double timeSeconds) {
    const auto& keyframes = track.keyframes;
    if (timeSeconds <= keyframes.front().timeSeconds) return keyframes.front().value;
    if (timeSeconds >= keyframes.back().timeSeconds) return keyframes.back().value;

    const auto upper = std::upper_bound(
        keyframes.begin(), keyframes.end(), timeSeconds,
        [](double value, const AnimationKeyframe& keyframe) {
            return value < keyframe.timeSeconds;
        });
    const auto lower = std::prev(upper);
    if (timeSeconds == lower->timeSeconds) return lower->value;
    if (lower->interpolation == AnimationInterpolation::Step) return lower->value;

    const double span = upper->timeSeconds - lower->timeSeconds;
    double factor = (timeSeconds - lower->timeSeconds) / span;
    if (lower->interpolation == AnimationInterpolation::Smoothstep) {
        factor = Smoothstep(factor);
    }

    if (descriptor.valueType == AnimationValueType::CompensatedReal) {
        return InterpolateCompensated(
            std::get<CompensatedAnimationValue>(lower->value),
            std::get<CompensatedAnimationValue>(upper->value), factor);
    }
    if (descriptor.valueType == AnimationValueType::Integer) {
        return lower->value;
    }

    const double from = std::get<double>(lower->value);
    const double to = std::get<double>(upper->value);
    if (descriptor.key == AnimationTargetKey::CameraScale) {
        const double logFrom = std::log(from);
        const double logTo = std::log(to);
        return std::exp(logFrom + (logTo - logFrom) * factor);
    }
    if (descriptor.key == AnimationTargetKey::RotationDegrees) {
        const double delta = std::remainder(to - from, 360.0);
        return std::remainder(from + delta * factor, 360.0);
    }
    return from + (to - from) * factor;
}

void ApplyValue(Preset& preset,
                AnimationTargetKey key,
                const AnimationValue& value) {
    switch (key) {
    case AnimationTargetKey::CameraCentreX: {
        const auto compensated = std::get<CompensatedAnimationValue>(value);
        preset.camera.centreX = compensated.high;
        preset.camera.centreXLow = compensated.low;
        return;
    }
    case AnimationTargetKey::CameraCentreY: {
        const auto compensated = std::get<CompensatedAnimationValue>(value);
        preset.camera.centreY = compensated.high;
        preset.camera.centreYLow = compensated.low;
        return;
    }
    case AnimationTargetKey::CameraScale:
        preset.camera.scale = std::get<double>(value);
        return;
    case AnimationTargetKey::RotationDegrees:
        preset.rotationDegrees = std::get<double>(value);
        return;
    case AnimationTargetKey::PaletteOffset:
        preset.colourOffset = std::get<double>(value);
        return;
    case AnimationTargetKey::PaletteFrequency:
        preset.paletteFrequency = std::get<double>(value);
        return;
    case AnimationTargetKey::PaletteGamma:
        preset.paletteGamma = std::get<double>(value);
        return;
    case AnimationTargetKey::Brightness:
        preset.brightness = std::get<double>(value);
        return;
    case AnimationTargetKey::Contrast:
        preset.contrast = std::get<double>(value);
        return;
    case AnimationTargetKey::Saturation:
        preset.saturation = std::get<double>(value);
        return;
    case AnimationTargetKey::StripeStrength:
        preset.equation.stripeStrength = std::get<double>(value);
        return;
    case AnimationTargetKey::BloomStrength:
        preset.equation.glowStrength = std::get<double>(value);
        return;
    case AnimationTargetKey::EdgeLightingStrength:
        preset.equation.edgeLightingStrength = std::get<double>(value);
        return;
    case AnimationTargetKey::EquationQuadraticReal:
        preset.equation.quadratic.real = std::get<double>(value);
        return;
    case AnimationTargetKey::EquationQuadraticImaginary:
        preset.equation.quadratic.imaginary = std::get<double>(value);
        return;
    case AnimationTargetKey::EquationPower:
        preset.equation.power = static_cast<int>(std::get<std::int64_t>(value));
        return;
    case AnimationTargetKey::EquationParameterPower:
        preset.equation.parameterPower = static_cast<int>(std::get<std::int64_t>(value));
        return;
    case AnimationTargetKey::EquationBailoutRadius:
        preset.equation.bailoutRadius = std::get<double>(value);
        return;
    }
}

std::string FirstValidationError(const AnimationTimelineValidationResult& validation) {
    if (validation.issues.empty()) return "Animation timeline validation failed.";
    return validation.issues.front().field + ": " + validation.issues.front().message;
}

struct JourneyConversionRow {
    CameraState camera;
    double transitionSeconds{0.0};
    double holdSeconds{0.0};
};

std::string TrimCopy(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1U);
}

bool ParseFiniteDouble(const std::string& text, double& value) {
    try {
        std::size_t consumed = 0U;
        value = std::stod(text, &consumed);
        return consumed == text.size() && std::isfinite(value);
    } catch (...) {
        return false;
    }
}

bool ParseJourneyForConversion(const std::string& script,
                               std::vector<JourneyConversionRow>& rows,
                               std::string& error) {
    rows.clear();
    error.clear();
    if (script.size() > 32768U) {
        error = "Journey text exceeds the 32,768-byte safety limit.";
        return false;
    }
    std::string normalised = script;
    for (char& character : normalised) {
        if (character == ';') character = '\n';
    }
    std::stringstream lines(normalised);
    std::string line;
    std::size_t sourceLine = 0U;
    while (std::getline(lines, line)) {
        ++sourceLine;
        line = TrimCopy(std::move(line));
        if (line.empty() || line.rfind("#", 0U) == 0U || line.rfind("//", 0U) == 0U) {
            continue;
        }
        if (rows.size() >= 128U) {
            error = "Journey conversion exceeds the 128-row safety limit.";
            return false;
        }
        for (char& character : line) {
            if (character == '|') character = ',';
        }
        std::stringstream columns(line);
        std::string column;
        std::vector<double> values;
        while (std::getline(columns, column, ',')) {
            column = TrimCopy(std::move(column));
            double value = 0.0;
            if (column.empty() || !ParseFiniteDouble(column, value)) {
                error = "Journey row " + std::to_string(sourceLine) +
                        " contains an invalid numeric value.";
                return false;
            }
            values.push_back(value);
        }
        if (values.size() != 4U && values.size() != 5U) {
            error = "Journey row " + std::to_string(sourceLine) +
                    " must contain X,Y,Scale,TransitionSeconds and optional HoldSeconds.";
            return false;
        }
        const double holdSeconds = values.size() == 5U ? values[4] : 2.0;
        if (values[0] < -4.0 || values[0] > 4.0 ||
            values[1] < -4.0 || values[1] > 4.0 ||
            values[2] < 1.0e-32 || values[2] > 4.0 ||
            values[3] < 1.0 || values[3] > 3600.0 ||
            holdSeconds < 0.0 || holdSeconds > 3600.0) {
            error = "Journey row " + std::to_string(sourceLine) +
                    " is outside the existing Journey validation ranges.";
            return false;
        }
        rows.push_back({{values[0], values[1], values[2], 0.0, 0.0},
                        values[3], holdSeconds});
    }
    if (rows.empty()) {
        error = "The Journey contains no convertible waypoint rows.";
        return false;
    }
    return true;
}

AnimationValue CameraValue(const CameraState& camera, AnimationTargetKey target) {
    switch (target) {
    case AnimationTargetKey::CameraCentreX:
        return CompensatedAnimationValue{camera.centreX, camera.centreXLow};
    case AnimationTargetKey::CameraCentreY:
        return CompensatedAnimationValue{camera.centreY, camera.centreYLow};
    case AnimationTargetKey::CameraScale:
        return camera.scale;
    default:
        return 0.0;
    }
}

bool CameraValuesEqual(const AnimationKeyframe& x,
                       const AnimationKeyframe& y,
                       const AnimationKeyframe& scale,
                       const CameraState& camera) {
    return std::get<CompensatedAnimationValue>(x.value) ==
               CompensatedAnimationValue{camera.centreX, camera.centreXLow} &&
           std::get<CompensatedAnimationValue>(y.value) ==
               CompensatedAnimationValue{camera.centreY, camera.centreYLow} &&
           std::get<double>(scale.value) == camera.scale;
}

CameraState CameraFromKeyframes(const AnimationKeyframe& x,
                                const AnimationKeyframe& y,
                                const AnimationKeyframe& scale) {
    const auto centreX = std::get<CompensatedAnimationValue>(x.value);
    const auto centreY = std::get<CompensatedAnimationValue>(y.value);
    return {centreX.high, centreY.high, std::get<double>(scale.value),
            centreX.low, centreY.low};
}

bool SameCameraKeyframeValue(const AnimationKeyframe& leftX,
                             const AnimationKeyframe& leftY,
                             const AnimationKeyframe& leftScale,
                             const AnimationKeyframe& rightX,
                             const AnimationKeyframe& rightY,
                             const AnimationKeyframe& rightScale) {
    return leftX.value == rightX.value && leftY.value == rightY.value &&
           leftScale.value == rightScale.value;
}

std::string FormatJourneyDouble(double value) {
    std::ostringstream stream;
    stream << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return stream.str();
}

} // namespace

std::span<const AnimationTargetDescriptor> GeneralAnimationTargetDescriptors() noexcept {
    return kTargetDescriptors;
}

std::optional<AnimationTargetKey> AnimationTargetFromStableName(
    std::string_view stableName) noexcept {
    const auto* descriptor = DescriptorForName(stableName);
    if (!descriptor) return std::nullopt;
    return descriptor->key;
}

std::string_view StableAnimationTargetName(AnimationTargetKey key) noexcept {
    const auto* descriptor = DescriptorForKey(key);
    return descriptor ? descriptor->stableName : std::string_view{};
}

AnimationTimelineValidationResult ValidateGeneralAnimationTimeline(
    const AnimationTimeline& timeline) {
    AnimationTimelineValidationResult result;
    if (!IsStableId(timeline.id)) {
        AddIssue(result, "timeline.id",
                 "Timeline ID must contain 1 to 80 ASCII letters, digits, '.', '_', '-' or ':'.");
    }
    if (!std::isfinite(timeline.durationSeconds) || timeline.durationSeconds <= 0.0 ||
        timeline.durationSeconds > kMaximumDurationSeconds) {
        AddIssue(result, "timeline.durationSeconds",
                 "Timeline duration must be finite, positive and no longer than seven days.");
    }
    if (timeline.tracks.size() > kMaximumTracks) {
        AddIssue(result, "timeline.tracks", "Timeline exceeds the 256-track safety limit.");
    }

    std::set<std::string> trackIds;
    std::set<std::string> keyframeIds;
    std::set<std::string> enabledTargets;
    std::size_t totalKeyframes = 0U;
    for (std::size_t trackIndex = 0U; trackIndex < timeline.tracks.size(); ++trackIndex) {
        const auto& track = timeline.tracks[trackIndex];
        const std::string prefix = "timeline.tracks[" + std::to_string(trackIndex) + "]";
        if (!IsStableId(track.id)) {
            AddIssue(result, prefix + ".id",
                     "Track ID must contain 1 to 80 stable ASCII identifier characters.");
        } else if (!trackIds.insert(track.id).second) {
            AddIssue(result, prefix + ".id", "Track IDs must be unique within a timeline.");
        }
        if (!IsStableTargetName(track.target)) {
            AddIssue(result, prefix + ".target",
                     "Track target must contain 1 to 120 stable ASCII identifier characters.");
        }

        totalKeyframes += track.keyframes.size();
        const auto* descriptor = DescriptorForName(track.target);
        if (!track.enabled && !descriptor) {
            AddWarning(result, prefix + ".target",
                       "Disabled unknown target was retained and will not be evaluated.");
        }
        if (track.enabled && !descriptor) {
            AddIssue(result, prefix + ".target", "Enabled track target is unknown or unsupported.");
        }
        if (track.enabled && descriptor && !enabledTargets.insert(track.target).second) {
            AddIssue(result, prefix + ".target",
                     "Duplicate enabled Replace targets are not allowed.");
        }
        if (track.enabled && track.keyframes.empty()) {
            AddIssue(result, prefix + ".keyframes", "Enabled tracks require at least one keyframe.");
        }

        double previousTime = -1.0;
        for (std::size_t keyframeIndex = 0U; keyframeIndex < track.keyframes.size();
             ++keyframeIndex) {
            const auto& keyframe = track.keyframes[keyframeIndex];
            const std::string keyframePrefix = prefix + ".keyframes[" +
                                               std::to_string(keyframeIndex) + "]";
            if (!IsStableId(keyframe.id)) {
                AddIssue(result, keyframePrefix + ".id",
                         "Keyframe ID must contain 1 to 80 stable ASCII identifier characters.");
            } else if (!keyframeIds.insert(keyframe.id).second) {
                AddIssue(result, keyframePrefix + ".id",
                         "Keyframe IDs must be unique within a timeline.");
            }
            if (!std::isfinite(keyframe.timeSeconds) || keyframe.timeSeconds < 0.0 ||
                keyframe.timeSeconds > timeline.durationSeconds) {
                AddIssue(result, keyframePrefix + ".timeSeconds",
                         "Keyframe time must be finite and within the timeline duration.");
            }
            if (keyframeIndex > 0U && keyframe.timeSeconds <= previousTime) {
                AddIssue(result, keyframePrefix + ".timeSeconds",
                         "Track keyframe times must be strictly increasing.");
            }
            previousTime = keyframe.timeSeconds;

            if (!track.enabled || !descriptor) continue;
            std::string valueMessage;
            if (!ValidateValue(*descriptor, keyframe.value, valueMessage)) {
                AddIssue(result, keyframePrefix + ".value", std::move(valueMessage));
            }
            if (!SupportsInterpolation(*descriptor, keyframe.interpolation)) {
                AddIssue(result, keyframePrefix + ".interpolation",
                         "Interpolation mode is unsupported for this target.");
            }
        }
    }
    if (totalKeyframes > kMaximumKeyframes) {
        AddIssue(result, "timeline.keyframes",
                 "Timeline exceeds the 4096-keyframe safety limit.");
    }
    return result;
}

bool ReadGeneralAnimationTargetValue(const Preset& preset,
                                     std::string_view stableTargetName,
                                     AnimationValue& value,
                                     std::string& error) {
    error.clear();
    const auto target = AnimationTargetFromStableName(stableTargetName);
    if (!target) {
        error = "Animation target is unknown or unsupported.";
        return false;
    }
    switch (*target) {
    case AnimationTargetKey::CameraCentreX:
        value = CompensatedAnimationValue{preset.camera.centreX, preset.camera.centreXLow};
        return true;
    case AnimationTargetKey::CameraCentreY:
        value = CompensatedAnimationValue{preset.camera.centreY, preset.camera.centreYLow};
        return true;
    case AnimationTargetKey::CameraScale: value = preset.camera.scale; return true;
    case AnimationTargetKey::RotationDegrees: value = preset.rotationDegrees; return true;
    case AnimationTargetKey::PaletteOffset: value = preset.colourOffset; return true;
    case AnimationTargetKey::PaletteFrequency: value = preset.paletteFrequency; return true;
    case AnimationTargetKey::PaletteGamma: value = preset.paletteGamma; return true;
    case AnimationTargetKey::Brightness: value = preset.brightness; return true;
    case AnimationTargetKey::Contrast: value = preset.contrast; return true;
    case AnimationTargetKey::Saturation: value = preset.saturation; return true;
    case AnimationTargetKey::StripeStrength: value = preset.equation.stripeStrength; return true;
    case AnimationTargetKey::BloomStrength: value = preset.equation.glowStrength; return true;
    case AnimationTargetKey::EdgeLightingStrength:
        value = preset.equation.edgeLightingStrength;
        return true;
    case AnimationTargetKey::EquationQuadraticReal:
        value = preset.equation.quadratic.real;
        return true;
    case AnimationTargetKey::EquationQuadraticImaginary:
        value = preset.equation.quadratic.imaginary;
        return true;
    case AnimationTargetKey::EquationPower:
        value = static_cast<std::int64_t>(preset.equation.power);
        return true;
    case AnimationTargetKey::EquationParameterPower:
        value = static_cast<std::int64_t>(preset.equation.parameterPower);
        return true;
    case AnimationTargetKey::EquationBailoutRadius:
        value = preset.equation.bailoutRadius;
        return true;
    }
    error = "Animation target is unsupported.";
    return false;
}

bool ConvertJourneyToGeneralAnimation(const Preset& preset,
                                      AnimationTimeline& timeline,
                                      std::string& error) {
    std::vector<JourneyConversionRow> rows;
    if (!ParseJourneyForConversion(preset.automaticJourneyWaypoints, rows, error)) {
        return false;
    }

    double duration = 0.0;
    for (const auto& row : rows) {
        duration += row.transitionSeconds + row.holdSeconds;
    }
    if (!std::isfinite(duration) || duration <= 0.0 || duration > kMaximumDurationSeconds) {
        error = "Converted Journey duration exceeds the seven-day timeline safety limit.";
        return false;
    }

    AnimationTimeline candidate;
    candidate.id = "journey-timeline";
    candidate.durationSeconds = duration;
    candidate.loopMode = AnimationLoopMode::Clamp;

    const std::array<AnimationTargetKey, 3> targets{{
        AnimationTargetKey::CameraCentreX,
        AnimationTargetKey::CameraCentreY,
        AnimationTargetKey::CameraScale,
    }};
    const std::array<std::string, 3> trackIds{{
        "journey-camera-centre-x",
        "journey-camera-centre-y",
        "journey-camera-scale",
    }};
    const std::array<std::string, 3> keyPrefixes{{"journey-x-", "journey-y-", "journey-scale-"}};

    for (std::size_t targetIndex = 0U; targetIndex < targets.size(); ++targetIndex) {
        AnimationTrack track;
        track.id = trackIds[targetIndex];
        track.target = std::string(StableAnimationTargetName(targets[targetIndex]));
        track.keyframes.push_back({keyPrefixes[targetIndex] + "0", 0.0,
                                   CameraValue(preset.camera, targets[targetIndex]),
                                   AnimationInterpolation::Smoothstep});
        candidate.tracks.push_back(std::move(track));
    }

    double cursor = 0.0;
    std::size_t keyIndex = 1U;
    for (std::size_t rowIndex = 0U; rowIndex < rows.size(); ++rowIndex) {
        const auto& row = rows[rowIndex];
        cursor += row.transitionSeconds;
        for (std::size_t targetIndex = 0U; targetIndex < targets.size(); ++targetIndex) {
            candidate.tracks[targetIndex].keyframes.push_back({
                keyPrefixes[targetIndex] + std::to_string(keyIndex), cursor,
                CameraValue(row.camera, targets[targetIndex]),
                row.holdSeconds > 0.0 ? AnimationInterpolation::Step
                                      : AnimationInterpolation::Smoothstep});
        }
        ++keyIndex;
        if (row.holdSeconds > 0.0) {
            cursor += row.holdSeconds;
            for (std::size_t targetIndex = 0U; targetIndex < targets.size(); ++targetIndex) {
                candidate.tracks[targetIndex].keyframes.push_back({
                    keyPrefixes[targetIndex] + std::to_string(keyIndex), cursor,
                    CameraValue(row.camera, targets[targetIndex]),
                    AnimationInterpolation::Smoothstep});
            }
            ++keyIndex;
        }
    }
    for (auto& track : candidate.tracks) {
        track.keyframes.back().interpolation = AnimationInterpolation::Step;
    }

    const auto validation = ValidateGeneralAnimationTimeline(candidate);
    if (!validation.valid) {
        error = FirstValidationError(validation);
        return false;
    }
    timeline = std::move(candidate);
    error.clear();
    return true;
}

bool ConvertGeneralAnimationToJourney(const Preset& baseSnapshot,
                                      const AnimationTimeline& timeline,
                                      std::string& journeyScript,
                                      std::string& error) {
    journeyScript.clear();
    error.clear();
    const auto validation = ValidateGeneralAnimationTimeline(timeline);
    if (!validation.valid) {
        error = FirstValidationError(validation);
        return false;
    }
    if (timeline.loopMode != AnimationLoopMode::Clamp) {
        error = "Journey conversion requires Clamp timing so no cyclic segment is silently lost.";
        return false;
    }

    const AnimationTrack* xTrack = nullptr;
    const AnimationTrack* yTrack = nullptr;
    const AnimationTrack* scaleTrack = nullptr;
    for (const auto& track : timeline.tracks) {
        if (!track.enabled) continue;
        const auto target = AnimationTargetFromStableName(track.target);
        if (!target) {
            error = "Journey conversion encountered an unsupported enabled target.";
            return false;
        }
        switch (*target) {
        case AnimationTargetKey::CameraCentreX: xTrack = &track; break;
        case AnimationTargetKey::CameraCentreY: yTrack = &track; break;
        case AnimationTargetKey::CameraScale: scaleTrack = &track; break;
        default:
            error = "Journey conversion supports only enabled camera centre and scale tracks.";
            return false;
        }
    }
    if (!xTrack || !yTrack || !scaleTrack) {
        error = "Journey conversion requires enabled camera centre X, centre Y and scale tracks.";
        return false;
    }
    const std::size_t count = xTrack->keyframes.size();
    if (count < 2U || yTrack->keyframes.size() != count || scaleTrack->keyframes.size() != count) {
        error = "Journey camera tracks must use the same keyframe count.";
        return false;
    }
    for (std::size_t index = 0U; index < count; ++index) {
        if (xTrack->keyframes[index].timeSeconds != yTrack->keyframes[index].timeSeconds ||
            xTrack->keyframes[index].timeSeconds != scaleTrack->keyframes[index].timeSeconds ||
            xTrack->keyframes[index].interpolation != yTrack->keyframes[index].interpolation ||
            xTrack->keyframes[index].interpolation != scaleTrack->keyframes[index].interpolation) {
            error = "Journey camera tracks must share an identical time and interpolation grid.";
            return false;
        }
    }
    if (xTrack->keyframes.front().timeSeconds != 0.0 ||
        !CameraValuesEqual(xTrack->keyframes.front(), yTrack->keyframes.front(),
                           scaleTrack->keyframes.front(), baseSnapshot.camera)) {
        error = "Journey camera tracks must begin at time zero with the current project camera.";
        return false;
    }
    if (xTrack->keyframes.back().timeSeconds != timeline.durationSeconds) {
        error = "Journey camera tracks must finish exactly at the timeline duration.";
        return false;
    }

    std::ostringstream script;
    std::size_t previous = 0U;
    while (previous + 1U < count) {
        const std::size_t arrival = previous + 1U;
        if (SameCameraKeyframeValue(xTrack->keyframes[previous], yTrack->keyframes[previous],
                                    scaleTrack->keyframes[previous], xTrack->keyframes[arrival],
                                    yTrack->keyframes[arrival], scaleTrack->keyframes[arrival])) {
            error = "Journey conversion found a hold without a preceding transition.";
            return false;
        }
        if (xTrack->keyframes[previous].interpolation != AnimationInterpolation::Smoothstep) {
            error = "Journey transitions require Smoothstep interpolation.";
            return false;
        }
        const double transition = xTrack->keyframes[arrival].timeSeconds -
                                  xTrack->keyframes[previous].timeSeconds;
        if (transition < 1.0 || transition > 3600.0) {
            error = "Journey transition timing must remain between 1 and 3600 seconds.";
            return false;
        }

        CameraState waypoint = CameraFromKeyframes(xTrack->keyframes[arrival],
                                                   yTrack->keyframes[arrival],
                                                   scaleTrack->keyframes[arrival]);
        if (waypoint.centreXLow != 0.0 || waypoint.centreYLow != 0.0) {
            error = "Journey text cannot preserve compensated low camera components.";
            return false;
        }
        if (waypoint.centreX < -4.0 || waypoint.centreX > 4.0 ||
            waypoint.centreY < -4.0 || waypoint.centreY > 4.0 ||
            waypoint.scale < 1.0e-32 || waypoint.scale > 4.0) {
            error = "Journey waypoint is outside the existing Journey camera ranges.";
            return false;
        }

        double hold = 0.0;
        std::size_t nextPrevious = arrival;
        if (arrival + 1U < count &&
            SameCameraKeyframeValue(xTrack->keyframes[arrival], yTrack->keyframes[arrival],
                                    scaleTrack->keyframes[arrival], xTrack->keyframes[arrival + 1U],
                                    yTrack->keyframes[arrival + 1U], scaleTrack->keyframes[arrival + 1U])) {
            if (xTrack->keyframes[arrival].interpolation != AnimationInterpolation::Step) {
                error = "Journey holds require Step interpolation.";
                return false;
            }
            hold = xTrack->keyframes[arrival + 1U].timeSeconds -
                   xTrack->keyframes[arrival].timeSeconds;
            if (hold <= 0.0 || hold > 3600.0) {
                error = "Journey hold timing must be greater than zero and no longer than 3600 seconds.";
                return false;
            }
            nextPrevious = arrival + 1U;
        } else if (arrival + 1U < count &&
                   xTrack->keyframes[arrival].interpolation != AnimationInterpolation::Smoothstep) {
            error = "A zero-hold Journey waypoint must begin the next Smoothstep transition.";
            return false;
        }

        script << FormatJourneyDouble(waypoint.centreX) << ','
               << FormatJourneyDouble(waypoint.centreY) << ','
               << FormatJourneyDouble(waypoint.scale) << ','
               << FormatJourneyDouble(transition) << ','
               << FormatJourneyDouble(hold) << '\n';
        previous = nextPrevious;
    }
    journeyScript = script.str();
    return true;
}

bool AddGeneralAnimationKeyframeFromPreset(AnimationTimeline& timeline,
                                           std::string_view trackId,
                                           std::string keyframeId,
                                           double timeSeconds,
                                           const Preset& preset,
                                           AnimationInterpolation interpolation,
                                           std::string& error) {
    error.clear();
    const auto trackIterator = std::find_if(
        timeline.tracks.begin(), timeline.tracks.end(),
        [&](const AnimationTrack& track) { return track.id == trackId; });
    if (trackIterator == timeline.tracks.end()) {
        error = "The selected animation track no longer exists.";
        return false;
    }
    if (!IsStableId(keyframeId)) {
        error = "Keyframe ID does not satisfy the stable-ID contract.";
        return false;
    }
    for (const auto& track : timeline.tracks) {
        for (const auto& keyframe : track.keyframes) {
            if (keyframe.id == keyframeId) {
                error = "Keyframe ID is already in use.";
                return false;
            }
        }
    }
    if (!std::isfinite(timeSeconds) || timeSeconds < 0.0 ||
        timeSeconds > timeline.durationSeconds) {
        error = "Keyframe time must be within the timeline duration.";
        return false;
    }
    for (const auto& keyframe : trackIterator->keyframes) {
        if (keyframe.timeSeconds == timeSeconds) {
            error = "The selected track already has a keyframe at this time.";
            return false;
        }
    }
    AnimationValue value;
    if (!ReadGeneralAnimationTargetValue(preset, trackIterator->target, value, error)) {
        return false;
    }

    AnimationTimeline candidate = timeline;
    auto candidateTrack = std::find_if(
        candidate.tracks.begin(), candidate.tracks.end(),
        [&](const AnimationTrack& track) { return track.id == trackId; });
    candidateTrack->keyframes.push_back(
        {std::move(keyframeId), timeSeconds, std::move(value), interpolation});
    std::sort(candidateTrack->keyframes.begin(), candidateTrack->keyframes.end(),
              [](const AnimationKeyframe& left, const AnimationKeyframe& right) {
                  return left.timeSeconds < right.timeSeconds;
              });
    const auto validation = ValidateGeneralAnimationTimeline(candidate);
    if (!validation.valid) {
        error = FirstValidationError(validation);
        return false;
    }
    timeline = std::move(candidate);
    return true;
}

bool EvaluateGeneralAnimation(const Preset& baseSnapshot,
                              const AnimationTimeline& timeline,
                              double timeSeconds,
                              std::uint64_t seed,
                              AnimationEvaluationResult& result,
                              std::string& error) {
    result = {};
    error.clear();
    if (!std::isfinite(timeSeconds)) {
        error = "Animation evaluation time must be finite.";
        return false;
    }

    const auto timelineValidation = ValidateGeneralAnimationTimeline(timeline);
    if (!timelineValidation.valid) {
        error = FirstValidationError(timelineValidation);
        return false;
    }

    Preset validatedBase = baseSnapshot;
    (void)ValidateAndNormalise(validatedBase);
    if (validatedBase != baseSnapshot) {
        error = "Animation base snapshot must already satisfy the authoritative Preset contract.";
        return false;
    }

    result.framePreset = baseSnapshot;
    result.resolvedTimeSeconds = ResolveTimelineTime(timeline, timeSeconds);
    result.seed = seed;
    result.warnings = timelineValidation.warnings;

    std::vector<const AnimationTrack*> enabledTracks;
    enabledTracks.reserve(timeline.tracks.size());
    for (const auto& track : timeline.tracks) {
        if (track.enabled) enabledTracks.push_back(&track);
    }
    std::sort(enabledTracks.begin(), enabledTracks.end(),
              [](const AnimationTrack* left, const AnimationTrack* right) {
                  return left->id < right->id;
              });

    bool cameraCentreChanged = false;
    bool cameraChanged = false;
    for (const AnimationTrack* track : enabledTracks) {
        const auto* descriptor = DescriptorForName(track->target);
        if (!descriptor) {
            error = "Validated animation target disappeared during evaluation.";
            return false;
        }
        const AnimationValue value =
            EvaluateTrackValue(*descriptor, *track, result.resolvedTimeSeconds);
        ApplyValue(result.framePreset, descriptor->key, value);
        cameraCentreChanged = cameraCentreChanged ||
                              descriptor->key == AnimationTargetKey::CameraCentreX ||
                              descriptor->key == AnimationTargetKey::CameraCentreY;
        cameraChanged = cameraChanged ||
                        descriptor->key == AnimationTargetKey::CameraCentreX ||
                        descriptor->key == AnimationTargetKey::CameraCentreY ||
                        descriptor->key == AnimationTargetKey::CameraScale;
        result.invalidationMask = static_cast<ParameterInvalidationMask>(
            result.invalidationMask | ParameterInvalidationBit(descriptor->invalidation));
        result.appliedTrackIds.push_back(track->id);
    }
    if (cameraCentreChanged) NormaliseCamera(result.framePreset.camera);
    if (cameraChanged) {
        if (baseSnapshot.exactCamera.has_value()) {
            LegacyCameraAdaptation legacyBase;
            if (!AdaptExactCameraToLegacy(*baseSnapshot.exactCamera, legacyBase, error)) {
                error = "Animation evaluation could not inspect exact camera authority: " + error;
                result = {};
                return false;
            }
            if (legacyBase.centreXLoss || legacyBase.centreYLoss || legacyBase.halfHeightLoss) {
                error = "Camera animation cannot modify a lossy exact camera until exact keyframes are implemented.";
                result = {};
                return false;
            }
        }
        result.framePreset.exactCamera.reset();
        if (!EnsureExactCamera(result.framePreset, error)) {
            error = "Animation evaluation could not rebuild exact camera authority: " + error;
            result = {};
            return false;
        }
    }

    Preset normalisedResult = result.framePreset;
    (void)ValidateAndNormalise(normalisedResult);
    if (normalisedResult != result.framePreset) {
        error = "Animation evaluation produced state outside the authoritative Preset contract.";
        result = {};
        return false;
    }
    return true;
}

std::optional<std::size_t> AnimationClockBank::Index(AnimationClockDomain domain) noexcept {
    switch (domain) {
    case AnimationClockDomain::Preview: return 0U;
    case AnimationClockDomain::Wallpaper: return 1U;
    case AnimationClockDomain::Export: return 2U;
    }
    return std::nullopt;
}

bool AnimationClockBank::SetTime(AnimationClockDomain domain,
                                 double timeSeconds,
                                 std::string& error) {
    error.clear();
    const auto index = Index(domain);
    if (!index) {
        error = "Animation clock domain is unsupported.";
        return false;
    }
    if (!std::isfinite(timeSeconds) || timeSeconds < 0.0) {
        error = "Animation clock time must be finite and non-negative.";
        return false;
    }
    times_[*index] = timeSeconds;
    return true;
}

bool AnimationClockBank::Advance(AnimationClockDomain domain,
                                 double deltaSeconds,
                                 std::string& error) {
    error.clear();
    const auto index = Index(domain);
    if (!index) {
        error = "Animation clock domain is unsupported.";
        return false;
    }
    if (!std::isfinite(deltaSeconds)) {
        error = "Animation clock delta must be finite.";
        return false;
    }
    const double candidate = times_[*index] + deltaSeconds;
    if (!std::isfinite(candidate) || candidate < 0.0) {
        error = "Animation clock advance would produce an invalid time.";
        return false;
    }
    times_[*index] = candidate;
    return true;
}

double AnimationClockBank::Time(AnimationClockDomain domain) const noexcept {
    const auto index = Index(domain);
    return index ? times_[*index] : 0.0;
}

void AnimationClockBank::Reset(AnimationClockDomain domain) noexcept {
    const auto index = Index(domain);
    if (index) times_[*index] = 0.0;
}

void AnimationClockBank::ResetAll() noexcept {
    times_.fill(0.0);
}

} // namespace mw
