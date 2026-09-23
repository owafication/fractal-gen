#pragma once

#include "Core/Models.h"

#include <array>
#include <string_view>
#include <string>
#include <string_view>
#include <vector>

namespace mw {

struct ReferenceOrbitPoint {
    // Four-float non-overlapping expansions retain up to roughly 96 bits when
    // uploaded to the perturbation shader.
    std::array<float, 4> real{};
    std::array<float, 4> imaginary{};
};

// Transport contract for the existing four-float expansion. This names the
// bytes sent to a renderer without claiming a measured deep-precision ceiling.
struct OrbitEncodingDescriptor {
    std::string_view identifier;
    int version{0};
    int componentsPerCoordinate{0};
    int componentBits{0};
    bool validatedCeilingAvailable{false};
    int validatedBits{0};
};

[[nodiscard]] const OrbitEncodingDescriptor& CurrentOrbitEncoding() noexcept;

struct ReferenceOrbit {
    std::vector<ReferenceOrbitPoint> points;
    bool escaped{false};
    int escapeIteration{0};
    int precisionBits{0};
};

enum class PerturbationProfile {
    Unsupported,
    AnalyticQuadratic,
    TricornQuadratic,
};

struct PerturbationFormulaCapability {
    PerturbationProfile profile{PerturbationProfile::Unsupported};
    std::string_view identifier{"unsupported"};
    int version{0};
    bool supported{false};
};

// A renderer must distinguish an ordinary stable perturbation sample from a
// sample that required a replacement reference, and from one that remains
// unresolved. The latter is never a valid input to final colouring/output.
enum class PerturbationSampleValidity {
    Stable,
    Rebased,
    Unresolved,
};

struct PerturbationSampleResult {
    int iterations{0};
    bool escaped{false};
    bool stable{true};
    bool referenceRefreshed{false};
    PerturbationSampleValidity validity{PerturbationSampleValidity::Stable};
    double finalReal{0.0};
    double finalImaginary{0.0};
    double maximumRelativeDelta{0.0};
};

void AddCompensated(double& high, double& low, double delta) noexcept;
void NormaliseCamera(CameraState& camera) noexcept;
void OffsetCamera(CameraState& camera, double deltaX, double deltaY) noexcept;
[[nodiscard]] double CameraCentreX(const CameraState& camera) noexcept;
[[nodiscard]] double CameraCentreY(const CameraState& camera) noexcept;
[[nodiscard]] double CameraZoom(const CameraState& camera) noexcept;

ReferenceOrbit BuildReferenceOrbitDouble(const CameraState& camera,
                                         const EquationSettings& equation,
                                         int maximumIterations);
ReferenceOrbit BuildReferenceOrbitArbitrary(const CameraState& camera,
                                            const EquationSettings& equation,
                                            int maximumIterations,
                                            int precisionBits);

[[nodiscard]] PerturbationProfile ResolvePerturbationProfile(
    const EquationSettings& equation) noexcept;
[[nodiscard]] PerturbationFormulaCapability DescribePerturbationProfile(
    PerturbationProfile profile) noexcept;
[[nodiscard]] bool EquationSupportsPerturbation(const EquationSettings& equation) noexcept;
PerturbationSampleResult EvaluatePerturbationSample(
    const CameraState& referenceCamera,
    const EquationSettings& equation,
    int maximumIterations,
    double normalisedOffsetX,
    double normalisedOffsetY,
    int arbitraryPrecisionBits = 0,
    bool allowReferenceRefresh = true);
[[nodiscard]] std::string PrecisionModeDisplayName(PrecisionMode mode);

} // namespace mw
