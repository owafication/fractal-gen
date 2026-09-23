#include "Core/FrameSequenceExport.h"

#include "Core/Json.h"
#include "Core/Precision/PrecisionPlanner.h"
#include "Core/ProjectState.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <system_error>

namespace mw {
namespace {

constexpr std::uint32_t kMaximumFrameCount = 1000000U;
constexpr std::uint32_t kMaximumFrameRateNumerator = 1000000U;
constexpr std::uint32_t kMaximumFrameRateDenominator = 100000U;
constexpr std::uint32_t kMaximumFrameNumberDigits = 12U;
constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr const char* kCpuProductionStillRenderer = "cpu-production-still";
constexpr const char* kCpuExactBoost512Renderer = "cpu-exact-boost512-v1";
constexpr const char* kCpuExactBoost2048Renderer = "cpu-exact-boost2048-v1";
constexpr const char* kCpuExactBoost8192Renderer = "cpu-exact-boost8192-v1";
constexpr const char* kCpuExactBoost16384Renderer = "cpu-exact-boost16384-v1";

bool IsCpuExactBoost512Renderer(const std::string& rendererId) noexcept {
    return rendererId == kCpuExactBoost512Renderer;
}

bool IsCpuExactBoost2048Renderer(const std::string& rendererId) noexcept {
    return rendererId == kCpuExactBoost2048Renderer;
}

bool IsCpuExactBoost8192Renderer(const std::string& rendererId) noexcept {
    return rendererId == kCpuExactBoost8192Renderer;
}

bool IsCpuExactBoost16384Renderer(const std::string& rendererId) noexcept {
    return rendererId == kCpuExactBoost16384Renderer;
}

bool IsCpuExactDirectRenderer(const std::string& rendererId) noexcept {
    return IsCpuExactBoost512Renderer(rendererId) || IsCpuExactBoost2048Renderer(rendererId) ||
           IsCpuExactBoost8192Renderer(rendererId) || IsCpuExactBoost16384Renderer(rendererId);
}

bool IsSupportedFrameSequenceRenderer(const std::string& rendererId) noexcept {
    return rendererId == kCpuProductionStillRenderer || IsCpuExactDirectRenderer(rendererId);
}

int ExactDirectRendererBits(const std::string& rendererId) noexcept {
    if (IsCpuExactBoost16384Renderer(rendererId)) return 16384;
    if (IsCpuExactBoost8192Renderer(rendererId)) return 8192;
    return IsCpuExactBoost2048Renderer(rendererId) ? 2048 : 512;
}

bool ValidateExactDirectPreset(const Preset& preset, std::string& error) {
    if (preset.rotationDegrees != 0.0) {
        error = "Exact CPU frame rendering does not support rotation yet.";
        return false;
    }
    if (preset.equation.animateCoefficients) {
        error = "Exact CPU frame rendering does not support animated equation coefficients.";
        return false;
    }
    if (preset.antiAliasingLevel < 1 || preset.antiAliasingLevel > 4) {
        error = "Exact CPU frame rendering requires an anti-aliasing level between 1 and 4.";
        return false;
    }
    return true;
}

std::string StableDouble(double value) {
    std::ostringstream stream;
    stream << std::hexfloat << value;
    return stream.str();
}

bool ParseStableDouble(const json::Value& value, double& result) {
    if (!value.IsString()) return false;
    try {
        std::size_t consumed = 0U;
        result = std::stod(value.AsString(), &consumed);
        return consumed == value.AsString().size() && std::isfinite(result);
    } catch (...) {
        return false;
    }
}

std::string UnsignedString(std::uint64_t value) {
    return std::to_string(value);
}

bool ParseUnsignedString(const json::Value& value, std::uint64_t& result) {
    if (!value.IsString()) return false;
    const std::string text = value.AsString();
    if (text.empty()) return false;
    try {
        std::size_t consumed = 0U;
        const unsigned long long parsed = std::stoull(text, &consumed, 10);
        if (consumed != text.size()) return false;
        result = static_cast<std::uint64_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool IsSafeFilePrefix(std::string_view prefix) noexcept {
    if (prefix.empty() || prefix.size() > 64U || prefix == "." || prefix == "..") return false;
    for (const char character : prefix) {
        const unsigned char value = static_cast<unsigned char>(character);
        const bool alphaNumeric = (value >= static_cast<unsigned char>('a') &&
                                   value <= static_cast<unsigned char>('z')) ||
                                  (value >= static_cast<unsigned char>('A') &&
                                   value <= static_cast<unsigned char>('Z')) ||
                                  (value >= static_cast<unsigned char>('0') &&
                                   value <= static_cast<unsigned char>('9'));
        if (!alphaNumeric && character != '-' && character != '_') return false;
    }
    return true;
}

std::string LoopModeName(AnimationLoopMode mode) {
    switch (mode) {
    case AnimationLoopMode::Clamp: return "clamp";
    case AnimationLoopMode::Loop: return "loop";
    case AnimationLoopMode::PingPong: return "ping-pong";
    }
    return "unknown";
}

std::string InterpolationName(AnimationInterpolation interpolation) {
    switch (interpolation) {
    case AnimationInterpolation::Step: return "step";
    case AnimationInterpolation::Linear: return "linear";
    case AnimationInterpolation::Smoothstep: return "smoothstep";
    }
    return "unknown";
}

json::Value AnimationValueJson(const AnimationValue& value) {
    if (const auto* real = std::get_if<double>(&value)) {
        return json::Value::Object{{"kind", "real"}, {"value", StableDouble(*real)}};
    }
    if (const auto* integer = std::get_if<std::int64_t>(&value)) {
        return json::Value::Object{{"kind", "integer"},
                                   {"value", std::to_string(*integer)}};
    }
    const auto& compensated = std::get<CompensatedAnimationValue>(value);
    return json::Value::Object{{"high", StableDouble(compensated.high)},
                               {"kind", "compensated-real"},
                               {"low", StableDouble(compensated.low)}};
}

std::string CanonicalTimeline(const AnimationTimeline& timeline) {
    std::vector<const AnimationTrack*> tracks;
    tracks.reserve(timeline.tracks.size());
    for (const auto& track : timeline.tracks) tracks.push_back(&track);
    std::sort(tracks.begin(), tracks.end(), [](const AnimationTrack* left,
                                               const AnimationTrack* right) {
        return left->id < right->id;
    });

    json::Value::Array trackValues;
    trackValues.reserve(tracks.size());
    for (const AnimationTrack* track : tracks) {
        std::vector<const AnimationKeyframe*> keyframes;
        keyframes.reserve(track->keyframes.size());
        for (const auto& keyframe : track->keyframes) keyframes.push_back(&keyframe);
        std::sort(keyframes.begin(), keyframes.end(), [](const AnimationKeyframe* left,
                                                         const AnimationKeyframe* right) {
            if (left->timeSeconds != right->timeSeconds) {
                return left->timeSeconds < right->timeSeconds;
            }
            return left->id < right->id;
        });
        json::Value::Array keyframeValues;
        keyframeValues.reserve(keyframes.size());
        for (const AnimationKeyframe* keyframe : keyframes) {
            keyframeValues.emplace_back(json::Value::Object{
                {"id", keyframe->id},
                {"interpolation", InterpolationName(keyframe->interpolation)},
                {"time", StableDouble(keyframe->timeSeconds)},
                {"value", AnimationValueJson(keyframe->value)},
            });
        }
        trackValues.emplace_back(json::Value::Object{
            {"enabled", track->enabled},
            {"id", track->id},
            {"keyframes", std::move(keyframeValues)},
            {"target", track->target},
        });
    }
    return json::Stringify(json::Value(json::Value::Object{
        {"duration", StableDouble(timeline.durationSeconds)},
        {"id", timeline.id},
        {"loopMode", LoopModeName(timeline.loopMode)},
        {"schema", "mw-animation-timeline-v1"},
        {"tracks", std::move(trackValues)},
    }), false);
}

std::string FrameFileName(const FrameSequenceExportSettings& settings,
                          std::uint32_t frameIndex) {
    const std::uint64_t frameNumber = static_cast<std::uint64_t>(settings.firstFrameNumber) +
                                      static_cast<std::uint64_t>(frameIndex);
    std::ostringstream stream;
    stream << settings.filePrefix << '-' << std::setw(static_cast<int>(settings.frameNumberDigits))
           << std::setfill('0') << frameNumber << ".png";
    return stream.str();
}

std::filesystem::path TemporaryFramePath(const FrameSequenceExportJob& job,
                                         std::uint32_t frameIndex) {
    return FrameSequenceMetadataDirectory(job) /
           (FrameFileName(job.settings, frameIndex) + ".part");
}

std::filesystem::path ReceiptPath(const FrameSequenceExportJob& job,
                                  std::uint32_t frameIndex) {
    std::ostringstream stream;
    stream << "receipt-" << std::setw(static_cast<int>(job.settings.frameNumberDigits))
           << std::setfill('0') << frameIndex << ".json";
    return FrameSequenceMetadataDirectory(job) / stream.str();
}

bool EnsureDirectory(const std::filesystem::path& directory, std::string& error) {
    std::error_code code;
    std::filesystem::create_directories(directory, code);
    if (code) {
        error = "The frame-sequence directory could not be created: " + code.message();
        return false;
    }
    return true;
}

bool PromoteNewFile(const std::filesystem::path& temporary,
                    const std::filesystem::path& finalPath,
                    std::string& error) {
    std::error_code code;
    if (std::filesystem::exists(finalPath, code)) {
        error = "A final output file already exists and was not overwritten: " +
                finalPath.string();
        return false;
    }
    code.clear();
    std::filesystem::rename(temporary, finalPath, code);
    if (code) {
        error = "The verified temporary frame could not be promoted: " + code.message();
        return false;
    }
    return true;
}

bool PromoteReplacingFile(const std::filesystem::path& temporary,
                          const std::filesystem::path& finalPath,
                          std::string& error) {
    const std::filesystem::path backup = finalPath.string() + ".bak";
    std::error_code code;
    std::filesystem::remove(backup, code);
    code.clear();
    const bool hadFinal = std::filesystem::exists(finalPath, code) && !code;
    if (hadFinal) {
        std::filesystem::rename(finalPath, backup, code);
        if (code) {
            error = "The previous manifest could not be protected before update: " + code.message();
            return false;
        }
    }
    code.clear();
    std::filesystem::rename(temporary, finalPath, code);
    if (code) {
        if (hadFinal) {
            std::error_code restoreCode;
            std::filesystem::rename(backup, finalPath, restoreCode);
        }
        error = "The updated manifest could not be promoted: " + code.message();
        return false;
    }
    if (hadFinal) {
        code.clear();
        std::filesystem::remove(backup, code);
    }
    return true;
}

bool WriteTextAtomically(const std::filesystem::path& path,
                         const std::string& text,
                         std::string& error) {
    if (!EnsureDirectory(path.parent_path(), error)) return false;
    const std::filesystem::path temporary = path.string() + ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            error = "A temporary metadata file could not be opened: " + temporary.string();
            return false;
        }
        stream.write(text.data(), static_cast<std::streamsize>(text.size()));
        stream.flush();
        if (!stream) {
            stream.close();
            std::error_code removeCode;
            std::filesystem::remove(temporary, removeCode);
            error = "A temporary metadata file could not be written completely.";
            return false;
        }
    }
    return PromoteReplacingFile(temporary, path, error);
}

bool ReadText(const std::filesystem::path& path, std::string& text, std::string& error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "The frame-sequence manifest could not be opened: " + path.string();
        return false;
    }
    stream.seekg(0, std::ios::end);
    const std::streamoff size = stream.tellg();
    if (size < 0 || size > static_cast<std::streamoff>(16 * 1024 * 1024)) {
        error = "The frame-sequence manifest exceeds the 16 MiB metadata limit.";
        return false;
    }
    stream.seekg(0, std::ios::beg);
    text.resize(static_cast<std::size_t>(size));
    stream.read(text.data(), size);
    if (!stream && size != 0) {
        error = "The frame-sequence manifest could not be read completely.";
        return false;
    }
    return true;
}

bool FileDigest(const std::filesystem::path& path,
                std::uint64_t& fileSize,
                std::string& digest,
                std::string& error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "The rendered frame could not be opened for verification: " + path.string();
        return false;
    }
    std::uint64_t hash = kFnvOffsetBasis;
    fileSize = 0U;
    std::array<char, 64 * 1024> buffer{};
    while (stream) {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = stream.gcount();
        if (count < 0) {
            error = "The rendered frame could not be read for verification.";
            return false;
        }
        for (std::streamsize index = 0; index < count; ++index) {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(index)]);
            hash *= kFnvPrime;
        }
        fileSize += static_cast<std::uint64_t>(count);
    }
    if (!stream.eof()) {
        error = "The rendered frame could not be read completely for verification.";
        return false;
    }
    std::ostringstream formatted;
    formatted << "fnv1a64:" << std::hex << std::setw(16) << std::setfill('0') << hash;
    digest = formatted.str();
    return true;
}

