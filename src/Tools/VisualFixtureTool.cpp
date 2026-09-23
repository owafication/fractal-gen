#include "Core/Json.h"
#include "Core/FractalScout.h"
#include "Core/ProjectState.h"
#include "Core/VisualRegression.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#ifndef MW_PROJECT_VERSION
#define MW_PROJECT_VERSION "unknown"
#endif
#ifndef MW_COMPILER_ID
#define MW_COMPILER_ID "unknown"
#endif
#ifndef MW_COMPILER_VERSION
#define MW_COMPILER_VERSION "unknown"
#endif
#ifndef MW_SYSTEM_NAME
#define MW_SYSTEM_NAME "unknown"
#endif
#ifndef MW_SOURCE_REVISION
#define MW_SOURCE_REVISION "unavailable"
#endif

namespace {

namespace fs = std::filesystem;

struct Options {
    fs::path outputDirectory{"test_artifacts/visual"};
    std::optional<fs::path> baselineDirectory;
    std::string fixtureId;
    unsigned baselineMaximumChannelError{0U};
    double baselineMaximumDifferingRatio{0.0};
    double baselineMinimumStructuralSimilarity{1.0};
    bool runSeamCheck{true};
    bool runMutationCheck{true};
    bool runScoutThumbnailCheck{true};
};

struct CheckRecord {
    std::string id;
    std::string status;
    std::string detail;
};

std::uint8_t Red(std::uint32_t pixel) noexcept {
    return static_cast<std::uint8_t>((pixel >> 16U) & 0xFFU);
}

std::uint8_t Green(std::uint32_t pixel) noexcept {
    return static_cast<std::uint8_t>((pixel >> 8U) & 0xFFU);
}

std::uint8_t Blue(std::uint32_t pixel) noexcept {
    return static_cast<std::uint8_t>(pixel & 0xFFU);
}

bool EnsureParentDirectory(const fs::path& path, std::string& error) {
    std::error_code code;
    const fs::path parent = path.parent_path();
    if (!parent.empty()) fs::create_directories(parent, code);
    if (code) {
        error = "Could not create artifact directory '" + parent.string() + "': " + code.message();
        return false;
    }
    return true;
}

bool PromoteTemporaryFile(const fs::path& temporary, const fs::path& finalPath,
                          std::string& error) {
    std::error_code code;
    fs::remove(finalPath, code);
    code.clear();
    fs::rename(temporary, finalPath, code);
    if (code) {
        fs::remove(temporary);
        error = "Could not promote artifact '" + finalPath.string() + "': " + code.message();
        return false;
    }
    return true;
}

bool WriteTextAtomically(const fs::path& path, const std::string& text, std::string& error) {
    if (!EnsureParentDirectory(path, error)) return false;
    const fs::path temporary = path.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = "Could not open temporary artifact '" + temporary.string() + "'.";
            return false;
        }
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.flush();
        if (!stream) {
            stream.close();
            fs::remove(temporary);
            error = "Could not write artifact '" + path.string() + "'.";
            return false;
        }
    }
    return PromoteTemporaryFile(temporary, path, error);
}

bool WritePpmAtomically(const fs::path& path, const mw::VisualImage& image, std::string& error) {
    const std::uint64_t expectedPixels = static_cast<std::uint64_t>(image.width) * image.height;
    if (image.width == 0U || image.height == 0U || image.pixels.size() != expectedPixels) {
        error = "Cannot write an invalid visual image.";
        return false;
    }
    if (!EnsureParentDirectory(path, error)) return false;
    const fs::path temporary = path.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = "Could not open temporary image '" + temporary.string() + "'.";
            return false;
        }
        stream << "P6\n" << image.width << ' ' << image.height << "\n255\n";
        for (const std::uint32_t pixel : image.pixels) {
            const std::array<char, 3> rgb{
                static_cast<char>(Red(pixel)),
                static_cast<char>(Green(pixel)),
                static_cast<char>(Blue(pixel)),
            };
            stream.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        }
        stream.flush();
        if (!stream) {
            stream.close();
            fs::remove(temporary);
            error = "Could not write image artifact '" + path.string() + "'.";
            return false;
        }
    }
    return PromoteTemporaryFile(temporary, path, error);
}

