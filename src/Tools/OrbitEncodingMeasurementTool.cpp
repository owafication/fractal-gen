#include "Core/Precision/HighPrecisionBackend.h"

#include <cmath>
#include <iomanip>
#include <iostream>

namespace {
bool ReportFixture(const char* name, const mw::CameraState& camera,
                   const mw::EquationSettings& equation) {
    const mw::OrbitEncodingMeasurement measurement =
        mw::MeasureIndependentOrbitEncoding(camera, equation, 128);
    const bool valid = measurement.sourcePrecisionBits == 512 &&
        measurement.requestedIterations == 128 && measurement.comparedCoordinates > 0 &&
        std::isfinite(measurement.maximumAbsoluteError) &&
        std::isfinite(measurement.maximumRelativeError) &&
        measurement.maximumAbsoluteError > 0.0 && measurement.maximumRelativeError > 0.0;
    std::cout << std::setprecision(17)
              << "fixture=" << name
              << " source_bits=" << measurement.sourcePrecisionBits
              << " coordinates=" << measurement.comparedCoordinates
              << " escaped=" << (measurement.escaped ? "true" : "false")
              << " escape_iteration=" << measurement.escapeIteration
              << " maximum_absolute_error=" << measurement.maximumAbsoluteError
              << " maximum_relative_error=" << measurement.maximumRelativeError << '\n';
    return valid;
}
}

int main() {
    mw::CameraState camera{-0.743643887037151, 0.131825904205330, 1.0e-24};
    mw::OffsetCamera(camera, 1.0e-24, -2.0e-24);
    const bool mandelbrot = ReportFixture("analytic-quadratic-mandelbrot", camera,
                                          mw::EquationSettings{});
    const bool tricorn = ReportFixture("tricorn-power2", camera, mw::EquationExample(4));
    if (!mandelbrot || !tricorn) {
        std::cerr << "FAIL: orbit encoding measurement fixture did not produce finite non-zero evidence.\n";
        return 1;
    }
    std::cout << "PASS: bounded float4 orbit encoding measurements completed.\n";
    return 0;
}