std::uint64_t FrameSeed(std::uint64_t baseSeed, std::uint32_t frameIndex) noexcept {
    std::uint64_t value = baseSeed + 0x9e3779b97f4a7c15ULL +
                          static_cast<std::uint64_t>(frameIndex);
    value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31U);
}

json::Value ManifestEntryJson(const FrameSequenceManifestEntry& entry) {
    return json::Value::Object{
        {"contentDigest", entry.contentDigest},
        {"fileName", entry.fileName},
        {"fileSizeBytes", UnsignedString(entry.fileSizeBytes)},
        {"frameIndex", static_cast<double>(entry.frameIndex)},
        {"timeSeconds", StableDouble(entry.timeSeconds)},
    };
}

json::Value ManifestJson(const FrameSequenceManifest& manifest) {
    json::Value::Array completed;
    completed.reserve(manifest.completedFrames.size());
    for (const auto& entry : manifest.completedFrames) completed.emplace_back(ManifestEntryJson(entry));
    return json::Value::Object{
        {"applicationVersion", manifest.applicationVersion},
        {"completedFrames", std::move(completed)},
        {"dpi", static_cast<double>(manifest.dpi)},
        {"evaluatorVersion", manifest.evaluatorVersion},
        {"filePrefix", manifest.filePrefix},
        {"firstFrameNumber", static_cast<double>(manifest.firstFrameNumber)},
        {"frameCount", static_cast<double>(manifest.frameCount)},
        {"frameNumberDigits", static_cast<double>(manifest.frameNumberDigits)},
        {"frameRateDenominator", static_cast<double>(manifest.frameRate.denominator)},
        {"frameRateNumerator", static_cast<double>(manifest.frameRate.numerator)},
        {"height", static_cast<double>(manifest.height)},
        {"jobFingerprint", manifest.jobFingerprint},
        {"projectFingerprint", manifest.projectFingerprint},
        {"projectFingerprintVersion", manifest.projectFingerprintVersion},
        {"rendererId", manifest.rendererId},
        {"scaleQualityToResolution", manifest.scaleQualityToResolution},
        {"schema", static_cast<double>(manifest.schema)},
        {"seed", UnsignedString(manifest.seed)},
        {"startTimeSeconds", StableDouble(manifest.startTimeSeconds)},
        {"timelineFingerprint", manifest.timelineFingerprint},
        {"width", static_cast<double>(manifest.width)},
    };
}