bool ReadPpmToken(std::istream& stream, std::string& token) {
    token.clear();
    char character = 0;
    while (stream.get(character)) {
        if (character == '#') {
            stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }
        if (!std::isspace(static_cast<unsigned char>(character))) {
            token.push_back(character);
            break;
        }
    }
    if (token.empty()) return false;
    while (stream.get(character)) {
        if (std::isspace(static_cast<unsigned char>(character))) break;
        token.push_back(character);
        if (token.size() > 64U) return false;
    }
    return true;
}

bool ParseUnsignedToken(const std::string& token, unsigned long& value) {
    try {
        std::size_t consumed = 0U;
        value = std::stoul(token, &consumed, 10);
        return consumed == token.size();
    } catch (...) {
        return false;
    }
}

bool ReadPpm(const fs::path& path, mw::VisualImage& image, std::string& error) {
    image = {};
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "Could not open baseline image '" + path.string() + "'.";
        return false;
    }
    std::string magic;
    std::string widthToken;
    std::string heightToken;
    std::string maximumToken;
    if (!ReadPpmToken(stream, magic) || !ReadPpmToken(stream, widthToken) ||
        !ReadPpmToken(stream, heightToken) || !ReadPpmToken(stream, maximumToken) ||
        magic != "P6") {
        error = "Baseline image is not a supported binary PPM file: " + path.string();
        return false;
    }
    unsigned long width = 0UL;
    unsigned long height = 0UL;
    unsigned long maximum = 0UL;
    if (!ParseUnsignedToken(widthToken, width) || !ParseUnsignedToken(heightToken, height) ||
        !ParseUnsignedToken(maximumToken, maximum) || maximum != 255UL || width == 0UL ||
        height == 0UL || width > 16384UL || height > 16384UL) {
        error = "Baseline PPM dimensions or channel range are invalid: " + path.string();
        return false;
    }
    const std::uint64_t pixelCount = static_cast<std::uint64_t>(width) * height;
    if (pixelCount > 100000000ULL ||
        pixelCount > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        error = "Baseline PPM exceeds the bounded fixture image limit: " + path.string();
        return false;
    }

    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.pixels.resize(static_cast<std::size_t>(pixelCount));
    for (std::size_t index = 0U; index < image.pixels.size(); ++index) {
        std::array<unsigned char, 3> rgb{};
        stream.read(reinterpret_cast<char*>(rgb.data()), static_cast<std::streamsize>(rgb.size()));
        if (!stream) {
            image = {};
            error = "Baseline PPM pixel data is truncated: " + path.string();
            return false;
        }
        image.pixels[index] = 0xFF000000U |
            (static_cast<std::uint32_t>(rgb[0]) << 16U) |
            (static_cast<std::uint32_t>(rgb[1]) << 8U) |
            static_cast<std::uint32_t>(rgb[2]);
    }
    return true;
}

mw::json::Value MetricsJson(const mw::VisualComparisonMetrics& metrics,
                            const mw::VisualImage& expected,
                            const mw::VisualImage& actual) {
    return mw::json::Value::Object{
        {"actualImageHash", mw::HexDigest(mw::VisualImageFnv1a64(actual))},
        {"differingPixelRatio", metrics.differingPixelRatio},
        {"differingPixels", static_cast<double>(metrics.differingPixels)},
        {"dimensionsMatch", metrics.dimensionsMatch},
        {"exactMatch", metrics.exactMatch},
        {"expectedImageHash", mw::HexDigest(mw::VisualImageFnv1a64(expected))},
        {"maximumChannelError", static_cast<int>(metrics.maximumChannelError)},
        {"meanAbsoluteChannelError", metrics.meanAbsoluteChannelError},
        {"pixelCount", static_cast<double>(metrics.pixelCount)},
        {"rootMeanSquareChannelError", metrics.rootMeanSquareChannelError},
        {"structuralSimilarity", metrics.structuralSimilarity},
    };
}

mw::json::Value StatisticsJson(const mw::StillRenderStatistics& statistics) {
    return mw::json::Value::Object{
        {"antiAliasingLevel", statistics.antiAliasingLevel},
        {"maximumIterations", statistics.maximumIterations},
        {"peakWorkingPixels", static_cast<double>(statistics.peakWorkingPixels)},
        {"renderedPixels", static_cast<double>(statistics.renderedPixels)},
        {"tileHeight", static_cast<double>(statistics.tileHeight)},
        {"tileWidth", static_cast<double>(statistics.tileWidth)},
    };
}

