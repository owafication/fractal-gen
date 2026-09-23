#include "Core/Precision/PrecisionPlanner.h"

#include <algorithm>
#include <charconv>
#include <cmath>

namespace mw {
namespace {
int Digits(const ExactDecimal& value) {
    const std::string& text = value.CanonicalText();
    const std::size_t exponentMarker = text.find('e');
    const std::size_t end = exponentMarker == std::string::npos ? text.size() : exponentMarker;
    int result = 0;
    for (std::size_t i = 0; i < end; ++i) if (text[i] >= '0' && text[i] <= '9') ++result;
    return result;
}

int Exponent(const ExactDecimal& value) noexcept {
    const std::string& text = value.CanonicalText();
    const std::size_t marker = text.find('e');
    int exponent = 0;
    if (marker != std::string::npos) {
        (void)std::from_chars(text.data() + marker + 1U, text.data() + text.size(), exponent);
    }
    return exponent;
}

bool GpuOrbitRequiresDirectCorrection(const PrecisionBackendCapabilities& capabilities,
                                      int requiredBits) noexcept {
    return capabilities.gpuOrbitFloat4 &&
        (capabilities.gpuOrbitFloat4ValidatedBits <= 0 ||
         capabilities.gpuOrbitFloat4ValidatedBits < requiredBits);
}

const char* GpuOrbitCorrectionReason(const PrecisionBackendCapabilities& capabilities) noexcept {
    return capabilities.gpuOrbitFloat4ValidatedBits <= 0
        ? "GPU orbit encoding has no validated precision envelope; direct correction is required."
        : "GPU orbit encoding is below the required precision; direct correction is required.";
}
}

const char* PrecisionExecutionBackendName(PrecisionExecutionBackend backend) noexcept {
    switch (backend) {
    case PrecisionExecutionBackend::CpuFloat64: return "cpu-float64";
    case PrecisionExecutionBackend::CpuBoost512Reference: return "cpu-boost-512-reference";
    case PrecisionExecutionBackend::CpuBoost2048Direct: return "cpu-boost-2048-direct";
    case PrecisionExecutionBackend::CpuBoost8192Direct: return "cpu-boost-8192-direct";
    case PrecisionExecutionBackend::CpuBoost16384Direct: return "cpu-boost-16384-direct";
    default: return "refused";
    }
}

const char* PrecisionFormulaCapabilityName(PerturbationProfile profile) noexcept {
    switch (profile) {
    case PerturbationProfile::AnalyticQuadratic:
        return "analytic-quadratic-mandelbrot/v1";
    case PerturbationProfile::TricornQuadratic:
        return "tricorn-power2/v1";
    default:
        return "unsupported";
    }
}

PrecisionPlan BuildPrecisionPlan(const ExactCamera& camera, const EquationSettings& equation,
                                 PrecisionMode requestedMode,
                                 const PrecisionBackendCapabilities& capabilities) {
    PrecisionPlan plan;
    const int decimalDigits = std::max({Digits(camera.centreX), Digits(camera.centreY),
                                        Digits(camera.halfHeight)});
    // Camera half-height determines the local pixel spacing. Its negative
    // exponent therefore contributes directly to required absolute precision.
    const int depthDigits = std::max(0, -Exponent(camera.halfHeight));
    plan.requiredBits = std::max(53, static_cast<int>(std::ceil(
        (decimalDigits + depthDigits) * 3.32192809489)) + 16);
    plan.formulaProfile = ResolvePerturbationProfile(equation);
    if (requestedMode == PrecisionMode::SplitFloat) {
        plan.reason = "Split-float was explicitly requested but has no validated planner-owned execution backend.";
        return plan;
    }
    if (plan.formulaProfile == PerturbationProfile::Unsupported && plan.requiredBits > 53) {
        plan.reason = "The selected formula has no validated deep-precision profile.";
        return plan;
    }
    if (requestedMode == PrecisionMode::Float64 && plan.requiredBits > 53) {
        plan.reason = "Float64 was explicitly requested but cannot preserve the exact camera requirement.";
        return plan;
    }
    const bool forceReference = requestedMode == PrecisionMode::ArbitraryPrecisionPerturbation ||
                                requestedMode == PrecisionMode::Perturbation;
    if (!forceReference && plan.requiredBits <= 53 && capabilities.cpuFloat64) {
        plan.backend = PrecisionExecutionBackend::CpuFloat64;
        plan.selectedBits = 53;
        plan.reason = "CPU Float64 satisfies the exact-camera requirement.";
        return plan;
    }
    if (plan.requiredBits <= 512 && capabilities.cpuBoost512Reference) {
        plan.backend = PrecisionExecutionBackend::CpuBoost512Reference;
        plan.selectedBits = 512;
        plan.requiresDirectCorrection = GpuOrbitRequiresDirectCorrection(capabilities,
                                                                          plan.requiredBits);
        plan.reason = plan.requiresDirectCorrection
            ? "Boost 512-bit reference selected; " +
                std::string(GpuOrbitCorrectionReason(capabilities))
            : "Boost 512-bit reference satisfies the exact-camera requirement.";
        return plan;
    }
    if (plan.requiredBits <= 2048 && capabilities.cpuBoost2048Direct) {
        plan.backend = PrecisionExecutionBackend::CpuBoost2048Direct;
        plan.selectedBits = 2048;
        plan.requiresDirectCorrection = GpuOrbitRequiresDirectCorrection(capabilities,
                                                                          plan.requiredBits);
        plan.reason = plan.requiresDirectCorrection
            ? "Boost 2048-bit direct CPU selected; " +
                std::string(GpuOrbitCorrectionReason(capabilities))
            : "Boost 2048-bit direct CPU satisfies the exact-camera requirement.";
        return plan;
    }
    if (plan.requiredBits <= 8192 && capabilities.cpuBoost8192Direct) {
        plan.backend = PrecisionExecutionBackend::CpuBoost8192Direct;
        plan.selectedBits = 8192;
        plan.requiresDirectCorrection = GpuOrbitRequiresDirectCorrection(capabilities,
                                                                          plan.requiredBits);
        plan.reason = plan.requiresDirectCorrection
            ? "Boost 8192-bit direct CPU selected; " +
                std::string(GpuOrbitCorrectionReason(capabilities))
            : "Boost 8192-bit direct CPU satisfies the exact-camera requirement.";
        return plan;
    }
    if (plan.requiredBits <= 16384 && capabilities.cpuBoost16384Direct) {
        plan.backend = PrecisionExecutionBackend::CpuBoost16384Direct;
        plan.selectedBits = 16384;
        plan.requiresDirectCorrection = GpuOrbitRequiresDirectCorrection(capabilities,
                                                                          plan.requiredBits);
        plan.reason = plan.requiresDirectCorrection
            ? "Boost 16384-bit direct CPU selected; " +
                std::string(GpuOrbitCorrectionReason(capabilities))
            : "Boost 16384-bit direct CPU satisfies the exact-camera requirement.";
        return plan;
    }
    plan.reason = "No configured backend can satisfy the exact-camera precision requirement without rounding.";
    return plan;
}

bool ResolveLegacyGpuPrecision(const CameraState& camera, const EquationSettings& equation,
                               const PrecisionSettings& settings,
                               const LegacyGpuPrecisionCapabilities& capabilities,
                               PrecisionMode& mode, std::string& error) {
    error.clear();
    const bool deepEquation = EquationSupportsPerturbation(equation);
    const auto available = [&](PrecisionMode candidate) {
        switch (candidate) {
        case PrecisionMode::Float32: return true;
        case PrecisionMode::Float64: return capabilities.nativeFloat64;
        case PrecisionMode::SplitFloat: return capabilities.splitFloat && deepEquation;
        case PrecisionMode::Perturbation: return capabilities.perturbation && deepEquation;
        case PrecisionMode::ArbitraryPrecisionPerturbation:
            return capabilities.arbitraryReference && deepEquation;
        case PrecisionMode::Automatic: return true;
        }
        return false;
    };
    const auto fallback = [&]() {
        const double zoom = CameraZoom(camera);
        if (!deepEquation) {
            return settings.allowFloat64 && available(PrecisionMode::Float64)
                ? PrecisionMode::Float64 : PrecisionMode::Float32;
        }
        if (zoom < 1.0e6) return PrecisionMode::Float32;
        if (settings.allowFloat64 && available(PrecisionMode::Float64) && zoom < 1.0e14) {
            return PrecisionMode::Float64;
        }
        if (settings.allowSplitFloat && available(PrecisionMode::SplitFloat) && zoom < 1.0e13) {
            return PrecisionMode::SplitFloat;
        }
        if (settings.allowArbitraryPrecision &&
            available(PrecisionMode::ArbitraryPrecisionPerturbation)) {
            return PrecisionMode::ArbitraryPrecisionPerturbation;
        }
        if (settings.allowPerturbation && available(PrecisionMode::Perturbation)) {
            return PrecisionMode::Perturbation;
        }
        if (settings.allowFloat64 && available(PrecisionMode::Float64)) {
            return PrecisionMode::Float64;
        }
        if (settings.allowSplitFloat && available(PrecisionMode::SplitFloat)) {
            return PrecisionMode::SplitFloat;
        }
        return PrecisionMode::Float32;
    };
    if (settings.mode == PrecisionMode::Automatic) {
        mode = fallback();
        return true;
    }
    if (available(settings.mode)) {
        mode = settings.mode;
        return true;
    }
    if (settings.automaticFallback) {
        mode = fallback();
        return true;
    }
    mode = settings.mode;
    error = "The selected precision strategy is unavailable in the reported GPU capabilities or incompatible with the selected equation operations.";
    return false;
}
} // namespace mw
