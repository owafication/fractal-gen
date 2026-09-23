#pragma once

#include "Core/Models.h"
#include "Core/ProjectState.h"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace mw {

// PH-06 timeline data is intentionally runtime-only. Caller-supplied IDs are
// stable within the timeline, while persistence remains deferred under DEC-014.
enum class AnimationLoopMode {
    Clamp,
    Loop,
    PingPong,
};

enum class AnimationInterpolation {
    Step,
    Linear,
    Smoothstep,
};

enum class AnimationValueType {
    Real,
    Integer,
    CompensatedReal,
};

enum class AnimationTargetKey {
    CameraCentreX,
    CameraCentreY,
    CameraScale,
    RotationDegrees,
    PaletteOffset,
    PaletteFrequency,
    PaletteGamma,
    Brightness,
    Contrast,
    Saturation,
    StripeStrength,
    BloomStrength,
    EdgeLightingStrength,
    EquationQuadraticReal,
    EquationQuadraticImaginary,
    EquationPower,
    EquationParameterPower,
    EquationBailoutRadius,
};

struct AnimationTargetDescriptor {
    AnimationTargetKey key;
    std::string_view stableName;
    AnimationValueType valueType;
    ParameterInvalidation invalidation;
    bool supportsStep;
    bool supportsLinear;
    bool supportsSmoothstep;
};

[[nodiscard]] std::span<const AnimationTargetDescriptor>
GeneralAnimationTargetDescriptors() noexcept;
[[nodiscard]] std::optional<AnimationTargetKey>
AnimationTargetFromStableName(std::string_view stableName) noexcept;
[[nodiscard]] std::string_view StableAnimationTargetName(AnimationTargetKey key) noexcept;

struct CompensatedAnimationValue {
    double high{0.0};
    double low{0.0};

    bool operator==(const CompensatedAnimationValue&) const = default;
};

using AnimationValue = std::variant<double, std::int64_t, CompensatedAnimationValue>;

struct AnimationKeyframe {
    std::string id;
    double timeSeconds{0.0};
    AnimationValue value{0.0};
    AnimationInterpolation interpolation{AnimationInterpolation::Linear};

    bool operator==(const AnimationKeyframe&) const = default;
};

struct AnimationTrack {
    std::string id;
    // Stable target name rather than an enum value so a disabled unknown future
    // target can be retained without being executed.
    std::string target;
    bool enabled{true};
    std::vector<AnimationKeyframe> keyframes;

    bool operator==(const AnimationTrack&) const = default;
};

struct AnimationTimeline {
    std::string id;
    double durationSeconds{1.0};
    AnimationLoopMode loopMode{AnimationLoopMode::Clamp};
    std::vector<AnimationTrack> tracks;

    bool operator==(const AnimationTimeline&) const = default;
};

struct AnimationTimelineValidationResult {
    bool valid{true};
    std::vector<ValidationIssue> issues;
    std::vector<ValidationIssue> warnings;
};

[[nodiscard]] AnimationTimelineValidationResult
ValidateGeneralAnimationTimeline(const AnimationTimeline& timeline);

// Reads one supported target from an authoritative preset snapshot. This is
// used by PH-07 authoring commands without creating another project authority.
bool ReadGeneralAnimationTargetValue(const Preset& preset,
                                     std::string_view stableTargetName,
                                     AnimationValue& value,
                                     std::string& error);

// Converts the existing structured Journey text into a deterministic,
// one-pass camera timeline. Malformed non-comment rows are rejected rather
// than silently omitted. Persistence remains deferred under DEC-014.
bool ConvertJourneyToGeneralAnimation(const Preset& preset,
                                      AnimationTimeline& timeline,
                                      std::string& error);

// Converts only the exact supported camera-only subset back to Journey text.
// Unsupported enabled tracks, mismatched keyframe grids, lossy compensated
// values or non-representable timing are refused with an explanation.
bool ConvertGeneralAnimationToJourney(const Preset& baseSnapshot,
                                      const AnimationTimeline& timeline,
                                      std::string& journeyScript,
                                      std::string& error);

// Adds a keyframe using the current authoritative preset value. Caller-supplied
// IDs preserve the PH-06 stable-ID contract.
bool AddGeneralAnimationKeyframeFromPreset(AnimationTimeline& timeline,
                                           std::string_view trackId,
                                           std::string keyframeId,
                                           double timeSeconds,
                                           const Preset& preset,
                                           AnimationInterpolation interpolation,
                                           std::string& error);


struct AnimationEvaluationResult {
    Preset framePreset;
    double resolvedTimeSeconds{0.0};
    std::uint64_t seed{0};
    ParameterInvalidationMask invalidationMask{0U};
    std::vector<std::string> appliedTrackIds;
    std::vector<ValidationIssue> warnings;
};

// Evaluates into a frame-local Preset copy. The base snapshot is never mutated,
// evaluation creates no undo entry, and identical inputs produce identical state.
bool EvaluateGeneralAnimation(const Preset& baseSnapshot,
                              const AnimationTimeline& timeline,
                              double timeSeconds,
                              std::uint64_t seed,
                              AnimationEvaluationResult& result,
                              std::string& error);

enum class AnimationClockDomain {
    Preview,
    Wallpaper,
    Export,
};

class AnimationClockBank {
public:
    bool SetTime(AnimationClockDomain domain, double timeSeconds, std::string& error);
    bool Advance(AnimationClockDomain domain, double deltaSeconds, std::string& error);
    [[nodiscard]] double Time(AnimationClockDomain domain) const noexcept;
    void Reset(AnimationClockDomain domain) noexcept;
    void ResetAll() noexcept;

private:
    [[nodiscard]] static std::optional<std::size_t> Index(AnimationClockDomain domain) noexcept;

    std::array<double, 3> times_{};
};

} // namespace mw
