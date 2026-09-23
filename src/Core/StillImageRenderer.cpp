#include "Core/StillImageRenderer.h"

#include "Core/MandelbrotMath.h"
#include "Core/DeepZoom.h"
#include "Core/Precision/HighPrecisionBackend.h"

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

std::array<double, 3> SamplePalette(const std::vector<Colour>& palette, double position,
                                    PaletteInterpolation interpolation) {
    if (palette.empty()) return {1.0, 1.0, 1.0};
    if (palette.size() == 1U) {
        return {palette.front().r, palette.front().g, palette.front().b};
    }
    position -= std::floor(position);
    if (position < 0.0) position += 1.0;
    const double scaled = position * static_cast<double>(palette.size());
    const auto firstIndex = static_cast<std::size_t>(std::floor(scaled)) % palette.size();
    const std::size_t secondIndex = (firstIndex + 1U) % palette.size();
    double fraction = scaled - std::floor(scaled);
    if (interpolation == PaletteInterpolation::Smoothstep) {
        fraction = fraction * fraction * (3.0 - 2.0 * fraction);
    }
    return MixColour(palette[firstIndex], palette[secondIndex], fraction);
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

} // namespace

std::array<double, 3> ColourForEscape(const Preset& preset,
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
    position *= preset.paletteFrequency;
    if (equation.stripeAverageEnabled && equation.stripeStrength > 0.0) {
        position += (escape.stripeAverage - 0.5) * equation.stripeStrength;
    }
    position += preset.colourOffset;
    double phase = position - std::floor(position);
    if (phase < 0.0) phase += 1.0;
    phase = std::pow(std::max(phase, 1.0e-12), preset.paletteGamma);

    std::array<double, 3> colour = SamplePalette(palette, phase, preset.paletteInterpolation);
    if (equation.edgeLightingStrength > 0.0) {
        double edgeFeature = 0.0;
        if (equation.colouringMethod == ColouringMethod::DistanceEstimation &&
            escape.distanceEstimate > 0.0) {
            edgeFeature = std::exp(-escape.distanceEstimate * 80.0);
        } else if (equation.colouringMethod == ColouringMethod::OrbitTrap &&
                   std::isfinite(escape.orbitTrapDistance)) {
            edgeFeature = std::exp(-std::min(escape.orbitTrapDistance, 10.0) * 18.0);
        }
        const double edge = edgeFeature * equation.edgeLightingStrength * 0.20;
        colour[0] = ClampUnit(colour[0] + edge);
        colour[1] = ClampUnit(colour[1] + edge);
        colour[2] = ClampUnit(colour[2] + edge);
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

ComplexPlanePoint MapStillRenderSample(const CameraState& camera,
                                        double rotationDegrees,
                                        std::uint32_t fullWidth,
                                        std::uint32_t fullHeight,
                                        double pixelX,
                                        double pixelY) noexcept {
    ComplexPlanePoint point{CameraCentreX(camera), CameraCentreY(camera)};
    if (fullWidth == 0U || fullHeight == 0U || !(camera.scale > 0.0) ||
        !std::isfinite(camera.scale) || !std::isfinite(rotationDegrees)) {
        return point;
    }
    const long double aspect = static_cast<long double>(fullWidth) /
                               static_cast<long double>(fullHeight);
    const long double nx = static_cast<long double>(pixelX) * 2.0L /
                               static_cast<long double>(fullWidth) - 1.0L;
    const long double ny = 1.0L - static_cast<long double>(pixelY) * 2.0L /
                                      static_cast<long double>(fullHeight);
    const long double localX = nx * aspect * static_cast<long double>(camera.scale);
    const long double localY = ny * static_cast<long double>(camera.scale);
    constexpr long double pi = 3.141592653589793238462643383279502884L;
    const long double radians = static_cast<long double>(rotationDegrees) * pi / 180.0L;
    const long double cosine = std::cos(radians);
    const long double sine = std::sin(radians);
    const long double rotatedX = localX * cosine - localY * sine;
    const long double rotatedY = localX * sine + localY * cosine;
    point.real += static_cast<double>(rotatedX);
    point.imaginary += static_cast<double>(rotatedY);
    return point;
}

std::uint32_t StillRenderTileOverlapPixels(const Preset& preset) noexcept {
    const int bloomRadius = preset.equation.glowStrength > 0.0
        ? std::clamp(preset.equation.bloomRadius, 0, 16)
        : 0;
    const int reconstructionRadius = preset.antiAliasingLevel > 1 ? 1 : 0;
    return static_cast<std::uint32_t>(bloomRadius + reconstructionRadius);
}

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
                                     std::uint32_t tileHeight,
                                     double rotationDegrees) noexcept {
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
    const long double localX = horizontalNormalised * fullAspect *
                               static_cast<long double>(fullCamera.scale);
    const long double localY = verticalNormalised *
                               static_cast<long double>(fullCamera.scale);
    constexpr long double pi = 3.141592653589793238462643383279502884L;
    const long double radians = static_cast<long double>(rotationDegrees) * pi / 180.0L;
    const long double cosine = std::cos(radians);
    const long double sine = std::sin(radians);
    const double deltaX = static_cast<double>(localX * cosine - localY * sine);
    const double deltaY = static_cast<double>(localX * sine + localY * cosine);
    OffsetCamera(tileCamera, deltaX, deltaY);
    tileCamera.scale = static_cast<double>(static_cast<long double>(fullCamera.scale) *
                                           static_cast<long double>(tileHeight) /
                                           fullHeightValue);
    NormaliseCamera(tileCamera);
    return tileCamera;
}

bool BuildExactStillRenderSubpixelSample(const ExactCamera& camera,
                                         double rotationDegrees,
                                         std::uint32_t fullWidth,
                                         std::uint32_t fullHeight,
                                         std::uint32_t pixelX,
                                         std::uint32_t pixelY,
                                         std::uint32_t samplesPerAxis,
                                         std::uint32_t sampleX,
                                         std::uint32_t sampleY,
                                         ExactStillRenderSample& result,
                                         std::string& error) {
    result = {};
    error.clear();
    if (fullWidth == 0U || fullHeight == 0U || pixelX >= fullWidth || pixelY >= fullHeight) {
        error = "Exact still sample dimensions or pixel coordinates are invalid.";
        return false;
    }
    if (!std::isfinite(rotationDegrees)) {
        error = "Exact still sample rotation is not finite.";
        return false;
    }
    if (samplesPerAxis == 0U || samplesPerAxis > 4U || sampleX >= samplesPerAxis ||
        sampleY >= samplesPerAxis ||
        fullHeight > (std::numeric_limits<std::uint32_t>::max)() / samplesPerAxis) {
        error = "Exact still subpixel sample grid or dimensions are invalid.";
        return false;
    }
    const std::int64_t width = static_cast<std::int64_t>(fullWidth);
    const std::int64_t height = static_cast<std::int64_t>(fullHeight);
    const std::int64_t samples = static_cast<std::int64_t>(samplesPerAxis);
    const std::int64_t subpixelX = 2LL * static_cast<std::int64_t>(sampleX) + 1LL;
    const std::int64_t subpixelY = 2LL * static_cast<std::int64_t>(sampleY) + 1LL;
    result.camera = camera;
    result.horizontalHalfHeightFactor = {
        samples * (2LL * static_cast<std::int64_t>(pixelX) - width) + subpixelX,
        samplesPerAxis * fullHeight};
    result.verticalHalfHeightFactor = {
        samples * (height - 2LL * static_cast<std::int64_t>(pixelY)) - subpixelY,
        samplesPerAxis * fullHeight};
    result.rotationDegrees = rotationDegrees;
    result.requiresRotationAdapter = rotationDegrees != 0.0;
    return true;
}

bool BuildExactStillRenderPixelSample(const ExactCamera& camera,
                                      double rotationDegrees,
                                      std::uint32_t fullWidth,
                                      std::uint32_t fullHeight,
                                      std::uint32_t pixelX,
                                      std::uint32_t pixelY,
                                      ExactStillRenderSample& result,
                                      std::string& error) {
    return BuildExactStillRenderSubpixelSample(camera, rotationDegrees, fullWidth, fullHeight,
                                                pixelX, pixelY, 1U, 0U, 0U, result, error);
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
                        const double samplePixelX = static_cast<double>(x) +
                            (static_cast<double>(sampleX) + 0.5) /
                                static_cast<double>(antiAliasing);
                        const double samplePixelY = static_cast<double>(y) +
                            (static_cast<double>(sampleY) + 0.5) /
                                static_cast<double>(antiAliasing);
                        const ComplexPlanePoint point = MapStillRenderSample(
                            preset.camera, preset.rotationDegrees, request.width, request.height,
                            samplePixelX, samplePixelY);
                        const EscapeResult escape = CalculateEscape(
                            point.real,
                            point.imaginary,
                            maximumIterations,
                            preset.equation,
                            request.timeSeconds);
                        const auto sampleColour = ColourForEscape(
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

bool RenderExactDirectStillImage(const ExactDirectStillRenderRequest& request,
                                 const StillRenderRowWriter& rowWriter,
                                 const StillRenderProgressCallback& progressCallback,
                                 const StillRenderCancellationCallback& cancellationCallback,
                                 StillRenderResult& result,
                                 std::string& error) {
    result = {};
    error.clear();
    if (request.width == 0U || request.height == 0U || !rowWriter) {
        error = "Exact direct still rendering requires dimensions and a row writer.";
        return false;
    }
    if (request.preset.rotationDegrees != 0.0) {
        error = "Exact direct still rendering does not support rotation yet.";
        return false;
    }
    if (request.preset.equation.animateCoefficients) {
        error = "Exact direct still rendering does not support animated equation coefficients.";
        return false;
    }
    const int antiAliasing = request.preset.antiAliasingLevel;
    if (antiAliasing < 1 || antiAliasing > 4) {
        error = "Exact direct still rendering requires an anti-aliasing level between 1 and 4.";
        return false;
    }
    if (request.maximumIterations < 32 || request.maximumIterations > 4096) {
        error = "Exact direct still rendering iterations must be between 32 and 4096.";
        return false;
    }
    if (request.precisionBits != 512 && request.precisionBits != 2048 &&
        request.precisionBits != 8192 && request.precisionBits != kMaximumDirectHighPrecisionBits) {
        error = "Exact direct still rendering requires a planner-selected 512-bit, 2048-bit, 8192-bit, or 16384-bit CPU tier.";
        return false;
    }
    constexpr std::size_t kMaximumExactDirectRowBytes = 64U * 1024U * 1024U;
    if (request.width > kMaximumExactDirectRowBytes / sizeof(std::uint32_t)) {
        error = "Exact direct still rendering refuses output rows larger than the 64 MiB working-memory bound.";
        return false;
    }
    const std::uint32_t fullWidth = request.fullWidth == 0U ? request.width : request.fullWidth;
    const std::uint32_t fullHeight = request.fullHeight == 0U ? request.height : request.fullHeight;
    if (fullWidth == 0U || fullHeight == 0U ||
        static_cast<std::uint64_t>(request.tileOriginX) + request.width > fullWidth ||
        static_cast<std::uint64_t>(request.tileOriginY) + request.height > fullHeight) {
        error = "Exact direct still tile origin or full-frame dimensions are invalid.";
        return false;
    }
    const auto palette = ActivePalette(request.preset);
    std::vector<std::uint32_t> row(request.width);
    result.statistics.tileWidth = request.width;
    result.statistics.tileHeight = 1U;
    result.statistics.peakWorkingPixels = row.size();
    result.statistics.maximumIterations = request.maximumIterations;
    result.statistics.antiAliasingLevel = antiAliasing;
    for (std::uint32_t y = 0U; y < request.height; ++y) {
        if (cancellationCallback && cancellationCallback()) {
            error = "Exact direct still rendering was cancelled.";
            return false;
        }
        for (std::uint32_t x = 0U; x < request.width; ++x) {
            std::array<double, 3> accumulated{};
            for (int sampleY = 0; sampleY < antiAliasing; ++sampleY) {
                for (int sampleX = 0; sampleX < antiAliasing; ++sampleX) {
                    ExactStillRenderSample sample;
                    if (!BuildExactStillRenderSubpixelSample(
                            request.camera, 0.0, fullWidth, fullHeight,
                            request.tileOriginX + x, request.tileOriginY + y,
                            static_cast<std::uint32_t>(antiAliasing),
                            static_cast<std::uint32_t>(sampleX),
                            static_cast<std::uint32_t>(sampleY), sample, error)) {
                        return false;
                    }
                    HighPrecisionEscapeResult escape;
                    if (!EvaluateIndependentHighPrecisionSample(sample, request.preset.equation,
                                                                request.maximumIterations,
                                                                cancellationCallback, escape, error,
                                                                request.precisionBits)) {
                        return false;
                    }
                    const auto colour = ColourForEscape(request.preset, palette,
                                                        ToEscapeResult(escape),
                                                        request.maximumIterations);
                    accumulated[0] += colour[0];
                    accumulated[1] += colour[1];
                    accumulated[2] += colour[2];
                }
            }
            const double sampleCount = static_cast<double>(antiAliasing * antiAliasing);
            accumulated[0] /= sampleCount;
            accumulated[1] /= sampleCount;
            accumulated[2] /= sampleCount;
            row[x] = PackPixel(accumulated);
        }
        std::string writerError;
        if (!rowWriter(y, row, writerError)) {
            error = writerError.empty() ? "The exact direct output row could not be written." : writerError;
            return false;
        }
        result.statistics.renderedPixels += request.width;
        if (progressCallback) progressCallback({y + 1U, request.height});
    }
    return true;
}

} // namespace mw
