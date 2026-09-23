#pragma once

#include "Core/Models.h"
#include "Rendering/GpuRenderer.h"
#include "WindowsIntegration/DesktopHost.h"
#include "WindowsIntegration/DisplayManager.h"

#ifdef _WIN32
#include <windows.h>
struct IMFPMediaPlayer;
struct IMFPMediaPlayerCallback;
#endif

#include <chrono>
#include <filesystem>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace mw {

class WallpaperController {
public:
    WallpaperController() = default;
    ~WallpaperController();

#ifdef _WIN32
    bool StartStaticGallery(HINSTANCE instance, const AppSettings& settings, const std::vector<Preset>& presets, std::string& error);
    bool StartVideo(HINSTANCE instance, const AppSettings& settings, const std::vector<Preset>& presets,
                    const std::filesystem::path& path, std::string& error);
    bool CaptureAndUseStatic(HINSTANCE instance, const AppSettings& settings, const std::vector<Preset>& presets,
                             const Preset& snapshot, const std::filesystem::path& storageDirectory,
                             std::string& savedPathUtf8, std::string& error);
    void Stop();
    void Pause(std::string reason = "Paused by user");
    bool PausePresentation(std::string reason, std::string& error);
    void Resume();
    void Tick(double deltaSeconds);
    void HandleDisplayChange();
#endif

    void UpdateConfiguration(const AppSettings& settings, const std::vector<Preset>& presets);
    [[nodiscard]] bool IsRunning() const noexcept { return running_; }
    [[nodiscard]] bool IsPaused() const noexcept { return paused_; }
    [[nodiscard]] std::string PauseReason() const { return pauseReason_; }
    [[nodiscard]] std::optional<std::string> TakeRuntimeError();
    [[nodiscard]] int CurrentStaticImageIndex() const noexcept {
#ifdef _WIN32
        return staticImageIndex_;
#else
        return 0;
#endif
    }
    [[nodiscard]] bool UsingUserStatic() const noexcept {
#ifdef _WIN32
        return userStatic_;
#else
        return false;
#endif
    }
    [[nodiscard]] bool UsingVideo() const noexcept {
#ifdef _WIN32
        return userVideo_;
#else
        return false;
#endif
    }

private:
#ifdef _WIN32
    static LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    bool RegisterWindowClass(HINSTANCE instance, std::string& error);
    bool CreateWallpaperWindow(HINSTANCE instance, std::string& error);
    bool InitialiseRendererWithRetry(std::string& error);
    bool RenderStaticSnapshot(const Preset& snapshot, std::string& error);
    bool LoadStaticImage(const std::string& pathUtf8, std::string& error);
    bool SaveStaticImage(const std::filesystem::path& path, const std::vector<std::uint32_t>& pixels,
                         int width, int height, std::string& error) const;
    bool LoadStaticImageByIndex(int requestedIndex, std::string& error);
    void ReattachIfNeeded();
    void PaintStaticImage(HDC dc);
    void HandleVideoPlaybackFailure(HRESULT result);
    void HandleVideoPlaybackLooped();
    void HandleVideoPlaybackReady();

    HWND window_{nullptr};
    HINSTANCE instance_{nullptr};
    RECT virtualBounds_{};
    std::vector<std::uint32_t> staticPixels_;
    int staticWidth_{0};
    int staticHeight_{0};
    bool userStatic_{false};
    bool userVideo_{false};
    bool videoReady_{false};
    bool videoPaintDeferred_{false};
    int staticImageIndex_{0};
    std::chrono::steady_clock::time_point lastStaticChange_{};
    std::mt19937 slideshowRandom_{std::random_device{}()};
    IMFPMediaPlayer* videoPlayer_{nullptr};
    IMFPMediaPlayerCallback* videoCallback_{nullptr};
    std::uint64_t videoLoopCount_{0};
#endif

    AppSettings settings_;
    std::vector<DisplayInfo> displays_;
    GpuRenderer renderer_;
    DesktopHost desktopHost_;
    bool running_{false};
    bool paused_{false};
    std::string pauseReason_;
    std::string lastRendererError_;
    std::string lastRuntimeError_;
    std::chrono::steady_clock::time_point lastAttachmentCheck_{};
};

} // namespace mw