bool NumberToUInt32(const json::Value* value, std::uint32_t& result) {
    if (!value || !value->IsNumber()) return false;
    const double number = value->AsNumber(-1.0);
    if (!std::isfinite(number) || number < 0.0 ||
        number > static_cast<double>(std::numeric_limits<std::uint32_t>::max()) ||
        std::floor(number) != number) {
        return false;
    }
    result = static_cast<std::uint32_t>(number);
    return true;
}

bool ParseManifestEntry(const json::Value& value,
                        FrameSequenceManifestEntry& entry) {
    if (!value.IsObject()) return false;
    const auto* index = value.Find("frameIndex");
    const auto* time = value.Find("timeSeconds");
    const auto* fileName = value.Find("fileName");
    const auto* fileSize = value.Find("fileSizeBytes");
    const auto* digest = value.Find("contentDigest");
    std::uint64_t parsedSize = 0U;
    if (!NumberToUInt32(index, entry.frameIndex) || !time ||
        !ParseStableDouble(*time, entry.timeSeconds) || !fileName || !fileName->IsString() ||
        !fileSize || !ParseUnsignedString(*fileSize, parsedSize) ||
        !digest || !digest->IsString()) {
        return false;
    }
    entry.fileName = fileName->AsString();
    entry.fileSizeBytes = parsedSize;
    entry.contentDigest = digest->AsString();
    return !entry.fileName.empty() && !entry.contentDigest.empty();
}

FrameSequenceManifest ManifestForJob(const FrameSequenceExportJob& job) {
    FrameSequenceManifest manifest;
    manifest.applicationVersion = job.settings.applicationVersion;
    manifest.jobFingerprint = job.jobFingerprint;
    manifest.projectFingerprint = job.projectFingerprint;
    manifest.projectFingerprintVersion = job.projectFingerprintVersion;
    manifest.timelineFingerprint = job.timelineFingerprint;
    manifest.rendererId = job.settings.rendererId;
    manifest.width = job.settings.width;
    manifest.height = job.settings.height;
    manifest.dpi = job.settings.dpi;
    manifest.frameRate = job.settings.frameRate;
    manifest.startTimeSeconds = job.settings.startTimeSeconds;
    manifest.frameCount = job.settings.frameCount;
    manifest.seed = job.settings.seed;
    manifest.filePrefix = job.settings.filePrefix;
    manifest.firstFrameNumber = job.settings.firstFrameNumber;
    manifest.frameNumberDigits = job.settings.frameNumberDigits;
    manifest.scaleQualityToResolution = job.settings.scaleQualityToResolution;
    return manifest;
}

