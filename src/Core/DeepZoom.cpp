#include "Core/DeepZoom.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace mw {

const OrbitEncodingDescriptor& CurrentOrbitEncoding() noexcept {
    static constexpr OrbitEncodingDescriptor descriptor{
        "mw-orbit-float4-expansion/v1", 1, 4, 32, false, 0};
    return descriptor;
}
namespace {

struct ComplexDouble {
    double real{0.0};
    double imaginary{0.0};
};

ComplexDouble Add(const ComplexDouble& first, const ComplexDouble& second) {
    return {first.real + second.real, first.imaginary + second.imaginary};
}

ComplexDouble Multiply(const ComplexDouble& first, const ComplexDouble& second) {
    return {
        first.real * second.real - first.imaginary * second.imaginary,
        first.real * second.imaginary + first.imaginary * second.real,
    };
}

ComplexDouble Scale(const ComplexDouble& value, double factor) {
    return {value.real * factor, value.imaginary * factor};
}

ComplexDouble Conjugate(const ComplexDouble& value) {
    return {value.real, -value.imaginary};
}

double Magnitude(const ComplexDouble& value) {
    return std::hypot(value.real, value.imaginary);
}

ComplexDouble Coefficient(const ComplexCoefficient& coefficient) {
    return {coefficient.real, coefficient.imaginary};
}

class FixedReal {
public:
    FixedReal() = default;
    explicit FixedReal(int fractionalBits)
        : fractionalBits_(NormaliseBits(fractionalBits)), fractionalLimbs_(fractionalBits_ / 16),
          limbs_(static_cast<std::size_t>(fractionalLimbs_ + kIntegerLimbs), 0U) {}

    static FixedReal FromDouble(double value, int fractionalBits) {
        FixedReal result(fractionalBits);
        result.AddDouble(value);
        return result;
    }

    void AddDouble(double value) {
        if (!std::isfinite(value) || value == 0.0) return;
        FixedReal part = FromDoubleBits(value, fractionalBits_);
        *this = *this + part;
    }

    [[nodiscard]] bool IsNegative() const noexcept { return negative_ && !IsZero(); }
    [[nodiscard]] bool IsZero() const noexcept {
        return std::all_of(limbs_.begin(), limbs_.end(), [](std::uint16_t limb) { return limb == 0U; });
    }

    [[nodiscard]] FixedReal Abs() const {
        FixedReal result = *this;
        result.negative_ = false;
        return result;
    }

    [[nodiscard]] double ToDouble() const noexcept {
        long double value = 0.0L;
        for (std::size_t index = limbs_.size(); index-- > 0;) {
            value = std::ldexp(value, 16) + static_cast<long double>(limbs_[index]);
        }
        value = std::ldexp(value, -fractionalBits_);
        if (IsNegative()) value = -value;
        if (value > static_cast<long double>(std::numeric_limits<double>::max())) {
            return std::numeric_limits<double>::infinity();
        }
        if (value < -static_cast<long double>(std::numeric_limits<double>::max())) {
            return -std::numeric_limits<double>::infinity();
        }
        return static_cast<double>(value);
    }

    [[nodiscard]] std::array<float, 4> ToFloatExpansion() const {
        std::array<float, 4> parts{};
        FixedReal residual = *this;
        for (float& part : parts) {
            const double value = residual.ToDouble();
            if (!std::isfinite(value) || value == 0.0) break;
            part = static_cast<float>(value);
            residual = residual - FixedReal::FromDouble(static_cast<double>(part), fractionalBits_);
        }
        return parts;
    }

    friend FixedReal operator+(const FixedReal& first, const FixedReal& second) {
        EnsureCompatible(first, second);
        if (first.IsZero()) return second;
        if (second.IsZero()) return first;
        FixedReal result(first.fractionalBits_);
        if (first.IsNegative() == second.IsNegative()) {
            result.limbs_ = AddMagnitudes(first.limbs_, second.limbs_);
            result.negative_ = first.IsNegative();
            return result;
        }
        const int comparison = CompareMagnitudes(first.limbs_, second.limbs_);
        if (comparison == 0) return result;
        if (comparison > 0) {
            result.limbs_ = SubtractMagnitudes(first.limbs_, second.limbs_);
            result.negative_ = first.IsNegative();
        } else {
            result.limbs_ = SubtractMagnitudes(second.limbs_, first.limbs_);
            result.negative_ = second.IsNegative();
        }
        return result;
    }

