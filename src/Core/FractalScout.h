#pragma once

#include "Core/Models.h"
#include "Core/StillImageRenderer.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace mw {

enum class FractalScoutGoal : std::uint8_t {
    Balanced,
    Boundary,
    Filaments,
    Symmetry,
};

struct FractalScoutRequest {
    Preset preset;
    CameraState searchCamera;
    std::uint32_t candidatePoolSize{36};
    std::uint32_t resultCount{9};
    std::uint32_t sampleGridWidth{17};
    std::uint32_t sampleGridHeight{11};
    std::uint32_t thumbnailWidth{144};
    std::uint32_t thumbnailHeight{90};
    int maximumIterations{192};
    // Radius of the deterministic candidate spiral relative to searchCamera.scale.
    double searchRadius{0.9};
    // Central result scale relative to the region being searched.
    double refinementScale{0.34};
    // Search one or more logarithmically spaced depth bands around refinementScale.
    std::uint32_t scaleBandCount{3};
    double scaleBandSpread{0.38};
    // Greedy ranked-result separation in normalised position/scale space.
    double minimumResultSeparation{0.20};
    FractalScoutGoal goal{FractalScoutGoal::Balanced};
    // Retained for compatibility with the original balanced score contract.
    bool includeSymmetryScore{true};
};

struct FractalScoutLimits {
    std::uint32_t candidatePoolSize{36};
    std::uint32_t resultCount{9};
    std::uint32_t sampleGridWidth{17};
    std::uint32_t sampleGridHeight{11};
    std::uint32_t thumbnailWidth{144};
    std::uint32_t thumbnailHeight{90};
    int maximumIterations{192};
    double searchRadius{0.9};
    double refinementScale{0.34};
    std::uint32_t scaleBandCount{3};
    double scaleBandSpread{0.38};
    double minimumResultSeparation{0.20};
};

struct FractalScoutMetrics {
    double boundaryMix{0.0};
    double iterationVariance{0.0};
    double edgeDensity{0.0};
    double symmetry{0.0};
    double detail{0.0};
};

struct FractalScoutCandidate {
    // Stable identity for this immutable search input and candidate render
    // state. It deliberately excludes the transient ranked-list position.
    std::string identity;
    Preset preset;
    FractalScoutMetrics metrics;
    double score{0.0};
    StillRenderPreview thumbnail;
};

struct FractalScoutProgress {
    std::uint32_t completedCandidates{0};
    std::uint32_t totalCandidates{0};
    bool renderingThumbnails{false};
};

struct FractalScoutResult {
    std::vector<FractalScoutCandidate> candidates;
    std::uint32_t evaluatedCandidates{0};
    std::uint32_t suppressedNearDuplicates{0};
    bool cancelled{false};
};

using FractalScoutProgressCallback =
    std::function<void(const FractalScoutProgress& progress)>;
using FractalScoutCancellationCallback = std::function<bool()>;

// Applies hard resource ceilings and finite fallbacks. The resolved values are
// deterministic and suitable for both UI reporting and tests.
[[nodiscard]] FractalScoutLimits ResolveFractalScoutLimits(
    const FractalScoutRequest& request) noexcept;

// Public score contract used by the Scout and deterministic regression tests.
[[nodiscard]] double CalculateFractalScoutScore(
    const FractalScoutMetrics& metrics,
    FractalScoutGoal goal,
    bool includeSymmetryScore = true) noexcept;

// Performs a deterministic bounded search. It never mutates request.preset or
// any persisted settings. Candidates are returned highest-score first and are
// spread across position and depth where the candidate pool permits it.
bool RunFractalScout(const FractalScoutRequest& request,
                     const FractalScoutProgressCallback& progressCallback,
                     const FractalScoutCancellationCallback& cancellationCallback,
                     FractalScoutResult& result,
                     std::string& error);

} // namespace mw
