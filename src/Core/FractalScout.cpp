#include "Core/FractalScout.h"

#include "Core/MandelbrotMath.h"
#include "Core/DeepZoom.h"
#include "Core/ProjectState.h"
#include "Core/Precision/ExactCameraAdapter.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <iomanip>
#include <sstream>
#include <utility>

namespace mw {
namespace {

constexpr double kGoldenAngle = 2.39996322972865332;

bool Cancelled(const FractalScoutCancellationCallback& callback) {
    return callback && callback();
}

void Report(const FractalScoutProgressCallback& callback,
            std::uint32_t completed,
            std::uint32_t total,
            bool thumbnails) {
    if (callback) callback({completed, total, thumbnails});
}

double ClampFinite(double value, double minimum, double maximum, double fallback) {
    return std::isfinite(value) ? std::clamp(value, minimum, maximum) : fallback;
}

struct ScoredPoint {
    CameraState camera;
    FractalScoutMetrics metrics;
    double score{0.0};
    std::uint32_t stableIndex{0};
};

CameraState CandidateCamera(const CameraState& searchCamera,
                            const FractalScoutLimits& limits,
                            std::uint32_t index) {
    CameraState camera = searchCamera;
    const std::uint32_t bands = std::max(1U, limits.scaleBandCount);
    const std::uint32_t nonCentralCount = limits.candidatePoolSize > 0U
        ? limits.candidatePoolSize - 1U : 0U;
    const std::uint32_t spatialCount = std::max(
        1U, (nonCentralCount + bands - 1U) / bands);

    std::uint32_t bandIndex = bands / 2U;
    if (index > 0U) {
        const std::uint32_t sequence = index - 1U;
        const std::uint32_t spatialIndex = sequence / bands + 1U;
        bandIndex = sequence % bands;
        const double radiusFraction = std::sqrt(
            (static_cast<double>(spatialIndex) - 0.5) /
            static_cast<double>(spatialCount));
        const double angle = static_cast<double>(spatialIndex) * kGoldenAngle;
        const double radius = searchCamera.scale * limits.searchRadius * radiusFraction;
        const double aspectSearch = static_cast<double>(limits.thumbnailWidth) /
                                    static_cast<double>(limits.thumbnailHeight);
        AddCompensated(camera.centreX, camera.centreXLow,
                       std::cos(angle) * radius * aspectSearch);
        AddCompensated(camera.centreY, camera.centreYLow,
                       std::sin(angle) * radius);
    }

    double bandPosition = 0.0;
    if (bands > 1U) {
        bandPosition = 2.0 * static_cast<double>(bandIndex) /
                       static_cast<double>(bands - 1U) - 1.0;
    }
    const double scaleFactor = std::exp(bandPosition * limits.scaleBandSpread);
    camera.scale = std::max(
        1.0e-300, searchCamera.scale * limits.refinementScale * scaleFactor);
    return camera;
}

FractalScoutMetrics ScoreCamera(const Preset& preset,
                                const CameraState& camera,
                                const FractalScoutLimits& limits,
                                const FractalScoutCancellationCallback& cancellation,
                                bool& wasCancelled) {
    const std::uint32_t width = limits.sampleGridWidth;
    const std::uint32_t height = limits.sampleGridHeight;
    const std::size_t count = static_cast<std::size_t>(width) * height;
    std::vector<double> values(count, 0.0);
    std::vector<unsigned char> escaped(count, 0U);

    std::size_t escapedCount = 0U;
    double sum = 0.0;
    double squaredSum = 0.0;
    double detailSum = 0.0;
    for (std::uint32_t y = 0; y < height; ++y) {
        if (Cancelled(cancellation)) {
            wasCancelled = true;
            return {};
        }
        for (std::uint32_t x = 0; x < width; ++x) {
            const auto point = MapStillRenderSample(
                camera, preset.rotationDegrees, width, height,
                static_cast<double>(x) + 0.5, static_cast<double>(y) + 0.5);
            const EscapeResult sample = CalculateEscape(
                point.real, point.imaginary, limits.maximumIterations,
                preset.equation, 0.0);
            const std::size_t offset = static_cast<std::size_t>(y) * width + x;
            const double normalised = std::clamp(
                sample.smoothValue / static_cast<double>(limits.maximumIterations),
                0.0, 1.0);
            values[offset] = normalised;
            if (sample.escaped || sample.converged) {
                escaped[offset] = 1U;
                ++escapedCount;
                detailSum += normalised;
            }
            sum += normalised;
            squaredSum += normalised * normalised;
        }
    }

    FractalScoutMetrics metrics;
    const double sampleCount = static_cast<double>(count);
    const double escapedFraction = static_cast<double>(escapedCount) / sampleCount;
    metrics.boundaryMix = std::clamp(4.0 * escapedFraction * (1.0 - escapedFraction), 0.0, 1.0);
    const double mean = sum / sampleCount;
    const double variance = std::max(0.0, squaredSum / sampleCount - mean * mean);
    // A variance around 0.08 is already visually rich for normalised iterations.
    metrics.iterationVariance = std::clamp(variance / 0.08, 0.0, 1.0);
    metrics.detail = escapedCount > 0U
        ? std::clamp((detailSum / static_cast<double>(escapedCount)) * 2.5, 0.0, 1.0)
        : 0.0;

    double edgeSum = 0.0;
    std::size_t edgePairs = 0U;
    for (std::uint32_t y = 0; y < height; ++y) {
        for (std::uint32_t x = 0; x < width; ++x) {
            const std::size_t offset = static_cast<std::size_t>(y) * width + x;
            const auto addEdge = [&](std::size_t other) {
                const double classification = escaped[offset] == escaped[other] ? 0.0 : 1.0;
                const double iterationEdge = std::min(1.0, std::abs(values[offset] - values[other]) * 6.0);
                edgeSum += std::max(classification, iterationEdge);
                ++edgePairs;
            };
            if (x + 1U < width) addEdge(offset + 1U);
            if (y + 1U < height) addEdge(offset + width);
        }
    }
    metrics.edgeDensity = edgePairs > 0U
        ? std::clamp(edgeSum / static_cast<double>(edgePairs), 0.0, 1.0)
        : 0.0;

    if (limits.sampleGridWidth > 1U && limits.sampleGridHeight > 1U) {
        double horizontalDifference = 0.0;
        double rotationalDifference = 0.0;
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const std::size_t offset = static_cast<std::size_t>(y) * width + x;
                const std::size_t mirror = static_cast<std::size_t>(height - 1U - y) * width + x;
                const std::size_t rotate = static_cast<std::size_t>(height - 1U - y) * width +
                                           (width - 1U - x);
                horizontalDifference += std::abs(values[offset] - values[mirror]);
                rotationalDifference += std::abs(values[offset] - values[rotate]);
            }
        }
        const double horizontalSimilarity = 1.0 - horizontalDifference / sampleCount;
        const double rotationalSimilarity = 1.0 - rotationalDifference / sampleCount;
        metrics.symmetry = std::clamp(std::max(horizontalSimilarity, rotationalSimilarity), 0.0, 1.0);
    }
    return metrics;
}

