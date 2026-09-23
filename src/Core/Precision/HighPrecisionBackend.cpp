#include "Core/Precision/HighPrecisionBackend.h"

#include "Core/StillImageRenderer.h"

#include <boost/multiprecision/cpp_bin_float.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>

namespace mw {
namespace {

using HighReal = boost::multiprecision::number<
    boost::multiprecision::backends::cpp_bin_float<
        512,
        boost::multiprecision::backends::digit_base_2,
        void>,
    boost::multiprecision::et_off>;

using HighReal2048 = boost::multiprecision::number<
    boost::multiprecision::backends::cpp_bin_float<
        2048,
        boost::multiprecision::backends::digit_base_2,
        void>,
    boost::multiprecision::et_off>;

using HighReal8192 = boost::multiprecision::number<
    boost::multiprecision::backends::cpp_bin_float<
        8192,
        boost::multiprecision::backends::digit_base_2,
        void>,
    boost::multiprecision::et_off>;

using HighReal16384 = boost::multiprecision::number<
    boost::multiprecision::backends::cpp_bin_float<
        16384,
        boost::multiprecision::backends::digit_base_2,
        void>,
    boost::multiprecision::et_off>;


struct HighComplex {
    HighReal real{};
    HighReal imaginary{};
};

struct HighComplex2048 {
    HighReal2048 real{};
    HighReal2048 imaginary{};
};

struct HighComplex8192 {
    HighReal8192 real{};
    HighReal8192 imaginary{};
};

struct HighComplex16384 {
    HighReal16384 real{};
    HighReal16384 imaginary{};
};


HighComplex Add(const HighComplex& first, const HighComplex& second) {
    return {first.real + second.real, first.imaginary + second.imaginary};
}

HighComplex2048 Add(const HighComplex2048& first, const HighComplex2048& second) {
    return {first.real + second.real, first.imaginary + second.imaginary};
}

HighComplex8192 Add(const HighComplex8192& first, const HighComplex8192& second) {
    return {first.real + second.real, first.imaginary + second.imaginary};
}

HighComplex16384 Add(const HighComplex16384& first, const HighComplex16384& second) {
    return {first.real + second.real, first.imaginary + second.imaginary};
}

HighComplex Multiply(const HighComplex& first, const HighComplex& second) {
    return {
        first.real * second.real - first.imaginary * second.imaginary,
        first.real * second.imaginary + first.imaginary * second.real,
    };
}

HighComplex2048 Multiply(const HighComplex2048& first, const HighComplex2048& second) {
    return {
        first.real * second.real - first.imaginary * second.imaginary,
        first.real * second.imaginary + first.imaginary * second.real,
    };
}

HighComplex8192 Multiply(const HighComplex8192& first, const HighComplex8192& second) {
    return {
        first.real * second.real - first.imaginary * second.imaginary,
        first.real * second.imaginary + first.imaginary * second.real,
    };
}

HighComplex16384 Multiply(const HighComplex16384& first, const HighComplex16384& second) {
    return {
        first.real * second.real - first.imaginary * second.imaginary,
        first.real * second.imaginary + first.imaginary * second.real,
    };
}

HighComplex Conjugate(const HighComplex& value) {
    return {value.real, -value.imaginary};
}

HighComplex2048 Conjugate(const HighComplex2048& value) {
    return {value.real, -value.imaginary};
}

HighComplex8192 Conjugate(const HighComplex8192& value) {
    return {value.real, -value.imaginary};
}

HighComplex16384 Conjugate(const HighComplex16384& value) {
    return {value.real, -value.imaginary};
}

HighComplex AbsComponents(const HighComplex& value, const EquationSettings& equation) {
    HighComplex result = value;
    if (equation.absoluteReal && result.real < 0) result.real = -result.real;
    if (equation.absoluteImaginary && result.imaginary < 0) result.imaginary = -result.imaginary;
    return result;
}

HighComplex2048 AbsComponents(const HighComplex2048& value, const EquationSettings& equation) {
    HighComplex2048 result = value;
    if (equation.absoluteReal && result.real < 0) result.real = -result.real;
    if (equation.absoluteImaginary && result.imaginary < 0) result.imaginary = -result.imaginary;
    return result;
}

HighComplex8192 AbsComponents(const HighComplex8192& value, const EquationSettings& equation) {
    HighComplex8192 result = value;
    if (equation.absoluteReal && result.real < 0) result.real = -result.real;
    if (equation.absoluteImaginary && result.imaginary < 0) result.imaginary = -result.imaginary;
    return result;
}

HighComplex16384 AbsComponents(const HighComplex16384& value, const EquationSettings& equation) {
    HighComplex16384 result = value;
    if (equation.absoluteReal && result.real < 0) result.real = -result.real;
    if (equation.absoluteImaginary && result.imaginary < 0) result.imaginary = -result.imaginary;
    return result;
}

HighComplex FromCoefficient(const ComplexCoefficient& coefficient) {
    return {HighReal(coefficient.real), HighReal(coefficient.imaginary)};
}

HighComplex2048 FromCoefficient2048(const ComplexCoefficient& coefficient) {
    return {HighReal2048(coefficient.real), HighReal2048(coefficient.imaginary)};
}

HighComplex8192 FromCoefficient8192(const ComplexCoefficient& coefficient) {
    return {HighReal8192(coefficient.real), HighReal8192(coefficient.imaginary)};
}

HighComplex16384 FromCoefficient16384(const ComplexCoefficient& coefficient) {
    return {HighReal16384(coefficient.real), HighReal16384(coefficient.imaginary)};
}


HighReal FromCompensated(double high, double low) {
    HighReal result(high);
    result += HighReal(low);
    return result;
}

std::size_t SignificantDigits(const ExactDecimal& value) noexcept {
    std::string_view text = value.CanonicalText();
    if (!text.empty() && text.front() == '-') text.remove_prefix(1U);
    const std::size_t exponentMarker = text.find('e');
    const std::size_t exponent = exponentMarker == std::string_view::npos
        ? text.size() : exponentMarker;
    std::size_t count = 0U;
    for (std::size_t index = 0U; index < exponent; ++index) {
        if (text[index] != '.') ++count;
    }
    return count;
}

HighReal FromExactDecimal(const ExactDecimal& value) {
    // floor(512 * log10(2)) is 154. Any longer significand would lose source
    // digits before the independent backend can produce an orbit.
    if (SignificantDigits(value) > 154U) {
        throw std::invalid_argument(
            "The exact camera exceeds the fixed 512-bit backend precision envelope.");
    }
    try {
        return HighReal(value.CanonicalText());
    } catch (const std::exception&) {
        throw std::invalid_argument(
            "The exact camera cannot be represented by the fixed 512-bit backend.");
    }
}

HighReal2048 FromExactDecimal2048(const ExactDecimal& value) {
    if (SignificantDigits(value) > 616U) {
        throw std::invalid_argument(
            "The exact camera exceeds the fixed 2048-bit backend precision envelope.");
    }
    try {
        return HighReal2048(value.CanonicalText());
    } catch (const std::exception&) {
        throw std::invalid_argument(
            "The exact camera cannot be represented by the fixed 2048-bit backend.");
    }
}

HighReal8192 FromExactDecimal8192(const ExactDecimal& value) {
    if (SignificantDigits(value) > 2464U) {
        throw std::invalid_argument(
            "The exact camera exceeds the fixed 8192-bit backend precision envelope.");
    }
    try {
        return HighReal8192(value.CanonicalText());
    } catch (const std::exception&) {
        throw std::invalid_argument(
            "The exact camera cannot be represented by the fixed 8192-bit backend.");
    }
}

HighReal16384 FromExactDecimal16384(const ExactDecimal& value) {
    if (SignificantDigits(value) > 4928U) {
        throw std::invalid_argument(
            "The exact camera exceeds the fixed 16384-bit backend precision envelope.");
    }
    try {
        return HighReal16384(value.CanonicalText());
    } catch (const std::exception&) {
        throw std::invalid_argument(
            "The exact camera cannot be represented by the fixed 16384-bit backend.");
    }
}


HighReal RationalOffset(const ExactRationalOffset& offset) {
    if (offset.denominator == 0U) {
        throw std::invalid_argument("Exact still sample has a zero rational denominator.");
    }
    return HighReal(offset.numerator) / HighReal(offset.denominator);
}

HighReal2048 RationalOffset2048(const ExactRationalOffset& offset) {
    if (offset.denominator == 0U) {
        throw std::invalid_argument("Exact still sample has a zero rational denominator.");
    }
    return HighReal2048(offset.numerator) / HighReal2048(offset.denominator);
}

HighReal8192 RationalOffset8192(const ExactRationalOffset& offset) {
    if (offset.denominator == 0U) {
        throw std::invalid_argument("Exact still sample has a zero rational denominator.");
    }
    return HighReal8192(offset.numerator) / HighReal8192(offset.denominator);
}

HighReal16384 RationalOffset16384(const ExactRationalOffset& offset) {
    if (offset.denominator == 0U) {
        throw std::invalid_argument("Exact still sample has a zero rational denominator.");
    }
    return HighReal16384(offset.numerator) / HighReal16384(offset.denominator);
}


bool IsFinite(const ComplexCoefficient& coefficient) noexcept {
    return std::isfinite(coefficient.real) && std::isfinite(coefficient.imaginary);
}

void ValidateInput(const CameraState& camera, const EquationSettings& equation) {
    if (ResolvePerturbationProfile(equation) == PerturbationProfile::Unsupported) {
        throw std::invalid_argument(
            "The independent high-precision backend supports only registered perturbation profiles.");
    }
    if (!std::isfinite(camera.centreX) || !std::isfinite(camera.centreXLow) ||
        !std::isfinite(camera.centreY) || !std::isfinite(camera.centreYLow) ||
        !IsFinite(equation.quadratic) || !IsFinite(equation.linear) ||
        !IsFinite(equation.parameter) || !IsFinite(equation.constant) ||
        !(equation.bailoutRadius > 0.0) || !std::isfinite(equation.bailoutRadius)) {
        throw std::invalid_argument(
            "The independent high-precision backend requires finite camera and equation input.");
    }
}

void ValidateInput(const ExactCamera& camera, const EquationSettings& equation) {
    CameraState ignored{};
    ValidateInput(ignored, equation);
    if (camera.halfHeight.IsZero() || camera.halfHeight.CanonicalText().front() == '-') {
        throw std::invalid_argument(
            "The independent high-precision backend requires a positive exact half-height.");
    }
    (void)FromExactDecimal(camera.centreX);
    (void)FromExactDecimal(camera.centreY);
}

template <typename Real>
std::array<float, 4> ToFloatExpansion(const Real& value) {
    std::array<float, 4> parts{};
    Real residual = value;
    for (float& part : parts) {
        const double converted = residual.convert_to<double>();
        if (!std::isfinite(converted) || converted == 0.0) break;
        part = static_cast<float>(converted);
        if (!std::isfinite(part)) {
            part = std::copysign((std::numeric_limits<float>::max)(), part);
        }
        residual -= Real(part);
    }
    return parts;
}

float ReconstructOrbitCoordinateForShader(const std::array<float, 4>& parts) noexcept {
    // Keep the source order used by both current perturbation shaders. Volatile
    // prevents this diagnostic calculation from being folded into a wider host sum.
    volatile float value = parts[0];
    value = static_cast<float>(value + parts[1]);
    value = static_cast<float>(value + parts[2]);
    value = static_cast<float>(value + parts[3]);
    return value;
}

void AccumulateOrbitEncodingError(const HighReal& source, const std::array<float, 4>& parts,
                                  OrbitEncodingMeasurement& measurement) {
    const HighReal reconstructed(ReconstructOrbitCoordinateForShader(parts));
    HighReal absoluteError = source - reconstructed;
    if (absoluteError < 0) absoluteError = -absoluteError;
    measurement.maximumAbsoluteError = std::max(
        measurement.maximumAbsoluteError, absoluteError.convert_to<double>());
    if (source != 0) {
        HighReal absoluteSource = source;
        if (absoluteSource < 0) absoluteSource = -absoluteSource;
        measurement.maximumRelativeError = std::max(
            measurement.maximumRelativeError,
            (absoluteError / absoluteSource).convert_to<double>());
    }
    ++measurement.comparedCoordinates;
}

template <typename Real, typename Complex, typename DecimalConverter, typename CoefficientConverter>
ReferenceOrbit BuildExactReferenceOrbit(const ExactCamera& camera,
                                        const EquationSettings& equation,
                                        int maximumIterations,
                                        const HighPrecisionCancellationCallback& cancellationCallback,
                                        int precisionBits,
                                        DecimalConverter&& decimal,
                                        CoefficientConverter&& coefficient) {
    const Real halfHeight = decimal(camera.halfHeight);
    if (!(halfHeight > 0)) {
        throw std::invalid_argument(
            "The independent high-precision backend requires a positive exact half-height.");
    }
    ReferenceOrbit orbit;
    orbit.precisionBits = precisionBits;
    orbit.points.reserve(static_cast<std::size_t>(maximumIterations));

    const Complex c{decimal(camera.centreX), decimal(camera.centreY)};
    const Complex quadratic = coefficient(equation.quadratic);
    const Complex linear = coefficient(equation.linear);
    const Complex parameter = coefficient(equation.parameter);
    const Complex constant = coefficient(equation.constant);
    const Real bailoutSquared = Real(equation.bailoutRadius) * Real(equation.bailoutRadius);
    Complex z{};

    for (int iteration = 0; iteration < maximumIterations; ++iteration) {
        if (cancellationCallback && cancellationCallback()) {
            throw std::runtime_error("The independent high-precision reference build was cancelled.");
        }
        orbit.points.push_back({ToFloatExpansion(z.real), ToFloatExpansion(z.imaginary)});
        const Real magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
        if (magnitudeSquared > bailoutSquared) {
            orbit.escaped = true;
            orbit.escapeIteration = iteration;
            break;
        }
        Complex working = AbsComponents(z, equation);
        if (equation.conjugate) working = Conjugate(working);
        z = Add(Add(Add(Multiply(quadratic, Multiply(working, working)),
                        Multiply(linear, working)),
                    Multiply(parameter, c)),
                constant);
    }
    if (orbit.points.empty()) orbit.points.push_back({});
    while (static_cast<int>(orbit.points.size()) < maximumIterations) {
        orbit.points.push_back(orbit.points.back());
    }
    return orbit;
}

} // namespace

const HighPrecisionBackendDescriptor& IndependentHighPrecisionBackend() noexcept {
    static constexpr HighPrecisionBackendDescriptor descriptor{
        "Boost.Multiprecision standalone source subset",
        "1.83.0",
        "BSL-1.0",
        "cpp_bin_float<512, digit_base_2, void> with expression templates disabled",
        512,
        true,
        false,
        true,
    };
    return descriptor;
}

EscapeResult ToEscapeResult(const HighPrecisionEscapeResult& result) noexcept {
    return {result.iterations, result.escaped, result.smoothValue};
}

ReferenceOrbit BuildIndependentHighPrecisionReferenceOrbit(
    const CameraState& camera,
    const EquationSettings& equation,
    int maximumIterations) {
    ValidateInput(camera, equation);
    maximumIterations = std::clamp(maximumIterations, 32, 4096);

    ReferenceOrbit orbit;
    orbit.precisionBits = IndependentHighPrecisionBackend().precisionBits;
    orbit.points.reserve(static_cast<std::size_t>(maximumIterations));

    const HighComplex c{
        FromCompensated(camera.centreX, camera.centreXLow),
        FromCompensated(camera.centreY, camera.centreYLow),
    };
    const HighComplex quadratic = FromCoefficient(equation.quadratic);
    const HighComplex linear = FromCoefficient(equation.linear);
    const HighComplex parameter = FromCoefficient(equation.parameter);
    const HighComplex constant = FromCoefficient(equation.constant);
    const HighReal bailoutSquared =
        HighReal(equation.bailoutRadius) * HighReal(equation.bailoutRadius);
    HighComplex z{};

    for (int iteration = 0; iteration < maximumIterations; ++iteration) {
        orbit.points.push_back({ToFloatExpansion(z.real), ToFloatExpansion(z.imaginary)});

        const HighReal magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
        if (magnitudeSquared > bailoutSquared) {
            orbit.escaped = true;
            orbit.escapeIteration = iteration;
            break;
        }

        HighComplex working = AbsComponents(z, equation);
        if (equation.conjugate) working = Conjugate(working);
        z = Add(Add(Add(Multiply(quadratic, Multiply(working, working)),
                        Multiply(linear, working)),
                    Multiply(parameter, c)),
                constant);
    }

    if (orbit.points.empty()) orbit.points.push_back({});
    while (static_cast<int>(orbit.points.size()) < maximumIterations) {
        orbit.points.push_back(orbit.points.back());
    }
    return orbit;
}

OrbitEncodingMeasurement MeasureIndependentOrbitEncoding(
    const CameraState& camera, const EquationSettings& equation, int maximumIterations) {
    ValidateInput(camera, equation);
    maximumIterations = std::clamp(maximumIterations, 32, 4096);

    OrbitEncodingMeasurement measurement;
    measurement.sourcePrecisionBits = IndependentHighPrecisionBackend().precisionBits;
    measurement.requestedIterations = maximumIterations;
    const HighComplex c{
        FromCompensated(camera.centreX, camera.centreXLow),
        FromCompensated(camera.centreY, camera.centreYLow),
    };
    const HighComplex quadratic = FromCoefficient(equation.quadratic);
    const HighComplex linear = FromCoefficient(equation.linear);
    const HighComplex parameter = FromCoefficient(equation.parameter);
    const HighComplex constant = FromCoefficient(equation.constant);
    const HighReal bailoutSquared =
        HighReal(equation.bailoutRadius) * HighReal(equation.bailoutRadius);
    HighComplex z{};
    for (int iteration = 0; iteration < maximumIterations; ++iteration) {
        AccumulateOrbitEncodingError(z.real, ToFloatExpansion(z.real), measurement);
        AccumulateOrbitEncodingError(z.imaginary, ToFloatExpansion(z.imaginary), measurement);
        const HighReal magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
        if (magnitudeSquared > bailoutSquared) {
            measurement.escaped = true;
            measurement.escapeIteration = iteration;
            break;
        }
        HighComplex working = AbsComponents(z, equation);
        if (equation.conjugate) working = Conjugate(working);
        z = Add(Add(Add(Multiply(quadratic, Multiply(working, working)),
                        Multiply(linear, working)),
                    Multiply(parameter, c)),
                constant);
    }
    return measurement;
}


ReferenceOrbit BuildIndependentHighPrecisionReferenceOrbit(
    const ExactCamera& camera,
    const EquationSettings& equation,
    int maximumIterations,
    const HighPrecisionCancellationCallback& cancellationCallback,
    int precisionBits) {
    ValidateInput(camera, equation);
    maximumIterations = std::clamp(maximumIterations, 32, 4096);
    switch (precisionBits) {
    case 512:
        return BuildExactReferenceOrbit<HighReal, HighComplex>(
            camera, equation, maximumIterations, cancellationCallback, 512,
            [](const ExactDecimal& value) { return FromExactDecimal(value); },
            [](const ComplexCoefficient& value) { return FromCoefficient(value); });
    case 2048:
        return BuildExactReferenceOrbit<HighReal2048, HighComplex2048>(
            camera, equation, maximumIterations, cancellationCallback, 2048,
            [](const ExactDecimal& value) { return FromExactDecimal2048(value); },
            [](const ComplexCoefficient& value) { return FromCoefficient2048(value); });
    case 8192:
        return BuildExactReferenceOrbit<HighReal8192, HighComplex8192>(
            camera, equation, maximumIterations, cancellationCallback, 8192,
            [](const ExactDecimal& value) { return FromExactDecimal8192(value); },
            [](const ComplexCoefficient& value) { return FromCoefficient8192(value); });
    case 16384:
        return BuildExactReferenceOrbit<HighReal16384, HighComplex16384>(
            camera, equation, maximumIterations, cancellationCallback, 16384,
            [](const ExactDecimal& value) { return FromExactDecimal16384(value); },
            [](const ComplexCoefficient& value) { return FromCoefficient16384(value); });
    default:
        throw std::invalid_argument(
            "The independent high-precision reference build has no selected execution tier for this request.");
    }
}

bool EvaluateIndependentHighPrecisionSample2048(
    const ExactStillRenderSample& sample,
    const EquationSettings& equation,
    int maximumIterations,
    const HighPrecisionCancellationCallback& cancellationCallback,
    HighPrecisionEscapeResult& result,
    std::string& error) {
    try {
        if (ResolvePerturbationProfile(equation) == PerturbationProfile::Unsupported ||
            !IsFinite(equation.quadratic) || !IsFinite(equation.linear) ||
            !IsFinite(equation.parameter) || !IsFinite(equation.constant) ||
            !(equation.bailoutRadius > 0.0) || !std::isfinite(equation.bailoutRadius)) {
            throw std::invalid_argument(
                "The direct high-precision evaluator requires a registered finite formula profile.");
        }
        const HighReal2048 halfHeight = FromExactDecimal2048(sample.camera.halfHeight);
        if (!(halfHeight > 0)) {
            throw std::invalid_argument(
                "The direct high-precision evaluator requires a positive exact half-height.");
        }
        const HighComplex2048 c{
            FromExactDecimal2048(sample.camera.centreX) +
                halfHeight * RationalOffset2048(sample.horizontalHalfHeightFactor),
            FromExactDecimal2048(sample.camera.centreY) +
                halfHeight * RationalOffset2048(sample.verticalHalfHeightFactor),
        };
        const HighComplex2048 quadratic = FromCoefficient2048(equation.quadratic);
        const HighComplex2048 linear = FromCoefficient2048(equation.linear);
        const HighComplex2048 parameter = FromCoefficient2048(equation.parameter);
        const HighComplex2048 constant = FromCoefficient2048(equation.constant);
        const HighReal2048 bailoutSquared =
            HighReal2048(equation.bailoutRadius) * HighReal2048(equation.bailoutRadius);
        HighComplex2048 z{};
        result.precisionBits = 2048;
        for (int iteration = 0; iteration < maximumIterations; ++iteration) {
            if (cancellationCallback && cancellationCallback()) {
                error = "The direct high-precision sample evaluation was cancelled.";
                result = {};
                return false;
            }
            const HighReal2048 magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
            if (magnitudeSquared > bailoutSquared) {
                result.iterations = iteration;
                result.escaped = true;
                const double magnitudeSquaredDouble = magnitudeSquared.convert_to<double>();
                result.smoothValue = static_cast<double>(iteration);
                if (magnitudeSquaredDouble > 1.0 && std::isfinite(magnitudeSquaredDouble)) {
                    const double logMagnitude = 0.5 * std::log(magnitudeSquaredDouble);
                    const double logPower = std::log(static_cast<double>(std::max(2, equation.power)));
                    if (logMagnitude > 0.0 && logPower > 0.0) {
                        const double correction = std::log(logMagnitude) / logPower;
                        if (std::isfinite(correction)) {
                            result.smoothValue = static_cast<double>(iteration) + 1.0 - correction;
                        }
                    }
                }
                return true;
            }
            HighComplex2048 working = AbsComponents(z, equation);
            if (equation.conjugate) working = Conjugate(working);
            z = Add(Add(Add(Multiply(quadratic, Multiply(working, working)),
                            Multiply(linear, working)),
                        Multiply(parameter, c)),
                    constant);
        }
        result.iterations = maximumIterations;
        result.smoothValue = static_cast<double>(maximumIterations);
        return true;
    } catch (const std::invalid_argument& exception) {
        error = exception.what();
        result = {};
        return false;
    }
}

bool EvaluateIndependentHighPrecisionSample8192(
    const ExactStillRenderSample& sample,
    const EquationSettings& equation,
    int maximumIterations,
    const HighPrecisionCancellationCallback& cancellationCallback,
    HighPrecisionEscapeResult& result,
    std::string& error) {
    try {
        if (ResolvePerturbationProfile(equation) == PerturbationProfile::Unsupported ||
            !IsFinite(equation.quadratic) || !IsFinite(equation.linear) ||
            !IsFinite(equation.parameter) || !IsFinite(equation.constant) ||
            !(equation.bailoutRadius > 0.0) || !std::isfinite(equation.bailoutRadius)) {
            throw std::invalid_argument(
                "The direct high-precision evaluator requires a registered finite formula profile.");
        }
        const HighReal8192 halfHeight = FromExactDecimal8192(sample.camera.halfHeight);
        if (!(halfHeight > 0)) {
            throw std::invalid_argument(
                "The direct high-precision evaluator requires a positive exact half-height.");
        }
        const HighComplex8192 c{
            FromExactDecimal8192(sample.camera.centreX) +
                halfHeight * RationalOffset8192(sample.horizontalHalfHeightFactor),
            FromExactDecimal8192(sample.camera.centreY) +
                halfHeight * RationalOffset8192(sample.verticalHalfHeightFactor),
        };
        const HighComplex8192 quadratic = FromCoefficient8192(equation.quadratic);
        const HighComplex8192 linear = FromCoefficient8192(equation.linear);
        const HighComplex8192 parameter = FromCoefficient8192(equation.parameter);
        const HighComplex8192 constant = FromCoefficient8192(equation.constant);
        const HighReal8192 bailoutSquared =
            HighReal8192(equation.bailoutRadius) * HighReal8192(equation.bailoutRadius);
        HighComplex8192 z{};
        result.precisionBits = 8192;
        for (int iteration = 0; iteration < maximumIterations; ++iteration) {
            if (cancellationCallback && cancellationCallback()) {
                error = "The direct high-precision sample evaluation was cancelled.";
                result = {};
                return false;
            }
            const HighReal8192 magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
            if (magnitudeSquared > bailoutSquared) {
                result.iterations = iteration;
                result.escaped = true;
                const double magnitudeSquaredDouble = magnitudeSquared.convert_to<double>();
                result.smoothValue = static_cast<double>(iteration);
                if (magnitudeSquaredDouble > 1.0 && std::isfinite(magnitudeSquaredDouble)) {
                    const double logMagnitude = 0.5 * std::log(magnitudeSquaredDouble);
                    const double logPower = std::log(static_cast<double>(std::max(2, equation.power)));
                    if (logMagnitude > 0.0 && logPower > 0.0) {
                        const double correction = std::log(logMagnitude) / logPower;
                        if (std::isfinite(correction)) {
                            result.smoothValue = static_cast<double>(iteration) + 1.0 - correction;
                        }
                    }
                }
                return true;
            }
            HighComplex8192 working = AbsComponents(z, equation);
            if (equation.conjugate) working = Conjugate(working);
            z = Add(Add(Add(Multiply(quadratic, Multiply(working, working)),
                            Multiply(linear, working)),
                        Multiply(parameter, c)),
                    constant);
        }
        result.iterations = maximumIterations;
        result.smoothValue = static_cast<double>(maximumIterations);
        return true;
    } catch (const std::invalid_argument& exception) {
        error = exception.what();
        result = {};
        return false;
    }
}

bool EvaluateIndependentHighPrecisionSample16384(
    const ExactStillRenderSample& sample,
    const EquationSettings& equation,
    int maximumIterations,
    const HighPrecisionCancellationCallback& cancellationCallback,
    HighPrecisionEscapeResult& result,
    std::string& error) {
    try {
        if (ResolvePerturbationProfile(equation) == PerturbationProfile::Unsupported ||
            !IsFinite(equation.quadratic) || !IsFinite(equation.linear) ||
            !IsFinite(equation.parameter) || !IsFinite(equation.constant) ||
            !(equation.bailoutRadius > 0.0) || !std::isfinite(equation.bailoutRadius)) {
            throw std::invalid_argument(
                "The direct high-precision evaluator requires a registered finite formula profile.");
        }
        const HighReal16384 halfHeight = FromExactDecimal16384(sample.camera.halfHeight);
        if (!(halfHeight > 0)) {
            throw std::invalid_argument(
                "The direct high-precision evaluator requires a positive exact half-height.");
        }
        const HighComplex16384 c{
            FromExactDecimal16384(sample.camera.centreX) +
                halfHeight * RationalOffset16384(sample.horizontalHalfHeightFactor),
            FromExactDecimal16384(sample.camera.centreY) +
                halfHeight * RationalOffset16384(sample.verticalHalfHeightFactor),
        };
        const HighComplex16384 quadratic = FromCoefficient16384(equation.quadratic);
        const HighComplex16384 linear = FromCoefficient16384(equation.linear);
        const HighComplex16384 parameter = FromCoefficient16384(equation.parameter);
        const HighComplex16384 constant = FromCoefficient16384(equation.constant);
        const HighReal16384 bailoutSquared =
            HighReal16384(equation.bailoutRadius) * HighReal16384(equation.bailoutRadius);
        HighComplex16384 z{};
        result.precisionBits = 16384;
        for (int iteration = 0; iteration < maximumIterations; ++iteration) {
            if (cancellationCallback && cancellationCallback()) {
                error = "The direct high-precision sample evaluation was cancelled.";
                result = {};
                return false;
            }
            const HighReal16384 magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
            if (magnitudeSquared > bailoutSquared) {
                result.iterations = iteration;
                result.escaped = true;
                const double magnitudeSquaredDouble = magnitudeSquared.convert_to<double>();
                result.smoothValue = static_cast<double>(iteration);
                if (magnitudeSquaredDouble > 1.0 && std::isfinite(magnitudeSquaredDouble)) {
                    const double logMagnitude = 0.5 * std::log(magnitudeSquaredDouble);
                    const double logPower = std::log(static_cast<double>(std::max(2, equation.power)));
                    if (logMagnitude > 1.0 && std::isfinite(logMagnitude) && logPower > 0.0) {
                        const double correction = std::log(logMagnitude) / logPower;
                        if (std::isfinite(correction)) {
                            result.smoothValue = static_cast<double>(iteration) + 1.0 - correction;
                        }
                    }
                }
                return true;
            }
            HighComplex16384 working{
                equation.absoluteReal ? abs(z.real) : z.real,
                equation.absoluteImaginary ? abs(z.imaginary) : z.imaginary};
            if (equation.conjugate) working.imaginary = -working.imaginary;
            const HighComplex16384 squared{
                working.real * working.real - working.imaginary * working.imaginary,
                HighReal16384(2) * working.real * working.imaginary};
            const HighComplex16384 quadraticTerm{
                quadratic.real * squared.real - quadratic.imaginary * squared.imaginary,
                quadratic.real * squared.imaginary + quadratic.imaginary * squared.real};
            const HighComplex16384 linearTerm{
                linear.real * working.real - linear.imaginary * working.imaginary,
                linear.real * working.imaginary + linear.imaginary * working.real};
            const HighComplex16384 parameterTerm{
                parameter.real * c.real - parameter.imaginary * c.imaginary,
                parameter.real * c.imaginary + parameter.imaginary * c.real};
            z = {quadraticTerm.real + linearTerm.real + parameterTerm.real + constant.real,
                 quadraticTerm.imaginary + linearTerm.imaginary + parameterTerm.imaginary + constant.imaginary};
        }
        result.iterations = maximumIterations;
        result.smoothValue = static_cast<double>(maximumIterations);
        return true;
    } catch (const std::invalid_argument& exception) {
        error = exception.what();
        result = {};
        return false;
    }
}

bool EvaluateIndependentHighPrecisionSample(
    const ExactStillRenderSample& sample,
    const EquationSettings& equation,
    int maximumIterations,
    const HighPrecisionCancellationCallback& cancellationCallback,
    HighPrecisionEscapeResult& result,
    std::string& error,
    int precisionBits) {
    result = {};
    error.clear();
    if (sample.requiresRotationAdapter || sample.rotationDegrees != 0.0) {
        error = "The direct high-precision sample evaluator does not support rotated coordinates.";
        return false;
    }
    if (equation.animateCoefficients) {
        error = "The direct high-precision sample evaluator does not support animated equation coefficients.";
        return false;
    }
    if (maximumIterations < 32 || maximumIterations > 4096) {
        error = "Direct high-precision sample iterations must be between 32 and 4096.";
        return false;
    }
    if (precisionBits == 2048) {
        return EvaluateIndependentHighPrecisionSample2048(
            sample, equation, maximumIterations, cancellationCallback, result, error);
    }
    if (precisionBits == 8192) {
        return EvaluateIndependentHighPrecisionSample8192(
            sample, equation, maximumIterations, cancellationCallback, result, error);
    }
    if (precisionBits == 16384) {
        return EvaluateIndependentHighPrecisionSample16384(
            sample, equation, maximumIterations, cancellationCallback, result, error);
    }
    if (precisionBits != 512) {
        error = "The direct high-precision sample evaluator has no selected execution tier for this request.";
        return false;
    }
    try {
        ValidateInput(sample.camera, equation);
        const HighReal halfHeight = FromExactDecimal(sample.camera.halfHeight);
        const HighComplex c{
            FromExactDecimal(sample.camera.centreX) +
                halfHeight * RationalOffset(sample.horizontalHalfHeightFactor),
            FromExactDecimal(sample.camera.centreY) +
                halfHeight * RationalOffset(sample.verticalHalfHeightFactor),
        };
        const HighComplex quadratic = FromCoefficient(equation.quadratic);
        const HighComplex linear = FromCoefficient(equation.linear);
        const HighComplex parameter = FromCoefficient(equation.parameter);
        const HighComplex constant = FromCoefficient(equation.constant);
        const HighReal bailoutSquared =
            HighReal(equation.bailoutRadius) * HighReal(equation.bailoutRadius);
        HighComplex z{};
        result.precisionBits = IndependentHighPrecisionBackend().precisionBits;
        for (int iteration = 0; iteration < maximumIterations; ++iteration) {
            if (cancellationCallback && cancellationCallback()) {
                error = "The direct high-precision sample evaluation was cancelled.";
                result = {};
                return false;
            }
            const HighReal magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
            if (magnitudeSquared > bailoutSquared) {
                result.iterations = iteration;
                result.escaped = true;
                const double magnitudeSquaredDouble = magnitudeSquared.convert_to<double>();
                result.smoothValue = static_cast<double>(iteration);
                if (magnitudeSquaredDouble > 1.0 && std::isfinite(magnitudeSquaredDouble)) {
                    const double logMagnitude = 0.5 * std::log(magnitudeSquaredDouble);
                    const double logPower = std::log(
                        static_cast<double>(std::max(2, equation.power)));
                    if (logMagnitude > 0.0 && logPower > 0.0) {
                        const double correction = std::log(logMagnitude) / logPower;
                        if (std::isfinite(correction)) {
                            result.smoothValue = static_cast<double>(iteration) + 1.0 - correction;
                        }
                    }
                }
                return true;
            }
            HighComplex working = AbsComponents(z, equation);
            if (equation.conjugate) working = Conjugate(working);
            z = Add(Add(Add(Multiply(quadratic, Multiply(working, working)),
                            Multiply(linear, working)),
                        Multiply(parameter, c)),
                    constant);
        }
        result.iterations = maximumIterations;
        result.smoothValue = static_cast<double>(maximumIterations);
        return true;
    } catch (const std::invalid_argument& exception) {
        error = exception.what();
        result = {};
        return false;
    }
}

} // namespace mw
