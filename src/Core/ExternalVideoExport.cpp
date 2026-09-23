#include "Core/ExternalVideoExport.h"

#include "Core/ProjectState.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <system_error>

namespace mw {
namespace {

constexpr std::uint64_t kFnvOffsetBasis = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;
constexpr std::size_t kMaximumCapturedLogBytes = 1024U * 1024U;

bool IsSupportedPreset(std::string_view value) noexcept {
    constexpr std::array<std::string_view, 9U> presets{
        "ultrafast", "superfast", "veryfast", "faster", "fast",
        "medium", "slow", "slower", "veryslow"};
    return std::find(presets.begin(), presets.end(), value) != presets.end();
}

bool HasMp4Extension(const std::filesystem::path& path) {
    std::wstring extension = path.extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](wchar_t character) {
        const auto lowered = std::towlower(static_cast<wint_t>(character));
        return static_cast<wchar_t>(lowered);
    });
    return extension == L".mp4";
}

bool IsRegularNonEmptyFile(const std::filesystem::path& path,
                           std::uint64_t& size,
                           std::string& error) {
    std::error_code code;
    if (!std::filesystem::is_regular_file(path, code) || code) {
        error = "The expected file is missing or is not a regular file: " + path.string();
        return false;
    }
    const auto fileSize = std::filesystem::file_size(path, code);
    if (code || fileSize == 0U) {
        error = "The expected file is empty or its size could not be read: " + path.string();
        return false;
    }
    size = static_cast<std::uint64_t>(fileSize);
    return true;
}

bool FileDigest(const std::filesystem::path& path,
                std::uint64_t& fileSize,
                std::string& digest,
                std::string& error) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        error = "A source frame could not be opened for verification: " + path.string();
        return false;
    }
    std::uint64_t hash = kFnvOffsetBasis;
    fileSize = 0U;
    std::array<char, 64U * 1024U> buffer{};
    while (stream) {
        stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = stream.gcount();
        if (count < 0) {
            error = "A source frame could not be read for verification.";
            return false;
        }
        for (std::streamsize index = 0; index < count; ++index) {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(index)]);
            hash *= kFnvPrime;
        }
        fileSize += static_cast<std::uint64_t>(count);
    }
    if (!stream.eof()) {
        error = "A source frame could not be read completely for verification.";
        return false;
    }
    std::ostringstream formatted;
    formatted << "fnv1a64:" << std::hex << std::setw(16) << std::setfill('0') << hash;
    digest = formatted.str();
    return true;
}

std::filesystem::path SequenceManifestPath(const std::filesystem::path& directory) {
    return directory / ".mw-frame-sequence" / "manifest.json";
}

std::string FramePattern(const FrameSequenceManifest& manifest) {
    std::ostringstream stream;
    stream << manifest.filePrefix << "-%0" << manifest.frameNumberDigits << "d.png";
    return stream.str();
}

std::string StableJobIdentity(const FrameSequenceManifest& manifest,
                              const ExternalVideoExportSettings& settings) {
    std::ostringstream stream;
    stream << kExternalVideoEncoderId << '\n'
           << manifest.jobFingerprint << '\n'
           << manifest.frameCount << '\n'
           << manifest.frameRate.numerator << '/' << manifest.frameRate.denominator << '\n'
           << settings.crf << '\n'
           << settings.encoderPreset << '\n'
           << settings.encoderVersionLine << '\n'
           << (settings.cleanupFramesAfterSuccess ? "cleanup" : "preserve") << '\n'
           << settings.ffmpegExecutable.generic_string() << '\n'
           << settings.outputPath.generic_string();
    return stream.str();
}

void AppendBounded(std::string& destination, std::string_view chunk) {
    if (destination.size() >= kMaximumCapturedLogBytes || chunk.empty()) return;
    const std::size_t remaining = kMaximumCapturedLogBytes - destination.size();
    destination.append(chunk.substr(0U, remaining));
}

