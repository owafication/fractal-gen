#include "WindowsIntegration/WallpaperController.h"

#include "Core/MandelbrotMath.h"
#include "Infrastructure/Logger.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>

namespace mw {

WallpaperController::~WallpaperController() {
#ifdef _WIN32
    Stop();
#endif
}

void WallpaperController::UpdateConfiguration(const AppSettings& settings, const std::vector<Preset>& presets) {
    const bool preserveRuntimeColourState = running_ && !userStatic_;
    const bool runtimeColourCyclingEnabled = settings_.general.colourCyclingEnabled;
    settings_ = settings;
    if (preserveRuntimeColourState) {
        settings_.general.colourCyclingEnabled = runtimeColourCyclingEnabled;
    }
    presets_ = presets;
#ifdef _WIN32
    if (userStatic_ && !settings_.staticWallpaper.imagePaths.empty()) {
        staticImageIndex_ = std::clamp(settings_.staticWallpaper.currentIndex, 0,
            static_cast<int>(settings_.staticWallpaper.imagePaths.size()) - 1);
    }
    RebuildAnimations();
    visibleChangeDetector_.Reset();
    forceNextRender_ = true;
#endif
}

void WallpaperController::SelectPreset(const std::string& presetId) {
    settings_.selectedPresetId = presetId;
#ifdef _WIN32
    RebuildAnimations();
    visibleChangeDetector_.Reset();
    forceNextRender_ = true;
#endif
}

void WallpaperController::SetColourCyclingEnabled(bool enabled) {
    settings_.general.colourCyclingEnabled = enabled;
    sharedAnimation_.SetColourCyclingEnabled(enabled);
    for (auto& [monitor, animation] : monitorAnimations_) {
        (void)monitor;
        animation.SetColourCyclingEnabled(enabled);
    }
#ifdef _WIN32
    visibleChangeDetector_.Reset();
    forceNextRender_ = true;
#endif
}


void WallpaperController::SetMotionEnabled(bool enabled) {
    motionEnabled_ = enabled;
    sharedAnimation_.SetMotionEnabled(enabled);
    for (auto& [monitor, animation] : monitorAnimations_) {
        (void)monitor;
        animation.SetMotionEnabled(enabled);
    }
#ifdef _WIN32
    visibleChangeDetector_.Reset();
    forceNextRender_ = true;
#endif
}

#ifdef _WIN32
namespace {
constexpr wchar_t kWallpaperClass[] = L"MandelbrotLiveWallpaperHost";

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
        error = "The wallpaper rendering window could not be created.";
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

bool WallpaperController::Start(HINSTANCE instance, const AppSettings& settings, const std::vector<Preset>& presets, std::string& error) {
    Stop();
    instance_ = instance;
    settings_ = settings;
    presets_ = presets;
    userStatic_ = false;
    pausedSnapshot_ = false;
    staticFallback_ = false;
    if (!RegisterWindowClass(instance_, error) || !CreateWallpaperWindow(instance_, error)) return false;
    RebuildAnimations();
    rendererRestartAttempted_ = false;
    lastRendererError_.clear();
    visibleChangeDetector_.Reset();
    forceNextRender_ = true;
    if (!InitialiseRendererWithRetry(error)) {
        lastRendererError_ = error;
        BuildStaticFallback();
        staticFallback_ = true;
        InvalidateRect(window_, nullptr, FALSE);
        LogWarning("GPU renderer unavailable; static Mandelbrot fallback enabled.");
    }
    running_ = true;
    paused_ = false;
    pauseReason_.clear();
    lastAttachmentCheck_ = std::chrono::steady_clock::now();
    LogInfo("Wallpaper started across " + std::to_string(displays_.size()) + " display(s) in " + ToString(settings_.monitorMode) + " mode.");
    return true;
}

bool WallpaperController::StartStaticGallery(HINSTANCE instance, const AppSettings& settings,
                                             const std::vector<Preset>& presets, std::string& error) {
    Stop();
    instance_ = instance;
    settings_ = settings;
    presets_ = presets;
    if (settings_.staticWallpaper.imagePaths.empty()) {
        error = "No saved static renders are available.";
        return false;
    }
    if (!RegisterWindowClass(instance_, error) || !CreateWallpaperWindow(instance_, error)) return false;
    staticFallback_ = false;
    pausedSnapshot_ = false;
    userStatic_ = true;
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

bool WallpaperController::RenderStaticSnapshot(const Preset& snapshot, std::string& error) {
    const int virtualWidth = std::max(1L, virtualBounds_.right - virtualBounds_.left);
    const int virtualHeight = std::max(1L, virtualBounds_.bottom - virtualBounds_.top);
    std::vector<RenderRegion> regions;
    if (settings_.monitorMode == MonitorMode::Span) {
        RenderRegion region;
        region.pixels = {0, 0, virtualWidth, virtualHeight};
        region.camera = snapshot.camera;
        region.palette = snapshot.palette;
        region.customPaletteColours = snapshot.customPaletteColours;
        region.equation = snapshot.equation;
        region.maximumIterations = snapshot.maximumIterations;
        region.colourOffset = snapshot.colourOffset;
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
            region.palette = snapshot.palette;
            region.customPaletteColours = snapshot.customPaletteColours;
            region.equation = snapshot.equation;
            region.maximumIterations = snapshot.maximumIterations;
            region.colourOffset = snapshot.colourOffset;
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
    if (width <= 0 || height <= 0 ||
        pixels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        error = "The captured static image dimensions are invalid.";
        return false;
    }
    std::error_code directoryError;
    std::filesystem::create_directories(path.parent_path(), directoryError);
    if (directoryError) {
        error = "The static render directory could not be created.";
        return false;
    }
    BITMAPFILEHEADER fileHeader{};
    BITMAPINFOHEADER infoHeader{};
    infoHeader.biSize = sizeof(infoHeader);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 32;
    infoHeader.biCompression = BI_RGB;
    const std::uint64_t imageBytes = static_cast<std::uint64_t>(pixels.size()) * sizeof(std::uint32_t);
    if (imageBytes > std::numeric_limits<DWORD>::max()) {
        error = "The captured static image is too large for BMP storage.";
        return false;
    }
    infoHeader.biSizeImage = static_cast<DWORD>(imageBytes);
    fileHeader.bfType = 0x4D42;
    fileHeader.bfOffBits = static_cast<DWORD>(sizeof(fileHeader) + sizeof(infoHeader));
    fileHeader.bfSize = fileHeader.bfOffBits + infoHeader.biSizeImage;

    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) {
        error = "The static render image could not be created.";
        return false;
    }
    stream.write(reinterpret_cast<const char*>(&fileHeader), static_cast<std::streamsize>(sizeof(fileHeader)));
    stream.write(reinterpret_cast<const char*>(&infoHeader), static_cast<std::streamsize>(sizeof(infoHeader)));
    stream.write(reinterpret_cast<const char*>(pixels.data()), static_cast<std::streamsize>(imageBytes));
    if (!stream) {
        error = "The complete static render image could not be written.";
        return false;
    }
    return true;
}

bool WallpaperController::LoadStaticImage(const std::string& pathUtf8, std::string& error) {
    const std::wstring widePath = Utf8ToWide(pathUtf8);
    if (widePath.empty()) {
        error = "A saved static render path is not valid UTF-8.";
        return false;
    }
    std::ifstream stream(std::filesystem::path(widePath), std::ios::binary);
    if (!stream) {
        error = "A saved static render image could not be opened.";
        return false;
    }
    BITMAPFILEHEADER fileHeader{};
    BITMAPINFOHEADER infoHeader{};
    stream.read(reinterpret_cast<char*>(&fileHeader), static_cast<std::streamsize>(sizeof(fileHeader)));
    stream.read(reinterpret_cast<char*>(&infoHeader), static_cast<std::streamsize>(sizeof(infoHeader)));
    if (!stream || fileHeader.bfType != 0x4D42 || infoHeader.biSize < sizeof(BITMAPINFOHEADER) ||
        infoHeader.biPlanes != 1 || infoHeader.biBitCount != 32 || infoHeader.biCompression != BI_RGB ||
        infoHeader.biWidth <= 0 || infoHeader.biHeight <= 0) {
        error = "A saved static render is not a supported 32-bit BMP image.";
        return false;
    }
    const std::uint64_t pixelCount = static_cast<std::uint64_t>(infoHeader.biWidth) *
                                     static_cast<std::uint64_t>(infoHeader.biHeight);
    if (pixelCount == 0 || pixelCount > 100000000ULL ||
        pixelCount > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t))) {
        error = "A saved static render has unsafe dimensions.";
        return false;
    }
    stream.seekg(static_cast<std::streamoff>(fileHeader.bfOffBits), std::ios::beg);
    std::vector<std::uint32_t> pixels(static_cast<std::size_t>(pixelCount));
    stream.read(reinterpret_cast<char*>(pixels.data()),
                static_cast<std::streamsize>(pixels.size() * sizeof(std::uint32_t)));
    if (!stream) {
        error = "A saved static render image is incomplete.";
        return false;
    }
    fallbackPixels_ = std::move(pixels);
    fallbackWidth_ = infoHeader.biWidth;
    fallbackHeight_ = infoHeader.biHeight;
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
    instance_ = instance;
    settings_ = settings;
    presets_ = presets;
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
    const auto path = storageDirectory / (L"static-render-" + std::to_wstring(timestamp) + L".bmp");
    if (!SaveStaticImage(path, pixels, width, height, error)) {
        Stop();
        return false;
    }
    renderer_.Shutdown();
    fallbackPixels_ = std::move(pixels);
    fallbackWidth_ = width;
    fallbackHeight_ = height;
    staticFallback_ = false;
    pausedSnapshot_ = false;
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
    if (!window_) {
        fallbackPixels_.clear();
        staticFallback_ = false;
        userStatic_ = false;
        pausedSnapshot_ = false;
        staticImageIndex_ = 0;
        running_ = false;
        paused_ = false;
        pauseReason_.clear();
        visibleChangeDetector_.Reset();
        forceNextRender_ = true;
        return;
    }
    renderer_.Shutdown();
    desktopHost_.Detach(window_);
    DestroyWindow(window_);
    window_ = nullptr;
    fallbackPixels_.clear();
    staticFallback_ = false;
    userStatic_ = false;
    pausedSnapshot_ = false;
    staticImageIndex_ = 0;
    running_ = false;
    paused_ = false;
    pauseReason_.clear();
    visibleChangeDetector_.Reset();
    forceNextRender_ = true;
    LogInfo("Wallpaper stopped and desktop host detached.");
}

void WallpaperController::Pause(std::string reason) {
    if (!running_) return;
    paused_ = true;
    pauseReason_ = std::move(reason);
}

bool WallpaperController::PauseAndReleaseGpu(std::string reason, std::string& error) {
    if (!running_) {
        error = "The wallpaper is not running.";
        return false;
    }
    if (paused_) return true;
    if (userStatic_ || staticFallback_ || !renderer_.IsReady()) {
        Pause(std::move(reason));
        return true;
    }

    std::vector<std::uint32_t> pixels;
    int width = 0;
    int height = 0;
    if (!renderer_.CapturePixels(pixels, width, height, error)) {
        // Still honour the pause request even if the zero-GPU snapshot could not be captured.
        Pause(std::move(reason));
        return false;
    }
    renderer_.Shutdown();
    fallbackPixels_ = std::move(pixels);
    fallbackWidth_ = width;
    fallbackHeight_ = height;
    pausedSnapshot_ = true;
    paused_ = true;
    pauseReason_ = std::move(reason);
    InvalidateRect(window_, nullptr, FALSE);
    LogInfo("Wallpaper paused on a captured frame and GPU resources were released.");
    return true;
}

void WallpaperController::Resume() {
    if (!running_) return;
    if (pausedSnapshot_) {
        std::string error;
        if (!InitialiseRendererWithRetry(error)) {
            lastRendererError_ = error;
            pauseReason_ = "Resume failed; captured paused frame retained";
            paused_ = true;
            LogError("Wallpaper resume after paused snapshot failed: " + error);
            return;
        }
        pausedSnapshot_ = false;
        rendererRestartAttempted_ = false;
        fallbackPixels_.clear();
        fallbackWidth_ = 0;
        fallbackHeight_ = 0;
    }
    paused_ = false;
    pauseReason_.clear();
    forceNextRender_ = true;
}

const Preset* WallpaperController::FindPreset(const std::string& id) const {
    const auto found = std::find_if(presets_.begin(), presets_.end(), [&](const Preset& preset) { return preset.id == id; });
    return found == presets_.end() ? nullptr : &*found;
}

void WallpaperController::RebuildAnimations() {
    const Preset* selected = FindPreset(settings_.selectedPresetId);
    if (!selected && !presets_.empty()) selected = &presets_.front();
    if (selected) sharedAnimation_.SetPreset(*selected, settings_.general.reducedMotion);
    sharedAnimation_.SetColourCyclingEnabled(settings_.general.colourCyclingEnabled);
    sharedAnimation_.SetMotionEnabled(motionEnabled_);

    monitorAnimations_.clear();
    for (const auto& display : displays_) {
        const std::string monitorKey = WideToUtf8(display.deviceName);
        const auto assignment = settings_.monitorPresetAssignments.find(monitorKey);
        const Preset* preset = assignment == settings_.monitorPresetAssignments.end() ? selected : FindPreset(assignment->second);
        if (!preset) preset = selected;
        AnimationController controller;
        if (preset) controller.SetPreset(*preset, settings_.general.reducedMotion);
        controller.SetColourCyclingEnabled(settings_.general.colourCyclingEnabled);
        controller.SetMotionEnabled(motionEnabled_);
        monitorAnimations_.emplace(display.deviceName, std::move(controller));
    }
}

bool WallpaperController::RenderFrame(double deltaSeconds, std::string& error) {
    if (staticFallback_) {
        InvalidateRect(window_, nullptr, FALSE);
        return true;
    }
    std::vector<RenderRegion> regions;
    std::vector<const Preset*> regionPresets;
    const Preset* selected = FindPreset(settings_.selectedPresetId);
    if (!selected && !presets_.empty()) selected = &presets_.front();
    if (!selected) {
        error = "No valid preset is available.";
        return false;
    }

    const int virtualWidth = std::max(1L, virtualBounds_.right - virtualBounds_.left);
    const int virtualHeight = std::max(1L, virtualBounds_.bottom - virtualBounds_.top);
    const int effectiveIterations = batteryQualityReduction_
        ? std::max(64, static_cast<int>(std::lround(settings_.performance.maximumIterations * 0.6)))
        : settings_.performance.maximumIterations;
    if (settings_.monitorMode == MonitorMode::Span) {
        const auto frame = sharedAnimation_.Update(deltaSeconds);
        RenderRegion region;
        region.pixels = {0, 0, virtualWidth, virtualHeight};
        region.camera = frame.camera;
        region.palette = selected->palette;
        region.customPaletteColours = selected->customPaletteColours;
        region.equation = selected->equation;
        region.maximumIterations = effectiveIterations;
        region.colourOffset = frame.colourOffset;
        region.brightness = selected->brightness;
        region.contrast = selected->contrast;
        region.saturation = selected->saturation;
        region.interiorColour = selected->interiorColour;
        region.backgroundColour = selected->backgroundColour;
        region.smoothColouring = selected->smoothColouring;
        regions.push_back(region);
        regionPresets.push_back(selected);
    } else {
        const auto sharedFrame = sharedAnimation_.Update(deltaSeconds);
        for (const auto& display : displays_) {
            const std::string monitorKey = WideToUtf8(display.deviceName);
            const auto assignment = settings_.monitorPresetAssignments.find(monitorKey);
            const Preset* preset = selected;
            AnimationFrame frame = sharedFrame;
            if (settings_.monitorMode == MonitorMode::Independent) {
                if (assignment != settings_.monitorPresetAssignments.end()) {
                    if (const Preset* assigned = FindPreset(assignment->second)) preset = assigned;
                }
                auto animation = monitorAnimations_.find(display.deviceName);
                if (animation != monitorAnimations_.end()) frame = animation->second.Update(deltaSeconds);
            }
            RenderRegion region;
            region.pixels = {
                display.bounds.left - virtualBounds_.left,
                display.bounds.top - virtualBounds_.top,
                display.bounds.right - virtualBounds_.left,
                display.bounds.bottom - virtualBounds_.top,
            };
            region.camera = frame.camera;
            region.palette = preset->palette;
            region.customPaletteColours = preset->customPaletteColours;
            region.equation = preset->equation;
            region.maximumIterations = effectiveIterations;
            region.colourOffset = frame.colourOffset;
            region.brightness = preset->brightness;
            region.contrast = preset->contrast;
            region.saturation = preset->saturation;
            region.interiorColour = preset->interiorColour;
            region.backgroundColour = preset->backgroundColour;
            region.smoothColouring = preset->smoothColouring;
            regions.push_back(region);
            regionPresets.push_back(preset);
        }
    }

    RenderOptions options;
    options.renderScale = batteryQualityReduction_ ? settings_.performance.renderScale * 0.66 : settings_.performance.renderScale;
    options.antiAliasingLevel = batteryQualityReduction_ ? 1 : settings_.performance.antiAliasingLevel;
    options.precision = settings_.performance.precision;

    std::vector<VisualFrameDescriptor> descriptors;
    descriptors.reserve(regions.size());
    for (std::size_t index = 0; index < regions.size(); ++index) {
        const auto& region = regions[index];
        const Preset* preset = index < regionPresets.size() ? regionPresets[index] : selected;
        VisualFrameDescriptor descriptor;
        descriptor.camera = region.camera;
        descriptor.colourOffset = region.colourOffset;
        descriptor.pixelWidth = static_cast<int>(std::max<LONG>(1L, region.pixels.right - region.pixels.left));
        descriptor.pixelHeight = static_cast<int>(std::max<LONG>(1L, region.pixels.bottom - region.pixels.top));
        descriptor.contentRevision = ComputeVisualRevision(*preset, region.maximumIterations,
                                                           options.renderScale, options.antiAliasingLevel,
                                                           options.precision);
        descriptors.push_back(descriptor);
    }
    const bool animatedEquation = std::any_of(regionPresets.begin(), regionPresets.end(),
        [](const Preset* preset) { return preset && preset->equation.animateCoefficients; });
    if (!visibleChangeDetector_.ShouldRender(descriptors, settings_.performance.adaptive,
                                             forceNextRender_ || animatedEquation)) {
        forceNextRender_ = false;
        return true;
    }
    forceNextRender_ = false;
    return renderer_.Render(regions, options, error);
}

void WallpaperController::Tick(double deltaSeconds) {
    if (!running_) return;
    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<double>(now - lastAttachmentCheck_).count() >= 2.0) {
        ReattachIfNeeded();
        lastAttachmentCheck_ = now;
    }
    if (paused_) return;
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
                } else {
                    LogWarning("Static render cycle failed: " + cycleError);
                }
                lastStaticChange_ = now;
            }
        }
        return;
    }

    std::string error;
    if (RenderFrame(deltaSeconds, error)) return;
    LogError("Wallpaper render failed: " + error);
    if (!rendererRestartAttempted_) {
        rendererRestartAttempted_ = true;
        renderer_.Shutdown();
        std::string retryError;
        forceNextRender_ = true;
        if (renderer_.Initialise(window_, retryError) && RenderFrame(0.0, retryError)) {
            lastRendererError_.clear();
            LogInfo("Renderer recovered after device/context loss.");
            return;
        }
        lastRendererError_ = retryError;
        LogError("Renderer recovery failed: " + retryError);
    }
    renderer_.Shutdown();
    BuildStaticFallback();
    staticFallback_ = true;
    pauseReason_ = "GPU renderer stopped; static fallback active";
    InvalidateRect(window_, nullptr, FALSE);
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
    if (!pausedSnapshot_) {
        renderer_.Resize(virtualBounds_.right - virtualBounds_.left, virtualBounds_.bottom - virtualBounds_.top);
    }
    RebuildAnimations();
    visibleChangeDetector_.Reset();
    forceNextRender_ = true;
    if (staticFallback_) BuildStaticFallback();
    if (userStatic_ || pausedSnapshot_) InvalidateRect(window_, nullptr, FALSE);
    LogInfo("Display configuration changed; wallpaper layout rebuilt.");
}

