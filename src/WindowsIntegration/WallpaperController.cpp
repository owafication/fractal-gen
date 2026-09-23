#include "WindowsIntegration/WallpaperController.h"
#include "WindowsIntegration/ImageCodec.h"

#include "Infrastructure/Logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cwctype>
#include <new>

#ifdef _WIN32
#include <mferror.h>
#include <mfplay.h>
#include <propvarutil.h>
#endif

namespace mw {

WallpaperController::~WallpaperController() {
#ifdef _WIN32
    Stop();
#endif
}

void WallpaperController::UpdateConfiguration(const AppSettings& settings, const std::vector<Preset>& presets) {
    settings_ = settings;
    (void)presets;
#ifdef _WIN32
    if (userStatic_ && !settings_.staticWallpaper.imagePaths.empty()) {
        staticImageIndex_ = std::clamp(settings_.staticWallpaper.currentIndex, 0,
            static_cast<int>(settings_.staticWallpaper.imagePaths.size()) - 1);
    }
#endif
}

#ifdef _WIN32
namespace {
constexpr wchar_t kWallpaperClass[] = L"MandelbrotLiveWallpaperHost";
constexpr UINT kVideoPlaybackFailedMessage = WM_APP + 41U;
constexpr UINT kVideoPlaybackLoopedMessage = WM_APP + 42U;
constexpr UINT kVideoPlaybackReadyMessage = WM_APP + 43U;

class LoopingVideoCallback final : public IMFPMediaPlayerCallback {
public:
    explicit LoopingVideoCallback(HWND targetWindow) : targetWindow_(targetWindow) {}

    STDMETHODIMP QueryInterface(REFIID interfaceId, void** object) override {
        if (!object) return E_POINTER;
        if (interfaceId == __uuidof(IUnknown) || interfaceId == __uuidof(IMFPMediaPlayerCallback)) {
            *object = static_cast<IMFPMediaPlayerCallback*>(this);
            AddRef();
            return S_OK;
        }
        *object = nullptr;
        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override {
        return static_cast<ULONG>(InterlockedIncrement(&references_));
    }

    STDMETHODIMP_(ULONG) Release() override {
        const ULONG remaining = static_cast<ULONG>(InterlockedDecrement(&references_));
        if (remaining == 0U) delete this;
        return remaining;
    }

    void STDMETHODCALLTYPE OnMediaPlayerEvent(MFP_EVENT_HEADER* event) override {
        if (!event) {
            ReportFailure(E_POINTER);
            return;
        }
        if (FAILED(event->hrEvent)) {
            LogError("Video event failed: type " + std::to_string(event->eEventType) +
                     ", HRESULT " + std::to_string(static_cast<unsigned long>(event->hrEvent)) + ".");
            ReportFailure(event->hrEvent);
            return;
        }
        if (!event->pMediaPlayer) {
            ReportFailure(E_POINTER);
            return;
        }
        HRESULT result = S_OK;
        if (event->eEventType == MFP_EVENT_TYPE_MEDIAITEM_SET) {
            result = event->pMediaPlayer->Play();
        } else if (event->eEventType == MFP_EVENT_TYPE_PLAYBACK_ENDED) {
            PROPVARIANT start{};
            result = InitPropVariantFromInt64(0, &start);
            if (SUCCEEDED(result)) {
                result = event->pMediaPlayer->SetPosition(MFP_POSITIONTYPE_100NS, &start);
                PropVariantClear(&start);
            }
            if (SUCCEEDED(result)) loopPending_ = true;
        } else if (event->eEventType == MFP_EVENT_TYPE_POSITION_SET) {
            result = event->pMediaPlayer->Play();
        } else if (event->eEventType == MFP_EVENT_TYPE_PLAY && targetWindow_) {
            PostMessageW(targetWindow_, kVideoPlaybackReadyMessage, 0, 0);
            if (loopPending_ && !loopReported_) {
                PostMessageW(targetWindow_, kVideoPlaybackLoopedMessage, 0, 0);
                loopReported_ = true;
            }
            loopPending_ = false;
        }
        if (FAILED(result)) {
            LogError("Video event action failed: type " + std::to_string(event->eEventType) + ".");
            ReportFailure(result);
        }
    }

private:
    void ReportFailure(HRESULT result) const {
        if (targetWindow_) {
            PostMessageW(targetWindow_, kVideoPlaybackFailedMessage, 0,
                         static_cast<LPARAM>(static_cast<LONG_PTR>(result)));
        }
    }

    ~LoopingVideoCallback() = default;
    LONG references_{1};
    HWND targetWindow_{nullptr};
    bool loopPending_{false};
    bool loopReported_{false};
};

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) return {};
    std::string result(static_cast<std::size_t>(size), '\0');
    const int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                                             static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    if (written != size) return {};
    return result;
}

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                         static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) return {};
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    const int written = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                             static_cast<int>(text.size()), result.data(), size);
    if (written != size) return {};
    return result;
}
}

