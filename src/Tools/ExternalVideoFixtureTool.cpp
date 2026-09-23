#include "Core/ExternalVideoExport.h"
#include "Core/FrameSequenceExport.h"
#include "Core/Json.h"
#include "Core/Precision/ExactCameraAdapter.h"
#include "Core/StillImageRenderer.h"
#include "WindowsIntegration/ExternalProcess.h"
#include "WindowsIntegration/ImageCodec.h"

#include <windows.h>
#include <objbase.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kFixtureWidth = 96U;
constexpr std::uint32_t kFixtureHeight = 64U;
constexpr std::uint32_t kFixtureFrameCount = 3U;
constexpr std::uint32_t kIntegratedCancellationWidth = 640U;
constexpr std::uint32_t kIntegratedCancellationHeight = 360U;
constexpr std::uint32_t kIntegratedCancellationFrameCount = 120U;
constexpr std::uint32_t kProcessTimeoutMilliseconds = 60U * 1000U;
constexpr std::uint32_t kCancellationAfterMilliseconds = 200U;
constexpr std::uint32_t kCancellationTimeoutMilliseconds = 5U * 1000U;

std::string Utf8Path(const std::filesystem::path& path) {
    const std::wstring value = path.wstring();
    if (value.empty()) return {};
    const int byteCount = WideCharToMultiByte(CP_UTF8, 0, value.data(),
                                               static_cast<int>(value.size()),
                                               nullptr, 0, nullptr, nullptr);
    if (byteCount <= 0) return {};
    std::string result(static_cast<std::size_t>(byteCount), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()),
                        result.data(), byteCount, nullptr, nullptr);
    return result;
}

bool RunProbe(const std::filesystem::path& executable,
              const std::vector<std::wstring>& arguments,
              std::string& output,
              std::string& error) {
    mw::ExternalProcessResult result;
    if (!mw::RunOwnedProcess(executable, arguments, {}, {}, result, error,
                             kProcessTimeoutMilliseconds)) {
        return false;
    }
    if (result.cancelled || result.exitCode != 0) {
        error = "The real FFmpeg capability probe failed.";
        return false;
    }
    output = std::move(result.standardOutput);
    output.push_back('\n');
    output.append(result.standardError);
    return true;
}

bool ProbeFfmpeg(const std::filesystem::path& executable,
                 mw::ExternalEncoderCapabilities& capabilities,
                 std::string& error) {
    std::string version;
    std::string encoder;
    std::string muxer;
    return RunProbe(executable, {L"-version"}, version, error) &&
           RunProbe(executable, {L"-hide_banner", L"-h", L"encoder=libx264"},
                    encoder, error) &&
           RunProbe(executable, {L"-hide_banner", L"-h", L"muxer=mp4"},
                    muxer, error) &&
           mw::ParseFfmpegCapabilities(version, encoder, muxer, capabilities, error);
}

