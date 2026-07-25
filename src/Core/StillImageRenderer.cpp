#include "Core/StillImageRenderer.h"

#include "Core/MandelbrotMath.h"
#include "Core/DeepZoom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace mw {
namespace {

double ClampUnit(double value) {
    return std::clamp(value, 0.0, 1.0);
}

std::array<double, 3> MixColour(const Colour& first, const Colour& second, double fraction) {
    fraction = std::clamp(fraction, 0.0, 1.0);
    return {
        static_cast<double>(first.r) +
            (static_cast<double>(second.r) - static_cast<double>(first.r)) * fraction,
        static_cast<double>(first.g) +
            (static_cast<double>(second.g) - static_cast<double>(first.g)) * fraction,
        static_cast<double>(first.b) +
            (static_cast<double>(second.b) - static_cast<double>(first.b)) * fraction,
    };
}

std::array<double, 3> SamplePalette(const std::vector<Colour>& palette, double position) {
    if (palette.empty()) return {1.0, 1.0, 1.0};
    if (palette.size() == 1U) {
        return {palette.front().r, palette.front().g, palette.front().b};
    }
    position -= std::floor(position);
    if (position < 0.0) position += 1.0;
    const double scaled = position * static_cast<double>(palette.size() - 1U);
    const auto firstIndex = static_cast<std::size_t>(std::floor(scaled));
    const std::size_t secondIndex = std::min(firstIndex + 1U, palette.size() - 1U);
    return MixColour(palette[firstIndex], palette[secondIndex], scaled - std::floor(scaled));
}

std::array<double, 3> ApplyAdjustments(std::array<double, 3> colour, const Preset& preset) {
    double red = ClampUnit(colour[0]) * preset.brightness;
    double green = ClampUnit(colour[1]) * preset.brightness;
    double blue = ClampUnit(colour[2]) * preset.brightness;

    red = (red - 0.5) * preset.contrast + 0.5;
    green = (green - 0.5) * preset.contrast + 0.5;
    blue = (blue - 0.5) * preset.contrast + 0.5;

    const double grey = red * 0.299 + green * 0.587 + blue * 0.114;
    red = grey + (red - grey) * preset.saturation;
    green = grey + (green - grey) * preset.saturation;
    blue = grey + (blue - grey) * preset.saturation;
    return {ClampUnit(red), ClampUnit(green), ClampUnit(blue)};
}

std::uint32_t PackPixel(const std::array<double, 3>& colour) {
    const auto channel = [](double value) {
        const long rounded = std::lround(ClampUnit(value) * 255.0);
        return static_cast<std::uint8_t>(std::clamp(rounded, 0L, 255L));
    };
    const std::uint8_t red = channel(colour[0]);
    const std::uint8_t green = channel(colour[1]);
    const std::uint8_t blue = channel(colour[2]);
    return 0xFF000000U | (static_cast<std::uint32_t>(red) << 16U) |
           (static_cast<std::uint32_t>(green) << 8U) | static_cast<std::uint32_t>(blue);
}

std::vector<Colour> ActivePalette(const Preset& preset) {
    if (preset.customPaletteColours.size() >= 2U) return preset.customPaletteColours;
    return PalettePreviewColours(preset.palette);
}

std::uint32_t PreviewDimension(std::uint32_t sourceWidth, std::uint32_t sourceHeight,
                               std::uint32_t maximumWidth, std::uint32_t maximumHeight,
                               bool widthDimension) {
    if (maximumWidth == 0U || maximumHeight == 0U) return 0U;
    const double widthScale = static_cast<double>(maximumWidth) / static_cast<double>(sourceWidth);
    const double heightScale = static_cast<double>(maximumHeight) / static_cast<double>(sourceHeight);
    const double scale = std::min({1.0, widthScale, heightScale});
    const double source = widthDimension ? static_cast<double>(sourceWidth) : static_cast<double>(sourceHeight);
    return std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::lround(source * scale)));
}

std::uint32_t MapPreviewCoordinate(std::uint32_t previewCoordinate,
                                   std::uint32_t previewSize,
                                   std::uint32_t sourceSize) {
    if (previewSize <= 1U || sourceSize <= 1U) return 0U;
    const double normalised = static_cast<double>(previewCoordinate) /
                              static_cast<double>(previewSize - 1U);
    return std::min(sourceSize - 1U,
                    static_cast<std::uint32_t>(std::lround(normalised *
                                                          static_cast<double>(sourceSize - 1U))));
}