bool WallpaperController::RegisterWindowClass(HINSTANCE instance, std::string& error) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = kWallpaperClass;
    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        error = "The wallpaper window class could not be registered.";
        return false;
    }
    return true;
}

bool WallpaperController::CreateWallpaperWindow(HINSTANCE instance, std::string& error) {
    displays_ = DisplayManager::Enumerate();
    virtualBounds_ = DisplayManager::VirtualDesktopBounds(displays_);
    const int width = std::max(1L, virtualBounds_.right - virtualBounds_.left);
    const int height = std::max(1L, virtualBounds_.bottom - virtualBounds_.top);
    window_ = CreateWindowExW(WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, kWallpaperClass, L"Mandelbrot Live Wallpaper",
                              WS_POPUP | WS_VISIBLE, virtualBounds_.left, virtualBounds_.top, width, height,
                              nullptr, nullptr, instance, this);
    if (!window_) {
        error = "The desktop presentation window could not be created.";
        return false;
    }
    if (!desktopHost_.Attach(window_, error)) {
        DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    const RECT hostBounds = desktopHost_.MapDesktopRectToHost(virtualBounds_);
    SetWindowPos(window_, HWND_BOTTOM, hostBounds.left, hostBounds.top,
                 hostBounds.right - hostBounds.left, hostBounds.bottom - hostBounds.top,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
    return true;
}

bool WallpaperController::StartStaticGallery(HINSTANCE instance, const AppSettings& settings,
                                             const std::vector<Preset>& presets, std::string& error) {
    Stop();
    lastRuntimeError_.clear();
    instance_ = instance;
    settings_ = settings;
    (void)presets;
    if (settings_.staticWallpaper.imagePaths.empty()) {
        error = "No saved static renders are available.";
        return false;
    }
    if (!RegisterWindowClass(instance_, error) || !CreateWallpaperWindow(instance_, error)) return false;
    userStatic_ = true;
    userVideo_ = false;
    if (!LoadStaticImageByIndex(settings_.staticWallpaper.currentIndex, error)) {
        desktopHost_.Detach(window_);
        DestroyWindow(window_);
        window_ = nullptr;
        userStatic_ = false;
        return false;
    }
    running_ = true;
    paused_ = false;
    pauseReason_.clear();
    lastStaticChange_ = std::chrono::steady_clock::now();
    lastAttachmentCheck_ = lastStaticChange_;
    InvalidateRect(window_, nullptr, FALSE);
    LogInfo("Static wallpaper started from the saved render gallery.");
    return true;
}

bool WallpaperController::StartVideo(HINSTANCE instance, const AppSettings& settings,
                                     const std::vector<Preset>& presets,
                                     const std::filesystem::path& path, std::string& error) {
    Stop();
    lastRuntimeError_.clear();
    std::error_code fileError;
    if (path.empty() || !std::filesystem::is_regular_file(path, fileError) || fileError) {
        error = "The selected video file does not exist or is not a regular file.";
        return false;
    }
    std::wstring extension = path.extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](wchar_t value) { return static_cast<wchar_t>(std::towlower(value)); });
    if (extension != L".mp4") {
        error = "Video wallpaper currently accepts exported MP4 files only.";
        return false;
    }

    instance_ = instance;
    settings_ = settings;
    (void)presets;
    userStatic_ = false;
    userVideo_ = true;
    videoReady_ = false;
    videoPaintDeferred_ = false;
    videoLoopCount_ = 0;
    if (!RegisterWindowClass(instance_, error) || !CreateWallpaperWindow(instance_, error)) {
        userVideo_ = false;
        return false;
    }

    videoCallback_ = new (std::nothrow) LoopingVideoCallback(window_);
    if (!videoCallback_) {
        error = "The video playback callback could not be allocated.";
        Stop();
        return false;
    }
    HRESULT result = MFPCreateMediaPlayer(nullptr, FALSE, 0, videoCallback_, window_, &videoPlayer_);
    IMFPMediaItem* mediaItem = nullptr;
    if (SUCCEEDED(result)) {
        result = videoPlayer_->CreateMediaItemFromURL(path.c_str(), TRUE, 0, &mediaItem);
    }
    BOOL hasVideo = FALSE;
    BOOL selected = FALSE;
    if (SUCCEEDED(result)) result = mediaItem->HasVideo(&hasVideo, &selected);
    if (SUCCEEDED(result) && (!hasVideo || !selected)) result = MF_E_INVALIDMEDIATYPE;
    if (SUCCEEDED(result)) result = videoPlayer_->SetMute(TRUE);
    BOOL muted = FALSE;
    if (SUCCEEDED(result)) result = videoPlayer_->GetMute(&muted);
    if (SUCCEEDED(result) && !muted) result = E_FAIL;
    if (SUCCEEDED(result)) result = videoPlayer_->SetMediaItem(mediaItem);
    if (mediaItem) mediaItem->Release();
    if (FAILED(result)) {
        error = "Windows Media Foundation could not open the selected MP4 video (HRESULT " +
            std::to_string(static_cast<unsigned long>(result)) + ").";
        Stop();
        return false;
    }

    running_ = true;
    paused_ = false;
    pauseReason_.clear();
    lastAttachmentCheck_ = std::chrono::steady_clock::now();
    LogInfo("Video wallpaper mute state verified by Windows Media Foundation.");
    LogInfo("Video wallpaper started from an exported MP4 file.");
    return true;
}