    friend FixedReal operator-(const FixedReal& first, const FixedReal& second) {
        FixedReal negated = second;
        if (!negated.IsZero()) negated.negative_ = !negated.negative_;
        return first + negated;
    }

    friend FixedReal operator*(const FixedReal& first, const FixedReal& second) {
        EnsureCompatible(first, second);
        FixedReal result(first.fractionalBits_);
        if (first.IsZero() || second.IsZero()) return result;
        const std::size_t count = first.limbs_.size();
        std::vector<std::uint64_t> accumulated(count * 2U + 1U, 0U);
        for (std::size_t i = 0; i < count; ++i) {
            for (std::size_t j = 0; j < count; ++j) {
                accumulated[i + j] += static_cast<std::uint64_t>(first.limbs_[i]) * second.limbs_[j];
            }
        }
        for (std::size_t index = 0; index + 1U < accumulated.size(); ++index) {
            accumulated[index + 1U] += accumulated[index] >> 16U;
            accumulated[index] &= 0xFFFFU;
        }
        const std::size_t shift = static_cast<std::size_t>(first.fractionalLimbs_);
        for (std::size_t index = 0; index < count; ++index) {
            const std::size_t source = index + shift;
            if (source < accumulated.size()) result.limbs_[index] = static_cast<std::uint16_t>(accumulated[source]);
        }
        result.negative_ = first.IsNegative() != second.IsNegative();
        if (result.IsZero()) result.negative_ = false;
        return result;
    }

private:
    static constexpr int kIntegerLimbs = 8;

    static int NormaliseBits(int bits) noexcept {
        bits = std::clamp(bits, 128, 512);
        return ((bits + 15) / 16) * 16;
    }

    static void EnsureCompatible(const FixedReal& first, const FixedReal& second) {
        if (first.fractionalBits_ != second.fractionalBits_ || first.limbs_.size() != second.limbs_.size()) {
            throw std::invalid_argument("Fixed precision values are incompatible.");
        }
    }

    static FixedReal FromDoubleBits(double value, int fractionalBits) {
        FixedReal result(fractionalBits);
        const std::uint64_t raw = std::bit_cast<std::uint64_t>(value);
        const bool negative = (raw >> 63U) != 0U;
        const std::uint64_t exponentBits = (raw >> 52U) & 0x7FFU;
        std::uint64_t mantissa = raw & ((std::uint64_t{1} << 52U) - 1U);
        int exponent = 0;
        if (exponentBits == 0U) {
            if (mantissa == 0U) return result;
            exponent = -1022;
        } else {
            mantissa |= std::uint64_t{1} << 52U;
            exponent = static_cast<int>(exponentBits) - 1023;
        }
        const int bitShift = exponent - 52 + result.fractionalBits_;
        if (bitShift <= -64) return result;
        if (bitShift < 0) {
            mantissa >>= static_cast<unsigned>(-bitShift);
            for (unsigned bit = 0; bit < 64U; ++bit) {
                if ((mantissa & (std::uint64_t{1} << bit)) == 0U) continue;
                const std::size_t limbIndex = bit / 16U;
                if (limbIndex < result.limbs_.size()) {
                    result.limbs_[limbIndex] = static_cast<std::uint16_t>(result.limbs_[limbIndex] | (1U << (bit % 16U)));
                }
            }
        } else {
            for (unsigned bit = 0; bit < 53U; ++bit) {
                if ((mantissa & (std::uint64_t{1} << bit)) == 0U) continue;
                const std::size_t targetBit = static_cast<std::size_t>(bitShift) + bit;
                const std::size_t limbIndex = targetBit / 16U;
                if (limbIndex < result.limbs_.size()) {
                    result.limbs_[limbIndex] = static_cast<std::uint16_t>(result.limbs_[limbIndex] | (1U << (targetBit % 16U)));
                }
            }
        }
        result.negative_ = negative && !result.IsZero();
        return result;
    }

    static int CompareMagnitudes(const std::vector<std::uint16_t>& first,
                                 const std::vector<std::uint16_t>& second) noexcept {
        for (std::size_t index = first.size(); index-- > 0;) {
            if (first[index] > second[index]) return 1;
            if (first[index] < second[index]) return -1;
        }
        return 0;
    }