bool RenderFixtureSequence(const std::filesystem::path& directory,
                           std::uint32_t width,
                           std::uint32_t height,
                           std::string filePrefix,
                           mw::FrameSequenceManifest& manifest,
                           std::string& error) {
    mw::Preset preset = mw::BuiltInPresets().front();
    preset.maximumIterations = 96;
    if (!mw::EnsureExactCamera(preset, error)) return false;

    mw::FrameSequenceExportSettings settings;
    settings.width = width;
    settings.height = height;
    settings.dpi = 96U;
    settings.tileWidth = 32U;
    settings.frameRate = {3U, 1U};
    settings.frameCount = kFixtureFrameCount;
    settings.seed = 0x4d5750483039ULL;
    settings.rendererId = "cpu-production-still";
    settings.applicationVersion = MW_PROJECT_VERSION;
    settings.filePrefix = std::move(filePrefix);
    settings.firstFrameNumber = 0U;
    settings.frameNumberDigits = 4U;
    settings.scaleQualityToResolution = false;

    mw::AnimationTimeline timeline;
    timeline.id = "ffmpeg-fixture-timeline";
    timeline.durationSeconds = 1.0;
    timeline.loopMode = mw::AnimationLoopMode::Clamp;
    mw::FrameSequenceExportJob job;
    if (!mw::BuildFrameSequenceExportJob(preset, timeline, settings, directory, job, error)) {
        return false;
    }

    mw::FrameSequenceExportResult exportResult;
    const bool exported = mw::RunFrameSequenceExport(
        job, false,
        [&job](const mw::FrameSequenceFrameRequest& frame,
               const std::filesystem::path& temporaryPath,
               std::string& renderError) {
            mw::WicRowEncoder encoder;
            if (!encoder.Initialise(temporaryPath, mw::SavedImageFormat::Png, 90,
                                    job.settings.width, job.settings.height,
                                    job.settings.dpi, renderError)) {
                return false;
            }
            mw::StillRenderRequest request;
            request.preset = frame.framePreset;
            request.width = job.settings.width;
            request.height = job.settings.height;
            request.tileWidth = job.settings.tileWidth;
            request.previewMaximumWidth = 0U;
            request.previewMaximumHeight = 0U;
            request.timeSeconds = frame.timeSeconds;
            request.scaleQualityToResolution = false;
            mw::StillRenderResult renderResult;
            const auto writeRow = [&encoder](std::uint32_t,
                                              std::span<const std::uint32_t> row,
                                              std::string& writerError) {
                return encoder.WriteRow(row, writerError);
            };
            if (!mw::RenderStillImageTiled(request, writeRow, {}, {},
                                           renderResult, renderError)) {
                return false;
            }
            return encoder.Commit(renderError);
        },
        [](const std::filesystem::path& path, std::uint32_t width,
           std::uint32_t height, std::string& validationError) {
            return mw::ValidateImageDimensionsWithWic(path, width, height,
                                                       validationError);
        },
        {}, {}, exportResult, error);
    if (!exported || exportResult.cancelled ||
        exportResult.renderedFrames != kFixtureFrameCount) {
        if (error.empty()) error = "The production frame-sequence fixture did not complete.";
        return false;
    }
    return mw::LoadVerifiedFrameSequence(directory, manifest, error) &&
           manifest.completedFrames.size() == kFixtureFrameCount;
}

bool RunFixtureProcess(const std::filesystem::path& executable,
                       const std::vector<std::wstring>& arguments,
                       const std::function<bool()>& cancellation,
                       const std::function<void(std::string_view)>& stdoutChunk,
                       mw::ExternalProcessResult& processResult,
                       std::string& processError) {
    return mw::RunOwnedProcess(executable, arguments, cancellation, stdoutChunk,
                               processResult, processError,
                               kProcessTimeoutMilliseconds);
}

