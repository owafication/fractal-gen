#include "Core/VisualRegression.h"

#include "Core/Json.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>

namespace mw {
namespace {

const Preset* FindPreset(const std::vector<Preset>& presets, const std::string& id) {
    const auto found = std::find_if(presets.begin(), presets.end(),
                                    [&](const Preset& preset) { return preset.id == id; });
    return found == presets.end() ? nullptr : &*found;
}

json::Value ComplexJson(const ComplexCoefficient& value) {
    return json::Value::Object{{"imaginary", value.imaginary}, {"real", value.real}};
}

json::Value ColourJson(const Colour& value) {
    return json::Value::Object{
        {"a", static_cast<double>(value.a)},
        {"b", static_cast<double>(value.b)},
        {"g", static_cast<double>(value.g)},
        {"r", static_cast<double>(value.r)},
    };
}

json::Value EquationJson(const EquationSettings& equation) {
    return json::Value::Object{
        {"absoluteImaginary", equation.absoluteImaginary},
        {"absoluteReal", equation.absoluteReal},
        {"animateCoefficients", equation.animateCoefficients},
        {"bailoutRadius", equation.bailoutRadius},
        {"bloomRadius", equation.bloomRadius},
        {"bloomSoftKnee", equation.bloomSoftKnee},
        {"bloomThreshold", equation.bloomThreshold},
        {"coefficientAnimationAmplitude", equation.coefficientAnimationAmplitude},
        {"coefficientAnimationSpeed", equation.coefficientAnimationSpeed},
        {"colouringMethod", static_cast<int>(equation.colouringMethod)},
        {"conjugate", equation.conjugate},
        {"constant", ComplexJson(equation.constant)},
        {"convergenceTolerance", equation.convergenceTolerance},
        {"depthStrength", equation.depthStrength},
        {"edgeLightingStrength", equation.edgeLightingStrength},
        {"glowStrength", equation.glowStrength},
        {"initialZ", ComplexJson(equation.initialZ)},
        {"initialZMode", static_cast<int>(equation.initialZMode)},
        {"iterationTerm", ComplexJson(equation.iterationTerm)},
        {"juliaMode", equation.juliaMode},
        {"juliaParameter", ComplexJson(equation.juliaParameter)},
        {"linear", ComplexJson(equation.linear)},
        {"newtonDegree", equation.newtonDegree},
        {"newtonMode", equation.newtonMode},
        {"newtonRelaxation", ComplexJson(equation.newtonRelaxation)},
        {"newtonTarget", ComplexJson(equation.newtonTarget)},
        {"orbitTrap", static_cast<int>(equation.orbitTrap)},
        {"orbitTrapPoint", ComplexJson(equation.orbitTrapPoint)},
        {"orbitTrapRadius", equation.orbitTrapRadius},
        {"parameter", ComplexJson(equation.parameter)},
        {"parameterPower", equation.parameterPower},
        {"power", equation.power},
        {"quadratic", ComplexJson(equation.quadratic)},
        {"reciprocalCoefficient", ComplexJson(equation.reciprocalCoefficient)},
        {"reciprocalPower", equation.reciprocalPower},
        {"renderMode", static_cast<int>(equation.renderMode)},
        {"stripeAverageEnabled", equation.stripeAverageEnabled},
        {"stripeDensity", equation.stripeDensity},
        {"stripePhase", equation.stripePhase},
        {"stripeStartIteration", equation.stripeStartIteration},
        {"stripeStrength", equation.stripeStrength},
        {"swapRealImaginary", equation.swapRealImaginary},
        {"unaryTransform", static_cast<int>(equation.unaryTransform)},
    };
}

json::Value PaletteColoursJson(const std::vector<Colour>& colours) {
    json::Value::Array result;
    result.reserve(colours.size());
    for (const auto& colour : colours) result.emplace_back(ColourJson(colour));
    return result;
}

std::uint8_t Channel(std::uint32_t pixel, unsigned shift) noexcept {
    return static_cast<std::uint8_t>((pixel >> shift) & 0xFFU);
}

std::uint8_t AbsoluteDifference(std::uint8_t first, std::uint8_t second) noexcept {
    return first >= second ? static_cast<std::uint8_t>(first - second)
                           : static_cast<std::uint8_t>(second - first);
}

std::uint32_t DifferencePixel(std::uint32_t expected, std::uint32_t actual,
                              std::uint8_t amplification) noexcept {
    const auto amplified = [amplification](std::uint8_t difference) {
        const unsigned value = static_cast<unsigned>(difference) *
                               static_cast<unsigned>(amplification);
        return static_cast<std::uint8_t>(std::min(value, 255U));
    };
    const std::uint8_t red = amplified(AbsoluteDifference(Channel(expected, 16U),
                                                          Channel(actual, 16U)));
    const std::uint8_t green = amplified(AbsoluteDifference(Channel(expected, 8U),
                                                            Channel(actual, 8U)));
    const std::uint8_t blue = amplified(AbsoluteDifference(Channel(expected, 0U),
                                                           Channel(actual, 0U)));
    return 0xFF000000U | (static_cast<std::uint32_t>(red) << 16U) |
           (static_cast<std::uint32_t>(green) << 8U) | static_cast<std::uint32_t>(blue);
}

void HashByte(std::uint64_t& hash, std::uint8_t byte) noexcept {
    constexpr std::uint64_t prime = 1099511628211ULL;
    hash ^= byte;
    hash *= prime;
}

void HashUint32(std::uint64_t& hash, std::uint32_t value) noexcept {
    HashByte(hash, static_cast<std::uint8_t>(value & 0xFFU));
    HashByte(hash, static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    HashByte(hash, static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    HashByte(hash, static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
}

} // namespace

std::vector<VisualFixtureDefinition> BuiltInVisualFixtures() {
    const auto presets = BuiltInPresets();
    std::vector<VisualFixtureDefinition> fixtures;
    fixtures.reserve(4U);

    if (const Preset* source = FindPreset(presets, "full-view")) {
        VisualFixtureDefinition fixture;
        fixture.id = "standard-mandelbrot";
        fixture.name = "Standard Mandelbrot Full View";
        fixture.coverage = "Canonical full-view escape-time mapping and palette output.";
        fixture.precisionExpectation = "Portable CPU double production still renderer.";
        fixture.preset = *source;
        fixture.preset.animationMode = AnimationMode::ManualView;
        fixture.preset.antiAliasingLevel = 1;
        fixture.width = 192U;
        fixture.height = 108U;
        fixture.tiledWidth = 37U;
        fixtures.push_back(std::move(fixture));
    }

    if (const Preset* source = FindPreset(presets, "tricorn-cyan-fire-ring")) {
        VisualFixtureDefinition fixture;
        fixture.id = "tricorn-cyan-fire-ring";
        fixture.name = "Tricorn Cyan Fire Ring";
        fixture.coverage = "Conjugate recurrence, distance colouring, stripes, edge lighting and smooth palette interpolation.";
        fixture.precisionExpectation = "Portable CPU double production still renderer; screen-space bloom is GPU-only.";
        fixture.preset = *source;
        fixture.preset.animationMode = AnimationMode::ManualView;
        fixture.width = 96U;
        fixture.height = 54U;
        fixture.tiledWidth = 23U;
        fixtures.push_back(std::move(fixture));
    }

    if (const Preset* source = FindPreset(presets, "reference-classic-blue-gold")) {
        VisualFixtureDefinition fixture;
        fixture.id = "rotated-bloom-state";
        fixture.name = "Rotated Bloom-State Scene";
        fixture.coverage = "Rotated global mapping, anti-aliasing, edge lighting and bloom overlap state.";
        fixture.precisionExpectation = "Portable CPU pixels cover rotation and mathematical lighting; GPU fixture expansion must validate bloom blur.";
        fixture.preset = *source;
        fixture.preset.animationMode = AnimationMode::ManualView;
        fixture.preset.rotationDegrees = 31.5;
        fixture.preset.antiAliasingLevel = 2;
        fixture.preset.equation.glowStrength = 0.55;
        fixture.preset.equation.bloomRadius = 4;
        fixture.preset.equation.edgeLightingStrength = 0.45;
        fixture.width = 128U;
        fixture.height = 72U;
        fixture.tiledWidth = 29U;
        fixtures.push_back(std::move(fixture));
    }

    if (const Preset* source = FindPreset(presets, "seahorse-valley")) {
        VisualFixtureDefinition fixture;
        fixture.id = "deep-mandelbrot-perturbation-state";
        fixture.name = "Deep Mandelbrot Perturbation-State Scene";
        fixture.coverage = "Deep compensated camera state and high-iteration Mandelbrot boundary output.";
        fixture.precisionExpectation = "Portable CPU double canonical output; D3D11/OpenGL perturbation execution remains a Windows PH-02 gate.";
        fixture.preset = *source;
        fixture.preset.animationMode = AnimationMode::ManualView;
        fixture.preset.camera.centreX = -0.743643887037151;
        fixture.preset.camera.centreY = 0.131825904205330;
        fixture.preset.camera.centreXLow = 2.6e-17;
        fixture.preset.camera.centreYLow = -1.1e-17;
        fixture.preset.camera.scale = 1.0e-9;
        fixture.preset.startingScale = fixture.preset.camera.scale;
        fixture.preset.maximumIterations = 1200;
        fixture.preset.antiAliasingLevel = 1;
        fixture.width = 112U;
        fixture.height = 63U;
        fixture.tiledWidth = 31U;
        fixtures.push_back(std::move(fixture));
    }

    return fixtures;
}

bool RenderVisualFixture(const VisualFixtureDefinition& fixture,
                         std::uint32_t tileWidth,
                         VisualImage& image,
                         StillRenderStatistics& statistics,
                         std::string& error) {
    image = {};
    statistics = {};
    error.clear();
    if (fixture.width == 0U || fixture.height == 0U) {
        error = "Visual fixture dimensions must be positive.";
        return false;
    }
    const std::uint64_t pixelCount = static_cast<std::uint64_t>(fixture.width) * fixture.height;
    if (pixelCount > static_cast<std::uint64_t>(
                         std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t))) {
        error = "Visual fixture is too large for this process.";
        return false;
    }

    image.width = fixture.width;
    image.height = fixture.height;
    image.pixels.assign(static_cast<std::size_t>(pixelCount), 0xFF000000U);

    StillRenderRequest request;
    request.preset = fixture.preset;
    request.width = fixture.width;
    request.height = fixture.height;
    request.tileWidth = std::clamp(tileWidth, 1U, fixture.width);
    request.previewMaximumWidth = 0U;
    request.previewMaximumHeight = 0U;
    request.timeSeconds = fixture.timeSeconds;
    request.scaleQualityToResolution = false;

    StillRenderResult result;
    const bool rendered = RenderStillImageTiled(
        request,
        [&](std::uint32_t rowIndex, std::span<const std::uint32_t> row,
            std::string& writerError) {
            if (rowIndex >= image.height || row.size() != image.width) {
                writerError = "Production still renderer returned an invalid fixture row.";
                return false;
            }
            const std::size_t offset = static_cast<std::size_t>(rowIndex) * image.width;
            std::copy(row.begin(), row.end(), image.pixels.begin() +
                      static_cast<std::ptrdiff_t>(offset));
            return true;
        },
        {}, {}, result, error);
    if (!rendered) {
        image = {};
        return false;
    }
    statistics = result.statistics;
    return true;
}

VisualComparisonMetrics CompareVisualImages(const VisualImage& expected,
                                             const VisualImage& actual,
                                             std::uint8_t errorFloor) noexcept {
    VisualComparisonMetrics metrics;
    metrics.dimensionsMatch = expected.width == actual.width &&
                              expected.height == actual.height &&
                              expected.pixels.size() == actual.pixels.size();
    if (!metrics.dimensionsMatch) return metrics;

    metrics.pixelCount = expected.pixels.size();
    long double absoluteErrorSum = 0.0L;
    long double squaredErrorSum = 0.0L;
    long double expectedLuminanceSum = 0.0L;
    long double actualLuminanceSum = 0.0L;
    long double expectedLuminanceSquaredSum = 0.0L;
    long double actualLuminanceSquaredSum = 0.0L;
    long double luminanceProductSum = 0.0L;
    for (std::size_t index = 0; index < expected.pixels.size(); ++index) {
        bool pixelDiffers = false;
        const long double expectedLuminance =
            0.299L * Channel(expected.pixels[index], 16U) +
            0.587L * Channel(expected.pixels[index], 8U) +
            0.114L * Channel(expected.pixels[index], 0U);
        const long double actualLuminance =
            0.299L * Channel(actual.pixels[index], 16U) +
            0.587L * Channel(actual.pixels[index], 8U) +
            0.114L * Channel(actual.pixels[index], 0U);
        expectedLuminanceSum += expectedLuminance;
        actualLuminanceSum += actualLuminance;
        expectedLuminanceSquaredSum += expectedLuminance * expectedLuminance;
        actualLuminanceSquaredSum += actualLuminance * actualLuminance;
        luminanceProductSum += expectedLuminance * actualLuminance;
        for (const unsigned shift : {16U, 8U, 0U}) {
            const std::uint8_t difference = AbsoluteDifference(
                Channel(expected.pixels[index], shift), Channel(actual.pixels[index], shift));
            metrics.maximumChannelError = std::max(metrics.maximumChannelError, difference);
            absoluteErrorSum += difference;
            squaredErrorSum += static_cast<long double>(difference) * difference;
            if (difference > errorFloor) pixelDiffers = true;
        }
        if (pixelDiffers) ++metrics.differingPixels;
    }

    if (metrics.pixelCount > 0U) {
        metrics.differingPixelRatio = static_cast<double>(metrics.differingPixels) /
                                      static_cast<double>(metrics.pixelCount);
        const long double channelCount = static_cast<long double>(metrics.pixelCount) * 3.0L;
        metrics.meanAbsoluteChannelError = static_cast<double>(absoluteErrorSum / channelCount);
        metrics.rootMeanSquareChannelError =
            static_cast<double>(std::sqrt(squaredErrorSum / channelCount));

        const long double sampleCount = static_cast<long double>(metrics.pixelCount);
        const long double expectedMean = expectedLuminanceSum / sampleCount;
        const long double actualMean = actualLuminanceSum / sampleCount;
        const long double expectedVariance = std::max(
            0.0L, expectedLuminanceSquaredSum / sampleCount - expectedMean * expectedMean);
        const long double actualVariance = std::max(
            0.0L, actualLuminanceSquaredSum / sampleCount - actualMean * actualMean);
        const long double covariance =
            luminanceProductSum / sampleCount - expectedMean * actualMean;
        constexpr long double c1 = 6.5025L;  // (0.01 * 255)^2
        constexpr long double c2 = 58.5225L; // (0.03 * 255)^2
        const long double denominator =
            (expectedMean * expectedMean + actualMean * actualMean + c1) *
            (expectedVariance + actualVariance + c2);
        if (denominator > 0.0L) {
            const long double similarity =
                ((2.0L * expectedMean * actualMean + c1) *
                 (2.0L * covariance + c2)) / denominator;
            metrics.structuralSimilarity = static_cast<double>(
                std::clamp(similarity, -1.0L, 1.0L));
        }
    }
    metrics.exactMatch = metrics.maximumChannelError == 0U;
    return metrics;
}

VisualImage CreateVisualDifferenceImage(const VisualImage& expected,
                                        const VisualImage& actual,
                                        std::uint8_t amplification) {
    VisualImage difference;
    if (expected.width != actual.width || expected.height != actual.height ||
        expected.pixels.size() != actual.pixels.size()) {
        return difference;
    }
    difference.width = expected.width;
    difference.height = expected.height;
    difference.pixels.resize(expected.pixels.size());
    for (std::size_t index = 0; index < expected.pixels.size(); ++index) {
        difference.pixels[index] = DifferencePixel(expected.pixels[index], actual.pixels[index],
                                                   amplification);
    }
    return difference;
}

std::uint64_t VisualImageFnv1a64(const VisualImage& image) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    HashUint32(hash, image.width);
    HashUint32(hash, image.height);
    for (const std::uint32_t pixel : image.pixels) HashUint32(hash, pixel);
    return hash;
}

std::string VisualFixtureStateJson(const VisualFixtureDefinition& fixture) {
    json::Value::Object camera{
        {"centreX", fixture.preset.camera.centreX},
        {"centreXLow", fixture.preset.camera.centreXLow},
        {"centreY", fixture.preset.camera.centreY},
        {"centreYLow", fixture.preset.camera.centreYLow},
        {"scale", fixture.preset.camera.scale},
    };
    json::Value::Object state{
        {"antiAliasingLevel", fixture.preset.antiAliasingLevel},
        {"backgroundColour", ColourJson(fixture.preset.backgroundColour)},
        {"brightness", fixture.preset.brightness},
        {"camera", std::move(camera)},
        {"colourOffset", fixture.preset.colourOffset},
        {"contrast", fixture.preset.contrast},
        {"coverage", fixture.coverage},
        {"customPaletteColours", PaletteColoursJson(fixture.preset.customPaletteColours)},
        {"equation", EquationJson(fixture.preset.equation)},
        {"fixtureId", fixture.id},
        {"fixtureName", fixture.name},
        {"height", static_cast<double>(fixture.height)},
        {"interiorColour", ColourJson(fixture.preset.interiorColour)},
        {"maximumIterations", fixture.preset.maximumIterations},
        {"palette", static_cast<int>(fixture.preset.palette)},
        {"paletteFrequency", fixture.preset.paletteFrequency},
        {"paletteGamma", fixture.preset.paletteGamma},
        {"paletteInterpolation", static_cast<int>(fixture.preset.paletteInterpolation)},
        {"precisionExpectation", fixture.precisionExpectation},
        {"productionRenderer", "RenderStillImageTiled"},
        {"rotationDegrees", fixture.preset.rotationDegrees},
        {"saturation", fixture.preset.saturation},
        {"scaleQualityToResolution", false},
        {"smoothColouring", fixture.preset.smoothColouring},
        {"tiledWidth", static_cast<double>(fixture.tiledWidth)},
        {"timeSeconds", fixture.timeSeconds},
        {"width", static_cast<double>(fixture.width)},
    };
    return json::Stringify(json::Value(std::move(state)), true);
}

std::string HexDigest(std::uint64_t value) {
    std::ostringstream stream;
    stream << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}

} // namespace mw