std::array<double, 3> PixelColour(const Preset& preset,
                                  const std::vector<Colour>& palette,
                                  const EscapeResult& escape,
                                  int maximumIterations) {
    if (!escape.escaped && !escape.converged) {
        return ApplyAdjustments(
            {preset.interiorColour.r, preset.interiorColour.g, preset.interiorColour.b}, preset);
    }

    const EquationSettings& equation = preset.equation;
    double position = escape.smoothValue / static_cast<double>(std::max(1, maximumIterations));
    if (equation.newtonMode || equation.renderMode == FractalRenderMode::Newton) {
        position = escape.rootIndex >= 0
            ? static_cast<double>(escape.rootIndex) /
                  static_cast<double>(std::max(2, equation.newtonDegree))
            : position;
        position += 0.08 *
                    (1.0 - static_cast<double>(escape.iterations) /
                               static_cast<double>(std::max(1, maximumIterations)));
    } else if (equation.colouringMethod == ColouringMethod::OrbitTrap) {
        position = -std::log(std::max(escape.orbitTrapDistance, 1.0e-8)) * 0.32;
    } else if (equation.colouringMethod == ColouringMethod::DistanceEstimation &&
               escape.distanceEstimate > 0.0) {
        position = -std::log(std::max(escape.distanceEstimate, 1.0e-10)) * 0.22;
    }
    position += preset.colourOffset;

    std::array<double, 3> colour = SamplePalette(palette, position);
    if (equation.glowStrength > 0.0) {
        const double glow =
            std::exp(-std::min(escape.orbitTrapDistance, 10.0) * 12.0) *
            equation.glowStrength * 0.20;
        colour[0] = ClampUnit(colour[0] + glow);
        colour[1] = ClampUnit(colour[1] + glow);
        colour[2] = ClampUnit(colour[2] + glow);
    }
    if (equation.depthStrength > 0.0 && escape.escaped) {
        const double depth = std::clamp(
            1.0 - static_cast<double>(escape.iterations) /
                      static_cast<double>(std::max(1, maximumIterations)),
            0.0, 1.0);
        const double factor = 1.0 + depth * equation.depthStrength * 0.25;
        colour[0] = ClampUnit(colour[0] * factor);
        colour[1] = ClampUnit(colour[1] * factor);
        colour[2] = ClampUnit(colour[2] * factor);
    }
    return ApplyAdjustments(colour, preset);
}

} // namespace

StillRenderQuality ResolveStillRenderQuality(const StillRenderRequest& request) noexcept {
    StillRenderQuality quality;
    quality.maximumIterations = std::clamp(request.preset.maximumIterations, 32, 4096);
    quality.antiAliasingLevel = std::clamp(request.preset.antiAliasingLevel, 1, 4);

    const double scale = std::abs(request.preset.camera.scale);
    const double height = static_cast<double>(std::max(request.height, 1U));
    if (!(scale > 0.0) || !std::isfinite(scale)) return quality;

    quality.outputPixelSpan = (2.0 * scale) / height;
    if (!request.scaleQualityToResolution || !(quality.outputPixelSpan > 0.0) ||
        !std::isfinite(quality.outputPixelSpan)) {
        return quality;
    }

    // A full Mandelbrot-height view at 1080p is the neutral reference. Every
    // additional resolved detail stop raises the escape budget by 32 iterations.
    // The preset value remains the minimum and the shader/core safety cap remains 4096.
    constexpr double referencePixelSpan = 3.0 / 1080.0;
    const double ratio = referencePixelSpan / quality.outputPixelSpan;
    if (ratio > 1.0 && std::isfinite(ratio)) {
        quality.detailStopsBeyond1080p = std::max(0.0, std::log2(ratio));
        const double calculatedFloor = 192.0 + quality.detailStopsBeyond1080p * 32.0;
        const int detailFloor = calculatedFloor >= 4096.0
            ? 4096
            : static_cast<int>(std::ceil(calculatedFloor));
        quality.maximumIterations = std::max(quality.maximumIterations, detailFloor);
    }
    return quality;
}

