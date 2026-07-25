#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <vector>

namespace mw {

struct RenderRegion {
#ifdef _WIN32
    RECT pixels{};
#endif
    CameraState camera;
    Palette palette{Palette::ClassicSpectrum};
    std::vector<Colour> customPaletteColours;
    int maximumIterations{300};
    EquationSettings equation;
    double colourOffset{0.0};
    double brightness{1.0};
    double contrast{1.0};
    double saturation{1.0};
    Colour interiorColour{0, 0, 0, 1};
    Colour backgroundColour{0, 0, 0, 1};
    bool smoothColouring{true};
};

struct RenderOptions {
    double renderScale{0.75};
    int antiAliasingLevel{1};
    PrecisionSettings precision;
    // Negative values use the live monotonic clock. Still exporters provide one
    // fixed phase so animated coefficients remain identical across GPU tiles.
    double timeSeconds{-1.0};
};

struct PrecisionCapabilities {
    bool nativeFloat64{false};
    bool splitFloat{true};
    bool perturbation{false};
    bool arbitraryReference{false};
};

enum class GpuBackendPreference {
    Automatic,
    Direct3D11,
    OpenGL,
};

enum class ActiveGpuBackend {
    None,
    Direct3D11,
    OpenGL,
};

} // namespace mw