mw::RenderFingerprintContext FixtureFingerprintContext(
    const mw::VisualFixtureDefinition& fixture) {
    mw::RenderFingerprintContext context;
    context.rendererId = "cpu-production-still";
    context.width = fixture.width;
    context.height = fixture.height;
    context.precision.mode = mw::PrecisionMode::Float64;
    context.precision.allowFloat64 = true;
    context.precision.allowSplitFloat = false;
    context.precision.allowPerturbation = false;
    context.precision.allowArbitraryPrecision = false;
    context.precision.automaticFallback = false;
    context.precision.arbitraryPrecisionBits = 0;
    context.timeSeconds = fixture.timeSeconds;
    context.seed = 0U;
    context.scaleQualityToResolution = false;
    return context;
}

std::string EnvironmentJson(const mw::StillRenderStatistics& statistics,
                            const mw::RenderFingerprint& fingerprint) {
    const mw::json::Value environment(mw::json::Value::Object{
        {"applicationVersion", MW_PROJECT_VERSION},
        {"backend", "cpu-production-still"},
        {"compilerId", MW_COMPILER_ID},
        {"compilerVersion", MW_COMPILER_VERSION},
        {"cplusplus", static_cast<double>(__cplusplus)},
        {"pointerBits", static_cast<double>(sizeof(void*) * 8U)},
        {"productionRenderer", "RenderStillImageTiled"},
        {"renderFingerprint", fingerprint.digest},
        {"renderFingerprintAlgorithm", fingerprint.algorithm},
        {"renderFingerprintCanonicalVersion", fingerprint.canonicalVersion},
        {"sourceRevision", MW_SOURCE_REVISION},
        {"statistics", StatisticsJson(statistics)},
        {"systemName", MW_SYSTEM_NAME},
    });
    return mw::json::Stringify(environment, true);
}

bool WriteComparisonArtifacts(const fs::path& directory,
                              const std::string& expectedName,
                              const mw::VisualImage& expected,
                              const std::string& actualName,
                              const mw::VisualImage& actual,
                              const mw::VisualComparisonMetrics& metrics,
                              std::string& error) {
    if (!WritePpmAtomically(directory / (expectedName + ".ppm"), expected, error) ||
        !WritePpmAtomically(directory / (actualName + ".ppm"), actual, error)) {
        return false;
    }
    const mw::VisualImage difference = mw::CreateVisualDifferenceImage(expected, actual);
    if (!difference.pixels.empty() &&
        !WritePpmAtomically(directory / "diff.ppm", difference, error)) {
        return false;
    }
    return WriteTextAtomically(directory / "metrics.json",
                               mw::json::Stringify(MetricsJson(metrics, expected, actual), true) + "\n",
                               error);
}

bool BaselineAccepted(const mw::VisualComparisonMetrics& metrics, const Options& options) {
    return metrics.dimensionsMatch &&
           static_cast<unsigned>(metrics.maximumChannelError) <=
               options.baselineMaximumChannelError &&
           metrics.differingPixelRatio <= options.baselineMaximumDifferingRatio &&
           metrics.structuralSimilarity >= options.baselineMinimumStructuralSimilarity;
}

