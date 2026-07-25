#pragma once

#include "Core/Models.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace mw {

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
                                                   std::uint32_t tileHeight) noexcept;

// Renders left-to-right tiles into one output scanline at a time. The full-resolution
// image is never allocated by this function. The writer receives rows in top-down order.
bool RenderStillImageTiled(const StillRenderRequest& request,
                           const StillRenderRowWriter& rowWriter,
                           const StillRenderProgressCallback& progressCallback,
                           const StillRenderCancellationCallback& cancellationCallback,
                           StillRenderResult& result,
                           std::string& error);

} // namespace mw