bool ManifestMatchesJob(const FrameSequenceManifest& manifest,
                        const FrameSequenceExportJob& job,
                        std::string& error) {
    if (manifest.schema != kFrameSequenceManifestSchema ||
        manifest.evaluatorVersion != kFrameSequenceEvaluatorVersion) {
        error = "The existing frame manifest uses an unsupported schema or evaluator version.";
        return false;
    }
    const auto& settings = job.settings;
    if (manifest.jobFingerprint != job.jobFingerprint ||
        manifest.applicationVersion != settings.applicationVersion ||
        manifest.projectFingerprint != job.projectFingerprint ||
        manifest.projectFingerprintVersion != job.projectFingerprintVersion ||
        manifest.timelineFingerprint != job.timelineFingerprint ||
        manifest.rendererId != settings.rendererId ||
        manifest.width != settings.width || manifest.height != settings.height ||
        manifest.dpi != settings.dpi || manifest.frameRate != settings.frameRate ||
        manifest.startTimeSeconds != settings.startTimeSeconds ||
        manifest.frameCount != settings.frameCount || manifest.seed != settings.seed ||
        manifest.filePrefix != settings.filePrefix ||
        manifest.firstFrameNumber != settings.firstFrameNumber ||
        manifest.frameNumberDigits != settings.frameNumberDigits ||
        manifest.scaleQualityToResolution != settings.scaleQualityToResolution) {
        error = "The existing frame manifest does not match the current project, timeline, timing, renderer, or output settings.";
        return false;
    }
    return true;
}

bool SaveReceipt(const FrameSequenceExportJob& job,
                 const FrameSequenceManifestEntry& entry,
                 std::string& error) {
    const json::Value value(json::Value::Object{
        {"entry", ManifestEntryJson(entry)},
        {"jobFingerprint", job.jobFingerprint},
        {"schema", static_cast<double>(kFrameSequenceManifestSchema)},
    });
    return WriteTextAtomically(ReceiptPath(job, entry.frameIndex),
                               json::Stringify(value, true) + "\n", error);
}

bool LoadReceipt(const FrameSequenceExportJob& job,
                 std::uint32_t frameIndex,
                 FrameSequenceManifestEntry& entry,
                 bool& found,
                 std::string& error) {
    found = false;
    const std::filesystem::path path = ReceiptPath(job, frameIndex);
    std::error_code code;
    if (!std::filesystem::exists(path, code)) {
        if (code) error = "A frame receipt path could not be inspected: " + code.message();
        return !code;
    }
    found = true;
    std::string text;
    if (!ReadText(path, text, error)) return false;
    const auto parsed = json::Parse(text, 16U * 1024U * 1024U);
    if (!parsed.value || !parsed.value->IsObject()) {
        error = "A frame receipt is invalid JSON: " + path.string();
        return false;
    }
    const auto* schema = parsed.value->Find("schema");
    const auto* fingerprint = parsed.value->Find("jobFingerprint");
    const auto* entryValue = parsed.value->Find("entry");
    std::uint32_t parsedSchema = 0U;
    if (!NumberToUInt32(schema, parsedSchema) || parsedSchema != kFrameSequenceManifestSchema ||
        !fingerprint || !fingerprint->IsString() ||
        fingerprint->AsString() != job.jobFingerprint || !entryValue ||
        !ParseManifestEntry(*entryValue, entry) || entry.frameIndex != frameIndex) {
        error = "A prepared frame receipt does not match the current export job.";
        return false;
    }
    return true;
}

bool VerifyFileAgainstEntry(const std::filesystem::path& path,
                            const FrameSequenceManifestEntry& entry,
                            const FrameSequenceExportJob& job,
                            const FrameSequenceValidateCallback& validateFrame,
                            std::string& error) {
    if (!validateFrame(path, job.settings.width, job.settings.height, error)) return false;
    std::uint64_t size = 0U;
    std::string digest;
    if (!FileDigest(path, size, digest, error)) return false;
    if (size != entry.fileSizeBytes || digest != entry.contentDigest) {
        error = "A completed frame no longer matches its verified receipt: " + path.string();
        return false;
    }
    return true;
}

bool ContainsFrame(const FrameSequenceManifest& manifest,
                   std::uint32_t frameIndex,
                   FrameSequenceManifestEntry* entry = nullptr) {
    const auto found = std::find_if(manifest.completedFrames.begin(),
                                    manifest.completedFrames.end(),
                                    [frameIndex](const FrameSequenceManifestEntry& candidate) {
        return candidate.frameIndex == frameIndex;
    });
    if (found == manifest.completedFrames.end()) return false;
    if (entry) *entry = *found;
    return true;
}

void AddManifestEntry(FrameSequenceManifest& manifest,
                      FrameSequenceManifestEntry entry) {
    auto found = std::find_if(manifest.completedFrames.begin(), manifest.completedFrames.end(),
                              [&](const FrameSequenceManifestEntry& candidate) {
        return candidate.frameIndex == entry.frameIndex;
    });
    if (found == manifest.completedFrames.end()) manifest.completedFrames.push_back(std::move(entry));
    else *found = std::move(entry);
    std::sort(manifest.completedFrames.begin(), manifest.completedFrames.end(),
              [](const FrameSequenceManifestEntry& left,
                 const FrameSequenceManifestEntry& right) {
        return left.frameIndex < right.frameIndex;
    });
}

bool RecoverManifestBackup(const std::filesystem::path& path, std::string& error) {
    std::error_code code;
    if (std::filesystem::exists(path, code)) return true;
    if (code) {
        error = "The manifest path could not be inspected: " + code.message();
        return false;
    }
    const std::filesystem::path backup = path.string() + ".bak";
    if (!std::filesystem::exists(backup, code)) return !code;
    if (code) {
        error = "The manifest backup path could not be inspected: " + code.message();
        return false;
    }
    std::filesystem::rename(backup, path, code);
    if (code) {
        error = "The protected manifest backup could not be restored: " + code.message();
        return false;
    }
    return true;
}

} // namespace

bool IsValidFrameRate(const FrameRate& frameRate) noexcept {
    return frameRate.numerator > 0U && frameRate.denominator > 0U &&
           frameRate.numerator <= kMaximumFrameRateNumerator &&
           frameRate.denominator <= kMaximumFrameRateDenominator;
}

double FrameTimeForIndex(const FrameSequenceExportSettings& settings,
                         std::uint32_t frameIndex) noexcept {
    if (!IsValidFrameRate(settings.frameRate)) return settings.startTimeSeconds;
    const long double offset = static_cast<long double>(frameIndex) *
                               static_cast<long double>(settings.frameRate.denominator) /
                               static_cast<long double>(settings.frameRate.numerator);
    return static_cast<double>(static_cast<long double>(settings.startTimeSeconds) + offset);
}