bool RunExpectedEncodeFailure(const std::filesystem::path& executable,
                              const mw::ExternalEncoderCapabilities& capabilities,
                              const std::filesystem::path& artifactDirectory,
                              mw::ExternalVideoExportJob& failureJob,
                              mw::ExternalVideoExportResult& failureResult,
                              std::string& error) {
    const std::filesystem::path frameDirectory = artifactDirectory / "failure-frames";
    mw::FrameSequenceManifest manifest;
    if (!RenderFixtureSequence(frameDirectory, kFixtureWidth,
                               kFixtureHeight, "ffmpeg-failure",
                               manifest, error)) {
        return false;
    }

    mw::ExternalVideoExportSettings settings;
    settings.ffmpegExecutable = executable;
    settings.frameSequenceDirectory = frameDirectory;
    settings.outputPath = artifactDirectory / "expected-failure.mp4";
    settings.encoderPreset = "ultrafast";
    settings.encoderVersionLine = capabilities.versionLine;
    settings.crf = 28U;
    settings.cleanupFramesAfterSuccess = false;
    if (!mw::BuildExternalVideoExportJob(manifest, settings, failureJob, error)) {
        return false;
    }

    const std::filesystem::path heldDirectory =
        artifactDirectory / ".mw-fixture-held-failure-frames";
    bool runnerInvoked = false;
    const auto failAfterPreflight =
        [&](const std::filesystem::path& processExecutable,
            const std::vector<std::wstring>& arguments,
            const std::function<bool()>& cancellation,
            const std::function<void(std::string_view)>& stdoutChunk,
            mw::ExternalProcessResult& processResult,
            std::string& processError) {
            if (runnerInvoked) {
                processError = "The expected-failure runner was invoked more than once.";
                return false;
            }
            runnerInvoked = true;
            std::error_code moveError;
            std::filesystem::rename(frameDirectory, heldDirectory, moveError);
            if (moveError) {
                processError = "The fixture source sequence could not be withheld: " +
                    moveError.message();
                return false;
            }
            const bool ran = mw::RunOwnedProcess(
                processExecutable, arguments, cancellation, stdoutChunk,
                processResult, processError, kProcessTimeoutMilliseconds);
            std::error_code restoreError;
            std::filesystem::rename(heldDirectory, frameDirectory, restoreError);
            if (restoreError) {
                processError = "The fixture source sequence could not be restored: " +
                    restoreError.message();
                return false;
            }
            return ran;
        };
    std::string expectedError;
    const bool unexpectedlySucceeded = mw::RunExternalVideoExport(
        failureJob, failAfterPreflight, {}, {}, failureResult, expectedError);
    if (unexpectedlySucceeded ||
        expectedError.find("FFmpeg encoding failed") == std::string::npos) {
        error = unexpectedlySucceeded
            ? "The source-unavailability fixture unexpectedly encoded."
            : "The real encoder failure did not reach the expected production boundary: " +
                expectedError;
        return false;
    }

    std::error_code filesystemError;
    if (std::filesystem::exists(settings.outputPath, filesystemError) || filesystemError ||
        std::filesystem::exists(failureJob.temporaryOutputPath, filesystemError) ||
        filesystemError ||
        !std::filesystem::is_regular_file(failureResult.stdoutLogPath, filesystemError) ||
        filesystemError ||
        !std::filesystem::is_regular_file(failureResult.stderrLogPath, filesystemError) ||
        filesystemError) {
        error = "The real encoder failure did not preserve the expected file boundary.";
        return false;
    }
    const auto stderrSize = std::filesystem::file_size(failureResult.stderrLogPath,
                                                       filesystemError);
    if (filesystemError || stderrSize == 0U) {
        error = "The real encoder failure did not retain a bounded diagnostic log.";
        return false;
    }

    mw::FrameSequenceManifest preservedManifest;
    if (!mw::LoadVerifiedFrameSequence(frameDirectory, preservedManifest, error) ||
        preservedManifest != manifest) {
        if (error.empty()) error = "The failed encode changed its verified source sequence.";
        return false;
    }
    error.clear();
    return true;
}

bool BuildLinkedCancellationSequence(const std::filesystem::path& directory,
                                     const std::filesystem::path& seedFrame,
                                     mw::FrameSequenceManifest& manifest,
                                     std::string& error) {
    mw::Preset preset = mw::BuiltInPresets().front();
    preset.maximumIterations = 96;
    if (!mw::EnsureExactCamera(preset, error)) return false;

    mw::FrameSequenceExportSettings settings;
    settings.width = kIntegratedCancellationWidth;
    settings.height = kIntegratedCancellationHeight;
    settings.dpi = 96U;
    settings.tileWidth = 128U;
    settings.frameRate = {60U, 1U};
    settings.frameCount = kIntegratedCancellationFrameCount;
    settings.seed = 0x4d575048303943ULL;
    settings.rendererId = "cpu-production-still";
    settings.applicationVersion = MW_PROJECT_VERSION;
    settings.filePrefix = "ffmpeg-cancel";
    settings.firstFrameNumber = 0U;
    settings.frameNumberDigits = 4U;
    settings.scaleQualityToResolution = false;

    mw::AnimationTimeline timeline;
    timeline.id = "ffmpeg-cancellation-timeline";
    timeline.durationSeconds = 10.0;
    timeline.loopMode = mw::AnimationLoopMode::Clamp;
    mw::FrameSequenceExportJob job;
    if (!mw::BuildFrameSequenceExportJob(preset, timeline, settings, directory,
                                          job, error)) {
        return false;
    }

    mw::FrameSequenceExportResult exportResult;
    const bool exported = mw::RunFrameSequenceExport(
        job, false,
        [&seedFrame](const mw::FrameSequenceFrameRequest&,
                     const std::filesystem::path& temporaryPath,
                     std::string& renderError) {
            std::error_code linkError;
            std::filesystem::create_hard_link(seedFrame, temporaryPath, linkError);
            if (!linkError) return true;
            linkError.clear();
            std::filesystem::copy_file(seedFrame, temporaryPath,
                                       std::filesystem::copy_options::none, linkError);
            if (linkError) {
                renderError = "The cancellation fixture frame could not be linked or copied: " +
                    linkError.message();
                return false;
            }
            return true;
        },
        [](const std::filesystem::path& path, std::uint32_t width,
           std::uint32_t height, std::string& validationError) {
            return mw::ValidateImageDimensionsWithWic(path, width, height,
                                                       validationError);
        },
        {}, {}, exportResult, error);
    if (!exported || exportResult.cancelled ||
        exportResult.renderedFrames != kIntegratedCancellationFrameCount) {
        if (error.empty()) error = "The linked cancellation source sequence did not complete.";
        return false;
    }
    return mw::LoadVerifiedFrameSequence(directory, manifest, error) &&
           manifest.completedFrames.size() == kIntegratedCancellationFrameCount;
}