    static std::vector<std::uint16_t> AddMagnitudes(const std::vector<std::uint16_t>& first,
                                                     const std::vector<std::uint16_t>& second) {
        std::vector<std::uint16_t> result(first.size(), 0U);
        std::uint32_t carry = 0U;
        for (std::size_t index = 0; index < first.size(); ++index) {
            const std::uint32_t total = static_cast<std::uint32_t>(first[index]) + second[index] + carry;
            result[index] = static_cast<std::uint16_t>(total & 0xFFFFU);
            carry = total >> 16U;
        }
        return result;
    }

    static std::vector<std::uint16_t> SubtractMagnitudes(const std::vector<std::uint16_t>& larger,
                                                          const std::vector<std::uint16_t>& smaller) {
        std::vector<std::uint16_t> result(larger.size(), 0U);
        std::int32_t borrow = 0;
        for (std::size_t index = 0; index < larger.size(); ++index) {
            std::int32_t value = static_cast<std::int32_t>(larger[index]) - smaller[index] - borrow;
            if (value < 0) {
                value += 65536;
                borrow = 1;
            } else {
                borrow = 0;
            }
            result[index] = static_cast<std::uint16_t>(value);
        }
        return result;
    }

    int fractionalBits_{256};
    int fractionalLimbs_{16};
    std::vector<std::uint16_t> limbs_;
    bool negative_{false};
};

struct FixedComplex {
    FixedReal real;
    FixedReal imaginary;

    explicit FixedComplex(int bits) : real(bits), imaginary(bits) {}
    FixedComplex(FixedReal realValue, FixedReal imaginaryValue)
        : real(std::move(realValue)), imaginary(std::move(imaginaryValue)) {}
};

FixedComplex Add(const FixedComplex& first, const FixedComplex& second) {
    return {first.real + second.real, first.imaginary + second.imaginary};
}

FixedComplex Multiply(const FixedComplex& first, const FixedComplex& second) {
    return {
        first.real * second.real - first.imaginary * second.imaginary,
        first.real * second.imaginary + first.imaginary * second.real,
    };
}

FixedComplex Conjugate(const FixedComplex& value) {
    const FixedReal zero = value.imaginary - value.imaginary;
    return {value.real, zero - value.imaginary};
}

FixedComplex AbsComponents(const FixedComplex& value, const EquationSettings& equation) {
    return {
        equation.absoluteReal ? value.real.Abs() : value.real,
        equation.absoluteImaginary ? value.imaginary.Abs() : value.imaginary,
    };
}

FixedComplex FromCoefficient(const ComplexCoefficient& coefficient, int bits) {
    return {FixedReal::FromDouble(coefficient.real, bits), FixedReal::FromDouble(coefficient.imaginary, bits)};
}

ReferenceOrbit BuildDoubleOrbit(const CameraState& camera, const EquationSettings& equation,
                                int maximumIterations) {
    ReferenceOrbit orbit;
    maximumIterations = std::clamp(maximumIterations, 32, 4096);
    orbit.points.reserve(static_cast<std::size_t>(maximumIterations));
    const ComplexDouble c{CameraCentreX(camera), CameraCentreY(camera)};
    const ComplexDouble quadratic = Coefficient(equation.quadratic);
    const ComplexDouble linear = Coefficient(equation.linear);
    const ComplexDouble parameter = Coefficient(equation.parameter);
    const ComplexDouble constant = Coefficient(equation.constant);
    const double bailoutSquared = equation.bailoutRadius * equation.bailoutRadius;
    ComplexDouble z{};
    for (int iteration = 0; iteration < maximumIterations; ++iteration) {
        ReferenceOrbitPoint point;
        point.real[0] = static_cast<float>(z.real);
        point.real[1] = static_cast<float>(z.real - static_cast<double>(point.real[0]));
        point.imaginary[0] = static_cast<float>(z.imaginary);
        point.imaginary[1] = static_cast<float>(z.imaginary - static_cast<double>(point.imaginary[0]));
        orbit.points.push_back(point);
        const double magnitudeSquared = z.real * z.real + z.imaginary * z.imaginary;
        if (!std::isfinite(magnitudeSquared) || magnitudeSquared > bailoutSquared) {
            orbit.escaped = true;
            orbit.escapeIteration = iteration;
            break;
        }
        if (equation.absoluteReal) z.real = std::abs(z.real);
        if (equation.absoluteImaginary) z.imaginary = std::abs(z.imaginary);
        if (equation.conjugate) z = Conjugate(z);
        z = Add(Add(Add(Multiply(quadratic, Multiply(z, z)), Multiply(linear, z)),
                    Multiply(parameter, c)), constant);
    }
    if (orbit.points.empty()) orbit.points.push_back({});
    while (static_cast<int>(orbit.points.size()) < maximumIterations) orbit.points.push_back(orbit.points.back());
    orbit.precisionBits = 53;
    return orbit;
}

} // namespace