std::uint32_t FrameCountForDuration(double durationSeconds,
                                    const FrameRate& frameRate,
                                    bool includeEndFrame,
                                    std::string& error) noexcept {
    error.clear();
    if (!std::isfinite(durationSeconds) || durationSeconds < 0.0) {
        error = "Frame-sequence duration must be finite and non-negative.";
        return 0U;
    }
    if (!IsValidFrameRate(frameRate)) {
        error = "Frame rate numerator and denominator must be positive and bounded.";
        return 0U;
    }
    const long double exactIntervals = static_cast<long double>(durationSeconds) *
                                       static_cast<long double>(frameRate.numerator) /
                                       static_cast<long double>(frameRate.denominator);
    const long double nearest = std::round(exactIntervals);
    const long double tolerance = std::max(1.0L, std::abs(exactIntervals)) * 1.0e-12L;
    const long double normalised = std::abs(exactIntervals - nearest) <= tolerance
        ? nearest : exactIntervals;
    long double count = includeEndFrame ? std::floor(normalised) + 1.0L
                                        : std::ceil(normalised);
    if (count < 1.0L) count = 1.0L;
    if (count > static_cast<long double>(kMaximumFrameCount)) {
        error = "Frame-sequence length exceeds the one-million-frame safety limit.";
        return 0U;
    }
    return static_cast<std::uint32_t>(count);
}

bool BuildFrameSequenceExportJob(const Preset& basePreset,
                                 const AnimationTimeline& timeline,
                                 const FrameSequenceExportSettings& settings,
                                 const std::filesystem::path& outputDirectory,
                                 FrameSequenceExportJob& job,
                                 std::string& error) {
    job = {};
    error.clear();
    if (outputDirectory.empty()) {
        error = "A frame-sequence output directory is required.";
        return false;
    }
    if (settings.width == 0U || settings.height == 0U || settings.dpi == 0U ||
        settings.tileWidth == 0U) {
        error = "Frame dimensions, DPI, and tile width must be positive.";
        return false;
    }
    if (!IsValidFrameRate(settings.frameRate)) {
        error = "Frame rate numerator and denominator must be positive and bounded.";
        return false;
    }
    if (!std::isfinite(settings.startTimeSeconds) || settings.startTimeSeconds < 0.0) {
        error = "Frame-sequence start time must be finite and non-negative.";
        return false;
    }
    if (settings.frameCount == 0U || settings.frameCount > kMaximumFrameCount) {
        error = "Frame count must be between one and one million.";
        return false;
    }
    if (!IsSafeFilePrefix(settings.filePrefix)) {
        error = "Frame filename prefix may contain only letters, numbers, '-' and '_'.";
        return false;
    }
    if (settings.frameNumberDigits == 0U ||
        settings.frameNumberDigits > kMaximumFrameNumberDigits) {
        error = "Frame-number padding must be between 1 and 12 digits.";
        return false;
    }
    const std::uint64_t lastFrameNumber =
        static_cast<std::uint64_t>(settings.firstFrameNumber) + settings.frameCount - 1ULL;
    std::uint64_t capacity = 1ULL;
    for (std::uint32_t index = 0U; index < settings.frameNumberDigits; ++index) {
        capacity *= 10ULL;
    }
    if (lastFrameNumber >= capacity) {
        error = "Frame-number padding is too small for the requested sequence.";
        return false;
    }
    if (settings.rendererId.empty() || settings.applicationVersion.empty()) {
        error = "Renderer and application version identifiers are required.";
        return false;
    }
    if (!IsSupportedFrameSequenceRenderer(settings.rendererId)) {
        error = "The selected frame export renderer is not an implemented deterministic route.";
        return false;
    }

    Preset snapshot = basePreset;
    ValidateAndNormalise(snapshot);
    if (!snapshot.exactCamera.has_value()) {
        error = "Frame-sequence export requires canonical exact camera state.";
        return false;
    }
    // The current frame exporter invokes only the established Float64 still
    // renderer. Do not create an export job that would require a lower-
    // precision reinterpretation of its authoritative camera text.
    const bool exactDirectRenderer = IsCpuExactDirectRenderer(settings.rendererId);
    if (exactDirectRenderer && !ValidateExactDirectPreset(snapshot, error)) return false;
    PrecisionBackendCapabilities capabilities;
    capabilities.cpuBoost512Reference = IsCpuExactBoost512Renderer(settings.rendererId);
    capabilities.cpuBoost2048Direct = IsCpuExactBoost2048Renderer(settings.rendererId);
    capabilities.cpuBoost8192Direct = IsCpuExactBoost8192Renderer(settings.rendererId);
    capabilities.cpuBoost16384Direct = IsCpuExactBoost16384Renderer(settings.rendererId);
    const PrecisionPlan basePrecisionPlan = BuildPrecisionPlan(
        *snapshot.exactCamera, snapshot.equation,
        exactDirectRenderer ? PrecisionMode::ArbitraryPrecisionPerturbation : PrecisionMode::Float64,
        capabilities);
    const PrecisionExecutionBackend requiredBackend = !exactDirectRenderer
        ? PrecisionExecutionBackend::CpuFloat64
        : (ExactDirectRendererBits(settings.rendererId) == 16384
            ? PrecisionExecutionBackend::CpuBoost16384Direct
            : (ExactDirectRendererBits(settings.rendererId) == 8192
            ? PrecisionExecutionBackend::CpuBoost8192Direct
            : (ExactDirectRendererBits(settings.rendererId) == 2048
                ? PrecisionExecutionBackend::CpuBoost2048Direct
                : PrecisionExecutionBackend::CpuBoost512Reference)));
    if (basePrecisionPlan.backend != requiredBackend) {
        error = "The selected frame export renderer cannot execute the base exact camera safely: " +
                basePrecisionPlan.reason;
        return false;
    }
    const auto timelineValidation = ValidateGeneralAnimationTimeline(timeline);
    if (!timelineValidation.valid) {
        error = timelineValidation.issues.empty()
            ? "The animation timeline is invalid."
            : timelineValidation.issues.front().message;
        return false;
    }

    RenderFingerprintContext context;
    context.rendererId = settings.rendererId;
    context.width = settings.width;
    context.height = settings.height;
    context.precision.mode = exactDirectRenderer ? PrecisionMode::ArbitraryPrecisionPerturbation
                                                 : PrecisionMode::Float64;
    context.precision.allowFloat64 = !exactDirectRenderer;
    context.precision.allowSplitFloat = false;
    context.precision.allowPerturbation = exactDirectRenderer;
    context.precision.allowArbitraryPrecision = exactDirectRenderer;
    context.precision.automaticFallback = false;
    context.precision.arbitraryPrecisionBits = basePrecisionPlan.selectedBits;
    context.precisionPlanVersion = kPrecisionPlanVersion;
    context.precisionExecutionBackend =
        PrecisionExecutionBackendName(basePrecisionPlan.backend);
    context.precisionFormulaCapability =
        PrecisionFormulaCapabilityName(basePrecisionPlan.formulaProfile);
    context.precisionReason = basePrecisionPlan.reason;
    context.precisionRequiredBits = basePrecisionPlan.requiredBits;
    context.precisionSelectedBits = basePrecisionPlan.selectedBits;
    context.timeSeconds = 0.0;
    context.seed = settings.seed;
    context.scaleQualityToResolution = settings.scaleQualityToResolution;
    RenderFingerprint project;
    if (!BuildExactRenderFingerprint(snapshot, context, project, error)) return false;

    const std::string timelineCanonical = CanonicalTimeline(timeline);
    const std::string timelineFingerprint = Sha256Hex(timelineCanonical);
    const json::Value jobValue(json::Value::Object{
        {"applicationVersion", settings.applicationVersion},
        {"dpi", static_cast<double>(settings.dpi)},
        {"evaluatorVersion", kFrameSequenceEvaluatorVersion},
        {"filePrefix", settings.filePrefix},
        {"firstFrameNumber", static_cast<double>(settings.firstFrameNumber)},
        {"frameCount", static_cast<double>(settings.frameCount)},
        {"frameNumberDigits", static_cast<double>(settings.frameNumberDigits)},
        {"frameRateDenominator", static_cast<double>(settings.frameRate.denominator)},
        {"frameRateNumerator", static_cast<double>(settings.frameRate.numerator)},
        {"height", static_cast<double>(settings.height)},
        {"manifestSchema", static_cast<double>(kFrameSequenceManifestSchema)},
        {"projectFingerprint", project.digest},
        {"projectFingerprintVersion", project.canonicalVersion},
        {"rendererId", settings.rendererId},
        {"scaleQualityToResolution", settings.scaleQualityToResolution},
        {"seed", UnsignedString(settings.seed)},
        {"startTimeSeconds", StableDouble(settings.startTimeSeconds)},
        {"tileWidth", static_cast<double>(settings.tileWidth)},
        {"timelineFingerprint", timelineFingerprint},
        {"width", static_cast<double>(settings.width)},
    });

    job.basePreset = std::move(snapshot);
    job.timeline = timeline;
    job.settings = settings;
    job.outputDirectory = outputDirectory;
    job.projectFingerprint = project.digest;
    job.projectFingerprintVersion = project.canonicalVersion;
    job.timelineFingerprint = timelineFingerprint;
    job.jobFingerprint = Sha256Hex(json::Stringify(jobValue, false));
    return true;
}