bool RunIntegratedExportCancellation(const std::filesystem::path& executable,
                                     const mw::ExternalEncoderCapabilities& capabilities,
                                     const std::filesystem::path& artifactDirectory,
                                     mw::ExternalVideoExportJob& cancellationJob,
                                     mw::ExternalVideoExportResult& cancellationResult,
                                     std::uint64_t& elapsedMilliseconds,
                                     int& processExitCode,
                                     std::string& error) {
    const std::filesystem::path seedDirectory = artifactDirectory / "cancellation-seed";
    mw::FrameSequenceManifest seedManifest;
    if (!RenderFixtureSequence(seedDirectory, kIntegratedCancellationWidth,
                               kIntegratedCancellationHeight, "ffmpeg-cancel-seed",
                               seedManifest, error)) {
        return false;
    }
    const std::filesystem::path seedFrame =
        seedDirectory / seedManifest.completedFrames.front().fileName;
    const std::filesystem::path frameDirectory = artifactDirectory / "cancellation-frames";
    mw::FrameSequenceManifest manifest;
    if (!BuildLinkedCancellationSequence(frameDirectory, seedFrame, manifest, error)) {
        return false;
    }

    mw::ExternalVideoExportSettings settings;
    settings.ffmpegExecutable = executable;
    settings.frameSequenceDirectory = frameDirectory;
    settings.outputPath = artifactDirectory / "cancelled-export.mp4";
    settings.encoderPreset = "veryslow";
    settings.encoderVersionLine = capabilities.versionLine;
    settings.crf = 28U;
    settings.cleanupFramesAfterSuccess = false;
    if (!mw::BuildExternalVideoExportJob(manifest, settings, cancellationJob, error)) {
        return false;
    }

    bool runnerStarted = false;
    bool processCancelled = false;
    const auto started = std::chrono::steady_clock::now();
    const auto cancellation = [&runnerStarted]() { return runnerStarted; };
    const auto runner =
        [&](const std::filesystem::path& processExecutable,
            const std::vector<std::wstring>& arguments,
            const std::function<bool()>& cancel,
            const std::function<void(std::string_view)>& stdoutChunk,
            mw::ExternalProcessResult& processResult,
            std::string& processError) {
            runnerStarted = true;
            const bool ran = mw::RunOwnedProcess(
                processExecutable, arguments, cancel, stdoutChunk,
                processResult, processError, kProcessTimeoutMilliseconds);
            processCancelled = processResult.cancelled;
            processExitCode = processResult.exitCode;
            return ran;
        };
    const bool cancelled = mw::RunExternalVideoExport(
        cancellationJob, runner, cancellation, {}, cancellationResult, error);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    elapsedMilliseconds = static_cast<std::uint64_t>(std::max<std::int64_t>(0, elapsed.count()));
    if (!cancelled || !runnerStarted || !processCancelled ||
        !cancellationResult.cancelled) {
        if (error.empty()) {
            error = "The integrated real video export did not terminate through owned cancellation.";
        }
        return false;
    }

    std::error_code filesystemError;
    if (std::filesystem::exists(settings.outputPath, filesystemError) || filesystemError ||
        std::filesystem::exists(cancellationJob.temporaryOutputPath, filesystemError) ||
        filesystemError ||
        !std::filesystem::is_regular_file(cancellationResult.stdoutLogPath, filesystemError) ||
        filesystemError ||
        !std::filesystem::is_regular_file(cancellationResult.stderrLogPath, filesystemError) ||
        filesystemError) {
        error = "Integrated cancellation did not retain the expected output/log boundary.";
        return false;
    }
    mw::FrameSequenceManifest preservedManifest;
    if (!mw::LoadVerifiedFrameSequence(frameDirectory, preservedManifest, error) ||
        preservedManifest != manifest) {
        if (error.empty()) error = "Integrated cancellation changed its verified source sequence.";
        return false;
    }
    error.clear();
    return true;
}