double CandidateSeparation(const CameraState& left,
                           const CameraState& right,
                           const CameraState& searchCamera,
                           const FractalScoutLimits& limits) {
    const double aspect = static_cast<double>(limits.thumbnailWidth) /
                          static_cast<double>(limits.thumbnailHeight);
    const double radiusX = std::max(
        1.0e-300, searchCamera.scale * limits.searchRadius * aspect);
    const double radiusY = std::max(
        1.0e-300, searchCamera.scale * limits.searchRadius);
    const double dx = (CameraCentreX(left) - CameraCentreX(right)) / radiusX;
    const double dy = (CameraCentreY(left) - CameraCentreY(right)) / radiusY;
    const double scaleRange = std::max(0.10, 2.0 * limits.scaleBandSpread);
    const double scaleDistance = std::abs(std::log(left.scale / right.scale)) / scaleRange;
    return std::sqrt(dx * dx + dy * dy + 0.35 * scaleDistance * scaleDistance);
}

std::vector<ScoredPoint> SelectDiverseResults(
    const std::vector<ScoredPoint>& scored,
    const CameraState& searchCamera,
    const FractalScoutLimits& limits,
    std::uint32_t& initialSuppressed) {
    initialSuppressed = 0U;
    const std::size_t wanted = std::min<std::size_t>(
        scored.size(), limits.resultCount);
    if (wanted == 0U) return {};

    auto selectAtThreshold = [&](double threshold, std::uint32_t* suppressed) {
        std::vector<ScoredPoint> selected;
        selected.reserve(wanted);
        std::uint32_t rejected = 0U;
        for (const auto& point : scored) {
            const bool separated = std::all_of(
                selected.begin(), selected.end(), [&](const ScoredPoint& chosen) {
                    return CandidateSeparation(point.camera, chosen.camera,
                                               searchCamera, limits) >= threshold;
                });
            if (separated) {
                selected.push_back(point);
                if (selected.size() == wanted) break;
            } else {
                ++rejected;
            }
        }
        if (suppressed) *suppressed = rejected;
        return selected;
    };

    double threshold = limits.minimumResultSeparation;
    std::vector<ScoredPoint> selected = selectAtThreshold(threshold, &initialSuppressed);
    while (selected.size() < wanted && threshold > 0.01) {
        threshold *= 0.65;
        selected = selectAtThreshold(threshold, nullptr);
    }
    if (selected.size() < wanted) {
        selected.assign(scored.begin(), scored.begin() + static_cast<std::ptrdiff_t>(wanted));
    }
    return selected;
}