std::filesystem::path FrameSequenceMetadataDirectory(const FrameSequenceExportJob& job) {
    return job.outputDirectory / ".mw-frame-sequence";
}

std::filesystem::path FrameSequenceManifestPath(const FrameSequenceExportJob& job) {
    return FrameSequenceMetadataDirectory(job) / "manifest.json";
}

std::filesystem::path FrameSequenceFinalFramePath(const FrameSequenceExportJob& job,
                                                  std::uint32_t frameIndex) {
    return job.outputDirectory / FrameFileName(job.settings, frameIndex);
}

bool SaveFrameSequenceManifest(const std::filesystem::path& path,
                               const FrameSequenceManifest& manifest,
                               std::string& error) {
    if (manifest.schema != kFrameSequenceManifestSchema ||
        manifest.completedFrames.size() > manifest.frameCount ||
        !IsSupportedFrameSequenceRenderer(manifest.rendererId)) {
        error = "The frame-sequence manifest is internally inconsistent.";
        return false;
    }
    return WriteTextAtomically(path, json::Stringify(ManifestJson(manifest), true) + "\n", error);
}

bool LoadFrameSequenceManifest(const std::filesystem::path& path,
                               FrameSequenceManifest& manifest,
                               std::string& error) {
    manifest = {};
    error.clear();
    if (!RecoverManifestBackup(path, error)) return false;
    std::string text;
    if (!ReadText(path, text, error)) return false;
    const auto parsed = json::Parse(text, 16U * 1024U * 1024U);
    if (!parsed.value || !parsed.value->IsObject()) {
        error = parsed.error.empty() ? "The frame-sequence manifest is not a JSON object."
                                     : parsed.error;
        return false;
    }
    const json::Value& root = *parsed.value;
    const auto* evaluatorVersion = root.Find("evaluatorVersion");
    const auto* applicationVersion = root.Find("applicationVersion");
    const auto* jobFingerprint = root.Find("jobFingerprint");
    const auto* projectFingerprint = root.Find("projectFingerprint");
    const auto* projectFingerprintVersion = root.Find("projectFingerprintVersion");
    const auto* timelineFingerprint = root.Find("timelineFingerprint");
    const auto* rendererId = root.Find("rendererId");
    const auto* filePrefix = root.Find("filePrefix");
    const auto* scaleQuality = root.Find("scaleQualityToResolution");
    const auto* seed = root.Find("seed");
    const auto* start = root.Find("startTimeSeconds");
    const auto* completed = root.Find("completedFrames");
    std::uint64_t parsedSeed = 0U;
    if (!NumberToUInt32(root.Find("schema"), manifest.schema) ||
        !NumberToUInt32(root.Find("width"), manifest.width) ||
        !NumberToUInt32(root.Find("height"), manifest.height) ||
        !NumberToUInt32(root.Find("dpi"), manifest.dpi) ||
        !NumberToUInt32(root.Find("frameRateNumerator"), manifest.frameRate.numerator) ||
        !NumberToUInt32(root.Find("frameRateDenominator"), manifest.frameRate.denominator) ||
        !NumberToUInt32(root.Find("frameCount"), manifest.frameCount) ||
        !NumberToUInt32(root.Find("firstFrameNumber"), manifest.firstFrameNumber) ||
        !NumberToUInt32(root.Find("frameNumberDigits"), manifest.frameNumberDigits) ||
        !evaluatorVersion || !evaluatorVersion->IsString() ||
        !applicationVersion || !applicationVersion->IsString() ||
        !jobFingerprint || !jobFingerprint->IsString() ||
        !projectFingerprint || !projectFingerprint->IsString() ||
        !projectFingerprintVersion || !projectFingerprintVersion->IsString() ||
        !timelineFingerprint || !timelineFingerprint->IsString() ||
        !rendererId || !rendererId->IsString() ||
        !filePrefix || !filePrefix->IsString() ||
        !scaleQuality || !scaleQuality->IsBool() ||
        !ParseUnsignedString(*seed, parsedSeed) || !start ||
        !ParseStableDouble(*start, manifest.startTimeSeconds) ||
        !completed || !completed->IsArray()) {
        error = "The frame-sequence manifest is missing required typed fields.";
        return false;
    }
    manifest.evaluatorVersion = evaluatorVersion->AsString();
    manifest.applicationVersion = applicationVersion->AsString();
    manifest.jobFingerprint = jobFingerprint->AsString();
    manifest.projectFingerprint = projectFingerprint->AsString();
    manifest.projectFingerprintVersion = projectFingerprintVersion->AsString();
    manifest.timelineFingerprint = timelineFingerprint->AsString();
    manifest.rendererId = rendererId->AsString();
    manifest.filePrefix = filePrefix->AsString();
    manifest.scaleQualityToResolution = scaleQuality->AsBool();
    manifest.seed = parsedSeed;
    if (manifest.frameCount == 0U || manifest.frameCount > kMaximumFrameCount ||
        !IsValidFrameRate(manifest.frameRate) ||
        manifest.width == 0U || manifest.height == 0U || manifest.dpi == 0U ||
        !std::isfinite(manifest.startTimeSeconds) || manifest.startTimeSeconds < 0.0 ||
        !IsSafeFilePrefix(manifest.filePrefix) || manifest.frameNumberDigits == 0U ||
        manifest.frameNumberDigits > kMaximumFrameNumberDigits ||
        !IsSupportedFrameSequenceRenderer(manifest.rendererId) ||
        manifest.applicationVersion.empty() ||
        manifest.jobFingerprint.empty() || manifest.projectFingerprint.empty() ||
        manifest.projectFingerprintVersion != "mw-render-state-v3-exact-precision-plan" ||
        manifest.timelineFingerprint.empty() ||
        manifest.completedFrames.size() > manifest.frameCount) {
        error = "The frame-sequence manifest exceeds supported bounds.";
        return false;
    }
    std::set<std::uint32_t> indexes;
    for (const auto& value : completed->AsArray()) {
        FrameSequenceManifestEntry entry;
        if (!ParseManifestEntry(value, entry) || entry.frameIndex >= manifest.frameCount ||
            !indexes.insert(entry.frameIndex).second) {
            error = "The frame-sequence manifest contains an invalid or duplicate frame entry.";
            return false;
        }
        manifest.completedFrames.push_back(std::move(entry));
    }
    std::sort(manifest.completedFrames.begin(), manifest.completedFrames.end(),
              [](const FrameSequenceManifestEntry& left,
                 const FrameSequenceManifestEntry& right) {
        return left.frameIndex < right.frameIndex;
    });
    return true;
}

