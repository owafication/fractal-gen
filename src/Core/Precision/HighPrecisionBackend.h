#pragma once

#include "Core/DeepZoom.h"
#include "Core/MandelbrotMath.h"
#include "Core/Precision/ExactCamera.h"

#include <string_view>
#include <cstdint>
#include <functional>

namespace mw {

struct ExactStillRenderSample;

struct HighPrecisionBackendDescriptor {
    std::string_view packageName;
    std::string_view packageVersion;
    std::string_view licence;
    std::string_view backend;
    int precisionBits{0};
    bool headerOnly{false};
    bool requiresRuntimeArtifact{false};
    bool fixedStorage{false};
};


[[nodiscard]] const HighPrecisionBackendDescriptor& IndependentHighPrecisionBackend() noexcept;

using HighPrecisionCancellationCallback = std::function<bool()>;

struct HighPrecisionEscapeResult {
    int iterations{0};
    bool escaped{false};
    double smoothValue{0.0};
    int precisionBits{0};
};

// Fixture-specific error report for the current GPU coordinate reconstruction:
// ordered float additions of the four uploaded orbit components. This describes
// one producer/operation pair and is deliberately not a validated GPU ceiling.
struct OrbitEncodingMeasurement {
    int sourcePrecisionBits{0};
    int requestedIterations{0};
    int comparedCoordinates{0};
    bool escaped{false};
    int escapeIteration{0};
    double maximumAbsoluteError{0.0};
    double maximumRelativeError{0.0};
};

constexpr int kMinimumDirectHighPrecisionBits = 512;
constexpr int kMaximumDirectHighPrecisionBits = 16384;

[[nodiscard]] EscapeResult ToEscapeResult(const HighPrecisionEscapeResult& result) noexcept;

// Independent direct-reference implementation used to validate the existing
// perturbation/orbit path. It deliberately does not replace production policy
// or persisted camera authority in this dependency-review slice.
// Unsupported formulas and non-finite input are rejected.
[[nodiscard]] ReferenceOrbit BuildIndependentHighPrecisionReferenceOrbit(
    const CameraState& camera,
    const EquationSettings& equation,
    int maximumIterations);

// Measures the existing float4 orbit-coordinate reconstruction against the
// independent Boost-512 producer for one bounded legacy-camera fixture.
[[nodiscard]] OrbitEncodingMeasurement MeasureIndependentOrbitEncoding(
    const CameraState& camera, const EquationSettings& equation, int maximumIterations);


// Consumes canonical exact camera text without first passing through CameraState.
// Values outside the fixed 512-bit precision envelope are rejected rather than
// silently rounded. This independent route is not production planner policy.
[[nodiscard]] ReferenceOrbit BuildIndependentHighPrecisionReferenceOrbit(
    const ExactCamera& camera,
    const EquationSettings& equation,
    int maximumIterations,
    const HighPrecisionCancellationCallback& cancellationCallback = {},
    int precisionBits = kMinimumDirectHighPrecisionBits);

// Directly evaluates one unrotated global still sample from its exact camera
// and rational offsets. This is an escape-classification primitive, not a
// colour renderer or a perturbation correction claim.
[[nodiscard]] bool EvaluateIndependentHighPrecisionSample(
    const ExactStillRenderSample& sample,
    const EquationSettings& equation,
    int maximumIterations,
    const HighPrecisionCancellationCallback& cancellationCallback,
    HighPrecisionEscapeResult& result,
    std::string& error,
    int precisionBits = kMinimumDirectHighPrecisionBits);

} // namespace mw