bool WallpaperController::RenderStaticSnapshot(const Preset& snapshot, std::string& error) {
    const int virtualWidth = std::max(1L, virtualBounds_.right - virtualBounds_.left);
    const int virtualHeight = std::max(1L, virtualBounds_.bottom - virtualBounds_.top);
    std::vector<RenderRegion> regions;
    if (settings_.monitorMode == MonitorMode::Span) {
        RenderRegion region;
        region.pixels = {0, 0, virtualWidth, virtualHeight};
        region.camera = snapshot.camera;
        region.rotationDegrees = snapshot.rotationDegrees;
        region.palette = snapshot.palette;
        region.customPaletteColours = snapshot.customPaletteColours;
        region.equation = snapshot.equation;
        region.maximumIterations = snapshot.maximumIterations;
        region.colourOffset = snapshot.colourOffset;
        region.paletteFrequency = snapshot.paletteFrequency;
        region.paletteGamma = snapshot.paletteGamma;
        region.paletteInterpolation = snapshot.paletteInterpolation;
        region.brightness = snapshot.brightness;
        region.contrast = snapshot.contrast;
        region.saturation = snapshot.saturation;
        region.interiorColour = snapshot.interiorColour;
        region.backgroundColour = snapshot.backgroundColour;
        region.smoothColouring = snapshot.smoothColouring;
        regions.push_back(std::move(region));
    } else {
        for (const auto& display : displays_) {
            RenderRegion region;
            region.pixels = {
                display.bounds.left - virtualBounds_.left,
                display.bounds.top - virtualBounds_.top,
                display.bounds.right - virtualBounds_.left,
                display.bounds.bottom - virtualBounds_.top,
            };
            region.camera = snapshot.camera;
            region.rotationDegrees = snapshot.rotationDegrees;
            region.palette = snapshot.palette;
            region.customPaletteColours = snapshot.customPaletteColours;
            region.equation = snapshot.equation;
            region.maximumIterations = snapshot.maximumIterations;
            region.colourOffset = snapshot.colourOffset;
            region.paletteFrequency = snapshot.paletteFrequency;
            region.paletteGamma = snapshot.paletteGamma;
            region.paletteInterpolation = snapshot.paletteInterpolation;
            region.brightness = snapshot.brightness;
            region.contrast = snapshot.contrast;
            region.saturation = snapshot.saturation;
            region.interiorColour = snapshot.interiorColour;
            region.backgroundColour = snapshot.backgroundColour;
            region.smoothColouring = snapshot.smoothColouring;
            regions.push_back(std::move(region));
        }
    }
    RenderOptions options;
    options.renderScale = 1.0;
    options.antiAliasingLevel = std::clamp(settings_.performance.antiAliasingLevel, 1, 4);
    options.precision = settings_.performance.precision;
    return renderer_.Render(regions, options, error);
}