bool RunFrameSequenceExport(const FrameSequenceExportJob& job,
                            bool allowResume,
                            const FrameSequenceRenderCallback& renderFrame,
                            const FrameSequenceValidateCallback& validateFrame,
                            const FrameSequenceProgressCallback& progressCallback,
                            const FrameSequenceCancellationCallback& cancellationCallback,
                            FrameSequenceExportResult& result,
                            std::string& error) {
    result = {};
    error.clear();
    if (!renderFrame || !validateFrame) {
        error = "Frame rendering and validation callbacks are required.";
        return false;
    }
    FrameSequenceExportJob validated;
    if (!BuildFrameSequenceExportJob(job.basePreset, job.timeline, job.settings,
                                     job.outputDirectory, validated, error)) {
        return false;
    }
    if (validated.jobFingerprint != job.jobFingerprint ||
        validated.projectFingerprint != job.projectFingerprint ||
        validated.timelineFingerprint != job.timelineFingerprint) {
        error = "The supplied frame export job was modified after it was built.";
        return false;
    }
    if (!EnsureDirectory(job.outputDirectory, error) ||
        !EnsureDirectory(FrameSequenceMetadataDirectory(job), error)) {
        return false;
    }

    const std::filesystem::path manifestPath = FrameSequenceManifestPath(job);
    result.manifestPath = manifestPath;
    FrameSequenceManifest manifest;
    std::error_code code;
    const bool manifestExists = std::filesystem::exists(manifestPath, code);
    if (code) {
        error = "The frame manifest path could not be inspected: " + code.message();
        return false;
    }
    if (manifestExists) {
        if (!allowResume) {
            error = "A frame-sequence manifest already exists. Enable Resume or choose another folder.";
            return false;
        }
        if (!LoadFrameSequenceManifest(manifestPath, manifest, error) ||
            !ManifestMatchesJob(manifest, job, error)) {
            return false;
        }
    } else {
        manifest = ManifestForJob(job);
        if (!SaveFrameSequenceManifest(manifestPath, manifest, error)) return false;
    }

    for (std::uint32_t frameIndex = 0U; frameIndex < job.settings.frameCount; ++frameIndex) {
        if (cancellationCallback && cancellationCallback()) {
            result.cancelled = true;
            return true;
        }
        const std::filesystem::path finalPath = FrameSequenceFinalFramePath(job, frameIndex);
        const std::filesystem::path temporaryPath = TemporaryFramePath(job, frameIndex);
        FrameSequenceManifestEntry entry;
        if (ContainsFrame(manifest, frameIndex, &entry)) {
            if (entry.fileName != finalPath.filename().string() ||
                !VerifyFileAgainstEntry(finalPath, entry, job, validateFrame, error)) {
                return false;
            }
            ++result.resumedFrames;
            if (progressCallback) {
                progressCallback({result.renderedFrames + result.resumedFrames,
                                  job.settings.frameCount, frameIndex, true});
            }
            continue;
        }

        bool receiptFound = false;
        if (!LoadReceipt(job, frameIndex, entry, receiptFound, error)) return false;
        if (receiptFound) {
            if (entry.fileName != finalPath.filename().string()) {
                error = "A prepared frame receipt names an unexpected output file.";
                return false;
            }
            if (std::filesystem::exists(finalPath, code) && !code) {
                if (!VerifyFileAgainstEntry(finalPath, entry, job, validateFrame, error)) return false;
            } else {
                code.clear();
                if (!std::filesystem::exists(temporaryPath, code) || code) {
                    error = "A prepared frame receipt exists, but neither its temporary nor final frame is available.";
                    return false;
                }
                if (!VerifyFileAgainstEntry(temporaryPath, entry, job, validateFrame, error) ||
                    !PromoteNewFile(temporaryPath, finalPath, error)) {
                    return false;
                }
            }
            AddManifestEntry(manifest, entry);
            if (!SaveFrameSequenceManifest(manifestPath, manifest, error)) return false;
            ++result.resumedFrames;
            if (progressCallback) {
                progressCallback({result.renderedFrames + result.resumedFrames,
                                  job.settings.frameCount, frameIndex, true});
            }
            continue;
        }

        code.clear();
        if (std::filesystem::exists(finalPath, code)) {
            if (code) {
                error = "A frame output path could not be inspected: " + code.message();
            } else {
                error = "An untracked final frame already exists. Use a clean folder or restore its matching manifest.";
            }
            return false;
        }
        code.clear();
        if (std::filesystem::exists(temporaryPath, code) && !code) {
            std::filesystem::remove(temporaryPath, code);
            if (code) {
                error = "An incomplete temporary frame could not be removed: " + code.message();
                return false;
            }
        }

        FrameSequenceFrameRequest request;
        request.frameIndex = frameIndex;
        request.timeSeconds = FrameTimeForIndex(job.settings, frameIndex);
        request.seed = FrameSeed(job.settings.seed, frameIndex);
        AnimationEvaluationResult evaluated;
        if (!EvaluateGeneralAnimation(job.basePreset, job.timeline, request.timeSeconds,
                                      request.seed, evaluated, error)) {
            return false;
        }
        request.framePreset = std::move(evaluated.framePreset);
        if (!request.framePreset.exactCamera.has_value()) {
            error = "Frame-sequence animation did not produce canonical exact camera state.";
            return false;
        }
        const bool exactDirectRenderer = IsCpuExactDirectRenderer(job.settings.rendererId);
        if (exactDirectRenderer && !ValidateExactDirectPreset(request.framePreset, error)) {
            error = "The selected exact frame renderer cannot execute frame " +
                    std::to_string(frameIndex) + " safely: " + error;
            return false;
        }
        PrecisionBackendCapabilities capabilities;
        capabilities.cpuBoost512Reference = IsCpuExactBoost512Renderer(job.settings.rendererId);
        capabilities.cpuBoost2048Direct = IsCpuExactBoost2048Renderer(job.settings.rendererId);
    capabilities.cpuBoost8192Direct = IsCpuExactBoost8192Renderer(job.settings.rendererId);
        capabilities.cpuBoost16384Direct = IsCpuExactBoost16384Renderer(job.settings.rendererId);
        const PrecisionPlan framePrecisionPlan = BuildPrecisionPlan(
            *request.framePreset.exactCamera, request.framePreset.equation,
            exactDirectRenderer ? PrecisionMode::ArbitraryPrecisionPerturbation : PrecisionMode::Float64,
            capabilities);
        const PrecisionExecutionBackend requiredBackend = !exactDirectRenderer
            ? PrecisionExecutionBackend::CpuFloat64
            : (ExactDirectRendererBits(job.settings.rendererId) == 16384
                ? PrecisionExecutionBackend::CpuBoost16384Direct
                : (ExactDirectRendererBits(job.settings.rendererId) == 8192
                ? PrecisionExecutionBackend::CpuBoost8192Direct
                : (ExactDirectRendererBits(job.settings.rendererId) == 2048
                    ? PrecisionExecutionBackend::CpuBoost2048Direct
                    : PrecisionExecutionBackend::CpuBoost512Reference)));
        if (framePrecisionPlan.backend != requiredBackend) {
            error = "The selected frame export renderer cannot execute frame " +
                    std::to_string(frameIndex) + " safely: " + framePrecisionPlan.reason;
            return false;
        }
        request.precisionPlan = framePrecisionPlan;
        if (!renderFrame(request, temporaryPath, error)) {
            std::error_code removeCode;
            std::filesystem::remove(temporaryPath, removeCode);
            if (cancellationCallback && cancellationCallback()) {
                result.cancelled = true;
                return true;
            }
            return false;
        }
        if (cancellationCallback && cancellationCallback()) {
            std::error_code removeCode;
            std::filesystem::remove(temporaryPath, removeCode);
            result.cancelled = true;
            return true;
        }
        if (!validateFrame(temporaryPath, job.settings.width, job.settings.height, error)) {
            std::error_code removeCode;
            std::filesystem::remove(temporaryPath, removeCode);
            return false;
        }
        entry.frameIndex = frameIndex;
        entry.timeSeconds = request.timeSeconds;
        entry.fileName = finalPath.filename().string();
        if (!FileDigest(temporaryPath, entry.fileSizeBytes, entry.contentDigest, error) ||
            !SaveReceipt(job, entry, error) ||
            !PromoteNewFile(temporaryPath, finalPath, error)) {
            return false;
        }
        AddManifestEntry(manifest, entry);
        if (!SaveFrameSequenceManifest(manifestPath, manifest, error)) return false;
        ++result.renderedFrames;
        if (progressCallback) {
            progressCallback({result.renderedFrames + result.resumedFrames,
                              job.settings.frameCount, frameIndex, false});
        }
    }
    return true;
}

} // namespace mw