bool RunOwnedCancellationProbe(const std::filesystem::path& executable,
                               const std::filesystem::path& verifiedVideo,
                               std::uint64_t& elapsedMilliseconds,
                               int& processExitCode,
                               std::string& error) {
    const auto started = std::chrono::steady_clock::now();
    const auto cancel = [&started]() {
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);
        return elapsed.count() >= static_cast<std::int64_t>(kCancellationAfterMilliseconds);
    };
    mw::ExternalProcessResult result;
    const bool ran = mw::RunOwnedProcess(
        executable,
        {L"-hide_banner", L"-nostdin", L"-v", L"error", L"-re",
         L"-stream_loop", L"-1", L"-i", verifiedVideo.wstring(),
         L"-map", L"0:v:0", L"-f", L"null", L"-"},
        cancel, {}, result, error, kCancellationTimeoutMilliseconds);
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started);
    elapsedMilliseconds = static_cast<std::uint64_t>(std::max<std::int64_t>(0, elapsed.count()));
    processExitCode = result.exitCode;
    if (!ran || !result.cancelled) {
        if (error.empty()) {
            error = "The real owned-process cancellation probe did not report cancellation.";
        }
        return false;
    }
    if (elapsedMilliseconds >= kCancellationTimeoutMilliseconds) {
        error = "The real owned-process cancellation probe exceeded its bounded timeout.";
        return false;
    }
    return true;
}