CameraState CameraForStillRenderTile(const CameraState& fullCamera,
                                     std::uint32_t fullWidth,
                                     std::uint32_t fullHeight,
                                     std::uint32_t tileX,
                                     std::uint32_t tileY,
                                     std::uint32_t tileWidth,
                                     std::uint32_t tileHeight) noexcept {
    CameraState tileCamera = fullCamera;
    if (fullWidth == 0U || fullHeight == 0U || tileWidth == 0U || tileHeight == 0U ||
        tileX >= fullWidth || tileY >= fullHeight || !(fullCamera.scale > 0.0) ||
        !std::isfinite(fullCamera.scale)) {
        return tileCamera;
    }

    tileWidth = std::min(tileWidth, fullWidth - tileX);
    tileHeight = std::min(tileHeight, fullHeight - tileY);
    const long double fullWidthValue = static_cast<long double>(fullWidth);
    const long double fullHeightValue = static_cast<long double>(fullHeight);
    const long double tileCentreX = static_cast<long double>(tileX) +
                                    static_cast<long double>(tileWidth) * 0.5L;
    const long double tileCentreY = static_cast<long double>(tileY) +
                                    static_cast<long double>(tileHeight) * 0.5L;
    const long double horizontalNormalised = tileCentreX * 2.0L / fullWidthValue - 1.0L;
    // Output rows are top-down, while the fractal coordinate system has +Y at the top.
    const long double verticalNormalised = 1.0L - tileCentreY * 2.0L / fullHeightValue;
    const long double fullAspect = fullWidthValue / fullHeightValue;
    const double deltaX = static_cast<double>(horizontalNormalised * fullAspect *
                                               static_cast<long double>(fullCamera.scale));
    const double deltaY = static_cast<double>(verticalNormalised *
                                               static_cast<long double>(fullCamera.scale));
    OffsetCamera(tileCamera, deltaX, deltaY);
    tileCamera.scale = static_cast<double>(static_cast<long double>(fullCamera.scale) *
                                           static_cast<long double>(tileHeight) /
                                           fullHeightValue);
    NormaliseCamera(tileCamera);
    return tileCamera;
}