std::uint32_t ParseProgressFrame(std::string_view chunk,
                                 std::string& pending,
                                 std::uint32_t current) {
    AppendBounded(pending, chunk);
    std::size_t lineEnd = pending.find('\n');
    while (lineEnd != std::string::npos) {
        std::string line = pending.substr(0U, lineEnd);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        pending.erase(0U, lineEnd + 1U);
        constexpr std::string_view prefix = "frame=";
        if (line.rfind(prefix, 0U) == 0U) {
            const std::string_view value(line.data() + prefix.size(), line.size() - prefix.size());
            std::uint32_t parsed = 0U;
            const auto conversion = std::from_chars(value.data(), value.data() + value.size(), parsed);
            if (conversion.ec == std::errc{} && conversion.ptr == value.data() + value.size()) {
                current = parsed;
            }
        }
        lineEnd = pending.find('\n');
    }
    return current;
}

bool WriteLog(const std::filesystem::path& path,
              std::string_view text,
              std::string& error) {
    std::error_code code;
    std::filesystem::create_directories(path.parent_path(), code);
    if (code) {
        error = "The encoder log directory could not be created: " + code.message();
        return false;
    }
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        error = "An encoder log could not be opened: " + path.string();
        return false;
    }
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    stream.flush();
    if (!stream) {
        error = "An encoder log could not be written completely: " + path.string();
        return false;
    }
    return true;
}

bool PromoteNewFile(const std::filesystem::path& temporary,
                    const std::filesystem::path& finalPath,
                    std::string& error) {
    std::error_code code;
    if (std::filesystem::exists(finalPath, code)) {
        error = "The final MP4 already exists and was not overwritten: " + finalPath.string();
        return false;
    }
    code.clear();
    std::filesystem::rename(temporary, finalPath, code);
    if (code) {
        error = "The verified temporary MP4 could not be promoted: " + code.message();
        return false;
    }
    return true;
}

bool CleanupTrackedSequence(const ExternalVideoExportJob& job,
                            const std::function<bool()>& cancellationCallback,
                            std::string& warning) {
    warning.clear();
    std::error_code code;
    for (const auto& entry : job.sequenceManifest.completedFrames) {
        if (cancellationCallback && cancellationCallback()) {
            warning = "Frame cleanup stopped after video verification; remaining source frames were preserved.";
            return false;
        }
        const std::filesystem::path framePath =
            job.settings.frameSequenceDirectory / entry.fileName;
        if (!std::filesystem::exists(framePath, code)) {
            code.clear();
            continue;
        }
        std::filesystem::remove(framePath, code);
        if (code) {
            warning = "The MP4 was verified, but at least one tracked PNG frame could not be removed: " +
                      code.message();
            return false;
        }
    }
    const std::filesystem::path metadata =
        job.settings.frameSequenceDirectory / ".mw-frame-sequence";
    if (std::filesystem::exists(metadata, code) && !code) {
        for (const auto& entry : std::filesystem::directory_iterator(metadata, code)) {
            if (code) break;
            if (entry.path().filename() == "video-export") continue;
            std::filesystem::remove_all(entry.path(), code);
            if (code) break;
        }
    }
    if (code) {
        warning = "The MP4 was verified, but frame-sequence metadata could not be removed: " +
                  code.message();
        return false;
    }
    return true;
}

} // namespace

bool ParseFfmpegCapabilities(std::string_view versionOutput,
                             std::string_view encoderOutput,
                             std::string_view muxerOutput,
                             ExternalEncoderCapabilities& capabilities,
                             std::string& error) {
    capabilities = {};
    error.clear();
    const std::size_t lineEnd = versionOutput.find_first_of("\r\n");
    capabilities.versionLine = std::string(versionOutput.substr(0U, lineEnd));
    capabilities.hasLibx264 = encoderOutput.find("Encoder libx264") != std::string_view::npos ||
                               encoderOutput.find("libx264") != std::string_view::npos;
    capabilities.hasMp4Muxer = muxerOutput.find("Muxer mp4") != std::string_view::npos ||
                                muxerOutput.find("mp4") != std::string_view::npos;
    if (capabilities.versionLine.rfind("ffmpeg version", 0U) != 0U) {
        error = "The selected executable did not report an FFmpeg version.";
        return false;
    }
    if (!capabilities.hasLibx264) {
        error = "This FFmpeg build does not provide the required libx264 H.264 encoder.";
        return false;
    }
    if (!capabilities.hasMp4Muxer) {
        error = "This FFmpeg build does not provide the required MP4 muxer.";
        return false;
    }
    return true;
}