void AddCompensated(double& high, double& low, double delta) noexcept {
    const double sum = high + delta;
    const double virtualDelta = sum - high;
    const double residual = (high - (sum - virtualDelta)) + (delta - virtualDelta);
    high = sum;
    low += residual;
    const double normalised = high + low;
    const double normalisedResidual = low - (normalised - high);
    high = normalised;
    low = normalisedResidual;
}

void NormaliseCamera(CameraState& camera) noexcept {
    AddCompensated(camera.centreX, camera.centreXLow, 0.0);
    AddCompensated(camera.centreY, camera.centreYLow, 0.0);
}

void OffsetCamera(CameraState& camera, double deltaX, double deltaY) noexcept {
    AddCompensated(camera.centreX, camera.centreXLow, deltaX);
    AddCompensated(camera.centreY, camera.centreYLow, deltaY);
}

double CameraCentreX(const CameraState& camera) noexcept {
    return camera.centreX + camera.centreXLow;
}

double CameraCentreY(const CameraState& camera) noexcept {
    return camera.centreY + camera.centreYLow;
}

double CameraZoom(const CameraState& camera) noexcept {
    if (!(camera.scale > 0.0) || !std::isfinite(camera.scale)) return 1.0;
    return 1.5 / camera.scale;
}

ReferenceOrbit BuildReferenceOrbitDouble(const CameraState& camera,
                                         const EquationSettings& equation,
                                         int maximumIterations) {
    return BuildDoubleOrbit(camera, equation, maximumIterations);
}

ReferenceOrbit BuildReferenceOrbitArbitrary(const CameraState& camera,
                                            const EquationSettings& equation,
                                            int maximumIterations,
                                            int precisionBits) {
    maximumIterations = std::clamp(maximumIterations, 32, 4096);
    precisionBits = std::clamp(precisionBits, 128, 512);
    precisionBits = ((precisionBits + 15) / 16) * 16;

    ReferenceOrbit orbit;
    orbit.precisionBits = precisionBits;
    orbit.points.reserve(static_cast<std::size_t>(maximumIterations));

    FixedReal cReal = FixedReal::FromDouble(camera.centreX, precisionBits);
    cReal.AddDouble(camera.centreXLow);
    FixedReal cImaginary = FixedReal::FromDouble(camera.centreY, precisionBits);
    cImaginary.AddDouble(camera.centreYLow);
    const FixedComplex c{std::move(cReal), std::move(cImaginary)};
    const FixedComplex quadratic = FromCoefficient(equation.quadratic, precisionBits);
    const FixedComplex linear = FromCoefficient(equation.linear, precisionBits);
    const FixedComplex parameter = FromCoefficient(equation.parameter, precisionBits);
    const FixedComplex constant = FromCoefficient(equation.constant, precisionBits);
    const double bailoutSquared = equation.bailoutRadius * equation.bailoutRadius;
    FixedComplex z(precisionBits);

    for (int iteration = 0; iteration < maximumIterations; ++iteration) {
        const double real = z.real.ToDouble();
        const double imaginary = z.imaginary.ToDouble();
        ReferenceOrbitPoint point;
        point.real = z.real.ToFloatExpansion();
        point.imaginary = z.imaginary.ToFloatExpansion();
        orbit.points.push_back(point);
        const double magnitudeSquared = real * real + imaginary * imaginary;
        if (!std::isfinite(magnitudeSquared) || magnitudeSquared > bailoutSquared) {
            orbit.escaped = true;
            orbit.escapeIteration = iteration;
            break;
        }
        FixedComplex working = AbsComponents(z, equation);
        if (equation.conjugate) working = Conjugate(working);
        z = Add(Add(Add(Multiply(quadratic, Multiply(working, working)), Multiply(linear, working)),
                    Multiply(parameter, c)), constant);
    }
    if (orbit.points.empty()) orbit.points.push_back({});
    while (static_cast<int>(orbit.points.size()) < maximumIterations) orbit.points.push_back(orbit.points.back());
    return orbit;
}

