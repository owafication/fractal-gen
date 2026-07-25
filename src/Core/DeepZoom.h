#pragma once

#include "Core/Models.h"

#include <array>
#include <string>
#include <vector>

namespace mw {

struct ReferenceOrbitPoint {
    // Four-float non-overlapping expansions retain up to roughly 96 bits when
    // uploaded to the perturbation shader.
    std::array<float, 4> real{};
    std::array<float, 4> imaginary{};
};

struct ReferenceOrbit {
    std::vector<ReferenceOrbitPoint> points;
    bool escaped{false};
    int escapeIteration{0};
    int precisionBits{0};
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

[[nodiscard]] bool EquationSupportsPerturbation(const EquationSettings& equation) noexcept;
[[nodiscard]] std::string PrecisionModeDisplayName(PrecisionMode mode);

} // namespace mw
