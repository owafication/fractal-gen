#pragma once

#include "Core/GeneralAnimation.h"
#include "Core/Models.h"
#include "Core/Precision/PrecisionPlanner.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace mw {

inline constexpr std::uint32_t kFrameSequenceManifestSchema = 3U;
inline constexpr const char* kFrameSequenceEvaluatorVersion = "general-animation-v1";

struct FrameRate {
    std::uint32_t numerator{30U};
    std::uint32_t denominator{1U};

    bool operator==(const FrameRate&) const = default;
};

struct FrameSequenceExportSettings {
    std::uint32_t width{1920U};
    std::uint32_t height{1080U};
    std::uint32_t dpi{96U};
    std::uint32_t tileWidth{256U};
    FrameRate frameRate{};
    double startTimeSeconds{0.0};
    std::uint32_t frameCount{1U};
    std::uint64_t seed{0U};
    std::string rendererId{"cpu-production-still"};
    std::string applicationVersion{"unknown"};
    std::string filePrefix{"frame"};
    std::uint32_t firstFrameNumber{0U};
    std::uint32_t frameNumberDigits{6U};
    bool scaleQualityToResolution{false};

    bool operator==(const FrameSequenceExportSettings&) const = default;
};

struct FrameSequenceExportJob {
    Preset basePreset;
    AnimationTimeline timeline;
    FrameSequenceExportSettings settings;
    std::filesystem::path outputDirectory;
    std::string projectFingerprint;
    std::string projectFingerprintVersion;
    std::string timelineFingerprint;
    std::string jobFingerprint;
};

struct FrameSequenceManifestEntry {
    std::uint32_t frameIndex{0U};
    double timeSeconds{0.0};
    std::string fileName;
    std::uint64_t fileSizeBytes{0U};
    std::string contentDigest;

    bool operator==(const FrameSequenceManifestEntry&) const = default;
};

struct FrameSequenceManifest {
    std::uint32_t schema{kFrameSequenceManifestSchema};
    std::string evaluatorVersion{kFrameSequenceEvaluatorVersion};
    std::string applicationVersion;
    std::string jobFingerprint;
    std::string projectFingerprint;
    std::string projectFingerprintVersion;
    std::string timelineFingerprint;
    std::string rendererId;
    std::uint32_t width{0U};
    std::uint32_t height{0U};
    std::uint32_t dpi{0U};
    FrameRate frameRate{};
    double startTimeSeconds{0.0};
    std::uint32_t frameCount{0U};
    std::uint64_t seed{0U};
    std::string filePrefix;
    std::uint32_t firstFrameNumber{0U};
    std::uint32_t frameNumberDigits{0U};
    bool scaleQualityToResolution{false};
    std::vector<FrameSequenceManifestEntry> completedFrames;

    bool operator==(const FrameSequenceManifest&) const = default;
};

struct FrameSequenceFrameRequest {
    std::uint32_t frameIndex{0U};
    double timeSeconds{0.0};
    std::uint64_t seed{0U};
    Preset framePreset;
    // Set only after the exact frame camera and selected renderer have passed
    // planner validation. Render callbacks must execute this immutable plan,
    // rather than independently deriving a precision tier from renderer text.
    PrecisionPlan precisionPlan;
};

struct FrameSequenceExportProgress {
    std::uint32_t completedFrames{0U};
    std::uint32_t totalFrames{0U};
    std::uint32_t activeFrameIndex{0U};
    bool resumedFrame{false};
};

struct FrameSequenceExportResult {
    bool cancelled{false};
    std::uint32_t renderedFrames{0U};
    std::uint32_t resumedFrames{0U};
    std::filesystem::path manifestPath;
};

using FrameSequenceRenderCallback =
    std::function<bool(const FrameSequenceFrameRequest& request,
                       const std::filesystem::path& temporaryPath,
                       std::string& error)>;
using FrameSequenceValidateCallback =
    std::function<bool(const std::filesystem::path& path,
                       std::uint32_t expectedWidth,
                       std::uint32_t expectedHeight,
                       std::string& error)>;
using FrameSequenceProgressCallback =
    std::function<void(const FrameSequenceExportProgress& progress)>;
using FrameSequenceCancellationCallback = std::function<bool()>;

[[nodiscard]] bool IsValidFrameRate(const FrameRate& frameRate) noexcept;
[[nodiscard]] double FrameTimeForIndex(const FrameSequenceExportSettings& settings,
                                       std::uint32_t frameIndex) noexcept;
[[nodiscard]] std::uint32_t FrameCountForDuration(double durationSeconds,
                                                  const FrameRate& frameRate,
                                                  bool includeEndFrame,
                                                  std::string& error) noexcept;

bool BuildFrameSequenceExportJob(const Preset& basePreset,
                                 const AnimationTimeline& timeline,
                                 const FrameSequenceExportSettings& settings,
                                 const std::filesystem::path& outputDirectory,
                                 FrameSequenceExportJob& job,
                                 std::string& error);

[[nodiscard]] std::filesystem::path FrameSequenceMetadataDirectory(
    const FrameSequenceExportJob& job);
[[nodiscard]] std::filesystem::path FrameSequenceManifestPath(
    const FrameSequenceExportJob& job);
[[nodiscard]] std::filesystem::path FrameSequenceFinalFramePath(
    const FrameSequenceExportJob& job, std::uint32_t frameIndex);

bool SaveFrameSequenceManifest(const std::filesystem::path& path,
                               const FrameSequenceManifest& manifest,
                               std::string& error);
bool LoadFrameSequenceManifest(const std::filesystem::path& path,
                               FrameSequenceManifest& manifest,
                               std::string& error);

bool RunFrameSequenceExport(const FrameSequenceExportJob& job,
                            bool allowResume,
                            const FrameSequenceRenderCallback& renderFrame,
                            const FrameSequenceValidateCallback& validateFrame,
                            const FrameSequenceProgressCallback& progressCallback,
                            const FrameSequenceCancellationCallback& cancellationCallback,
                            FrameSequenceExportResult& result,
                            std::string& error);

} // namespace mw