PerturbationProfile ResolvePerturbationProfile(
    const EquationSettings& equation) noexcept {
    const auto near = [](const ComplexCoefficient& value, double real, double imaginary) {
        return std::abs(value.real - real) < 1.0e-12 &&
               std::abs(value.imaginary - imaginary) < 1.0e-12;
    };
    const auto nearZero = [&](const ComplexCoefficient& value) {
        return near(value, 0.0, 0.0);
    };
    const bool common =
        equation.renderMode == FractalRenderMode::EscapeTime &&
        !equation.newtonMode && equation.power == 2 &&
        equation.parameterPower == 1 && equation.reciprocalPower == 0 &&
        !equation.absoluteReal && !equation.absoluteImaginary &&
        !equation.swapRealImaginary &&
        equation.unaryTransform == EquationUnaryTransform::None &&
        !equation.juliaMode && equation.initialZMode == InitialZMode::Zero &&
        nearZero(equation.iterationTerm) &&
        nearZero(equation.reciprocalCoefficient) &&
        !equation.animateCoefficients;
    if (!common) return PerturbationProfile::Unsupported;
    if (!equation.conjugate) return PerturbationProfile::AnalyticQuadratic;
    if (near(equation.quadratic, 1.0, 0.0) &&
        near(equation.linear, 0.0, 0.0) &&
        near(equation.parameter, 1.0, 0.0) &&
        near(equation.constant, 0.0, 0.0)) {
        return PerturbationProfile::TricornQuadratic;
    }
    return PerturbationProfile::Unsupported;
}

PerturbationFormulaCapability DescribePerturbationProfile(
    PerturbationProfile profile) noexcept {
    switch (profile) {
    case PerturbationProfile::AnalyticQuadratic:
        return {profile, "analytic-quadratic-mandelbrot", 1, true};
    case PerturbationProfile::TricornQuadratic:
        return {profile, "tricorn-power2", 1, true};
    case PerturbationProfile::Unsupported:
        return {profile, "unsupported", 0, false};
    }
    return {PerturbationProfile::Unsupported, "unsupported", 0, false};
}

bool EquationSupportsPerturbation(const EquationSettings& equation) noexcept {
    return ResolvePerturbationProfile(equation) != PerturbationProfile::Unsupported;
}

namespace {

ComplexDouble ExpansionValue(const ReferenceOrbitPoint& point) {
    ComplexDouble value;
    for (float component : point.real) value.real += static_cast<double>(component);
    for (float component : point.imaginary) value.imaginary += static_cast<double>(component);
    return value;
}

PerturbationSampleResult ResultFromReferenceOrbit(const ReferenceOrbit& orbit,
                                                   int maximumIterations,
                                                   double bailoutSquared) {
    PerturbationSampleResult result;
    result.referenceRefreshed = true;
    result.stable = true;
    result.validity = PerturbationSampleValidity::Rebased;
    result.escaped = orbit.escaped;
    result.iterations = orbit.escaped ? orbit.escapeIteration : maximumIterations;
    const int index = std::clamp(result.iterations, 0,
                                 static_cast<int>(orbit.points.size()) - 1);
    const ComplexDouble finalValue = ExpansionValue(orbit.points[static_cast<std::size_t>(index)]);
    result.finalReal = finalValue.real;
    result.finalImaginary = finalValue.imaginary;
    const double magnitudeSquared = finalValue.real * finalValue.real +
                                    finalValue.imaginary * finalValue.imaginary;
    if (!result.escaped && magnitudeSquared > bailoutSquared) result.escaped = true;
    return result;
}

} // namespace

