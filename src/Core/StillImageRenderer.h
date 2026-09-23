#pragma once

#include "Core/Models.h"
#include "Core/Precision/ExactCamera.h"
#include "Core/Precision/HighPrecisionBackend.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace mw {

struct EscapeResult;

struct ComplexPlanePoint {
    double real{0.0};
    double imaginary{0.0};
};

// A symbolic global sample. The camera values and rational offsets are exact;
// a non-zero rotation remains an explicit backend/planner responsibility because
// its trigonometric transform cannot in general be represented as a decimal.
struct ExactRationalOffset {
    std::int64_t numerator{0};
    std::uint32_t denominator{1};

    bool operator==(const ExactRationalOffset&) const = default;
};

struct ExactStillRenderSample {
    ExactCamera camera;
    ExactRationalOffset horizontalHalfHeightFactor;
    ExactRationalOffset verticalHalfHeightFactor;
    double rotationDegrees{0.0};
    bool requiresRotationAdapter{false};
};

struct StillRenderRequest {
    Preset preset;
    std::uint32_t width{3840};
    std::uint32_t height{2160};
    std::uint32_t tileWidth{256};
    std::uint32_t previewMaximumWidth{1600};
    std::uint32_t previewMaximumHeight{1000};
    // Freezes animated equation coefficients at one still-frame phase.
    double timeSeconds{0.0};
    // Raises the iteration budget when the requested pixel scale resolves detail
    // beyond a normal 1080p view. The configured preset remains the minimum.
    bool scaleQualityToResolution{true};
};

struct ExactDirectStillRenderRequest {
    Preset preset;
    ExactCamera camera;
    std::uint32_t width{0U};
    std::uint32_t height{0U};
    int maximumIterations{0};
    int precisionBits{kMinimumDirectHighPrecisionBits};
    // Zero retains the historical whole-image request. Non-zero full dimensions
    // and origin select a bounded crop but retain the same global exact mapping.
    std::uint32_t fullWidth{0U};
    std::uint32_t fullHeight{0U};
    std::uint32_t tileOriginX{0U};
    std::uint32_t tileOriginY{0U};
};

struct StillRenderQuality {
    int maximumIterations{300};
    int antiAliasingLevel{1};
    double outputPixelSpan{0.0};
    double detailStopsBeyond1080p{0.0};
};

struct StillRenderProgress {
    std::uint32_t completedRows{0};
    std::uint32_t totalRows{0};
};

struct StillRenderPreview {
    std::uint32_t width{0};
    std::uint32_t height{0};
    // Top-down BGRA pixels encoded as 0xAARRGGBB.
    std::vector<std::uint32_t> pixels;
};

struct StillRenderStatistics {
    std::uint64_t renderedPixels{0};
    std::size_t peakWorkingPixels{0};
    std::uint32_t tileWidth{0};
    std::uint32_t tileHeight{0};
    int maximumIterations{0};
    int antiAliasingLevel{1};
};

struct StillRenderResult {
    StillRenderPreview preview;
    StillRenderStatistics statistics;
};

// Shared CPU colour contract for an already-classified escape sample.
[[nodiscard]] std::array<double, 3> ColourForEscape(const Preset& preset,
                                                     const std::vector<Colour>& palette,
                                                     const EscapeResult& escape,
                                                     int maximumIterations);

using StillRenderRowWriter =
    std::function<bool(std::uint32_t rowIndex, std::span<const std::uint32_t> bgraPixels,
                       std::string& error)>;
using StillRenderProgressCallback = std::function<void(const StillRenderProgress& progress)>;
using StillRenderCancellationCallback = std::function<bool()>;

// Resolves a deterministic quality budget from the camera scale and output height.
// This keeps ordinary exports at their configured quality while adding iterations for
// deep zooms and resolutions that reveal finer boundary detail.
[[nodiscard]] StillRenderQuality ResolveStillRenderQuality(const StillRenderRequest& request) noexcept;

// Returns the exact camera viewport for a top-down rectangular tile of a larger still.
// Compensated centre coordinates are retained for deep-zoom tile placement.
[[nodiscard]] CameraState CameraForStillRenderTile(const CameraState& fullCamera,
                                                   std::uint32_t fullWidth,
                                                   std::uint32_t fullHeight,
                                                   std::uint32_t tileX,
                                                   std::uint32_t tileY,
                                                   std::uint32_t tileWidth,
                                                   std::uint32_t tileHeight,
                                                   double rotationDegrees = 0.0) noexcept;

// Maps a top-down global output sample to the complex plane using the same
// centre, scale, aspect and rotation convention as both GPU backends.
[[nodiscard]] ComplexPlanePoint MapStillRenderSample(const CameraState& camera,
                                                      double rotationDegrees,
                                                      std::uint32_t fullWidth,
                                                      std::uint32_t fullHeight,
                                                      double pixelX,
                                                      double pixelY) noexcept;

// Builds the unrotated exact relationship for the centre of a global output
// pixel. It intentionally does not convert the camera or factors to double.
[[nodiscard]] bool BuildExactStillRenderPixelSample(const ExactCamera& camera,
                                                     double rotationDegrees,
                                                     std::uint32_t fullWidth,
                                                     std::uint32_t fullHeight,
                                                     std::uint32_t pixelX,
                                                     std::uint32_t pixelY,
                                                     ExactStillRenderSample& result,
                                                     std::string& error);

// Builds an unrotated exact subpixel sample. `samplesPerAxis` and the sample
// indices define a regular AA grid without converting the coordinate to a
// binary floating-point value.
[[nodiscard]] bool BuildExactStillRenderSubpixelSample(const ExactCamera& camera,
                                                        double rotationDegrees,
                                                        std::uint32_t fullWidth,
                                                        std::uint32_t fullHeight,
                                                        std::uint32_t pixelX,
                                                        std::uint32_t pixelY,
                                                        std::uint32_t samplesPerAxis,
                                                        std::uint32_t sampleX,
                                                        std::uint32_t sampleY,
                                                        ExactStillRenderSample& result,
                                                        std::string& error);

// Required cropped overlap for tiled GPU still rendering. Bloom radius is in
// render pixels; supersampling contributes one additional reconstruction pixel.
[[nodiscard]] std::uint32_t StillRenderTileOverlapPixels(const Preset& preset) noexcept;

// Renders left-to-right tiles into one output scanline at a time. The full-resolution
// image is never allocated by this function. The writer receives rows in top-down order.
bool RenderStillImageTiled(const StillRenderRequest& request,
                           const StillRenderRowWriter& rowWriter,
                           const StillRenderProgressCallback& progressCallback,
                           const StillRenderCancellationCallback& cancellationCallback,
                           StillRenderResult& result,
                           std::string& error);

// Bounded direct exact route for the currently validated Boost CPU tiers,
// unrotated, AA 1–4 escape profiles. Rows stream through the same
// writer contract as the legacy still renderer.
bool RenderExactDirectStillImage(const ExactDirectStillRenderRequest& request,
                                 const StillRenderRowWriter& rowWriter,
                                 const StillRenderProgressCallback& progressCallback,
                                 const StillRenderCancellationCallback& cancellationCallback,
                                 StillRenderResult& result,
                                 std::string& error);

} // namespace mw
