#include "Core/Precision/ExactCameraAdapter.h"

#include <charconv>
#include <cmath>

namespace mw {
namespace {

bool ToFiniteDouble(const ExactDecimal& value, double& result, bool& loss,
                    std::string& error) {
    const std::string& text = value.CanonicalText();
    const auto converted = std::from_chars(text.data(), text.data() + text.size(), result,
                                           std::chars_format::general);
    if (converted.ec != std::errc{} || converted.ptr != text.data() + text.size() ||
        !std::isfinite(result)) {
        error = "Exact camera value cannot be represented by the legacy finite-double adapter.";
        return false;
    }
    ExactDecimal roundTrip;
    if (!ExactDecimal::FromFiniteDouble(result, roundTrip, error)) return false;
    loss = roundTrip != value;
    return true;
}

bool IsPositiveDoubleUnderflow(const ExactDecimal& value) noexcept {
    if (value.IsZero()) return false;
    const std::string& text = value.CanonicalText();
    if (text.empty() || text.front() == '-') return false;
    const std::size_t exponentMarker = text.find('e');
    if (exponentMarker == std::string::npos) return false;
    int exponent = 0;
    const auto parsed = std::from_chars(text.data() + exponentMarker + 1U,
                                        text.data() + text.size(), exponent);
    // Canonical text has one non-zero leading digit. Values at e-324 and
    // below may not survive from_chars as a positive finite double.
    return parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size() &&
           exponent <= -324;
}

bool ToPositiveLegacyScale(const ExactDecimal& value, double& result, bool& loss,
                           std::string& error) {
    if (ToFiniteDouble(value, result, loss, error)) return result > 0.0;
    if (!IsPositiveDoubleUnderflow(value)) return false;
    result = std::numeric_limits<double>::denorm_min();
    loss = true;
    error.clear();
    return true;
}

} // namespace

bool BuildExactCameraFromLegacy(const CameraState& legacy, ExactCamera& result,
                                std::string& error) {
    result = {};
    error.clear();
    ExactDecimal centreXHigh;
    ExactDecimal centreXLow;
    ExactDecimal centreYHigh;
    ExactDecimal centreYLow;
    if (!ExactDecimal::FromFiniteDouble(legacy.centreX, centreXHigh, error) ||
        !ExactDecimal::FromFiniteDouble(legacy.centreXLow, centreXLow, error) ||
        !ExactDecimal::FromFiniteDouble(legacy.centreY, centreYHigh, error) ||
        !ExactDecimal::FromFiniteDouble(legacy.centreYLow, centreYLow, error) ||
        !ExactDecimal::FromFiniteDouble(legacy.scale, result.halfHeight, error) ||
        !ExactDecimal::Add(centreXHigh, centreXLow, result.centreX, error) ||
        !ExactDecimal::Add(centreYHigh, centreYLow, result.centreY, error)) {
        return false;
    }
    if (legacy.scale <= 0.0) {
        error = "Legacy camera scale must be positive for exact-camera migration.";
        return false;
    }
    return true;
}

bool AdaptExactCameraToLegacy(const ExactCamera& exact, LegacyCameraAdaptation& result,
                              std::string& error) {
    result = {};
    error.clear();
    if (!ToFiniteDouble(exact.centreX, result.camera.centreX, result.centreXLoss, error) ||
        !ToFiniteDouble(exact.centreY, result.camera.centreY, result.centreYLoss, error) ||
        !ToPositiveLegacyScale(exact.halfHeight, result.camera.scale, result.halfHeightLoss, error)) {
        return false;
    }
    if (result.camera.scale <= 0.0) {
        error = "Exact camera half-height must adapt to a positive finite double.";
        return false;
    }
    return true;
}

} // namespace mw
