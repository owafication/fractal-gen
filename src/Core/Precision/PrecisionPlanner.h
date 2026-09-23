#pragma once

#include "Core/Models.h"
#include "Core/DeepZoom.h"
#include "Core/Precision/ExactCamera.h"

#include <string>
#include <string_view>

namespace mw {

inline constexpr std::string_view kPrecisionPlanVersion{"mw-precision-plan-v1"};

enum class PrecisionExecutionBackend {
    Refused,
    CpuFloat64,
    CpuBoost512Reference,
    CpuBoost2048Direct,
    CpuBoost8192Direct,
    CpuBoost16384Direct,
};

struct PrecisionBackendCapabilities {
    bool cpuFloat64{true};
    bool cpuBoost512Reference{true};
    bool cpuBoost2048Direct{false};
    bool cpuBoost8192Direct{false};
    bool cpuBoost16384Direct{false};
    bool gpuOrbitFloat4{false};
    int gpuOrbitFloat4ValidatedBits{0};
};

// Capability report supplied by a legacy GPU backend. It contains no policy:
// threshold and fallback selection stay in this platform-neutral planner.
struct LegacyGpuPrecisionCapabilities {
    bool nativeFloat64{false};
    bool splitFloat{true};
    bool perturbation{false};
    bool arbitraryReference{false};
};

struct PrecisionPlan {
    std::string_view version{kPrecisionPlanVersion};
    PrecisionExecutionBackend backend{PrecisionExecutionBackend::Refused};
    int requiredBits{0};
    int selectedBits{0};
    PerturbationProfile formulaProfile{PerturbationProfile::Unsupported};
    bool requiresDirectCorrection{false};
    std::string reason;
};

// Deterministically selects an equal-or-higher precision path, or returns a
// classified refusal. It never silently reduces requested precision.
[[nodiscard]] PrecisionPlan BuildPrecisionPlan(const ExactCamera& camera,
                                                const EquationSettings& equation,
                                                PrecisionMode requestedMode,
                                                const PrecisionBackendCapabilities& capabilities);

// Compatibility planning for legacy CameraState GPU previews/stills. Exact
// deep work uses BuildPrecisionPlan above; this keeps pre-existing GPU modes
// behind one deterministic policy until an immutable exact GPU plan exists.
[[nodiscard]] bool ResolveLegacyGpuPrecision(const CameraState& camera,
                                              const EquationSettings& equation,
                                              const PrecisionSettings& settings,
                                              const LegacyGpuPrecisionCapabilities& capabilities,
                                              PrecisionMode& mode,
                                              std::string& error);

[[nodiscard]] const char* PrecisionExecutionBackendName(PrecisionExecutionBackend backend) noexcept;
[[nodiscard]] const char* PrecisionFormulaCapabilityName(PerturbationProfile profile) noexcept;

} // namespace mw
