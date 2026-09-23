#pragma once

#include "Core/FrameSequenceExport.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace mw {

inline constexpr const char* kExternalVideoEncoderId = "ffmpeg-libx264-mp4-v1";

struct ExternalEncoderCapabilities {
    std::string versionLine;
    bool hasLibx264{false};
    bool hasMp4Muxer{false};
};

struct ExternalVideoExportSettings {
    std::filesystem::path ffmpegExecutable;
    std::filesystem::path frameSequenceDirectory;
    std::filesystem::path outputPath;
    std::string encoderPreset{"medium"};
    std::string encoderVersionLine;
    std::uint32_t crf{18U};
    bool cleanupFramesAfterSuccess{false};
};

struct ExternalVideoExportJob {
    FrameSequenceManifest sequenceManifest;
    ExternalVideoExportSettings settings;
    std::filesystem::path temporaryOutputPath;
    std::filesystem::path stdoutLogPath;
    std::filesystem::path stderrLogPath;
    std::vector<std::wstring> encodeArguments;
    std::vector<std::wstring> verifyArguments;
    std::string jobFingerprint;
};

struct ExternalProcessResult {
    int exitCode{-1};
    bool cancelled{false};
    std::string standardOutput;
    std::string standardError;
};

struct ExternalVideoExportProgress {
    std::uint32_t encodedFrames{0U};
    std::uint32_t totalFrames{0U};
};

struct ExternalVideoExportResult {
    bool cancelled{false};
    bool sourceFramesCleaned{false};
    std::filesystem::path outputPath;
    std::filesystem::path stdoutLogPath;
    std::filesystem::path stderrLogPath;
    std::string cleanupWarning;
};

using ExternalProcessRunner =
    std::function<bool(const std::filesystem::path& executable,
                       const std::vector<std::wstring>& arguments,
                       const std::function<bool()>& cancellationCallback,
                       const std::function<void(std::string_view)>& stdoutChunkCallback,
                       ExternalProcessResult& result,
                       std::string& error)>;
using ExternalVideoCancellationCallback = std::function<bool()>;
using ExternalVideoProgressCallback =
    std::function<void(const ExternalVideoExportProgress& progress)>;

[[nodiscard]] bool ParseFfmpegCapabilities(std::string_view versionOutput,
                                           std::string_view encoderOutput,
                                           std::string_view muxerOutput,
                                           ExternalEncoderCapabilities& capabilities,
                                           std::string& error);

[[nodiscard]] std::wstring QuoteWindowsProcessArgument(std::wstring_view argument);
[[nodiscard]] std::wstring BuildWindowsCommandLine(
    const std::vector<std::wstring>& arguments);

bool LoadVerifiedFrameSequence(
    const std::filesystem::path& frameSequenceDirectory,
    FrameSequenceManifest& manifest,
    std::string& error,
    const std::function<bool()>& cancellationCallback = {});

bool BuildExternalVideoExportJob(const FrameSequenceManifest& manifest,
                                 const ExternalVideoExportSettings& settings,
                                 ExternalVideoExportJob& job,
                                 std::string& error);

bool RunExternalVideoExport(const ExternalVideoExportJob& job,
                            const ExternalProcessRunner& processRunner,
                            const ExternalVideoCancellationCallback& cancellationCallback,
                            const ExternalVideoProgressCallback& progressCallback,
                            ExternalVideoExportResult& result,
                            std::string& error);

} // namespace mw
