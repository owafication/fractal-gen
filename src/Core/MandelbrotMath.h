#pragma once

#include "Core/Models.h"

#include <utility>

namespace mw {

struct EscapeResult {
    int iterations{0};
    bool escaped{false};
    double smoothValue{0.0};
    bool converged{false};
    int rootIndex{-1};
    double orbitTrapDistance{1.0e30};
    double distanceEstimate{0.0};
};

EscapeResult CalculateEscape(double real, double imaginary, int maximumIterations);
EscapeResult CalculateEscape(double real, double imaginary, int maximumIterations,
                             const EquationSettings& equation, double timeSeconds = 0.0);
bool IsInterestingMandelbrotTarget(double real, double imaginary, int maximumIterations);
bool IsInterestingFractalTarget(double real, double imaginary, int maximumIterations,
                                const EquationSettings& equation);
bool IsBoundaryRichFractalTarget(double real, double imaginary, double viewScale,
                                 int maximumIterations, const EquationSettings& equation);
std::pair<double, double> FindInterestingMandelbrotTarget(
    double requestedReal, double requestedImaginary, double searchScale, int maximumIterations);
std::pair<double, double> FindInterestingFractalTarget(
    double requestedReal, double requestedImaginary, double searchScale, int maximumIterations,
    const EquationSettings& equation);

} // namespace mw