void WallpaperController::BuildStaticFallback() {
    const int desktopWidth = std::max(1L, virtualBounds_.right - virtualBounds_.left);
    const int desktopHeight = std::max(1L, virtualBounds_.bottom - virtualBounds_.top);
    const double desktopAspect = static_cast<double>(desktopWidth) / desktopHeight;

    // Keep the CPU fallback bounded while preserving the virtual desktop aspect ratio.
    // This is substantially sharper than the old fixed 480x270 image, especially on multi-monitor desktops.
    if (desktopAspect >= 1.0) {
        fallbackWidth_ = std::min(desktopWidth, 1280);
        fallbackHeight_ = std::max(1, static_cast<int>(std::lround(fallbackWidth_ / desktopAspect)));
    } else {
        fallbackHeight_ = std::min(desktopHeight, 720);
        fallbackWidth_ = std::max(1, static_cast<int>(std::lround(fallbackHeight_ * desktopAspect)));
    }
    fallbackPixels_.assign(static_cast<std::size_t>(fallbackWidth_ * fallbackHeight_), 0xFF000000U);
    const Preset* preset = FindPreset(settings_.selectedPresetId);
    if (!preset && !presets_.empty()) preset = &presets_.front();
    const CameraState camera = preset ? preset->camera : CameraState{};
    const int iterations = std::clamp(settings_.performance.maximumIterations, 64, 400);
    const double aspect = static_cast<double>(fallbackWidth_) / fallbackHeight_;
    for (int y = 0; y < fallbackHeight_; ++y) {
        for (int x = 0; x < fallbackWidth_; ++x) {
            const double real = camera.centreX + ((static_cast<double>(x) / (fallbackWidth_ - 1)) * 2.0 - 1.0) * camera.scale * aspect;
            const double imaginary = camera.centreY + ((static_cast<double>(y) / (fallbackHeight_ - 1)) * 2.0 - 1.0) * camera.scale;
            const EquationSettings equation = preset ? preset->equation : EquationSettings{};
            const auto escape = CalculateEscape(real, imaginary, iterations, equation);
            std::uint8_t red = 0;
            std::uint8_t green = 0;
            std::uint8_t blue = 0;
            if (escape.escaped || escape.converged) {
                double t = escape.smoothValue / std::max(1, iterations) * 8.0;
                if (equation.newtonMode || equation.renderMode == FractalRenderMode::Newton) {
                    t = escape.rootIndex >= 0
                        ? static_cast<double>(escape.rootIndex) / std::max(2, equation.newtonDegree)
                        : 0.0;
                    t += 0.08 * (1.0 - static_cast<double>(escape.iterations) / std::max(1, iterations));
                } else if (equation.colouringMethod == ColouringMethod::OrbitTrap) {
                    t = -std::log(std::max(escape.orbitTrapDistance, 1.0e-8)) * 0.32;
                } else if (equation.colouringMethod == ColouringMethod::DistanceEstimation &&
                           escape.distanceEstimate > 0.0) {
                    t = -std::log(std::max(escape.distanceEstimate, 1.0e-10)) * 0.22;
                }
                t = std::fmod(t + (preset ? preset->colourOffset : 0.0), 1.0);
                if (t < 0.0) t += 1.0;
                const double hue = t * 6.0;
                const int sector = static_cast<int>(std::floor(hue)) % 6;
                const double fraction = hue - std::floor(hue);
                const double q = 1.0 - fraction;
                const double values[6][3] = {
                    {1.0, fraction, 0.0}, {q, 1.0, 0.0}, {0.0, 1.0, fraction},
                    {0.0, q, 1.0}, {fraction, 0.0, 1.0}, {1.0, 0.0, q},
                };
                double glow = 0.0;
                if (equation.glowStrength > 0.0) {
                    glow = std::exp(-std::min(escape.orbitTrapDistance, 10.0) * 12.0) *
                           equation.glowStrength * 0.2;
                }
                red = static_cast<std::uint8_t>(std::clamp((values[sector][0] + glow) * 255.0, 0.0, 255.0));
                green = static_cast<std::uint8_t>(std::clamp((values[sector][1] + glow) * 255.0, 0.0, 255.0));
                blue = static_cast<std::uint8_t>(std::clamp((values[sector][2] + glow) * 255.0, 0.0, 255.0));
            }
            fallbackPixels_[static_cast<std::size_t>((fallbackHeight_ - 1 - y) * fallbackWidth_ + x)] =
                0xFF000000U | (static_cast<std::uint32_t>(red) << 16U) | (static_cast<std::uint32_t>(green) << 8U) | blue;
        }
    }
}

void WallpaperController::PaintStaticFallback(HDC dc) {
    if (fallbackPixels_.empty()) return;
    RECT client{};
    GetClientRect(window_, &client);
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = fallbackWidth_;
    info.bmiHeader.biHeight = fallbackHeight_;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    const int previousMode = SetStretchBltMode(dc, HALFTONE);
    SetBrushOrgEx(dc, 0, 0, nullptr);
    StretchDIBits(dc, 0, 0, client.right, client.bottom, 0, 0, fallbackWidth_, fallbackHeight_,
                  fallbackPixels_.data(), &info, DIB_RGB_COLORS, SRCCOPY);
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
        if (message == WM_PAINT && (self->staticFallback_ || self->userStatic_ || self->pausedSnapshot_)) {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(window, &paint);
            self->PaintStaticFallback(dc);
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