std::wstring QuoteWindowsProcessArgument(std::wstring_view argument) {
    if (argument.empty()) return L"\"\"";
    const bool needsQuotes = argument.find_first_of(L" \t\n\v\"") != std::wstring_view::npos;
    if (!needsQuotes) return std::wstring(argument);
    std::wstring quoted;
    quoted.push_back(L'\"');
    std::size_t backslashes = 0U;
    for (const wchar_t character : argument) {
        if (character == L'\\') {
            ++backslashes;
            continue;
        }
        if (character == L'\"') {
            quoted.append(backslashes * 2U + 1U, L'\\');
            quoted.push_back(L'\"');
            backslashes = 0U;
            continue;
        }
        quoted.append(backslashes, L'\\');
        backslashes = 0U;
        quoted.push_back(character);
    }
    quoted.append(backslashes * 2U, L'\\');
    quoted.push_back(L'\"');
    return quoted;
}

std::wstring BuildWindowsCommandLine(const std::vector<std::wstring>& arguments) {
    std::wstring commandLine;
    for (const auto& argument : arguments) {
        if (!commandLine.empty()) commandLine.push_back(L' ');
        commandLine += QuoteWindowsProcessArgument(argument);
    }
    return commandLine;
}

bool LoadVerifiedFrameSequence(
    const std::filesystem::path& frameSequenceDirectory,
    FrameSequenceManifest& manifest,
    std::string& error,
    const std::function<bool()>& cancellationCallback) {
    manifest = {};
    error.clear();
    if (frameSequenceDirectory.empty()) {
        error = "Choose a frame-sequence directory.";
        return false;
    }
    if (!LoadFrameSequenceManifest(SequenceManifestPath(frameSequenceDirectory), manifest, error)) {
        return false;
    }
    if (manifest.completedFrames.size() != manifest.frameCount) {
        error = "The frame sequence is incomplete. Finish or resume PNG export before encoding.";
        return false;
    }
    std::set<std::string> names;
    for (std::uint32_t frameIndex = 0U; frameIndex < manifest.frameCount; ++frameIndex) {
        if (cancellationCallback && cancellationCallback()) {
            error = "Frame sequence verification was cancelled.";
            return false;
        }
        const auto& entry = manifest.completedFrames[static_cast<std::size_t>(frameIndex)];
        if (entry.frameIndex != frameIndex || !names.insert(entry.fileName).second) {
            error = "The frame-sequence manifest is not a complete contiguous sequence.";
            return false;
        }
        const std::filesystem::path framePath = frameSequenceDirectory / entry.fileName;
        std::uint64_t fileSize = 0U;
        std::string digest;
        if (!FileDigest(framePath, fileSize, digest, error)) return false;
        if (fileSize != entry.fileSizeBytes || digest != entry.contentDigest) {
            error = "A source frame no longer matches its verified manifest entry: " +
                    entry.fileName;
            return false;
        }
    }
    return true;
}