bool WallpaperController::SaveStaticImage(const std::filesystem::path& path,
                                          const std::vector<std::uint32_t>& pixels,
                                          int width, int height, std::string& error) const {
    if (width <= 0 || height <= 0) {
        error = "The captured static image dimensions are invalid.";
        return false;
    }
    return SavePixelsWithWic(path, settings_.staticWallpaper.savedImageFormat,
                             settings_.staticWallpaper.compressionQuality,
                             pixels, static_cast<std::uint32_t>(width),
                             static_cast<std::uint32_t>(height), 96.0, error);
}

bool WallpaperController::LoadStaticImage(const std::string& pathUtf8, std::string& error) {
    const std::wstring widePath = Utf8ToWide(pathUtf8);
    if (widePath.empty()) {
        error = "A saved static render path is not valid UTF-8.";
        return false;
    }
    std::vector<std::uint32_t> pixels;
    int width = 0;
    int height = 0;
    if (!LoadPixelsWithWic(std::filesystem::path(widePath), pixels, width, height, error)) return false;
    staticPixels_ = std::move(pixels);
    staticWidth_ = width;
    staticHeight_ = height;
    return true;
}

bool WallpaperController::LoadStaticImageByIndex(int requestedIndex, std::string& error) {
    const auto& paths = settings_.staticWallpaper.imagePaths;
    if (paths.empty()) {
        error = "No saved static renders are available.";
        return false;
    }
    const int count = static_cast<int>(paths.size());
    int index = ((requestedIndex % count) + count) % count;
    std::string lastError;
    for (int attempt = 0; attempt < count; ++attempt) {
        if (LoadStaticImage(paths[static_cast<std::size_t>(index)], lastError)) {
            staticImageIndex_ = index;
            settings_.staticWallpaper.currentIndex = index;
            return true;
        }
        index = (index + 1) % count;
    }
    error = lastError.empty() ? "No saved static render could be loaded." : lastError;
    return false;
}

bool WallpaperController::CaptureAndUseStatic(HINSTANCE instance, const AppSettings& settings,
                                              const std::vector<Preset>& presets, const Preset& snapshot,
                                              const std::filesystem::path& storageDirectory,
                                              std::string& savedPathUtf8, std::string& error) {
    Stop();
    lastRuntimeError_.clear();
    instance_ = instance;
    settings_ = settings;
    (void)presets;
    if (!RegisterWindowClass(instance_, error) || !CreateWallpaperWindow(instance_, error)) return false;
    if (!InitialiseRendererWithRetry(error)) {
        desktopHost_.Detach(window_);
        DestroyWindow(window_);
        window_ = nullptr;
        return false;
    }
    if (!RenderStaticSnapshot(snapshot, error)) {
        Stop();
        return false;
    }
    std::vector<std::uint32_t> pixels;
    int width = 0;
    int height = 0;
    if (!renderer_.CapturePixels(pixels, width, height, error)) {
        Stop();
        return false;
    }
    const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::wstring fileName = L"static-render-" + std::to_wstring(timestamp) + L".";
    fileName += SavedImageExtension(settings_.staticWallpaper.savedImageFormat);
    const auto path = storageDirectory / fileName;
    if (!SaveStaticImage(path, pixels, width, height, error)) {
        Stop();
        return false;
    }
    renderer_.Shutdown();
    staticPixels_ = std::move(pixels);
    staticWidth_ = width;
    staticHeight_ = height;
    userStatic_ = true;
    staticImageIndex_ = static_cast<int>(settings_.staticWallpaper.imagePaths.size());
    running_ = true;
    paused_ = false;
    pauseReason_.clear();
    lastStaticChange_ = std::chrono::steady_clock::now();
    lastAttachmentCheck_ = lastStaticChange_;
    savedPathUtf8 = WideToUtf8(path.wstring());
    if (savedPathUtf8.empty()) {
        error = "The static render path could not be encoded for settings storage.";
        Stop();
        return false;
    }
    InvalidateRect(window_, nullptr, FALSE);
    LogInfo("Captured the current render as a static wallpaper image.");
    return true;
}

bool WallpaperController::InitialiseRendererWithRetry(std::string& error) {
    if (renderer_.Initialise(window_, error)) {
        lastRendererError_.clear();
        return true;
    }
    LogError("Renderer startup failed: " + error);
    renderer_.Shutdown();
    std::string retryError;
    if (renderer_.Initialise(window_, retryError)) {
        lastRendererError_.clear();
        LogInfo("Renderer startup succeeded on the single safe retry.");
        return true;
    }
    error += " Retry failed: " + retryError;
    lastRendererError_ = error;
    return false;
}