bool WriteReport(const std::filesystem::path& reportPath,
                 const std::filesystem::path& ffmpeg,
                 const mw::ExternalEncoderCapabilities& capabilities,
                 const mw::ExternalVideoExportJob& job,
                 const mw::ExternalVideoExportResult& result,
                 std::uint32_t maximumProgressFrame,
                 std::uint64_t outputBytes,
                 std::uint64_t cancellationElapsedMilliseconds,
                 int cancellationExitCode,
                 const mw::ExternalVideoExportJob& failureJob,
                 const mw::ExternalVideoExportResult& failureResult,
                 const mw::ExternalVideoExportJob& integratedCancellationJob,
                 const mw::ExternalVideoExportResult& integratedCancellationResult,
                 std::uint64_t integratedCancellationElapsedMilliseconds,
                 int integratedCancellationExitCode,
                 std::string& error) {
    const mw::json::Value report(mw::json::Value::Object{
        {"schema", 1.0},
        {"fixture", "real-ffmpeg-production-export"},
        {"applicationVersion", MW_PROJECT_VERSION},
        {"ffmpegExecutableName", Utf8Path(ffmpeg.filename())},
        {"ffmpegPathPersisted", false},
        {"ffmpegVersion", capabilities.versionLine},
        {"libx264", capabilities.hasLibx264},
        {"mp4Muxer", capabilities.hasMp4Muxer},
        {"frameCount", static_cast<double>(job.sequenceManifest.frameCount)},
        {"width", static_cast<double>(job.sequenceManifest.width)},
        {"height", static_cast<double>(job.sequenceManifest.height)},
        {"jobFingerprint", job.jobFingerprint},
        {"maximumProgressFrame", static_cast<double>(maximumProgressFrame)},
        {"outputFile", Utf8Path(result.outputPath.filename())},
        {"outputBytes", static_cast<double>(outputBytes)},
        {"sourceFramesPreserved", !result.sourceFramesCleaned},
        {"temporaryOutputAbsent", !std::filesystem::exists(job.temporaryOutputPath)},
        {"ownedProcessCancellation", true},
        {"cancellationRequestedAfterMilliseconds",
         static_cast<double>(kCancellationAfterMilliseconds)},
        {"cancellationElapsedMilliseconds",
         static_cast<double>(cancellationElapsedMilliseconds)},
        {"cancellationExitCode", static_cast<double>(cancellationExitCode)},
        {"realEncodeFailureContained", true},
        {"failureInjection", "sequence-directory-unavailable-after-preflight"},
        {"failureWidth", static_cast<double>(failureJob.sequenceManifest.width)},
        {"failureHeight", static_cast<double>(failureJob.sequenceManifest.height)},
        {"failureSourceFramesPreserved", true},
        {"failureFinalOutputAbsent", !std::filesystem::exists(failureJob.settings.outputPath)},
        {"failureTemporaryOutputAbsent", !std::filesystem::exists(failureJob.temporaryOutputPath)},
        {"failureStderrLogFile", Utf8Path(failureResult.stderrLogPath.filename())},
        {"integratedExportCancellation", true},
        {"integratedCancellationWidth",
         static_cast<double>(integratedCancellationJob.sequenceManifest.width)},
        {"integratedCancellationHeight",
         static_cast<double>(integratedCancellationJob.sequenceManifest.height)},
        {"integratedCancellationFrameCount",
         static_cast<double>(integratedCancellationJob.sequenceManifest.frameCount)},
        {"integratedCancellationElapsedMilliseconds",
         static_cast<double>(integratedCancellationElapsedMilliseconds)},
        {"integratedCancellationExitCode",
         static_cast<double>(integratedCancellationExitCode)},
        {"integratedCancellationFinalOutputAbsent",
         !std::filesystem::exists(integratedCancellationJob.settings.outputPath)},
        {"integratedCancellationTemporaryOutputAbsent",
         !std::filesystem::exists(integratedCancellationJob.temporaryOutputPath)},
        {"integratedCancellationSourceFramesPreserved", true},
        {"integratedCancellationStdoutLogFile",
         Utf8Path(integratedCancellationResult.stdoutLogPath.filename())},
        {"integratedCancellationStderrLogFile",
         Utf8Path(integratedCancellationResult.stderrLogPath.filename())},
        {"stdoutLogFile", Utf8Path(result.stdoutLogPath.filename())},
        {"stderrLogFile", Utf8Path(result.stderrLogPath.filename())},
    });
    std::ofstream stream(reportPath, std::ios::binary | std::ios::trunc);
    if (!stream) {
        error = "The real FFmpeg fixture report could not be created.";
        return false;
    }
    stream << mw::json::Stringify(report, true) << '\n';
    if (!stream) {
        error = "The real FFmpeg fixture report could not be completed.";
        return false;
    }
    return true;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::wcerr << L"Usage: MandelbrotExternalVideoFixture <ffmpeg.exe> <new-artifact-directory>\n";
        return 2;
    }
    const std::filesystem::path ffmpeg = argv[1];
    const std::filesystem::path artifactDirectory = argv[2];
    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(ffmpeg, filesystemError) || filesystemError) {
        std::cerr << "FAIL: the explicitly supplied FFmpeg executable is not a regular file.\n";
        return 2;
    }
    if (std::filesystem::exists(artifactDirectory, filesystemError) || filesystemError) {
        std::cerr << "FAIL: the artifact directory must not already exist.\n";
        return 2;
    }
    if (!std::filesystem::create_directories(artifactDirectory, filesystemError) ||
        filesystemError) {
        std::cerr << "FAIL: the artifact directory could not be created.\n";
        return 1;
    }

    const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    const bool comInitialised = SUCCEEDED(comResult);
    if (!comInitialised && comResult != RPC_E_CHANGED_MODE) {
        std::cerr << "FAIL: Windows Imaging Component COM initialisation failed.\n";
        return 1;
    }

    int exitCode = 1;
    std::string error;
    do {
        mw::ExternalEncoderCapabilities capabilities;
        if (!ProbeFfmpeg(ffmpeg, capabilities, error)) break;

        const std::filesystem::path frameDirectory = artifactDirectory / "frames";
        mw::FrameSequenceManifest manifest;
        if (!RenderFixtureSequence(frameDirectory, kFixtureWidth, kFixtureHeight,
                                   "ffmpeg-fixture", manifest, error)) {
            break;
        }

        mw::ExternalVideoExportSettings settings;
        settings.ffmpegExecutable = ffmpeg;
        settings.frameSequenceDirectory = frameDirectory;
        settings.outputPath = artifactDirectory / "verified-fixture.mp4";
        settings.encoderPreset = "ultrafast";
        settings.encoderVersionLine = capabilities.versionLine;
        settings.crf = 28U;
        settings.cleanupFramesAfterSuccess = false;
        mw::ExternalVideoExportJob job;
        if (!mw::BuildExternalVideoExportJob(manifest, settings, job, error)) break;

        std::uint32_t maximumProgressFrame = 0U;
        mw::ExternalVideoExportResult result;
        if (!mw::RunExternalVideoExport(
                job, RunFixtureProcess, {},
                [&maximumProgressFrame](const mw::ExternalVideoExportProgress& progress) {
                    maximumProgressFrame = std::max(maximumProgressFrame,
                                                    progress.encodedFrames);
                },
                result, error)) {
            break;
        }

        std::uint64_t outputBytes = 0U;
        const auto outputSize = std::filesystem::file_size(result.outputPath, filesystemError);
        if (filesystemError || outputSize == 0U || result.outputPath != settings.outputPath ||
            result.sourceFramesCleaned || std::filesystem::exists(job.temporaryOutputPath)) {
            error = "The real FFmpeg fixture did not retain the verified final/source boundary.";
            break;
        }
        outputBytes = static_cast<std::uint64_t>(outputSize);
        mw::FrameSequenceManifest preservedManifest;
        if (!mw::LoadVerifiedFrameSequence(frameDirectory, preservedManifest, error) ||
            preservedManifest != manifest) {
            if (error.empty()) error = "The verified source sequence changed after encoding.";
            break;
        }
        std::uint64_t cancellationElapsedMilliseconds = 0U;
        int cancellationExitCode = -1;
        if (!RunOwnedCancellationProbe(ffmpeg, result.outputPath,
                                       cancellationElapsedMilliseconds,
                                       cancellationExitCode, error)) {
            break;
        }

        mw::ExternalVideoExportJob failureJob;
        mw::ExternalVideoExportResult failureResult;
        if (!RunExpectedEncodeFailure(ffmpeg, capabilities, artifactDirectory,
                                      failureJob, failureResult, error)) {
            break;
        }

        mw::ExternalVideoExportJob integratedCancellationJob;
        mw::ExternalVideoExportResult integratedCancellationResult;
        std::uint64_t integratedCancellationElapsedMilliseconds = 0U;
        int integratedCancellationExitCode = -1;
        if (!RunIntegratedExportCancellation(
                ffmpeg, capabilities, artifactDirectory,
                integratedCancellationJob, integratedCancellationResult,
                integratedCancellationElapsedMilliseconds,
                integratedCancellationExitCode, error)) {
            break;
        }
        if (!WriteReport(artifactDirectory / "report.json", ffmpeg, capabilities,
                         job, result, maximumProgressFrame, outputBytes,
                         cancellationElapsedMilliseconds, cancellationExitCode,
                         failureJob, failureResult,
                         integratedCancellationJob, integratedCancellationResult,
                         integratedCancellationElapsedMilliseconds,
                         integratedCancellationExitCode, error)) {
            break;
        }
        std::cout << "PASS: real FFmpeg libx264/MP4 capability, production PNG sequence, "
                     "fixed-vector encode, decode probe, atomic promotion and source "
                     "preservation, owned-process and integrated export cancellation, "
                     "and real encoder-failure containment completed; bytes="
                  << outputBytes << "; cancellation-ms="
                  << cancellationElapsedMilliseconds << "; integrated-cancellation-ms="
                  << integratedCancellationElapsedMilliseconds << '\n';
        exitCode = 0;
    } while (false);

    if (comInitialised) CoUninitialize();
    if (exitCode != 0) std::cerr << "FAIL: " << error << '\n';
    return exitCode;
}