bool BuildExternalVideoExportJob(const FrameSequenceManifest& manifest,
                                 const ExternalVideoExportSettings& settings,
                                 ExternalVideoExportJob& job,
                                 std::string& error) {
    job = {};
    error.clear();
    if (settings.ffmpegExecutable.empty() || settings.frameSequenceDirectory.empty() ||
        settings.outputPath.empty()) {
        error = "FFmpeg, frame-sequence, and output paths are required.";
        return false;
    }
    std::error_code code;
    if (!std::filesystem::is_regular_file(settings.ffmpegExecutable, code) || code) {
        error = "The selected FFmpeg executable does not exist.";
        return false;
    }
    if (!HasMp4Extension(settings.outputPath)) {
        error = "The encoded output path must use the .mp4 extension.";
        return false;
    }
    if (settings.outputPath.filename().empty() || settings.outputPath.parent_path().empty() ||
        !std::filesystem::is_directory(settings.outputPath.parent_path(), code) || code) {
        error = "The MP4 output directory does not exist.";
        return false;
    }
    if (std::filesystem::exists(settings.outputPath, code)) {
        error = "The final MP4 already exists. Choose a new output name.";
        return false;
    }
    if (settings.crf > 51U || !IsSupportedPreset(settings.encoderPreset)) {
        error = "The H.264 quality settings are outside supported bounds.";
        return false;
    }
    if (settings.encoderVersionLine.rfind("ffmpeg version", 0U) != 0U ||
        settings.encoderVersionLine.size() > 512U) {
        error = "A verified FFmpeg version identity is required.";
        return false;
    }
    if (manifest.width == 0U || manifest.height == 0U || manifest.frameCount == 0U ||
        manifest.completedFrames.size() != manifest.frameCount ||
        !IsValidFrameRate(manifest.frameRate) || manifest.jobFingerprint.empty() ||
        manifest.filePrefix.empty() || manifest.frameNumberDigits == 0U) {
        error = "The frame-sequence manifest is incomplete or unsupported.";
        return false;
    }
    if ((manifest.width % 2U) != 0U || (manifest.height % 2U) != 0U) {
        error = "H.264 yuv420p export requires even source width and height.";
        return false;
    }

    const std::string fingerprint = Sha256Hex(StableJobIdentity(manifest, settings));
    const std::filesystem::path metadata =
        settings.frameSequenceDirectory / ".mw-frame-sequence" / "video-export";
    const std::string shortFingerprint = fingerprint.substr(0U, 16U);
    const std::wstring temporaryName = settings.outputPath.stem().wstring() + L".mw-" +
        std::wstring(shortFingerprint.begin(), shortFingerprint.end()) + L".part.mp4";
    job.sequenceManifest = manifest;
    job.settings = settings;
    job.jobFingerprint = fingerprint;
    job.temporaryOutputPath = settings.outputPath.parent_path() / temporaryName;
    job.stdoutLogPath = metadata / (fingerprint + ".stdout.log");
    job.stderrLogPath = metadata / (fingerprint + ".stderr.log");

    const std::filesystem::path inputPattern =
        settings.frameSequenceDirectory / FramePattern(manifest);
    const std::wstring frameRate = std::to_wstring(manifest.frameRate.numerator) + L"/" +
                                   std::to_wstring(manifest.frameRate.denominator);
    job.encodeArguments = {
        L"-hide_banner", L"-nostdin", L"-loglevel", L"warning", L"-nostats",
        L"-progress", L"pipe:1", L"-n", L"-framerate", frameRate,
        L"-start_number", std::to_wstring(manifest.firstFrameNumber),
        L"-i", inputPattern.wstring(), L"-frames:v", std::to_wstring(manifest.frameCount),
        L"-c:v", L"libx264", L"-preset",
        std::wstring(settings.encoderPreset.begin(), settings.encoderPreset.end()),
        L"-crf", std::to_wstring(settings.crf), L"-pix_fmt", L"yuv420p",
        L"-movflags", L"+faststart", job.temporaryOutputPath.wstring()};
    job.verifyArguments = {
        L"-hide_banner", L"-nostdin", L"-v", L"error", L"-i",
        job.temporaryOutputPath.wstring(), L"-map", L"0:v:0", L"-frames:v", L"1",
        L"-f", L"null", L"-"};
    return true;
}