void WallpaperController::Stop() {
    videoReady_ = false;
    if (videoPlayer_) {
        videoPlayer_->Shutdown();
        videoPlayer_->Release();
        videoPlayer_ = nullptr;
    }
    if (videoCallback_) {
        videoCallback_->Release();
        videoCallback_ = nullptr;
    }
    if (!window_) {
        staticPixels_.clear();
        userStatic_ = false;
        userVideo_ = false;
        videoLoopCount_ = 0;
        staticImageIndex_ = 0;
        running_ = false;
        paused_ = false;
        pauseReason_.clear();
        return;
    }
    renderer_.Shutdown();
    desktopHost_.Detach(window_);
    DestroyWindow(window_);
    window_ = nullptr;
    staticPixels_.clear();
    userStatic_ = false;
    userVideo_ = false;
    videoLoopCount_ = 0;
    staticImageIndex_ = 0;
    running_ = false;
    paused_ = false;
    pauseReason_.clear();
    LogInfo("Wallpaper stopped and desktop host detached.");
}

void WallpaperController::Pause(std::string reason) {
    if (!running_) return;
    if (userVideo_ && videoPlayer_ && videoReady_) {
        const HRESULT result = videoPlayer_->Pause();
        if (FAILED(result)) {
            LogError("Video pause request failed: " + reason + ".");
            PostMessageW(window_, kVideoPlaybackFailedMessage, 0,
                         static_cast<LPARAM>(static_cast<LONG_PTR>(result)));
            return;
        }
    }
    paused_ = true;
    pauseReason_ = std::move(reason);
}

bool WallpaperController::PausePresentation(std::string reason, std::string& error) {
    if (!running_) {
        error = "The wallpaper is not running.";
        return false;
    }
    if (paused_) return true;
    // File-backed image and video modes do not retain fractal GPU resources.
    Pause(std::move(reason));
    error.clear();
    return true;
}

std::optional<std::string> WallpaperController::TakeRuntimeError() {
    if (lastRuntimeError_.empty()) return std::nullopt;
    std::string error = std::move(lastRuntimeError_);
    lastRuntimeError_.clear();
    return error;
}

void WallpaperController::HandleVideoPlaybackFailure(HRESULT result) {
    if (!running_ || !userVideo_) return;
    lastRuntimeError_ = "Video playback stopped after Windows Media Foundation reported a failure (HRESULT " +
        std::to_string(static_cast<unsigned long>(result)) + ").";
    LogError(lastRuntimeError_);
    Stop();
}

void WallpaperController::HandleVideoPlaybackLooped() {
    if (!running_ || !userVideo_) return;
    ++videoLoopCount_;
    if (videoLoopCount_ == 1U) {
        LogInfo("Video wallpaper reached end of file and restarted from position zero.");
    }
}

void WallpaperController::HandleVideoPlaybackReady() {
    if (!running_ || !userVideo_) return;
    const bool firstReady = !videoReady_;
    videoReady_ = true;
    if (firstReady) LogInfo("Video playback is ready for presentation.");
    if (paused_) Pause(pauseReason_);
    InvalidateRect(window_, nullptr, FALSE);
}

void WallpaperController::Resume() {
    if (!running_) return;
    paused_ = false;
    pauseReason_.clear();
    if (userVideo_ && videoPlayer_ && videoReady_) {
        const HRESULT result = videoPlayer_->Play();
        if (FAILED(result)) {
            PostMessageW(window_, kVideoPlaybackFailedMessage, 0,
                         static_cast<LPARAM>(static_cast<LONG_PTR>(result)));
        }
    }
}


void WallpaperController::Tick(double deltaSeconds) {
    (void)deltaSeconds;
    if (!running_) return;
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<double>(now - lastAttachmentCheck_).count() >= 2.0) {
        ReattachIfNeeded();
        lastAttachmentCheck_ = now;
    }
    if (paused_) return;
    if (userVideo_) return;
    if (userStatic_) {
        if (settings_.staticWallpaper.cycleEnabled && settings_.staticWallpaper.imagePaths.size() > 1) {
            const double elapsed = std::chrono::duration<double>(now - lastStaticChange_).count();
            if (elapsed >= settings_.staticWallpaper.cycleSeconds) {
                int nextIndex = staticImageIndex_ + 1;
                if (settings_.staticWallpaper.order == StaticSlideshowOrder::Shuffle) {
                    const int count = static_cast<int>(settings_.staticWallpaper.imagePaths.size());
                    std::uniform_int_distribution<int> distribution(0, count - 2);
                    nextIndex = distribution(slideshowRandom_);
                    if (nextIndex >= staticImageIndex_) ++nextIndex;
                }
                std::string cycleError;
                if (LoadStaticImageByIndex(nextIndex, cycleError)) {
                    InvalidateRect(window_, nullptr, FALSE);
                    LogInfo("Slideshow advanced to the next saved image.");
                } else {
                    lastRuntimeError_ = "The slideshow stopped because none of its configured image files could be loaded. " +
                        cycleError;
                    LogError(lastRuntimeError_);
                    Stop();
                    return;
                }
                lastStaticChange_ = now;
            }
        }
        return;
    }

}