bool RenderCandidateThumbnail(const FractalScoutLimits& limits,
                              FractalScoutCandidate& candidate,
                              const FractalScoutCancellationCallback& cancellation,
                              std::string& error) {
    StillRenderRequest renderRequest;
    renderRequest.preset = candidate.preset;
    renderRequest.preset.maximumIterations = limits.maximumIterations;
    renderRequest.preset.antiAliasingLevel = 1;
    // Scout thumbnails prioritise deterministic bounded work. Bloom is a GPU
    // post-process and is not needed for the candidate geometry decision.
    renderRequest.preset.equation.glowStrength = 0.0;
    renderRequest.width = limits.thumbnailWidth;
    renderRequest.height = limits.thumbnailHeight;
    renderRequest.tileWidth = std::min<std::uint32_t>(limits.thumbnailWidth, 128U);
    renderRequest.previewMaximumWidth = limits.thumbnailWidth;
    renderRequest.previewMaximumHeight = limits.thumbnailHeight;
    renderRequest.scaleQualityToResolution = false;

    StillRenderResult rendered;
    const bool succeeded = RenderStillImageTiled(
        renderRequest,
        [](std::uint32_t, std::span<const std::uint32_t>, std::string&) { return true; },
        {}, cancellation, rendered, error);
    if (!succeeded) return false;
    candidate.thumbnail = std::move(rendered.preview);
    return true;
}

bool BuildCandidateIdentity(const FractalScoutRequest& request,
                            const FractalScoutLimits& limits,
                            const Preset& candidate,
                            std::string& identity,
                            std::string& error) {
    RenderFingerprintContext sourceContext;
    sourceContext.rendererId = "cpu-fractal-scout-source";
    sourceContext.width = limits.thumbnailWidth;
    sourceContext.height = limits.thumbnailHeight;
    sourceContext.scaleQualityToResolution = false;
    RenderFingerprint sourceFingerprint;
    Preset sourcePreset = request.preset;
    if (!EnsureExactCamera(sourcePreset, error) ||
        !BuildExactRenderFingerprint(sourcePreset, sourceContext, sourceFingerprint, error)) return false;

    RenderFingerprintContext candidateContext = sourceContext;
    candidateContext.rendererId = "cpu-fractal-scout-thumbnail";
    RenderFingerprint candidateFingerprint;
    Preset thumbnailPreset = candidate;
    thumbnailPreset.maximumIterations = limits.maximumIterations;
    thumbnailPreset.antiAliasingLevel = 1;
    thumbnailPreset.equation.glowStrength = 0.0;
    if (!EnsureExactCamera(thumbnailPreset, error) ||
        !BuildExactRenderFingerprint(thumbnailPreset, candidateContext, candidateFingerprint, error)) return false;

    ExactCamera exactSearchCamera;
    if (!BuildExactCameraFromLegacy(request.searchCamera, exactSearchCamera, error)) return false;

    std::ostringstream context;
    context << std::setprecision(17)
            << "FRACTAL-SCOUT-CANDIDATE/V2-EXACT-CAMERA" << '\0'
            << sourceFingerprint.digest << '\0'
            << candidateFingerprint.digest << '\0'
            << exactSearchCamera.centreX.CanonicalText() << '\0'
            << exactSearchCamera.centreY.CanonicalText() << '\0'
            << exactSearchCamera.halfHeight.CanonicalText() << '\0'
            << static_cast<unsigned int>(request.goal) << '\0'
            << request.includeSymmetryScore << '\0'
            << limits.candidatePoolSize << '\0' << limits.resultCount << '\0'
            << limits.sampleGridWidth << '\0' << limits.sampleGridHeight << '\0'
            << limits.thumbnailWidth << '\0' << limits.thumbnailHeight << '\0'
            << limits.maximumIterations << '\0' << limits.searchRadius << '\0'
            << limits.refinementScale << '\0' << limits.scaleBandCount << '\0'
            << limits.scaleBandSpread << '\0' << limits.minimumResultSeparation;
    identity = Sha256Hex(context.str());
    return true;
}

} // namespace