bool RunFixture(const mw::VisualFixtureDefinition& fixture, const Options& options,
                CheckRecord& record) {
    record.id = fixture.id;
    const fs::path artifactDirectory =
        options.outputDirectory / fixture.id / "cpu" / "candidate";
    mw::VisualImage current;
    mw::VisualImage repeat;
    mw::StillRenderStatistics currentStatistics;
    mw::StillRenderStatistics repeatStatistics;
    mw::RenderFingerprint fingerprint;
    std::string error;
    if (!mw::BuildRenderFingerprint(fixture.preset, FixtureFingerprintContext(fixture),
                                    fingerprint, error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    if (!mw::RenderVisualFixture(fixture, fixture.tiledWidth, current, currentStatistics, error) ||
        !mw::RenderVisualFixture(fixture, fixture.tiledWidth, repeat, repeatStatistics, error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }

    const mw::VisualComparisonMetrics repeatability = mw::CompareVisualImages(current, repeat);
    if (!WriteComparisonArtifacts(artifactDirectory, "current", current, "repeat", repeat,
                                  repeatability, error) ||
        !WriteTextAtomically(artifactDirectory / "state.json",
                             mw::VisualFixtureStateJson(fixture) + "\n", error) ||
        !WriteTextAtomically(artifactDirectory / "render-state.canonical",
                             fingerprint.canonicalState, error) ||
        !WriteTextAtomically(artifactDirectory / "environment.json",
                             EnvironmentJson(currentStatistics, fingerprint) + "\n", error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    if (!repeatability.exactMatch) {
        record.status = "failed";
        record.detail = "Repeated CPU production renders were not pixel-identical.";
        return false;
    }

    if (options.baselineDirectory) {
        const fs::path baselinePath = *options.baselineDirectory / fixture.id / "cpu" / "baseline.ppm";
        mw::VisualImage baseline;
        if (!ReadPpm(baselinePath, baseline, error)) {
            record.status = "failed";
            record.detail = error;
            return false;
        }
        const auto baselineMetrics = mw::CompareVisualImages(baseline, current);
        const fs::path baselineArtifacts = artifactDirectory / "baseline-comparison";
        if (!WriteComparisonArtifacts(baselineArtifacts, "baseline", baseline,
                                      "current", current, baselineMetrics, error)) {
            record.status = "failed";
            record.detail = error;
            return false;
        }
        if (!BaselineAccepted(baselineMetrics, options)) {
            record.status = "failed";
            record.detail = "Current output exceeded the explicitly supplied baseline thresholds.";
            return false;
        }
    }

    record.status = "passed";
    record.detail = "Exact same-process CPU repeatability passed; diagnostic candidate artifacts written.";
    return true;
}

mw::VisualImage ExtractVerticalSeamStrips(const mw::VisualImage& image,
                                          std::uint32_t tileWidth) {
    mw::VisualImage strips;
    if (image.width == 0U || image.height == 0U || tileWidth == 0U ||
        image.pixels.size() != static_cast<std::uint64_t>(image.width) * image.height) {
        return strips;
    }
    std::vector<std::uint32_t> sourceColumns;
    for (std::uint32_t boundary = tileWidth; boundary < image.width; boundary += tileWidth) {
        if (boundary > 0U) sourceColumns.push_back(boundary - 1U);
        sourceColumns.push_back(boundary);
        if (boundary + 1U < image.width) sourceColumns.push_back(boundary + 1U);
    }
    if (sourceColumns.empty()) return strips;
    strips.width = static_cast<std::uint32_t>(sourceColumns.size());
    strips.height = image.height;
    strips.pixels.resize(static_cast<std::size_t>(strips.width) * strips.height);
    for (std::uint32_t y = 0U; y < image.height; ++y) {
        const std::size_t sourceRow = static_cast<std::size_t>(y) * image.width;
        const std::size_t targetRow = static_cast<std::size_t>(y) * strips.width;
        for (std::size_t x = 0U; x < sourceColumns.size(); ++x) {
            strips.pixels[targetRow + x] = image.pixels[sourceRow + sourceColumns[x]];
        }
    }
    return strips;
}

bool RunSeamCheck(const mw::VisualFixtureDefinition& fixture, const Options& options,
                  CheckRecord& record) {
    record.id = "tiled-versus-full-seams";
    mw::VisualImage full;
    mw::VisualImage tiled;
    mw::StillRenderStatistics fullStatistics;
    mw::StillRenderStatistics tiledStatistics;
    std::string error;
    if (!mw::RenderVisualFixture(fixture, fixture.width, full, fullStatistics, error) ||
        !mw::RenderVisualFixture(fixture, fixture.tiledWidth, tiled, tiledStatistics, error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    const auto globalMetrics = mw::CompareVisualImages(full, tiled);
    const mw::VisualImage fullStrips = ExtractVerticalSeamStrips(full, fixture.tiledWidth);
    const mw::VisualImage tiledStrips = ExtractVerticalSeamStrips(tiled, fixture.tiledWidth);
    const auto seamMetrics = mw::CompareVisualImages(fullStrips, tiledStrips);
    const fs::path directory = options.outputDirectory / "tiled-versus-full" / "cpu" / "candidate";
    if (!WriteComparisonArtifacts(directory, "full", full, "tiled", tiled, globalMetrics, error) ||
        !WriteComparisonArtifacts(directory / "seam-strips", "full-seams", fullStrips,
                                  "tiled-seams", tiledStrips, seamMetrics, error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    const mw::json::Value metadata(mw::json::Value::Object{
        {"fixtureId", fixture.id},
        {"fullStatistics", StatisticsJson(fullStatistics)},
        {"seamStripColumns", static_cast<double>(fullStrips.width)},
        {"tileWidth", static_cast<double>(fixture.tiledWidth)},
        {"tiledStatistics", StatisticsJson(tiledStatistics)},
    });
    if (!WriteTextAtomically(directory / "state.json",
                             mw::json::Stringify(metadata, true) + "\n", error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    if (!globalMetrics.exactMatch || !seamMetrics.exactMatch) {
        record.status = "failed";
        record.detail = "Tiled/full output or dedicated seam strips differed.";
        return false;
    }
    record.status = "passed";
    record.detail = "Full-width and tiled production renders, including boundary strips, matched exactly.";
    return true;
}

bool RunMutationCheck(const mw::VisualFixtureDefinition& source, const Options& options,
                      CheckRecord& record) {
    record.id = "deliberate-visual-mutation";
    mw::VisualFixtureDefinition mutated = source;
    mutated.id += "-mutated";
    mutated.name += " (Deliberate Mutation)";
    mutated.preset.colourOffset += 0.125;
    mutated.preset.equation.depthStrength =
        std::min(1.0, mutated.preset.equation.depthStrength + 0.2);

    mw::VisualImage expected;
    mw::VisualImage actual;
    mw::StillRenderStatistics expectedStatistics;
    mw::StillRenderStatistics actualStatistics;
    mw::RenderFingerprint expectedFingerprint;
    mw::RenderFingerprint mutatedFingerprint;
    std::string error;
    if (!mw::BuildRenderFingerprint(source.preset, FixtureFingerprintContext(source),
                                    expectedFingerprint, error) ||
        !mw::BuildRenderFingerprint(mutated.preset, FixtureFingerprintContext(mutated),
                                    mutatedFingerprint, error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    if (!mw::RenderVisualFixture(source, source.tiledWidth, expected, expectedStatistics, error) ||
        !mw::RenderVisualFixture(mutated, mutated.tiledWidth, actual, actualStatistics, error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    const auto metrics = mw::CompareVisualImages(expected, actual);
    const fs::path directory = options.outputDirectory / "deliberate-mutation" / "cpu" / "candidate";
    if (!WriteComparisonArtifacts(directory, "expected", expected, "mutated", actual,
                                  metrics, error) ||
        !WriteTextAtomically(directory / "expected-state.json",
                             mw::VisualFixtureStateJson(source) + "\n", error) ||
        !WriteTextAtomically(directory / "mutated-state.json",
                             mw::VisualFixtureStateJson(mutated) + "\n", error) ||
        !WriteTextAtomically(directory / "expected-render-state.canonical",
                             expectedFingerprint.canonicalState, error) ||
        !WriteTextAtomically(directory / "mutated-render-state.canonical",
                             mutatedFingerprint.canonicalState, error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }
    if (expectedFingerprint.digest == mutatedFingerprint.digest) {
        record.status = "failed";
        record.detail = "The canonical render fingerprint did not detect the deliberate mutation.";
        return false;
    }
    if (metrics.exactMatch || metrics.differingPixels == 0U) {
        record.status = "failed";
        record.detail = "The deliberate palette/depth mutation was not detected.";
        return false;
    }
    record.status = "passed";
    record.detail = "The deliberate render-affecting mutation produced an actionable non-zero diff.";
    return true;
}

mw::VisualImage ScoutThumbnailImage(const mw::StillRenderPreview& thumbnail) {
    return {thumbnail.width, thumbnail.height, thumbnail.pixels};
}

mw::json::Value ScoutCandidateJson(const mw::FractalScoutCandidate& candidate,
                                   std::size_t rank,
                                   const mw::VisualImage& image) {
    mw::json::Value::Object exactCamera;
    if (candidate.preset.exactCamera) {
        exactCamera = {
            {"centreX", candidate.preset.exactCamera->centreX.CanonicalText()},
            {"centreY", candidate.preset.exactCamera->centreY.CanonicalText()},
            {"halfHeight", candidate.preset.exactCamera->halfHeight.CanonicalText()},
        };
    }
    return mw::json::Value::Object{
        {"camera", mw::json::Value::Object{
            {"centreX", candidate.preset.camera.centreX},
            {"centreXLow", candidate.preset.camera.centreXLow},
            {"centreY", candidate.preset.camera.centreY},
            {"centreYLow", candidate.preset.camera.centreYLow},
            {"scale", candidate.preset.camera.scale},
        }},
        {"exactCamera", mw::json::Value(std::move(exactCamera))},
        {"identity", candidate.identity},
        {"imageHash", mw::HexDigest(mw::VisualImageFnv1a64(image))},
        {"metrics", mw::json::Value::Object{
            {"boundaryMix", candidate.metrics.boundaryMix},
            {"detail", candidate.metrics.detail},
            {"edgeDensity", candidate.metrics.edgeDensity},
            {"iterationVariance", candidate.metrics.iterationVariance},
            {"symmetry", candidate.metrics.symmetry},
        }},
        {"rank", static_cast<double>(rank)},
        {"score", candidate.score},
    };
}

bool RunScoutThumbnailCheck(const Options& options, CheckRecord& record) {
    record.id = "fractal-scout-thumbnails";
    mw::FractalScoutRequest request;
    request.preset = mw::BuiltInPresets().front();
    request.searchCamera = request.preset.camera;
    request.candidatePoolSize = 12U;
    request.resultCount = 3U;
    request.sampleGridWidth = 7U;
    request.sampleGridHeight = 5U;
    request.thumbnailWidth = 64U;
    request.thumbnailHeight = 40U;
    request.maximumIterations = 64;
    request.searchRadius = 0.8;
    request.refinementScale = 0.4;
    request.scaleBandCount = 3U;
    request.scaleBandSpread = 0.5;
    request.minimumResultSeparation = 0.45;
    request.goal = mw::FractalScoutGoal::Filaments;

    mw::FractalScoutResult current;
    mw::FractalScoutResult repeat;
    std::string error;
    if (!mw::RunFractalScout(request, {}, [] { return false; }, current, error) ||
        !mw::RunFractalScout(request, {}, [] { return false; }, repeat, error)) {
        record.status = "failed";
        record.detail = "The bounded production Scout search failed: " + error;
        return false;
    }
    if (current.cancelled || repeat.cancelled ||
        current.evaluatedCandidates != request.candidatePoolSize ||
        repeat.evaluatedCandidates != request.candidatePoolSize ||
        current.candidates.size() != request.resultCount ||
        repeat.candidates.size() != current.candidates.size()) {
        record.status = "failed";
        record.detail = "The Scout thumbnail fixture returned incomplete bounded results.";
        return false;
    }

    const fs::path directory =
        options.outputDirectory / "fractal-scout-thumbnails" / "cpu" / "candidate";
    mw::json::Value::Array candidates;
    std::vector<std::uint64_t> imageHashes;
    imageHashes.reserve(current.candidates.size());
    for (std::size_t index = 0U; index < current.candidates.size(); ++index) {
        const mw::FractalScoutCandidate& expectedCandidate = current.candidates[index];
        const mw::FractalScoutCandidate& actualCandidate = repeat.candidates[index];
        const mw::VisualImage expected = ScoutThumbnailImage(expectedCandidate.thumbnail);
        const mw::VisualImage actual = ScoutThumbnailImage(actualCandidate.thumbnail);
        const mw::VisualComparisonMetrics metrics = mw::CompareVisualImages(expected, actual);
        if (expectedCandidate.identity != actualCandidate.identity ||
            expectedCandidate.score != actualCandidate.score || !metrics.exactMatch ||
            expected.pixels.empty() ||
            std::all_of(expected.pixels.begin(), expected.pixels.end(),
                        [&](std::uint32_t pixel) { return pixel == expected.pixels.front(); })) {
            record.status = "failed";
            record.detail = "Scout candidate identity, score or non-uniform thumbnail output was not exactly repeatable.";
            return false;
        }
        const fs::path candidateDirectory = directory / ("rank-" + std::to_string(index + 1U));
        if (!WriteComparisonArtifacts(candidateDirectory, "current", expected,
                                      "repeat", actual, metrics, error)) {
            record.status = "failed";
            record.detail = error;
            return false;
        }
        imageHashes.push_back(mw::VisualImageFnv1a64(expected));
        candidates.push_back(ScoutCandidateJson(expectedCandidate, index + 1U, expected));
    }
    std::sort(imageHashes.begin(), imageHashes.end());
    if (std::unique(imageHashes.begin(), imageHashes.end()) == imageHashes.begin() + 1) {
        record.status = "failed";
        record.detail = "All retained Scout candidates produced the same thumbnail image.";
        return false;
    }

    const mw::FractalScoutLimits limits = mw::ResolveFractalScoutLimits(request);
    const mw::json::Value state(mw::json::Value::Object{
        {"backend", "cpu-production-still"},
        {"candidateIdentityVersion", "FRACTAL-SCOUT-CANDIDATE/V2-EXACT-CAMERA"},
        {"candidates", std::move(candidates)},
        {"compilerId", MW_COMPILER_ID},
        {"compilerVersion", MW_COMPILER_VERSION},
        {"fixtureId", record.id},
        {"goal", "filaments"},
        {"limits", mw::json::Value::Object{
            {"candidatePoolSize", static_cast<double>(limits.candidatePoolSize)},
            {"maximumIterations", limits.maximumIterations},
            {"resultCount", static_cast<double>(limits.resultCount)},
            {"sampleGridHeight", static_cast<double>(limits.sampleGridHeight)},
            {"sampleGridWidth", static_cast<double>(limits.sampleGridWidth)},
            {"thumbnailHeight", static_cast<double>(limits.thumbnailHeight)},
            {"thumbnailWidth", static_cast<double>(limits.thumbnailWidth)},
        }},
        {"productionRenderer", "RunFractalScout / RenderStillImageTiled"},
        {"sourceRevision", MW_SOURCE_REVISION},
        {"suppressedNearDuplicates", static_cast<double>(current.suppressedNearDuplicates)},
        {"systemName", MW_SYSTEM_NAME},
        {"version", MW_PROJECT_VERSION},
    });
    if (!WriteTextAtomically(directory / "state.json",
                             mw::json::Stringify(state, true) + "\n", error)) {
        record.status = "failed";
        record.detail = error;
        return false;
    }

    record.status = "passed";
    record.detail = "Three deterministic production Scout candidates retained distinct, non-uniform, exactly repeatable thumbnails and identities.";
    return true;
}

void PrintUsage() {
    std::cout
        << "MandelbrotVisualFixtures [options]\n"
        << "  --output-dir <path>                 Artifact root (default: test_artifacts/visual)\n"
        << "  --fixture <id>                     Run one canonical fixture\n"
        << "  --baseline-dir <path>              Compare <path>/<fixture>/cpu/baseline.ppm\n"
        << "  --baseline-max-channel-error <0-255>\n"
        << "  --baseline-max-differing-ratio <0-1>\n"
        << "  --baseline-minimum-ssim <-1-1>\n"
        << "  --no-seam-check                    Skip tiled/full seam validation\n"
        << "  --no-mutation-check                Skip deliberate mutation validation\n"
        << "  --no-scout-thumbnail-check         Skip deterministic Scout thumbnail validation\n"
        << "  --help                              Show this message\n";
}

bool RequireValue(int argc, char** argv, int& index, std::string& value, std::string& error) {
    if (index + 1 >= argc) {
        error = std::string("Missing value for ") + argv[index] + '.';
        return false;
    }
    value = argv[++index];
    return true;
}

bool ParseOptions(int argc, char** argv, Options& options, bool& showHelp, std::string& error) {
    showHelp = false;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        std::string value;
        if (argument == "--help" || argument == "-h") {
            showHelp = true;
            return true;
        }
        if (argument == "--no-seam-check") {
            options.runSeamCheck = false;
            continue;
        }
        if (argument == "--no-mutation-check") {
            options.runMutationCheck = false;
            continue;
        }
        if (argument == "--no-scout-thumbnail-check") {
            options.runScoutThumbnailCheck = false;
            continue;
        }
        if (!RequireValue(argc, argv, index, value, error)) return false;
        try {
            if (argument == "--output-dir") {
                options.outputDirectory = value;
            } else if (argument == "--fixture") {
                options.fixtureId = value;
            } else if (argument == "--baseline-dir") {
                options.baselineDirectory = fs::path(value);
            } else if (argument == "--baseline-max-channel-error") {
                const unsigned long parsed = std::stoul(value);
                if (parsed > 255UL) throw std::out_of_range("channel error");
                options.baselineMaximumChannelError = static_cast<unsigned>(parsed);
            } else if (argument == "--baseline-max-differing-ratio") {
                options.baselineMaximumDifferingRatio = std::stod(value);
                if (options.baselineMaximumDifferingRatio < 0.0 ||
                    options.baselineMaximumDifferingRatio > 1.0) {
                    throw std::out_of_range("differing ratio");
                }
            } else if (argument == "--baseline-minimum-ssim") {
                options.baselineMinimumStructuralSimilarity = std::stod(value);
                if (options.baselineMinimumStructuralSimilarity < -1.0 ||
                    options.baselineMinimumStructuralSimilarity > 1.0) {
                    throw std::out_of_range("ssim");
                }
            } else {
                error = "Unknown option: " + argument;
                return false;
            }
        } catch (...) {
            error = "Invalid value for " + argument + ": " + value;
            return false;
        }
    }
    if (options.outputDirectory.empty()) {
        error = "The output directory may not be empty.";
        return false;
    }
    return true;
}

std::string SummaryJson(const std::vector<CheckRecord>& records, bool passed) {
    mw::json::Value::Array checks;
    checks.reserve(records.size());
    for (const auto& record : records) {
        checks.emplace_back(mw::json::Value::Object{
            {"detail", record.detail},
            {"id", record.id},
            {"status", record.status},
        });
    }
    return mw::json::Stringify(mw::json::Value(mw::json::Value::Object{
        {"backend", "cpu-production-still"},
        {"checks", std::move(checks)},
        {"passed", passed},
        {"sourceRevision", MW_SOURCE_REVISION},
        {"version", MW_PROJECT_VERSION},
    }), true);
}

} // namespace

int main(int argc, char** argv) {
    Options options;
    bool showHelp = false;
    std::string error;
    if (!ParseOptions(argc, argv, options, showHelp, error)) {
        std::cerr << "ERROR: " << error << "\n\n";
        PrintUsage();
        return 2;
    }
    if (showHelp) {
        PrintUsage();
        return 0;
    }

    const auto allFixtures = mw::BuiltInVisualFixtures();
    if (allFixtures.size() != 4U) {
        std::cerr << "ERROR: Expected four canonical visual fixtures, found "
                  << allFixtures.size() << ".\n";
        return 1;
    }
    std::vector<mw::VisualFixtureDefinition> selected;
    if (options.fixtureId.empty()) {
        selected = allFixtures;
    } else {
        const auto found = std::find_if(allFixtures.begin(), allFixtures.end(),
            [&](const mw::VisualFixtureDefinition& fixture) {
                return fixture.id == options.fixtureId;
            });
        if (found == allFixtures.end()) {
            std::cerr << "ERROR: Unknown fixture id: " << options.fixtureId << "\n";
            return 2;
        }
        selected.push_back(*found);
    }

    std::vector<CheckRecord> records;
    bool passed = true;
    for (const auto& fixture : selected) {
        CheckRecord record;
        const bool fixturePassed = RunFixture(fixture, options, record);
        passed = fixturePassed && passed;
        std::cout << (fixturePassed ? "PASS: " : "FAIL: ") << record.id
                  << " - " << record.detail << '\n';
        records.push_back(std::move(record));
    }

    if (options.runSeamCheck) {
        const auto seamFixture = std::find_if(allFixtures.begin(), allFixtures.end(),
            [](const mw::VisualFixtureDefinition& fixture) {
                return fixture.id == "rotated-bloom-state";
            });
        CheckRecord record;
        const bool seamPassed = seamFixture != allFixtures.end() &&
                                RunSeamCheck(*seamFixture, options, record);
        if (seamFixture == allFixtures.end()) {
            record.id = "tiled-versus-full-seams";
            record.status = "failed";
            record.detail = "The seam fixture definition is missing.";
        }
        passed = seamPassed && passed;
        std::cout << (seamPassed ? "PASS: " : "FAIL: ") << record.id
                  << " - " << record.detail << '\n';
        records.push_back(std::move(record));
    }

    if (options.runMutationCheck) {
        CheckRecord record;
        const bool mutationPassed = RunMutationCheck(allFixtures.front(), options, record);
        passed = mutationPassed && passed;
        std::cout << (mutationPassed ? "PASS: " : "FAIL: ") << record.id
                  << " - " << record.detail << '\n';
        records.push_back(std::move(record));
    }

    if (options.runScoutThumbnailCheck) {
        CheckRecord record;
        const bool scoutPassed = RunScoutThumbnailCheck(options, record);
        passed = scoutPassed && passed;
        std::cout << (scoutPassed ? "PASS: " : "FAIL: ") << record.id
                  << " - " << record.detail << '\n';
        records.push_back(std::move(record));
    }

    const std::string summary = SummaryJson(records, passed) + "\n";
    if (!WriteTextAtomically(options.outputDirectory / "summary.json", summary, error)) {
        std::cerr << "ERROR: " << error << '\n';
        return 1;
    }
    std::cout << "Artifacts: " << fs::absolute(options.outputDirectory).string() << '\n';
    return passed ? 0 : 1;
}