PerturbationSampleResult EvaluatePerturbationSample(
    const CameraState& referenceCamera,
    const EquationSettings& equation,
    int maximumIterations,
    double normalisedOffsetX,
    double normalisedOffsetY,
    int arbitraryPrecisionBits,
    bool allowReferenceRefresh) {
    maximumIterations = std::clamp(maximumIterations, 32, 4096);
    const PerturbationProfile profile = ResolvePerturbationProfile(equation);
    if (profile == PerturbationProfile::Unsupported) {
        throw std::invalid_argument("The selected equation does not support perturbation.");
    }
    const bool arbitrary = arbitraryPrecisionBits > 0;
    const ReferenceOrbit orbit = arbitrary
        ? BuildReferenceOrbitArbitrary(referenceCamera, equation, maximumIterations,
                                       arbitraryPrecisionBits)
        : BuildReferenceOrbitDouble(referenceCamera, equation, maximumIterations);
    const ComplexDouble quadratic = Coefficient(equation.quadratic);
    const ComplexDouble linear = Coefficient(equation.linear);
    const ComplexDouble parameter = Coefficient(equation.parameter);
    const ComplexDouble local{normalisedOffsetX, normalisedOffsetY};
    const double scale = referenceCamera.scale;
    const double bailoutSquared = equation.bailoutRadius * equation.bailoutRadius;
    ComplexDouble q{};
    PerturbationSampleResult result;

    for (int iteration = 0; iteration < maximumIterations; ++iteration) {
        const ComplexDouble reference = ExpansionValue(
            orbit.points[static_cast<std::size_t>(iteration)]);
        const ComplexDouble delta = Scale(q, scale);
        const ComplexDouble approximate = Add(reference, delta);
        result.finalReal = approximate.real;
        result.finalImaginary = approximate.imaginary;
        const double relativeDelta = Magnitude(delta) /
            std::max(1.0, Magnitude(reference));
        result.maximumRelativeDelta = std::max(result.maximumRelativeDelta, relativeDelta);
        if (!std::isfinite(relativeDelta) || relativeDelta > 0.5 ||
            !std::isfinite(q.real) || !std::isfinite(q.imaginary)) {
            result.stable = false;
            break;
        }
        const double magnitudeSquared = approximate.real * approximate.real +
                                        approximate.imaginary * approximate.imaginary;
        if (!std::isfinite(magnitudeSquared) || magnitudeSquared > bailoutSquared) {
            result.iterations = iteration;
            result.escaped = true;
            return result;
        }

        ComplexDouble workingReference = reference;
        ComplexDouble workingQ = q;
        if (profile == PerturbationProfile::TricornQuadratic) {
            workingReference = Conjugate(workingReference);
            workingQ = Conjugate(workingQ);
        }
        const ComplexDouble quadraticDelta = Add(
            Scale(Multiply(workingReference, workingQ), 2.0),
            Scale(Multiply(workingQ, workingQ), scale));
        q = Add(Add(Multiply(quadratic, quadraticDelta),
                    Multiply(linear, workingQ)),
                Multiply(parameter, local));
        result.iterations = iteration + 1;
    }

    if (result.stable) return result;
    if (!allowReferenceRefresh) {
        result.validity = PerturbationSampleValidity::Unresolved;
        return result;
    }

    CameraState refreshedCamera = referenceCamera;
    OffsetCamera(refreshedCamera, scale * normalisedOffsetX,
                  scale * normalisedOffsetY);
    const ReferenceOrbit refreshed = arbitrary
        ? BuildReferenceOrbitArbitrary(refreshedCamera, equation, maximumIterations,
                                       arbitraryPrecisionBits)
        : BuildReferenceOrbitDouble(refreshedCamera, equation, maximumIterations);
    PerturbationSampleResult refreshedResult =
        ResultFromReferenceOrbit(refreshed, maximumIterations, bailoutSquared);
    refreshedResult.maximumRelativeDelta = result.maximumRelativeDelta;
    return refreshedResult;
}

std::string PrecisionModeDisplayName(PrecisionMode mode) {
    switch (mode) {
    case PrecisionMode::Automatic: return "Automatic";
    case PrecisionMode::Float32: return "GPU float32";
    case PrecisionMode::Float64: return "GPU float64";
    case PrecisionMode::SplitFloat: return "Split high/low float";
    case PrecisionMode::Perturbation: return "GPU perturbation / double reference";
    case PrecisionMode::ArbitraryPrecisionPerturbation: return "GPU perturbation / arbitrary-precision reference";
    }
    return "Automatic";
}

} // namespace mw