FractalScoutLimits ResolveFractalScoutLimits(const FractalScoutRequest& request) noexcept {
    FractalScoutLimits limits;
    limits.candidatePoolSize = std::clamp(request.candidatePoolSize, 1U, 96U);
    limits.resultCount = std::clamp(request.resultCount, 1U,
                                    std::min<std::uint32_t>(24U, limits.candidatePoolSize));
    limits.sampleGridWidth = std::clamp(request.sampleGridWidth, 5U, 41U);
    limits.sampleGridHeight = std::clamp(request.sampleGridHeight, 5U, 31U);
    limits.thumbnailWidth = std::clamp(request.thumbnailWidth, 48U, 256U);
    limits.thumbnailHeight = std::clamp(request.thumbnailHeight, 32U, 180U);
    limits.maximumIterations = std::clamp(request.maximumIterations, 32, 768);
    limits.searchRadius = ClampFinite(request.searchRadius, 0.05, 2.5, 0.9);
    limits.refinementScale = ClampFinite(request.refinementScale, 0.05, 0.95, 0.34);
    limits.scaleBandCount = std::clamp(request.scaleBandCount, 1U, 3U);
    limits.scaleBandSpread = ClampFinite(request.scaleBandSpread, 0.0, 0.80, 0.38);
    limits.minimumResultSeparation = ClampFinite(
        request.minimumResultSeparation, 0.0, 0.75, 0.20);

    // Bound retained thumbnail memory to 16 MiB of BGRA pixels.
    constexpr std::uint64_t maximumRetainedBytes = 16ULL * 1024ULL * 1024ULL;
    const std::uint64_t bytesPerThumbnail =
        static_cast<std::uint64_t>(limits.thumbnailWidth) * limits.thumbnailHeight * 4ULL;
    if (bytesPerThumbnail > 0ULL) {
        const std::uint64_t memoryCount = maximumRetainedBytes / bytesPerThumbnail;
        limits.resultCount = std::min<std::uint32_t>(
            limits.resultCount,
            static_cast<std::uint32_t>(std::max<std::uint64_t>(1ULL, memoryCount)));
    }
    return limits;
}


double CalculateFractalScoutScore(const FractalScoutMetrics& metrics,
                                  FractalScoutGoal goal,
                                  bool includeSymmetryScore) noexcept {
    double score = 0.0;
    switch (goal) {
    case FractalScoutGoal::Boundary:
        score = 0.43 * metrics.boundaryMix +
                0.17 * metrics.iterationVariance +
                0.34 * metrics.edgeDensity +
                0.06 * metrics.detail;
        break;
    case FractalScoutGoal::Filaments:
        score = 0.10 * metrics.boundaryMix +
                0.30 * metrics.iterationVariance +
                0.40 * metrics.edgeDensity +
                0.20 * metrics.detail;
        break;
    case FractalScoutGoal::Symmetry:
        score = 0.22 * metrics.boundaryMix +
                0.15 * metrics.iterationVariance +
                0.20 * metrics.edgeDensity +
                0.08 * metrics.detail +
                0.35 * metrics.symmetry;
        break;
    case FractalScoutGoal::Balanced:
    default: {
        const double symmetryWeight = includeSymmetryScore ? 0.08 : 0.0;
        const double baseWeight = 1.0 - symmetryWeight;
        score = baseWeight * (
            0.34 * metrics.boundaryMix +
            0.28 * metrics.iterationVariance +
            0.27 * metrics.edgeDensity +
            0.11 * metrics.detail) +
            symmetryWeight * metrics.symmetry;
        break;
    }
    }
    return std::clamp(score, 0.0, 1.0);
}