bool RunExternalVideoExport(const ExternalVideoExportJob& job,
                            const ExternalProcessRunner& processRunner,
                            const ExternalVideoCancellationCallback& cancellationCallback,
                            const ExternalVideoProgressCallback& progressCallback,
                            ExternalVideoExportResult& result,
                            std::string& error) {
    result = {};
    error.clear();
    if (!processRunner) {
        error = "An external process runner is required.";
        return false;
    }
    ExternalVideoExportJob validated;
    if (!BuildExternalVideoExportJob(job.sequenceManifest, job.settings, validated, error) ||
        validated.jobFingerprint != job.jobFingerprint ||
        validated.encodeArguments != job.encodeArguments ||
        validated.verifyArguments != job.verifyArguments) {
        if (error.empty()) error = "The external video export job was modified after it was built.";
        return false;
    }
    FrameSequenceManifest currentManifest;
    if (!LoadVerifiedFrameSequence(job.settings.frameSequenceDirectory, currentManifest, error,
                                   cancellationCallback) ||
        currentManifest != job.sequenceManifest) {
        if (error.empty()) error = "The source sequence changed after the video job was built.";
        return false;
    }
    std::error_code code;
    std::filesystem::create_directories(job.temporaryOutputPath.parent_path(), code);
    if (code) {
        error = "The encoder working directory could not be created: " + code.message();
        return false;
    }
    std::filesystem::remove(job.temporaryOutputPath, code);
    if (code) {
        error = "A previous owned temporary MP4 could not be removed: " + code.message();
        return false;
    }

    ExternalProcessResult encodeResult;
    std::string progressBuffer;
    std::uint32_t encodedFrames = 0U;
    const auto cancel = [&]() {
        return cancellationCallback && cancellationCallback();
    };
    const auto stdoutChunk = [&](std::string_view chunk) {
        const std::uint32_t parsed = ParseProgressFrame(chunk, progressBuffer, encodedFrames);
        encodedFrames = std::min(parsed, job.sequenceManifest.frameCount);
        if (progressCallback) {
            progressCallback({encodedFrames, job.sequenceManifest.frameCount});
        }
    };
    if (!processRunner(job.settings.ffmpegExecutable, job.encodeArguments, cancel,
                       stdoutChunk, encodeResult, error)) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        return false;
    }
    const std::string capabilityHeader = "encoder=" + job.settings.encoderVersionLine +
        "\ncapability.libx264=true\ncapability.mp4=true\n--- encoding stdout ---\n";
    encodeResult.standardOutput.insert(0U, capabilityHeader);
    if (!WriteLog(job.stdoutLogPath, encodeResult.standardOutput, error) ||
        !WriteLog(job.stderrLogPath, encodeResult.standardError, error)) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        return false;
    }
    result.stdoutLogPath = job.stdoutLogPath;
    result.stderrLogPath = job.stderrLogPath;
    if (encodeResult.cancelled || cancel()) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        result.cancelled = true;
        return true;
    }
    if (encodeResult.exitCode != 0) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        error = "FFmpeg encoding failed. Source frames were preserved; inspect the encoder logs.";
        return false;
    }
    std::uint64_t outputSize = 0U;
    if (!IsRegularNonEmptyFile(job.temporaryOutputPath, outputSize, error)) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        return false;
    }

    ExternalProcessResult verifyResult;
    if (!processRunner(job.settings.ffmpegExecutable, job.verifyArguments, cancel,
                       {}, verifyResult, error)) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        return false;
    }
    AppendBounded(encodeResult.standardOutput, "\n--- verification stdout ---\n");
    AppendBounded(encodeResult.standardOutput, verifyResult.standardOutput);
    AppendBounded(encodeResult.standardError, "\n--- verification stderr ---\n");
    AppendBounded(encodeResult.standardError, verifyResult.standardError);
    std::string logUpdateError;
    (void)WriteLog(job.stdoutLogPath, encodeResult.standardOutput, logUpdateError);
    logUpdateError.clear();
    (void)WriteLog(job.stderrLogPath, encodeResult.standardError, logUpdateError);
    if (verifyResult.cancelled || cancel()) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        result.cancelled = true;
        return true;
    }
    if (verifyResult.exitCode != 0) {
        std::filesystem::remove(job.temporaryOutputPath, code);
        error = "The encoded MP4 could not be decoded for verification. Source frames were preserved.";
        return false;
    }
    if (!PromoteNewFile(job.temporaryOutputPath, job.settings.outputPath, error)) return false;

    result.outputPath = job.settings.outputPath;
    if (job.settings.cleanupFramesAfterSuccess) {
        result.sourceFramesCleaned = CleanupTrackedSequence(
            job, cancellationCallback, result.cleanupWarning);
    }
    if (progressCallback) {
        progressCallback({job.sequenceManifest.frameCount, job.sequenceManifest.frameCount});
    }
    return true;
}

} // namespace mw