void WallpaperController::ReattachIfNeeded() {
    if (!window_ || desktopHost_.IsAttachmentValid(window_)) return;
    std::string error;
    if (desktopHost_.Attach(window_, error)) {
        const RECT hostBounds = desktopHost_.MapDesktopRectToHost(virtualBounds_);
        SetWindowPos(window_, HWND_BOTTOM, hostBounds.left, hostBounds.top,
                     hostBounds.right - hostBounds.left, hostBounds.bottom - hostBounds.top,
                     SWP_NOACTIVATE | SWP_SHOWWINDOW | SWP_FRAMECHANGED);
        LogInfo("Wallpaper reattached after Explorer desktop host change.");
    } else {
        LogWarning("Wallpaper reattachment failed: " + error);
    }
}

void WallpaperController::HandleDisplayChange() {
    if (!running_ || !window_) return;
    displays_ = DisplayManager::Enumerate();
    virtualBounds_ = DisplayManager::VirtualDesktopBounds(displays_);
    const RECT hostBounds = desktopHost_.MapDesktopRectToHost(virtualBounds_);
    SetWindowPos(window_, HWND_BOTTOM, hostBounds.left, hostBounds.top,
                 hostBounds.right - hostBounds.left, hostBounds.bottom - hostBounds.top,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);
    // Repaint through WM_PAINT, after playback readiness and BeginPaint.
    if (userStatic_ || userVideo_) InvalidateRect(window_, nullptr, FALSE);
    LogInfo("Display configuration changed; wallpaper layout rebuilt.");
}


void WallpaperController::PaintStaticImage(HDC dc) {
    if (staticPixels_.empty()) return;
    RECT client{};
    GetClientRect(window_, &client);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = staticWidth_;
    info.bmiHeader.biHeight = staticHeight_;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    const int previousMode = SetStretchBltMode(dc, HALFTONE);
    SetBrushOrgEx(dc, 0, 0, nullptr);
    StretchDIBits(dc, 0, 0, client.right, client.bottom, 0, 0, staticWidth_, staticHeight_,
                  staticPixels_.data(), &info, DIB_RGB_COLORS, SRCCOPY);
    SetStretchBltMode(dc, previousMode);
}

LRESULT CALLBACK WallpaperController::WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* self = reinterpret_cast<WallpaperController*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        self = static_cast<WallpaperController*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    if (self) {
        if (message == kVideoPlaybackFailedMessage) {
            self->HandleVideoPlaybackFailure(static_cast<HRESULT>(static_cast<LONG_PTR>(lParam)));
            return 0;
        }
        if (message == kVideoPlaybackLoopedMessage) {
            self->HandleVideoPlaybackLooped();
            return 0;
        }
        if (message == kVideoPlaybackReadyMessage) {
            self->HandleVideoPlaybackReady();
            return 0;
        }
        if (message == WM_PAINT && (self->userStatic_ || self->userVideo_)) {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            if (self->userVideo_ && self->videoPlayer_ && self->videoReady_) {
                const HRESULT result = self->videoPlayer_->UpdateVideo();
                if (FAILED(result)) {
                    LogError("Video repaint failed after playback readiness.");
                    PostMessageW(window, kVideoPlaybackFailedMessage, 0,
                                 static_cast<LPARAM>(static_cast<LONG_PTR>(result)));
                }
            } else if (self->userVideo_) {
                if (!self->videoPaintDeferred_) {
                    LogInfo("Video repaint deferred until playback is ready.");
                    self->videoPaintDeferred_ = true;
                }
                FillRect(dc, &paint.rcPaint, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            } else {
                self->PaintStaticImage(dc);
            }
            EndPaint(window, &paint);
            return 0;
        }
        if (message == WM_ERASEBKGND) return 1;
        if (message == WM_DISPLAYCHANGE || message == WM_DPICHANGED) {
            self->HandleDisplayChange();
            return 0;
        }
        if (message == WM_CLOSE) return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
#endif

} // namespace mw
