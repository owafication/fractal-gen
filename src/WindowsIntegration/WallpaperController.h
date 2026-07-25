#pragma once

#include "Core/AdaptivePerformance.h"
#include "Core/Animation.h"
#include "Core/Models.h"
#include "Rendering/GpuRenderer.h"
#include "WindowsIntegration/DesktopHost.h"
#include "WindowsIntegration/DisplayManager.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <chrono>
#include <filesystem>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace mw {

class WallpaperController {
public:
    WallpaperController() = default;
    ~WallpaperController();

#ifdef _WIN32
    bool Start(HINSTANCE instance, const AppSettings& settings, const std::vector<Preset>& presets, std::string& error);
    bool StartStaticGallery(HINSTANCE instance, const AppSettings& settings, const std::vector<Preset>& presets, std::string& error);
    bool CaptureAndUseStatic(HINSTANCE instance, const AppSettings& settings, const std::vector<Preset>& presets,
                             const Preset& snapshot, const std::filesystem::path& storageDirectory,
                             std::string& savedPathUtf8, std::string& error);
    void Stop();
    void Pause(std::string reason = "Paused by user");
    bool PauseAndReleaseGpu(std::string reason, std::string& error);
    void Resume();
    void Tick(double deltaSeconds);
    void HandleDisplayChange();
#endif

    void UpdateConfiguration(const AppSettings& settings, const std::vector<Preset>& presets);
    void SelectPreset(const std::string& presetId);
    void SetColourCyclingEnabled(bool enabled);
    void SetMotionEnabled(bool enabled);
    void SetBatteryQualityReduction(bool enabled) noexcept { batteryQualityReduction_ = enabled; }
    [[nodiscard]] bool IsRunning() const noexcept { return running_; }
    [[nodiscard]] bool IsPaused() const noexcept { return paused_; }
    [[nodiscard]] std::string PauseReason() const { return pauseReason_; }
    [[nodiscard]] double FramesPerSecond() const noexcept {
#ifdef _WIN32
        return (userStatic_ || paused_ || pausedSnapshot_ || visibleChangeDetector_.IsVisuallyIdle()) ? 0.0 : renderer_.FramesPerSecond();
#else
        return renderer_.FramesPerSecond();
#endif
    }
    [[nodiscard]] std::string GraphicsDescription() const { return renderer_.GraphicsDescription(); }
    [[nodiscard]] std::string PrecisionDescription() const { return renderer_.PrecisionDescription(); }
    [[nodiscard]] bool UsingStaticFallback() const noexcept {
#ifdef _WIN32
        return staticFallback_;
#else
        return false;
#endif
    }
    [[nodiscard]] int CurrentStaticImageIndex() const noexcept {
#ifdef _WIN32
        return staticImageIndex_;
#else
        return 0;
#endif
    }
    [[nodiscard]] bool UsingPausedSnapshot() const noexcept {
#ifdef _WIN32
        return pausedSnapshot_;
#else
        return false;
#endif
    }
    [[nodiscard]] bool UsingUserStatic() const noexcept {
#ifdef _WIN32
        return userStatic_;
#else
        return false;
#endif
    }
    [[nodiscard]] std::string LastRendererError() const { return lastRendererError_; }
    [[nodiscard]] bool IsVisuallyIdle() const noexcept { return visibleChangeDetector_.IsVisuallyIdle(); }
    [[nodiscard]] std::uint64_t SkippedInvisibleFrames() const noexcept { return visibleChangeDetector_.SkippedFrameCount(); }

private:
#ifdef _WIN32
    static LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    bool RegisterWindowClass(HINSTANCE instance, std::string& error);
    bool CreateWallpaperWindow(HINSTANCE instance, std::string& error);
    bool InitialiseRendererWithRetry(std::string& error);
    bool RenderFrame(double deltaSeconds, std::string& error);
    bool RenderStaticSnapshot(const Preset& snapshot, std::string& error);
    bool LoadStaticImage(const std::string& pathUtf8, std::string& error);
    bool SaveStaticImage(const std::filesystem::path& path, const std::vector<std::uint32_t>& pixels,
                         int width, int height, std::string& error) const;
    bool LoadStaticImageByIndex(int requestedIndex, std::string& error);
    void RebuildAnimations();
    void ReattachIfNeeded();
    void BuildStaticFallback();
    void PaintStaticFallback(HDC dc);
    const Preset* FindPreset(const std::string& id) const;

    HWND window_{nullptr};
    HINSTANCE instance_{nullptr};
    RECT virtualBounds_{};
    std::vector<std::uint32_t> fallbackPixels_;
    int fallbackWidth_{0};
    int fallbackHeight_{0};
    bool staticFallback_{false};
    bool userStatic_{false};
    bool pausedSnapshot_{false};
    int staticImageIndex_{0};
    std::chrono::steady_clock::time_point lastStaticChange_{};
    std::mt19937 slideshowRandom_{std::random_device{}()};
#endif

    AppSettings settings_;
    std::vector<Preset> presets_;
    std::vector<DisplayInfo> displays_;
    std::map<std::wstring, AnimationController> monitorAnimations_;
    AnimationController sharedAnimation_;
    GpuRenderer renderer_;
    DesktopHost desktopHost_;
    bool running_{false};
    bool paused_{false};
    std::string pauseReason_;
    bool rendererRestartAttempted_{false};
    bool batteryQualityReduction_{false};
    bool motionEnabled_{false};
    std::string lastRendererError_;
    std::chrono::steady_clock::time_point lastAttachmentCheck_{};
    VisibleChangeDetector visibleChangeDetector_;
    bool forceNextRender_{true};
};

} // namespace mw