bool RunFractalScout(const FractalScoutRequest& request,
                     const FractalScoutProgressCallback& progressCallback,
                     const FractalScoutCancellationCallback& cancellationCallback,
                     FractalScoutResult& result,
                     std::string& error) {
    result = {};
    error.clear();
    const FractalScoutLimits limits = ResolveFractalScoutLimits(request);
    if (request.preset.exactCamera.has_value()) {
        LegacyCameraAdaptation legacySource;
        if (!AdaptExactCameraToLegacy(*request.preset.exactCamera, legacySource, error)) {
            error = "Fractal Scout could not inspect exact camera authority: " + error;
            return false;
        }
        if (legacySource.centreXLoss || legacySource.centreYLoss ||
            legacySource.halfHeightLoss) {
            error = "Fractal Scout cannot search a lossy exact camera until exact Scout coordinates are implemented.";
            return false;
        }
    }
    if (!std::isfinite(request.searchCamera.centreX) ||
        !std::isfinite(request.searchCamera.centreY) ||
        !std::isfinite(request.searchCamera.scale) || request.searchCamera.scale <= 0.0) {
        error = "The Fractal Scout search camera is invalid.";
        return false;
    }

    std::vector<ScoredPoint> scored;
    scored.reserve(limits.candidatePoolSize);
    Report(progressCallback, 0U, limits.candidatePoolSize, false);
    for (std::uint32_t index = 0U; index < limits.candidatePoolSize; ++index) {
        if (Cancelled(cancellationCallback)) {
            result.cancelled = true;
            return true;
        }
        ScoredPoint point;
        point.camera = CandidateCamera(request.searchCamera, limits, index);
        point.stableIndex = index;
        bool wasCancelled = false;
        point.metrics = ScoreCamera(request.preset, point.camera, limits,
                                    cancellationCallback, wasCancelled);
        if (wasCancelled) {
            result.cancelled = true;
            return true;
        }
        point.score = CalculateFractalScoutScore(
            point.metrics, request.goal, request.includeSymmetryScore);
        scored.push_back(point);
        result.evaluatedCandidates = index + 1U;
        Report(progressCallback, index + 1U, limits.candidatePoolSize, false);
    }

    std::stable_sort(scored.begin(), scored.end(), [](const ScoredPoint& left, const ScoredPoint& right) {
        if (left.score != right.score) return left.score > right.score;
        return left.stableIndex < right.stableIndex;
    });
    scored = SelectDiverseResults(scored, request.searchCamera, limits,
                                  result.suppressedNearDuplicates);

    Report(progressCallback, 0U, static_cast<std::uint32_t>(scored.size()), true);
    result.candidates.reserve(scored.size());
    for (std::size_t index = 0U; index < scored.size(); ++index) {
        if (Cancelled(cancellationCallback)) {
            result.cancelled = true;
            result.candidates.clear();
            return true;
        }
        FractalScoutCandidate candidate;
        candidate.preset = request.preset;
        candidate.preset.id.clear();
        candidate.preset.builtIn = false;
        candidate.preset.name = "Fractal Scout Candidate " + std::to_string(index + 1U);
        candidate.preset.camera = scored[index].camera;
        candidate.preset.exactCamera.reset();
        std::string exactCameraError;
        if (!EnsureExactCamera(candidate.preset, exactCameraError)) {
            error = "A Fractal Scout camera could not be represented as exact state: " + exactCameraError;
            return false;
        }
        candidate.preset.startingScale = scored[index].camera.scale;
        candidate.preset.animationMode = AnimationMode::ManualView;
        candidate.metrics = scored[index].metrics;
        candidate.score = scored[index].score;
        if (!BuildCandidateIdentity(request, limits, candidate.preset,
                                    candidate.identity, error)) {
            return false;
        }
        if (!RenderCandidateThumbnail(limits, candidate,
                                      cancellationCallback, error)) {
            if (Cancelled(cancellationCallback)) {
                result.cancelled = true;
                result.candidates.clear();
                error.clear();
                return true;
            }
            return false;
        }
        result.candidates.push_back(std::move(candidate));
        Report(progressCallback, static_cast<std::uint32_t>(index + 1U),
               static_cast<std::uint32_t>(scored.size()), true);
    }
    return true;
}

} // namespace mw