bool RenderStillImageTiled(const StillRenderRequest& request,
                           const StillRenderRowWriter& rowWriter,
                           const StillRenderProgressCallback& progressCallback,
                           const StillRenderCancellationCallback& cancellationCallback,
                           StillRenderResult& result,
                           std::string& error) {
    result = {};
    error.clear();
    if (request.width == 0U || request.height == 0U) {
        error = "Still-render dimensions must be positive.";
        return false;
    }
    if (!rowWriter) {
        error = "A still-render row writer is required.";
        return false;
    }
    if (request.width > static_cast<std::uint32_t>(std::numeric_limits<std::size_t>::max() /
                                                   sizeof(std::uint32_t))) {
        error = "A single output row is too wide for this process.";
        return false;
    }

    const std::uint32_t tileWidth = std::clamp(request.tileWidth, 1U, request.width);
    std::vector<std::uint32_t> row(static_cast<std::size_t>(request.width));
    std::vector<std::uint32_t> tile(static_cast<std::size_t>(tileWidth));
    result.statistics.peakWorkingPixels = row.size() + tile.size();
    result.statistics.tileWidth = tileWidth;
    result.statistics.tileHeight = 1U;

    result.preview.width = PreviewDimension(request.width, request.height,
                                            request.previewMaximumWidth,
                                            request.previewMaximumHeight, true);
    result.preview.height = PreviewDimension(request.width, request.height,
                                             request.previewMaximumWidth,
                                             request.previewMaximumHeight, false);
    if (result.preview.width > 0U && result.preview.height > 0U) {
        const std::uint64_t previewCount =
            static_cast<std::uint64_t>(result.preview.width) * result.preview.height;
        if (previewCount <= static_cast<std::uint64_t>(
                                std::numeric_limits<std::size_t>::max() /
                                sizeof(std::uint32_t))) {
            result.preview.pixels.assign(static_cast<std::size_t>(previewCount), 0xFF000000U);
        } else {
            result.preview = {};
        }
    }

    std::vector<std::uint32_t> previewSourceX;
    if (!result.preview.pixels.empty()) {
        previewSourceX.resize(result.preview.width);
        for (std::uint32_t previewX = 0; previewX < result.preview.width; ++previewX) {
            previewSourceX[previewX] = MapPreviewCoordinate(
                previewX, result.preview.width, request.width);
        }
    }

    Preset preset = request.preset;
    const StillRenderQuality quality = ResolveStillRenderQuality(request);
    preset.maximumIterations = quality.maximumIterations;
    preset.antiAliasingLevel = quality.antiAliasingLevel;
    const auto palette = ActivePalette(preset);
    const int maximumIterations = quality.maximumIterations;
    const int antiAliasing = quality.antiAliasingLevel;
    result.statistics.maximumIterations = maximumIterations;
    result.statistics.antiAliasingLevel = antiAliasing;
    const double aspect = static_cast<double>(request.width) /
                          static_cast<double>(request.height);
    std::uint32_t nextPreviewY = 0U;
    std::uint32_t nextPreviewSourceY = result.preview.height > 0U
        ? MapPreviewCoordinate(0U, result.preview.height, request.height)
        : request.height;

    if (progressCallback) progressCallback({0U, request.height});
    for (std::uint32_t y = 0; y < request.height; ++y) {
        if (cancellationCallback && cancellationCallback()) {
            error = "Still render cancelled.";
            return false;
        }
        std::array<double, 4> imaginarySamples{};
        for (int sampleY = 0; sampleY < antiAliasing; ++sampleY) {
            const double vertical =
                (static_cast<double>(y) +
                 (static_cast<double>(sampleY) + 0.5) /
                     static_cast<double>(antiAliasing)) /
                static_cast<double>(request.height);
            imaginarySamples[static_cast<std::size_t>(sampleY)] =
                CameraCentreY(preset.camera) +
                (vertical * 2.0 - 1.0) * preset.camera.scale;
        }

        for (std::uint32_t tileStart = 0U; tileStart < request.width;
             tileStart += tileWidth) {
            if (cancellationCallback && cancellationCallback()) {
                error = "Still render cancelled.";
                return false;
            }
            const std::uint32_t pixelsInTile =
                std::min(tileWidth, request.width - tileStart);
            for (std::uint32_t localX = 0U; localX < pixelsInTile; ++localX) {
                if ((localX % 16U) == 0U && cancellationCallback && cancellationCallback()) {
                    error = "Still render cancelled.";
                    return false;
                }
                const std::uint32_t x = tileStart + localX;
                std::array<double, 3> accumulated{};
                for (int sampleY = 0; sampleY < antiAliasing; ++sampleY) {
                    for (int sampleX = 0; sampleX < antiAliasing; ++sampleX) {
                        const double horizontal =
                            (static_cast<double>(x) +
                             (static_cast<double>(sampleX) + 0.5) /
                                 static_cast<double>(antiAliasing)) /
                            static_cast<double>(request.width);
                        const double real = CameraCentreX(preset.camera) +
                            (horizontal * 2.0 - 1.0) * preset.camera.scale * aspect;
                        const EscapeResult escape = CalculateEscape(
                            real,
                            imaginarySamples[static_cast<std::size_t>(sampleY)],
                            maximumIterations,
                            preset.equation,
                            request.timeSeconds);
                        const auto sampleColour = PixelColour(
                            preset, palette, escape, maximumIterations);
                        accumulated[0] += sampleColour[0];
                        accumulated[1] += sampleColour[1];
                        accumulated[2] += sampleColour[2];
                    }
                }
                const double sampleCount =
                    static_cast<double>(antiAliasing * antiAliasing);
                accumulated[0] /= sampleCount;
                accumulated[1] /= sampleCount;
                accumulated[2] /= sampleCount;
                tile[localX] = PackPixel(accumulated);
            }
            std::copy_n(tile.begin(), static_cast<std::ptrdiff_t>(pixelsInTile),
                        row.begin() + static_cast<std::ptrdiff_t>(tileStart));
        }

        while (nextPreviewY < result.preview.height && nextPreviewSourceY == y) {
            const std::size_t previewRowOffset =
                static_cast<std::size_t>(nextPreviewY) * result.preview.width;
            for (std::uint32_t previewX = 0U; previewX < result.preview.width; ++previewX) {
                result.preview.pixels[previewRowOffset + previewX] =
                    row[previewSourceX[previewX]];
            }
            ++nextPreviewY;
            nextPreviewSourceY = nextPreviewY < result.preview.height
                ? MapPreviewCoordinate(nextPreviewY, result.preview.height, request.height)
                : request.height;
        }

        std::string writerError;
        if (!rowWriter(y, row, writerError)) {
            error = writerError.empty() ? "The still-render output row could not be written."
                                        : writerError;
            return false;
        }
        result.statistics.renderedPixels += request.width;
        if (progressCallback) progressCallback({y + 1U, request.height});
    }
    return true;
}

} // namespace mw
