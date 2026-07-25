#include "Core/MandelbrotMath.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>

namespace mw {
namespace {

using Complex = std::complex<double>;
constexpr double kTwoPi = 6.283185307179586476925286766559;

Complex ToComplex(const ComplexCoefficient& value) {
    return {value.real, value.imaginary};
}

Complex PowInteger(Complex value, int power) {
    if (power == 0) return {1.0, 0.0};
    const bool negative = power < 0;
    unsigned exponent = static_cast<unsigned>(negative ? -power : power);
    Complex result{1.0, 0.0};
    while (exponent > 0U) {
        if ((exponent & 1U) != 0U) result *= value;
        exponent >>= 1U;
        if (exponent > 0U) value *= value;
    }
    if (!negative) return result;
    const double denominator = std::norm(result);
    if (denominator < 1.0e-300) return {std::numeric_limits<double>::infinity(), 0.0};
    return Complex{1.0, 0.0} / result;
}

Complex Animate(const ComplexCoefficient& base, const EquationSettings& equation,
                double timeSeconds, double phase) {
    Complex value = ToComplex(base);
    if (!equation.animateCoefficients || equation.coefficientAnimationAmplitude <= 0.0) return value;
    const double wave = std::sin(timeSeconds * equation.coefficientAnimationSpeed * kTwoPi + phase) *
                        equation.coefficientAnimationAmplitude;
    return value + Complex{wave, wave * 0.35};
}

Complex ApplyTransform(Complex z, const EquationSettings& equation) {
    if (equation.absoluteReal) z.real(std::abs(z.real()));
    if (equation.absoluteImaginary) z.imag(std::abs(z.imag()));
    if (equation.swapRealImaginary) z = {z.imag(), z.real()};
    if (equation.conjugate) z = std::conj(z);
    switch (equation.unaryTransform) {
    case EquationUnaryTransform::None: break;
    case EquationUnaryTransform::Sin: z = std::sin(z); break;
    case EquationUnaryTransform::Cos: z = std::cos(z); break;
    case EquationUnaryTransform::Exp: z = std::exp(z); break;
    case EquationUnaryTransform::Log:
        if (std::abs(z) < 1.0e-300) z = {1.0e-300, 0.0};
        z = std::log(z);
        break;
    }
    return z;
}

Complex PrincipalRoot(Complex value, int degree) {
    degree = std::max(1, degree);
    if (std::abs(value) < 1.0e-300) return {};
    return std::polar(std::pow(std::abs(value), 1.0 / static_cast<double>(degree)),
                      std::arg(value) / static_cast<double>(degree));
}

Complex CriticalPoint(const EquationSettings& equation, double timeSeconds) {
    const Complex a = Animate(equation.quadratic, equation, timeSeconds, 0.0);
    const Complex lambda = Animate(equation.reciprocalCoefficient, equation, timeSeconds, 2.2);
    if (equation.reciprocalPower > 0 && std::abs(lambda) > 1.0e-14 &&
        equation.power > 0 && std::abs(a) > 1.0e-14) {
        // For A z^p + lambda/z^q, f'(z)=0 gives z^(p+q)=q*lambda/(p*A).
        const Complex rhs = static_cast<double>(equation.reciprocalPower) * lambda /
                            (static_cast<double>(equation.power) * a);
        return PrincipalRoot(rhs, equation.power + equation.reciprocalPower);
    }
    // z^p+c parameter sets (p>=2) have the critical point z=0.
    return {};
}

double TrapDistance(Complex z, const EquationSettings& equation) {
    const Complex relative = z - ToComplex(equation.orbitTrapPoint);
    switch (equation.orbitTrap) {
    case OrbitTrapType::Point: return std::abs(relative);
    case OrbitTrapType::Cross: return std::min(std::abs(relative.real()), std::abs(relative.imag()));
    case OrbitTrapType::Circle: return std::abs(std::abs(relative) - equation.orbitTrapRadius);
    }
    return std::abs(relative);
}

EscapeResult CalculateNewton(double real, double imaginary, int maximumIterations,
                             const EquationSettings& equation) {
    Complex z{real, imaginary};
    const Complex target = ToComplex(equation.newtonTarget);
    const Complex relaxation = ToComplex(equation.newtonRelaxation);
    const int degree = std::clamp(equation.newtonDegree, 2, 12);
    double trap = std::numeric_limits<double>::infinity();
    for (int iteration = 0; iteration < maximumIterations; ++iteration) {
        trap = std::min(trap, TrapDistance(z, equation));
        const Complex power = PowInteger(z, degree);
        const Complex residual = power - target;
        if (std::abs(residual) <= equation.convergenceTolerance) {
            const double angle = std::arg(z);
            int root = static_cast<int>(std::llround((angle < 0.0 ? angle + kTwoPi : angle) /
                                                     kTwoPi * degree)) % degree;
            if (root < 0) root += degree;
            return {iteration, true, static_cast<double>(iteration), true, root, trap, 0.0};
        }
        const Complex derivative = static_cast<double>(degree) * PowInteger(z, degree - 1);
        if (std::abs(derivative) < 1.0e-300) break;
        z -= relaxation * residual / derivative;
        if (!std::isfinite(z.real()) || !std::isfinite(z.imag())) break;
    }
    return {maximumIterations, false, static_cast<double>(maximumIterations), false, -1, trap, 0.0};
}

bool SupportsDistanceDerivative(const EquationSettings& equation) {
    return equation.unaryTransform == EquationUnaryTransform::None && !equation.absoluteReal &&
           !equation.absoluteImaginary && !equation.conjugate && !equation.swapRealImaginary &&
           equation.reciprocalPower == 0;
}

} // namespace

EscapeResult CalculateEscape(double real, double imaginary, int maximumIterations) {
    return CalculateEscape(real, imaginary, maximumIterations, EquationSettings{}, 0.0);
}

EscapeResult CalculateEscape(double real, double imaginary, int maximumIterations,
                             const EquationSettings& equation, double timeSeconds) {
    maximumIterations = std::clamp(maximumIterations, 1, 100000);
    if (equation.newtonMode || equation.renderMode == FractalRenderMode::Newton) {
        return CalculateNewton(real, imaginary, maximumIterations, equation);
    }

    const Complex pixel{real, imaginary};
    const Complex c = equation.juliaMode ? ToComplex(equation.juliaParameter) : pixel;
    Complex z{};
    if (equation.juliaMode) {
        z = pixel;
    } else {
        switch (equation.initialZMode) {
        case InitialZMode::Zero: z = {}; break;
        case InitialZMode::Fixed: z = ToComplex(equation.initialZ); break;
        case InitialZMode::Parameter: z = c; break;
        case InitialZMode::CriticalPoint: z = CriticalPoint(equation, timeSeconds); break;
        }
    }

    const Complex a = Animate(equation.quadratic, equation, timeSeconds, 0.0);
    const Complex b = Animate(equation.linear, equation, timeSeconds, 1.1);
    const Complex parameter = Animate(equation.parameter, equation, timeSeconds, 2.2);
    const Complex d = Animate(equation.constant, equation, timeSeconds, 3.3);
    const Complex iterationTerm = Animate(equation.iterationTerm, equation, timeSeconds, 4.4);
    const Complex reciprocal = Animate(equation.reciprocalCoefficient, equation, timeSeconds, 5.5);
    const double bailoutSquared = equation.bailoutRadius * equation.bailoutRadius;
    double trap = std::numeric_limits<double>::infinity();
    Complex derivative = equation.juliaMode ? Complex{1.0, 0.0} : Complex{};
    const bool trackDerivative = SupportsDistanceDerivative(equation);

    int iteration = 0;
    double magnitudeSquared = std::norm(z);
    for (; iteration < maximumIterations; ++iteration) {
        magnitudeSquared = std::norm(z);
        if (magnitudeSquared > bailoutSquared) break;
        trap = std::min(trap, TrapDistance(z, equation));
        const Complex w = ApplyTransform(z, equation);
        const Complex powered = PowInteger(w, equation.power);
        const Complex poweredParameter = PowInteger(c, equation.parameterPower);
        Complex next = a * powered + b * w + parameter * poweredParameter + d +
                       iterationTerm * static_cast<double>(iteration);
        if (equation.reciprocalPower > 0 && std::abs(reciprocal) > 1.0e-14) {
            const Complex denominator = PowInteger(w, equation.reciprocalPower);
            if (std::abs(denominator) < 1.0e-300) {
                return {iteration + 1, true, static_cast<double>(iteration + 1), false, -1, trap, 0.0};
            }
            next += reciprocal / denominator;
        }
        if (trackDerivative) {
            const Complex localDerivative = a * static_cast<double>(equation.power) *
                                            PowInteger(w, equation.power - 1) + b;
            Complex parameterDerivative{};
            if (!equation.juliaMode) {
                parameterDerivative = parameter * static_cast<double>(equation.parameterPower) *
                                      PowInteger(c, equation.parameterPower - 1);
            }
            derivative = localDerivative * derivative + parameterDerivative;
        }
        z = next;
        if (!std::isfinite(z.real()) || !std::isfinite(z.imag())) {
            return {iteration + 1, true, static_cast<double>(iteration + 1), false, -1, trap, 0.0};
        }
    }
    if (iteration >= maximumIterations) {
        return {maximumIterations, false, static_cast<double>(maximumIterations), false, -1, trap, 0.0};
    }

    magnitudeSquared = std::norm(z);
    double smooth = static_cast<double>(iteration);
    if (magnitudeSquared > 1.0 && std::isfinite(magnitudeSquared)) {
        const double logMagnitude = 0.5 * std::log(magnitudeSquared);
        const double logPower = std::log(static_cast<double>(std::max(2, equation.power)));
        if (logMagnitude > 0.0 && logPower > 0.0) {
            const double correction = std::log(logMagnitude) / logPower;
            if (std::isfinite(correction)) smooth = iteration + 1.0 - correction;
        }
    }
    double distance = 0.0;
    const double derivativeMagnitude = std::abs(derivative);
    const double magnitude = std::sqrt(std::max(0.0, magnitudeSquared));
    if (trackDerivative && derivativeMagnitude > 1.0e-300 && magnitude > 1.0) {
        distance = 0.5 * std::log(magnitude) * magnitude / derivativeMagnitude;
        if (!std::isfinite(distance) || distance < 0.0) distance = 0.0;
    }
    return {iteration, true, std::isfinite(smooth) ? smooth : static_cast<double>(iteration),
            false, -1, trap, distance};
}

bool IsInterestingMandelbrotTarget(double real, double imaginary, int maximumIterations) {
    return IsInterestingFractalTarget(real, imaginary, maximumIterations, EquationSettings{});
}

bool IsInterestingFractalTarget(double real, double imaginary, int maximumIterations,
                                const EquationSettings& equation) {
    maximumIterations = std::clamp(maximumIterations, 32, 4096);
    const auto escape = CalculateEscape(real, imaginary, maximumIterations, equation);
    if (equation.newtonMode || equation.renderMode == FractalRenderMode::Newton) {
        return escape.converged && escape.iterations >= 2 && escape.iterations < maximumIterations;
    }
    if (!escape.escaped) return false;
    const int minimumDetail = std::max(8, maximumIterations / 80);
    return escape.iterations >= minimumDetail && escape.iterations < maximumIterations;
}

bool IsBoundaryRichFractalTarget(double real, double imaginary, double viewScale,
                                 int maximumIterations, const EquationSettings& equation) {
    maximumIterations = std::clamp(maximumIterations, 32, 4096);
    const int samplingIterations = std::min(maximumIterations, 512);
    viewScale = std::clamp(std::abs(viewScale), 1.0e-12, 4.0);
    constexpr int sampleRadius = 2;
    int escapedCount = 0;
    int interiorCount = 0;
    int detailedCount = 0;
    int minimumIterations = maximumIterations;
    int maximumSeenIterations = 0;
    int rootMask = 0;
    for (int y = -sampleRadius; y <= sampleRadius; ++y) {
        for (int x = -sampleRadius; x <= sampleRadius; ++x) {
            const double nx = static_cast<double>(x) / static_cast<double>(sampleRadius);
            const double ny = static_cast<double>(y) / static_cast<double>(sampleRadius);
            const auto sample = CalculateEscape(real + nx * viewScale * 0.85,
                                                imaginary + ny * viewScale * 0.85,
                                                samplingIterations, equation);
            if (equation.newtonMode || equation.renderMode == FractalRenderMode::Newton) {
                if (sample.converged) {
                    ++escapedCount;
                    if (sample.rootIndex >= 0 && sample.rootIndex < 30) rootMask |= 1 << sample.rootIndex;
                    minimumIterations = std::min(minimumIterations, sample.iterations);
                    maximumSeenIterations = std::max(maximumSeenIterations, sample.iterations);
                } else {
                    ++interiorCount;
                }
                continue;
            }
            if (sample.escaped) {
                ++escapedCount;
                minimumIterations = std::min(minimumIterations, sample.iterations);
                maximumSeenIterations = std::max(maximumSeenIterations, sample.iterations);
                if (sample.iterations >= std::max(8, samplingIterations / 80)) ++detailedCount;
            } else {
                ++interiorCount;
                maximumSeenIterations = samplingIterations;
            }
        }
    }
    if (equation.newtonMode || equation.renderMode == FractalRenderMode::Newton) {
        const bool multipleRoots = rootMask != 0 && (rootMask & (rootMask - 1)) != 0;
        return escapedCount >= 8 && multipleRoots && maximumSeenIterations - minimumIterations >= 3;
    }
    return escapedCount >= 6 && interiorCount >= 1 &&
           maximumSeenIterations - minimumIterations >= std::max(10, samplingIterations / 50) &&
           detailedCount >= 3;
}

std::pair<double, double> FindInterestingMandelbrotTarget(
    double requestedReal, double requestedImaginary, double searchScale, int maximumIterations) {
    return FindInterestingFractalTarget(requestedReal, requestedImaginary, searchScale,
                                        maximumIterations, EquationSettings{});
}

std::pair<double, double> FindInterestingFractalTarget(
    double requestedReal, double requestedImaginary, double searchScale, int maximumIterations,
    const EquationSettings& equation) {
    maximumIterations = std::clamp(maximumIterations, 32, 4096);
    searchScale = std::clamp(std::abs(searchScale), 1.0e-8, 4.0);
    if (IsInterestingFractalTarget(requestedReal, requestedImaginary, maximumIterations, equation) &&
        IsBoundaryRichFractalTarget(requestedReal, requestedImaginary, searchScale,
                                    maximumIterations, equation)) {
        return {requestedReal, requestedImaginary};
    }
    double bestReal = requestedReal;
    double bestImaginary = requestedImaginary;
    double bestScore = -std::numeric_limits<double>::infinity();
    constexpr int gridRadius = 8;
    for (int ring = 1; ring <= 5; ++ring) {
        const double radius = searchScale * (0.025 * static_cast<double>(ring * ring));
        for (int gy = -gridRadius; gy <= gridRadius; ++gy) {
            for (int gx = -gridRadius; gx <= gridRadius; ++gx) {
                if (gx == 0 && gy == 0) continue;
                const double nx = static_cast<double>(gx) / gridRadius;
                const double ny = static_cast<double>(gy) / gridRadius;
                if (nx * nx + ny * ny > 1.05) continue;
                const double candidateReal = requestedReal + nx * radius;
                const double candidateImaginary = requestedImaginary + ny * radius;
                const auto result = CalculateEscape(candidateReal, candidateImaginary,
                                                    maximumIterations, equation);
                const bool valid = (equation.newtonMode || equation.renderMode == FractalRenderMode::Newton)
                    ? result.converged : result.escaped;
                if (!valid || result.iterations < 3) continue;
                const double detail = static_cast<double>(result.iterations);
                const double distancePenalty = std::hypot(nx, ny) * maximumIterations * 0.04;
                const double score = detail - distancePenalty;
                if (score > bestScore &&
                    IsBoundaryRichFractalTarget(candidateReal, candidateImaginary, searchScale,
                                                maximumIterations, equation)) {
                    bestScore = score;
                    bestReal = candidateReal;
                    bestImaginary = candidateImaginary;
                }
            }
        }
        if (bestScore >= maximumIterations * 0.12) break;
    }
    return bestScore > 0.0 ? std::pair<double, double>{bestReal, bestImaginary}
                           : std::pair<double, double>{requestedReal, requestedImaginary};
}

} // namespace mw
