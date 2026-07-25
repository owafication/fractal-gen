#include "App/HighResRenderDialog.h"

#include "App/DialogSupport.h"
#include "Core/StillImageRenderer.h"
#include "Core/DeepZoom.h"
#include "Rendering/GpuRenderer.h"

#ifdef _WIN32
#include <commctrl.h>
#include <commdlg.h>
#include <wincodec.h>
#include <wrl/client.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cwchar>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <new>
#include <sstream>
#include <string>
#include <thread>
#include <cwctype>
#include <vector>

namespace mw {
#ifdef _WIN32
namespace {

using Microsoft::WRL::ComPtr;

constexpr wchar_t kClassName[] = L"MandelbrotHighResRenderDialog";
constexpr wchar_t kPreviewClassName[] = L"MandelbrotHighResPreviewWindow";
constexpr UINT kRenderProgressMessage = WM_APP + 201U;
constexpr UINT kRenderCompleteMessage = WM_APP + 202U;

enum Id : int {
    CoordinatesEdit = 9101,
    WidthEdit,
    HeightEdit,
    DpiEdit,
    FormatCombo,
    BackendCombo,
    ProgressBar,
    StatusLabel,
    RenderButton,
    CancelRenderButton,
    PreviewButton,
    SaveAsButton,
    CloseButton,
};

enum class ImageFormat { Png, Tiff, Bmp };
enum class RenderBackend { GpuDirect3D11, GpuOpenGl, CpuTiled };

struct PreviewWindowState {
    const StillRenderPreview* preview{};
};

struct State {
    HWND owner{};
    HWND window{};
    HINSTANCE instance{};
    Preset snapshot;
    PerformanceSettings performance;
    HFONT font{};
    ResponsiveDialogLayout layout;
    UINT dpi{96};

    std::thread worker;
    std::atomic_bool cancelRequested{false};
    bool rendering{false};
    bool rendered{false};
    bool done{false};
    bool workerSucceeded{false};
    bool workerCancelled{false};
    std::string workerError;
    StillRenderResult completedResult;
    std::filesystem::path completedPath;
    ImageFormat completedFormat{ImageFormat::Png};
    std::uint32_t completedWidth{0};
    std::uint32_t completedHeight{0};
    std::uint32_t completedDpi{0};
};

HWND Add(State& state, const wchar_t* className, const wchar_t* text, DWORD style, int id,
         int x, int y, int width, int height) {
    const bool edit = className && std::wcscmp(className, WC_EDITW) == 0;
    HWND control = CreateWindowExW(
        edit ? WS_EX_CLIENTEDGE : 0,
        className,
        text,
        WS_CHILD | WS_VISIBLE | AccessibleControlStyle(className, style),
        x, y, width, height,
        state.window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
        state.instance,
        nullptr);
    if (control) SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(state.font), TRUE);
    return control;
}

std::wstring ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (size <= 1) return {};
    std::wstring wide(static_cast<std::size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wide.data(), size);
    wide.resize(static_cast<std::size_t>(size - 1));
    return wide;
}

std::wstring GetText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    std::wstring text(static_cast<std::size_t>(std::max(length, 0)) + 1U, L'\0');
    if (length > 0) GetWindowTextW(control, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(std::max(length, 0)));
    return text;
}

std::wstring MakeCoordinateString(const CameraState& camera) {
    std::wostringstream stream;
    stream << std::setprecision(17)
           << CameraCentreX(camera) << L"," << CameraCentreY(camera) << L"," << camera.scale;
    return stream.str();
}

bool ParseCoordinateString(const std::wstring& text, CameraState& camera) {
    std::wstring copy = text;
    for (wchar_t& character : copy) {
        if (character == L';' || character == L'|') character = L',';
    }
    std::wstringstream stream(copy);
    std::wstring part;
    std::array<double, 3> values{};
    std::size_t count = 0U;
    while (std::getline(stream, part, L',')) {
        if (part.find_first_not_of(L" \t\r\n") == std::wstring::npos) continue;
        if (count >= values.size()) return false;
        try {
            std::size_t consumed = 0U;
            values[count] = std::stod(part, &consumed);
            if (part.find_first_not_of(L" \t\r\n", consumed) != std::wstring::npos) return false;
        } catch (...) {
            return false;
        }
        ++count;
    }
    if (count != values.size()) return false;
    camera.centreX = values[0];
    camera.centreY = values[1];
    camera.centreXLow = 0.0;
    camera.centreYLow = 0.0;
    camera.scale = values[2];
    return std::isfinite(camera.centreX) && std::isfinite(camera.centreY) &&
           std::isfinite(camera.scale) && camera.scale > 0.0;
}

bool ParsePositiveUInt32(HWND control, std::uint32_t& value) {
    try {
        std::size_t consumed = 0U;
        const std::wstring text = GetText(control);
        const unsigned long long parsed = std::stoull(text, &consumed);
        if (text.find_first_not_of(L" \t\r\n", consumed) != std::wstring::npos ||
            parsed == 0ULL || parsed > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
        value = static_cast<std::uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

const wchar_t* FormatName(ImageFormat format) {
    switch (format) {
    case ImageFormat::Png: return L"PNG";
    case ImageFormat::Tiff: return L"TIFF";
    case ImageFormat::Bmp: return L"BMP";
    }
    return L"PNG";
}

const wchar_t* FormatExtension(ImageFormat format) {
    switch (format) {
    case ImageFormat::Png: return L"png";
    case ImageFormat::Tiff: return L"tiff";
    case ImageFormat::Bmp: return L"bmp";
    }
    return L"png";
}

const GUID& ContainerGuid(ImageFormat format) {
    switch (format) {
    case ImageFormat::Png: return GUID_ContainerFormatPng;
    case ImageFormat::Tiff: return GUID_ContainerFormatTiff;
    case ImageFormat::Bmp: return GUID_ContainerFormatBmp;
    }
    return GUID_ContainerFormatPng;
}

ImageFormat SelectedFormat(HWND window) {
    const int selected = static_cast<int>(SendMessageW(
        GetDlgItem(window, FormatCombo), CB_GETCURSEL, 0, 0));
    if (selected == 1) return ImageFormat::Tiff;
    if (selected == 2) return ImageFormat::Bmp;
    return ImageFormat::Png;
}

RenderBackend SelectedBackend(HWND window) {
    const int selected = static_cast<int>(SendMessageW(
        GetDlgItem(window, BackendCombo), CB_GETCURSEL, 0, 0));
    if (selected == 1) return RenderBackend::GpuOpenGl;
    if (selected == 2) return RenderBackend::CpuTiled;
    return RenderBackend::GpuDirect3D11;
}

std::wstring BackendStatus(RenderBackend backend) {
    if (backend == RenderBackend::GpuDirect3D11)
        return L"Rendering in the background using GPU Direct3D 11 tiles...";
    if (backend == RenderBackend::GpuOpenGl)
        return L"Rendering in the background using GPU OpenGL tiles...";
    return L"Rendering in the background using the CPU tiled exporter...";
}

std::uint32_t PreviewDimension(std::uint32_t sourceWidth, std::uint32_t sourceHeight,
                               std::uint32_t maximumWidth, std::uint32_t maximumHeight,
                               bool widthDimension) {
    if (sourceWidth == 0U || sourceHeight == 0U || maximumWidth == 0U || maximumHeight == 0U) return 0U;
    const double scale = std::min({1.0,
        static_cast<double>(maximumWidth) / static_cast<double>(sourceWidth),
        static_cast<double>(maximumHeight) / static_cast<double>(sourceHeight)});
    const double source = widthDimension ? static_cast<double>(sourceWidth)
                                         : static_cast<double>(sourceHeight);
    return std::max(1U, static_cast<std::uint32_t>(std::lround(source * scale)));
}

std::uint32_t MapPreviewCoordinate(std::uint32_t previewCoordinate,
                                   std::uint32_t previewSize,
                                   std::uint32_t sourceSize) {
    if (previewSize <= 1U || sourceSize <= 1U) return 0U;
    const double normalised = static_cast<double>(previewCoordinate) /
                              static_cast<double>(previewSize - 1U);
    return std::min(sourceSize - 1U,
                    static_cast<std::uint32_t>(std::lround(
                        normalised * static_cast<double>(sourceSize - 1U))));
}

RenderRegion BuildRenderRegion(const Preset& preset, std::uint32_t width, std::uint32_t height) {
    RenderRegion region;
    region.pixels = RECT{0, 0, static_cast<LONG>(std::min<std::uint32_t>(width, static_cast<std::uint32_t>(std::numeric_limits<LONG>::max()))), static_cast<LONG>(std::min<std::uint32_t>(height, static_cast<std::uint32_t>(std::numeric_limits<LONG>::max())))};
    region.camera = preset.camera;
    region.palette = preset.palette;
    region.customPaletteColours = preset.customPaletteColours;
    region.maximumIterations = preset.maximumIterations;
    region.equation = preset.equation;
    region.colourOffset = preset.colourOffset;
    region.brightness = preset.brightness;
    region.contrast = preset.contrast;
    region.saturation = preset.saturation;
    region.interiorColour = preset.interiorColour;
    region.backgroundColour = preset.backgroundColour;
    region.smoothColouring = preset.smoothColouring;
    return region;
}

LRESULT CALLBACK HiddenGpuHostProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    return DefWindowProcW(window, message, wParam, lParam);
}

bool EnsureHiddenGpuHostClass(HINSTANCE instance) {
    static bool registered = false;
    if (registered) return true;
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = HiddenGpuHostProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.style = CS_OWNDC;
    windowClass.lpszClassName = L"MandelbrotHighResGpuHost";
    if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    registered = true;
    return true;
}

class WicRowEncoder {
public:
    bool Initialise(const std::filesystem::path& path, ImageFormat format,
                    std::uint32_t width, std::uint32_t height, std::uint32_t dpi,
                    std::string& error) {
        if (width > std::numeric_limits<UINT>::max() / 3U) {
            error = "The requested width exceeds the encoder scanline limit.";
            return false;
        }
        const HRESULT factoryResult = CoCreateInstance(
            CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
            IID_PPV_ARGS(factory_.ReleaseAndGetAddressOf()));
        if (FAILED(factoryResult)) {
            error = "Windows Imaging Component could not start.";
            return false;
        }
        HRESULT result = factory_->CreateStream(stream_.ReleaseAndGetAddressOf());
        if (FAILED(result)) {
            error = "The image output stream could not be created.";
            return false;
        }
        result = stream_->InitializeFromFilename(path.c_str(), GENERIC_WRITE);
        if (FAILED(result)) {
            error = "The temporary image file could not be opened for writing.";
            return false;
        }
        result = factory_->CreateEncoder(ContainerGuid(format), nullptr,
                                         encoder_.ReleaseAndGetAddressOf());
        if (FAILED(result)) {
            error = "The selected Windows image encoder is unavailable.";
            return false;
        }
        result = encoder_->Initialize(stream_.Get(), WICBitmapEncoderNoCache);
        if (FAILED(result)) {
            error = "The selected image encoder could not initialise.";
            return false;
        }
        ComPtr<IPropertyBag2> options;
        result = encoder_->CreateNewFrame(frame_.ReleaseAndGetAddressOf(),
                                          options.ReleaseAndGetAddressOf());
        if (FAILED(result)) {
            error = "The encoded image frame could not be created.";
            return false;
        }
        result = frame_->Initialize(options.Get());
        if (FAILED(result)) {
            error = "The encoded image frame could not initialise.";
            return false;
        }
        result = frame_->SetSize(width, height);
        if (FAILED(result)) {
            error = "The selected encoder rejected the requested resolution.";
            return false;
        }
        result = frame_->SetResolution(static_cast<double>(dpi), static_cast<double>(dpi));
        if (FAILED(result)) {
            error = "The selected encoder rejected the requested DPI metadata.";
            return false;
        }
        WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat24bppBGR;
        result = frame_->SetPixelFormat(&pixelFormat);
        if (FAILED(result) || !IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppBGR)) {
            error = "The selected encoder does not support the required BGR pixel format.";
            return false;
        }
        width_ = width;
        stride_ = width * 3U;
        bgrRow_.resize(static_cast<std::size_t>(stride_));
        return true;
    }

    bool WriteRow(std::span<const std::uint32_t> pixels, std::string& error) {
        if (!frame_ || pixels.size() != width_) {
            error = "The encoder received an invalid scanline.";
            return false;
        }
        for (std::size_t index = 0U; index < pixels.size(); ++index) {
            const std::uint32_t pixel = pixels[index];
            const std::size_t offset = index * 3U;
            bgrRow_[offset] = static_cast<BYTE>(pixel & 0xFFU);
            bgrRow_[offset + 1U] = static_cast<BYTE>((pixel >> 8U) & 0xFFU);
            bgrRow_[offset + 2U] = static_cast<BYTE>((pixel >> 16U) & 0xFFU);
        }
        const HRESULT result = frame_->WritePixels(1U, stride_, stride_, bgrRow_.data());
        if (FAILED(result)) {
            error = "The image encoder could not write an output scanline.";
            return false;
        }
        return true;
    }

    bool Commit(std::string& error) {
        if (!frame_ || !encoder_) {
            error = "The image encoder was not ready to commit.";
            return false;
        }
        HRESULT result = frame_->Commit();
        if (FAILED(result)) {
            error = "The encoded image frame could not be finalised.";
            return false;
        }
        result = encoder_->Commit();
        if (FAILED(result)) {
            error = "The encoded image file could not be finalised.";
            return false;
        }
        return true;
    }

private:
    ComPtr<IWICImagingFactory> factory_;
    ComPtr<IWICStream> stream_;
    ComPtr<IWICBitmapEncoder> encoder_;
    ComPtr<IWICBitmapFrameEncode> frame_;
    std::vector<BYTE> bgrRow_;
    std::uint32_t width_{0};
    UINT stride_{0};
};

bool RenderStillImageGpu(HINSTANCE instance, GpuBackendPreference backendPreference,
                         const StillRenderRequest& request,
                         const PerformanceSettings& performance,
                         const std::filesystem::path& outputPath, ImageFormat format,
                         std::uint32_t dpi, std::atomic_bool& cancelRequested,
                         const std::function<void(unsigned)>& progressCallback,
                         StillRenderResult& result, std::string& error) {
    result = {};
    error.clear();
    if (request.width == 0U || request.height == 0U) {
        error = "The requested GPU render size is invalid.";
        return false;
    }
    if (!EnsureHiddenGpuHostClass(instance)) {
        error = "The hidden GPU export host window class could not be registered.";
        return false;
    }
    if (cancelRequested.load()) {
        error = "Still render cancelled.";
        return false;
    }

    struct HostWindowGuard {
        HWND value{};
        ~HostWindowGuard() { Reset(); }
        void Reset() noexcept {
            if (value) DestroyWindow(value);
            value = nullptr;
        }
    };

    constexpr int initialHostSize = 64;
    HostWindowGuard host{CreateWindowExW(
        0, L"MandelbrotHighResGpuHost", L"", WS_POPUP,
        0, 0, initialHostSize, initialHostSize,
        nullptr, nullptr, instance, nullptr)};
    if (!host.value) {
        error = "The hidden GPU export host window could not be created.";
        return false;
    }

    GpuRenderer renderer;
    if (progressCallback) progressCallback(10U);
    std::string rendererError;
    if (!renderer.Initialise(host.value, rendererError, backendPreference)) {
        renderer.Shutdown();
        host.Reset();
        error = rendererError;
        return false;
    }

    const int deviceLimit = renderer.MaximumRenderDimension();
    constexpr std::uint32_t overlapPixels = 1U;
    if (deviceLimit <= static_cast<int>(overlapPixels * 2U)) {
        renderer.Shutdown();
        host.Reset();
        error = "The GPU reports an unusable texture or viewport limit for tiled export.";
        return false;
    }
    const std::uint32_t maximumCoreDimension = static_cast<std::uint32_t>(
        deviceLimit - static_cast<int>(overlapPixels * 2U));
    const std::uint32_t tileCoreWidth = std::min({request.width, maximumCoreDimension, 2048U});

    // A complete output scanline is unavoidable for row-streamed image encoders. Keep
    // additional band storage near 64 MiB, then render each band in bounded GPU tiles.
    constexpr std::size_t maximumBandPixels = 16U * 1024U * 1024U;
    const std::size_t widthPixels = static_cast<std::size_t>(request.width);
    const std::size_t heightFromBudget = widthPixels == 0U ? 1U : maximumBandPixels / widthPixels;
    const std::uint32_t tileBandHeight = std::min({
        request.height,
        maximumCoreDimension,
        2048U,
        static_cast<std::uint32_t>(std::max<std::size_t>(1U,
            std::min<std::size_t>(heightFromBudget,
                                  static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max()))))});

    WicRowEncoder encoder;
    if (!encoder.Initialise(outputPath, format, request.width, request.height, dpi, error)) {
        renderer.Shutdown();
        host.Reset();
        return false;
    }

    const StillRenderQuality quality = ResolveStillRenderQuality(request);
    Preset qualityPreset = request.preset;
    qualityPreset.maximumIterations = quality.maximumIterations;
    qualityPreset.antiAliasingLevel = quality.antiAliasingLevel;

    result.statistics.tileWidth = tileCoreWidth;
    result.statistics.tileHeight = tileBandHeight;
    result.statistics.maximumIterations = quality.maximumIterations;
    result.statistics.antiAliasingLevel = quality.antiAliasingLevel;
    result.preview.width = PreviewDimension(request.width, request.height,
                                            request.previewMaximumWidth,
                                            request.previewMaximumHeight, true);
    result.preview.height = PreviewDimension(request.width, request.height,
                                             request.previewMaximumWidth,
                                             request.previewMaximumHeight, false);
    if (result.preview.width > 0U && result.preview.height > 0U) {
        const std::uint64_t previewPixels = static_cast<std::uint64_t>(result.preview.width) *
                                            result.preview.height;
        if (previewPixels > static_cast<std::uint64_t>(
                                std::numeric_limits<std::size_t>::max() /
                                sizeof(std::uint32_t))) {
            renderer.Shutdown();
            host.Reset();
            error = "The bounded render preview is too large for this process.";
            return false;
        }
        result.preview.pixels.assign(static_cast<std::size_t>(previewPixels), 0xFF000000U);
    }

    std::vector<std::uint32_t> previewSourceX(result.preview.width);
    for (std::uint32_t previewX = 0U; previewX < result.preview.width; ++previewX) {
        previewSourceX[previewX] = MapPreviewCoordinate(
            previewX, result.preview.width, request.width);
    }
    std::uint32_t nextPreviewY = 0U;
    std::uint32_t nextPreviewSourceY = result.preview.height > 0U
        ? MapPreviewCoordinate(0U, result.preview.height, request.height)
        : request.height;

    const std::uint64_t tilesAcross =
        (static_cast<std::uint64_t>(request.width) + tileCoreWidth - 1U) / tileCoreWidth;
    const std::uint64_t tileBands =
        (static_cast<std::uint64_t>(request.height) + tileBandHeight - 1U) / tileBandHeight;
    const std::uint64_t totalTiles = std::max<std::uint64_t>(1U, tilesAcross * tileBands);
    std::uint64_t completedTiles = 0U;

    RenderOptions options;
    options.renderScale = 1.0;
    options.antiAliasingLevel = quality.antiAliasingLevel;
    options.precision = performance.precision;
    options.timeSeconds = request.timeSeconds;
    std::vector<std::uint32_t> captured;
    std::vector<RenderRegion> regions(1U);

    for (std::uint32_t bandY = 0U; bandY < request.height;) {
        if (cancelRequested.load()) {
            renderer.Shutdown();
            host.Reset();
            error = "Still render cancelled.";
            return false;
        }
        const std::uint32_t coreBandHeight = std::min(tileBandHeight, request.height - bandY);
        if (widthPixels > std::numeric_limits<std::size_t>::max() /
                              static_cast<std::size_t>(coreBandHeight)) {
            renderer.Shutdown();
            host.Reset();
            error = "A GPU tile band is too large for this process.";
            return false;
        }
        std::vector<std::uint32_t> bandPixels(
            widthPixels * static_cast<std::size_t>(coreBandHeight), 0xFF000000U);
        result.statistics.peakWorkingPixels = std::max(
            result.statistics.peakWorkingPixels, bandPixels.size());

        for (std::uint32_t tileX = 0U; tileX < request.width;) {
            if (cancelRequested.load()) {
                renderer.Shutdown();
                host.Reset();
                error = "Still render cancelled.";
                return false;
            }
            const std::uint32_t coreTileWidth = std::min(tileCoreWidth, request.width - tileX);
            const std::uint32_t overlapLeft = tileX > 0U ? overlapPixels : 0U;
            const std::uint32_t overlapRight = tileX + coreTileWidth < request.width
                ? overlapPixels : 0U;
            const std::uint32_t overlapTop = bandY > 0U ? overlapPixels : 0U;
            const std::uint32_t overlapBottom = bandY + coreBandHeight < request.height
                ? overlapPixels : 0U;
            const std::uint32_t renderX = tileX - overlapLeft;
            const std::uint32_t renderY = bandY - overlapTop;
            const std::uint32_t renderWidth = coreTileWidth + overlapLeft + overlapRight;
            const std::uint32_t renderHeight = coreBandHeight + overlapTop + overlapBottom;

            if (!SetWindowPos(host.value, nullptr, 0, 0,
                              static_cast<int>(renderWidth), static_cast<int>(renderHeight),
                              SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)) {
                renderer.Shutdown();
                host.Reset();
                error = "The hidden GPU tile surface could not be resized.";
                return false;
            }
            renderer.Resize(static_cast<int>(renderWidth), static_cast<int>(renderHeight));

            Preset tilePreset = qualityPreset;
            tilePreset.camera = CameraForStillRenderTile(
                qualityPreset.camera, request.width, request.height,
                renderX, renderY, renderWidth, renderHeight);
            regions[0] = BuildRenderRegion(tilePreset, renderWidth, renderHeight);
            rendererError.clear();
            if (!renderer.Render(regions, options, rendererError)) {
                renderer.Shutdown();
                host.Reset();
                error = rendererError.empty() ? "A GPU export tile could not be rendered."
                                              : rendererError;
                return false;
            }

            int capturedWidth = 0;
            int capturedHeight = 0;
            if (!renderer.CapturePixels(captured, capturedWidth, capturedHeight, rendererError)) {
                renderer.Shutdown();
                host.Reset();
                error = rendererError;
                return false;
            }
            if (capturedWidth != static_cast<int>(renderWidth) ||
                capturedHeight != static_cast<int>(renderHeight)) {
                renderer.Shutdown();
                host.Reset();
                error = "The GPU returned a tile with unexpected dimensions.";
                return false;
            }

            for (std::uint32_t localY = 0U; localY < coreBandHeight; ++localY) {
                const std::uint32_t topDownSourceY = overlapTop + localY;
                const std::uint32_t bottomUpSourceY = renderHeight - 1U - topDownSourceY;
                const std::size_t sourceOffset =
                    static_cast<std::size_t>(bottomUpSourceY) * renderWidth + overlapLeft;
                const std::size_t destinationOffset =
                    static_cast<std::size_t>(localY) * request.width + tileX;
                std::copy_n(captured.begin() + static_cast<std::ptrdiff_t>(sourceOffset),
                            coreTileWidth,
                            bandPixels.begin() + static_cast<std::ptrdiff_t>(destinationOffset));
            }

            const std::size_t tileWorkingPixels = captured.size() <=
                    std::numeric_limits<std::size_t>::max() / 2U
                ? captured.size() * 2U
                : captured.size();
            result.statistics.peakWorkingPixels = std::max(
                result.statistics.peakWorkingPixels,
                bandPixels.size() + tileWorkingPixels);
            result.statistics.renderedPixels +=
                static_cast<std::uint64_t>(coreTileWidth) * coreBandHeight;
            ++completedTiles;
            if (progressCallback) {
                const long double fraction = static_cast<long double>(completedTiles) /
                                             static_cast<long double>(totalTiles);
                const unsigned permille = 20U + static_cast<unsigned>(
                    std::floor(std::min(1.0L, fraction) * 940.0L));
                progressCallback(std::min(permille, 960U));
            }
            tileX += coreTileWidth;
        }

        for (std::uint32_t localY = 0U; localY < coreBandHeight; ++localY) {
            if (cancelRequested.load()) {
                renderer.Shutdown();
                host.Reset();
                error = "Still render cancelled.";
                return false;
            }
            const std::uint32_t outputY = bandY + localY;
            const auto row = std::span<const std::uint32_t>(
                bandPixels.data() + static_cast<std::size_t>(localY) * request.width,
                request.width);
            while (nextPreviewY < result.preview.height && nextPreviewSourceY == outputY) {
                const std::size_t previewOffset =
                    static_cast<std::size_t>(nextPreviewY) * result.preview.width;
                for (std::uint32_t previewX = 0U; previewX < result.preview.width; ++previewX) {
                    result.preview.pixels[previewOffset + previewX] = row[previewSourceX[previewX]];
                }
                ++nextPreviewY;
                nextPreviewSourceY = nextPreviewY < result.preview.height
                    ? MapPreviewCoordinate(nextPreviewY, result.preview.height, request.height)
                    : request.height;
            }
            if (!encoder.WriteRow(row, error)) {
                renderer.Shutdown();
                host.Reset();
                return false;
            }
        }
        bandY += coreBandHeight;
    }

    if (cancelRequested.load()) {
        renderer.Shutdown();
        host.Reset();
        error = "Still render cancelled.";
        return false;
    }
    if (!encoder.Commit(error)) {
        renderer.Shutdown();
        host.Reset();
        return false;
    }
    renderer.Shutdown();
    host.Reset();
    if (progressCallback) progressCallback(1000U);
    return true;
}

std::filesystem::path MakeTemporaryPath(ImageFormat format) {
    std::error_code error;
    std::filesystem::path folder = std::filesystem::temp_directory_path(error);
    if (error) folder = std::filesystem::current_path(error);
    folder /= L"MandelbrotLiveWallpaper";
    std::filesystem::create_directories(folder, error);
    const auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::wostringstream fileName;
    fileName << L"hires-render-" << timestamp << L"-" << GetCurrentThreadId()
             << L"." << FormatExtension(format);
    return folder / fileName.str();
}

void DeleteTemporaryOutput(State& state) {
    if (state.completedPath.empty()) return;
    std::error_code error;
    std::filesystem::remove(state.completedPath, error);
    state.completedPath.clear();
}


void JoinWorker(State& state) {
    if (state.worker.joinable()) state.worker.join();
}

void SetInputEnabled(State& state, bool enabled) {
    for (const int id : {CoordinatesEdit, WidthEdit, HeightEdit, DpiEdit, FormatCombo, BackendCombo}) {
        EnableWindow(GetDlgItem(state.window, id), enabled ? TRUE : FALSE);
    }
    EnableWindow(GetDlgItem(state.window, RenderButton), enabled ? TRUE : FALSE);
    EnableWindow(GetDlgItem(state.window, CancelRenderButton), enabled ? FALSE : TRUE);
    EnableWindow(GetDlgItem(state.window, PreviewButton),
                 enabled && state.rendered ? TRUE : FALSE);
    EnableWindow(GetDlgItem(state.window, SaveAsButton),
                 enabled && state.rendered ? TRUE : FALSE);
}

void SetStatus(State& state, const std::wstring& text) {
    SetWindowTextW(GetDlgItem(state.window, StatusLabel), text.c_str());
}

void FinishRender(State& state) {
    JoinWorker(state);
    state.rendering = false;
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETPOS,
                 state.workerSucceeded ? 1000U : 0U, 0);
    if (state.workerSucceeded) {
        state.rendered = true;
        std::wostringstream status;
        status << L"Rendered " << state.completedWidth << L" x " << state.completedHeight
               << L" at " << state.completedDpi << L" DPI as "
               << FormatName(state.completedFormat)
               << L" using " << state.completedResult.statistics.maximumIterations
               << L" iterations and " << state.completedResult.statistics.antiAliasingLevel
               << L"x AA";
        if (state.completedResult.statistics.tileWidth > 0U &&
            state.completedResult.statistics.tileHeight > 0U) {
            status << L"; tiles up to " << state.completedResult.statistics.tileWidth
                   << L" x " << state.completedResult.statistics.tileHeight;
        }
        status << L". Preview or Save As when ready.";
        SetStatus(state, status.str());
    } else {
        state.rendered = false;
        if (state.workerCancelled) {
            SetStatus(state, L"Render cancelled. No output file was retained.");
        } else {
            SetStatus(state, L"Render failed.");
            MessageBoxW(state.window, ToWide(state.workerError).c_str(),
                        L"Render Hi-Res", MB_OK | MB_ICONERROR);
        }
    }
    SetInputEnabled(state, true);
}

bool StartRender(State& state) {
    if (state.rendering) return false;
    CameraState camera = state.snapshot.camera;
    if (!ParseCoordinateString(GetText(GetDlgItem(state.window, CoordinatesEdit)), camera)) {
        MessageBoxW(state.window, L"Use the coordinate format centreX,centreY,scale.",
                    L"Render Hi-Res", MB_OK | MB_ICONERROR);
        return false;
    }
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    std::uint32_t dpi = 0U;
    if (!ParsePositiveUInt32(GetDlgItem(state.window, WidthEdit), width) ||
        !ParsePositiveUInt32(GetDlgItem(state.window, HeightEdit), height) ||
        !ParsePositiveUInt32(GetDlgItem(state.window, DpiEdit), dpi)) {
        MessageBoxW(state.window,
                    L"Resolution and DPI must be positive whole numbers supported by Windows.",
                    L"Render Hi-Res", MB_OK | MB_ICONERROR);
        return false;
    }

    JoinWorker(state);
    DeleteTemporaryOutput(state);
    state.completedResult = {};
    state.rendered = false;
    state.workerSucceeded = false;
    state.workerCancelled = false;
    state.workerError.clear();
    state.cancelRequested.store(false);
    const ImageFormat format = SelectedFormat(state.window);
    const RenderBackend backend = SelectedBackend(state.window);
    const std::filesystem::path outputPath = MakeTemporaryPath(format);

    Preset snapshot = state.snapshot;
    snapshot.camera = camera;
    ValidateAndNormalise(snapshot);
    StillRenderRequest request;
    request.preset = snapshot;
    request.width = width;
    request.height = height;
    request.tileWidth = 256U;
    request.previewMaximumWidth = 1600U;
    request.previewMaximumHeight = 1000U;
    request.timeSeconds = std::fmod(
        std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()).count(),
        100000.0);

    state.rendering = true;
    SetInputEnabled(state, false);
    SendMessageW(GetDlgItem(state.window, ProgressBar), PBM_SETPOS, 0, 0);
    SetStatus(state, BackendStatus(backend));

    State* statePointer = &state;
    try {
        state.worker = std::thread([statePointer, request, outputPath, format, backend, width, height, dpi]() {
            const HRESULT comResult = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            const bool comInitialised = SUCCEEDED(comResult);
            std::string error;
            StillRenderResult renderResult;
            bool succeeded = false;
            bool cancelled = false;
            try {
                if (!comInitialised && comResult != RPC_E_CHANGED_MODE) {
                    error = "Windows Imaging Component could not initialise on the render thread.";
                } else {
                    auto postPermille = [statePointer](unsigned permille) {
                        PostMessageW(statePointer->window, kRenderProgressMessage,
                                     static_cast<WPARAM>(std::min(permille, 1000U)), 0);
                    };
                    if (backend != RenderBackend::CpuTiled) {
                        postPermille(10U);
                        const GpuBackendPreference gpuPreference = backend == RenderBackend::GpuDirect3D11
                            ? GpuBackendPreference::Direct3D11
                            : GpuBackendPreference::OpenGL;
                        succeeded = RenderStillImageGpu(statePointer->instance, gpuPreference, request, statePointer->performance,
                                                        outputPath, format, dpi, statePointer->cancelRequested,
                                                        postPermille, renderResult, error);
                        cancelled = statePointer->cancelRequested.load() || error == "Still render cancelled.";
                    } else {
                        WicRowEncoder encoder;
                        if (encoder.Initialise(outputPath, format, width, height, dpi, error)) {
                            unsigned lastPermille = 0U;
                            const bool rendered = RenderStillImageTiled(
                                request,
                                [&encoder](std::uint32_t, std::span<const std::uint32_t> row,
                                           std::string& writerError) {
                                    return encoder.WriteRow(row, writerError);
                                },
                                [statePointer, &lastPermille](const StillRenderProgress& progress) {
                                    const unsigned permille = progress.totalRows == 0U
                                        ? 0U
                                        : static_cast<unsigned>((static_cast<std::uint64_t>(progress.completedRows) *
                                                                 1000ULL) /
                                                                progress.totalRows);
                                    if (permille != lastPermille ||
                                        progress.completedRows == progress.totalRows) {
                                        lastPermille = permille;
                                        PostMessageW(statePointer->window, kRenderProgressMessage,
                                                     static_cast<WPARAM>(permille),
                                                     static_cast<LPARAM>(progress.completedRows));
                                    }
                                },
                                [statePointer]() { return statePointer->cancelRequested.load(); },
                                renderResult,
                                error);
                            cancelled = statePointer->cancelRequested.load() ||
                                        error == "Still render cancelled.";
                            if (rendered && !cancelled) succeeded = encoder.Commit(error);
                        }
                    }
                }
            } catch (const std::bad_alloc&) {
                error = "There is not enough memory for the requested scanline or bounded preview.";
            } catch (const std::exception& exception) {
                error = std::string("The background render failed: ") + exception.what();
            } catch (...) {
                error = "The background render failed with an unknown error.";
            }
            if (!succeeded) {
                std::error_code removeError;
                std::filesystem::remove(outputPath, removeError);
            }
            statePointer->workerSucceeded = succeeded;
            statePointer->workerCancelled = cancelled;
            statePointer->workerError = error;
            if (succeeded) {
                statePointer->completedResult = std::move(renderResult);
                statePointer->completedPath = outputPath;
                statePointer->completedFormat = format;
                statePointer->completedWidth = width;
                statePointer->completedHeight = height;
                statePointer->completedDpi = dpi;
            }
            if (comInitialised) CoUninitialize();
            PostMessageW(statePointer->window, kRenderCompleteMessage, 0, 0);
        });
    } catch (const std::exception& exception) {
        state.rendering = false;
        state.workerError = std::string("The background render thread could not start: ") + exception.what();
        SetInputEnabled(state, true);
        SetStatus(state, L"Render could not start.");
        MessageBoxW(state.window, ToWide(state.workerError).c_str(),
                    L"Render Hi-Res", MB_OK | MB_ICONERROR);
        return false;
    }
    return true;
}

std::wstring SaveFileDialog(HWND owner, ImageFormat format) {
    wchar_t fileName[MAX_PATH]{};
    std::wstring defaultName = L"mandelbrot-hires.";
    defaultName += FormatExtension(format);
    wcsncpy_s(fileName, MAX_PATH, defaultName.c_str(), _TRUNCATE);

    const wchar_t* filter = nullptr;
    switch (format) {
    case ImageFormat::Png: filter = L"PNG image (*.png)\0*.png\0All files (*.*)\0*.*\0"; break;
    case ImageFormat::Tiff: filter = L"TIFF image (*.tif;*.tiff)\0*.tif;*.tiff\0All files (*.*)\0*.*\0"; break;
    case ImageFormat::Bmp: filter = L"Bitmap image (*.bmp)\0*.bmp\0All files (*.*)\0*.*\0"; break;
    }
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = owner;
    dialog.lpstrFilter = filter;
    dialog.lpstrFile = fileName;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_OVERWRITEPROMPT;
    dialog.lpstrDefExt = FormatExtension(format);
    return GetSaveFileNameW(&dialog) ? std::wstring(fileName) : std::wstring{};
}


std::filesystem::path NormaliseSavedPath(const std::wstring& selected, ImageFormat format) {
    std::filesystem::path path(selected);
    std::wstring extension = path.extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](wchar_t character) { return static_cast<wchar_t>(std::towlower(character)); });
    bool valid = false;
    if (format == ImageFormat::Png) valid = extension == L".png";
    else if (format == ImageFormat::Tiff) valid = extension == L".tif" || extension == L".tiff";
    else valid = extension == L".bmp";
    if (!valid) path.replace_extension(FormatExtension(format));
    return path;
}

void SaveCompletedOutput(State& state) {
    if (!state.rendered || state.completedPath.empty()) return;
    const std::wstring selected = SaveFileDialog(state.window, state.completedFormat);
    if (selected.empty()) return;
    const std::filesystem::path targetPath =
        NormaliseSavedPath(selected, state.completedFormat);
    std::error_code error;
    std::filesystem::copy_file(
        state.completedPath, targetPath,
        std::filesystem::copy_options::overwrite_existing, error);
    if (error) {
        MessageBoxW(state.window, L"The completed image could not be copied to the selected path.",
                    L"Save Render", MB_OK | MB_ICONERROR);
        return;
    }
    SetStatus(state, L"Image saved successfully.");
}

LRESULT CALLBACK PreviewProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    (void)wParam;
    auto* previewState = reinterpret_cast<PreviewWindowState*>(
        GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        previewState = static_cast<PreviewWindowState*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA,
                          reinterpret_cast<LONG_PTR>(previewState));
    }
    if (!previewState) return DefWindowProcW(window, message, wParam, lParam);
    if (message == WM_PAINT) {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        RECT client{};
        GetClientRect(window, &client);
        FillRect(dc, &client, reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1));
        const StillRenderPreview* preview = previewState->preview;
        if (preview && !preview->pixels.empty() && preview->width > 0U && preview->height > 0U) {
            BITMAPINFO info{};
            info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            info.bmiHeader.biWidth = static_cast<LONG>(preview->width);
            info.bmiHeader.biHeight = -static_cast<LONG>(preview->height);
            info.bmiHeader.biPlanes = 1;
            info.bmiHeader.biBitCount = 32;
            info.bmiHeader.biCompression = BI_RGB;
            const int availableWidth = std::max(1L, client.right - client.left - 20L);
            const int availableHeight = std::max(1L, client.bottom - client.top - 20L);
            const double scale = std::min(
                static_cast<double>(availableWidth) / preview->width,
                static_cast<double>(availableHeight) / preview->height);
            const int drawWidth = std::max(1, static_cast<int>(std::lround(preview->width * scale)));
            const int drawHeight = std::max(1, static_cast<int>(std::lround(preview->height * scale)));
            const int drawX = (client.right - client.left - drawWidth) / 2;
            const int drawY = (client.bottom - client.top - drawHeight) / 2;
            SetStretchBltMode(dc, HALFTONE);
            StretchDIBits(dc, drawX, drawY, drawWidth, drawHeight,
                          0, 0, static_cast<int>(preview->width),
                          static_cast<int>(preview->height), preview->pixels.data(),
                          &info, DIB_RGB_COLORS, SRCCOPY);
        }
        EndPaint(window, &paint);
        return 0;
    }
    if (message == WM_CLOSE) {
        DestroyWindow(window);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

void ShowPreviewWindow(State& state) {
    if (!state.rendered || state.completedResult.preview.pixels.empty()) return;
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = PreviewProcedure;
        windowClass.hInstance = state.instance;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        windowClass.lpszClassName = kPreviewClassName;
        RegisterClassExW(&windowClass);
        registered = true;
    }
    PreviewWindowState previewState{&state.completedResult.preview};
    RECT ownerRect{};
    GetWindowRect(state.window, &ownerRect);
    HWND previewWindow = CreateWindowExW(
        WS_EX_APPWINDOW, kPreviewClassName, L"Rendered Hi-Res Preview",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        ownerRect.left + 36, ownerRect.top + 36, 920, 720,
        state.window, nullptr, state.instance, &previewState);
    if (!previewWindow) return;
    EnableWindow(state.window, FALSE);
    MSG message{};
    while (IsWindow(previewWindow) && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(previewWindow, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    EnableWindow(state.window, TRUE);
    SetForegroundWindow(state.window);
}

void CancelRender(State& state) {
    if (!state.rendering) return;
    state.cancelRequested.store(true);
    EnableWindow(GetDlgItem(state.window, CancelRenderButton), FALSE);
    SetStatus(state, L"Cancelling after the current tile...");
}

void CloseDialog(State& state) {
    if (state.rendering) {
        CancelRender(state);
        JoinWorker(state);
        state.rendering = false;
    } else {
        JoinWorker(state);
    }
    DestroyWindow(state.window);
}

LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<State*>(GetWindowLongPtrW(window, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        state = static_cast<State*>(create->lpCreateParams);
        state->window = window;
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    }
    if (!state) return DefWindowProcW(window, message, wParam, lParam);

    if (message == WM_CREATE) {
        state->font = CreateResponsiveDialogFont(state->dpi);
        int y = 18;
        Add(*state, WC_STATICW, L"Coordinate string", SS_LEFT, 0, 18, y + 4, 140, 20);
        Add(*state, WC_EDITW, MakeCoordinateString(state->snapshot.camera).c_str(),
            ES_AUTOHSCROLL | WS_TABSTOP, CoordinatesEdit, 160, y, 450, 26);
        y += 38;
        Add(*state, WC_STATICW, L"Width", SS_LEFT, 0, 18, y + 4, 140, 20);
        Add(*state, WC_EDITW, L"3840", ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
            WidthEdit, 160, y, 130, 26);
        Add(*state, WC_STATICW, L"Height", SS_LEFT, 0, 310, y + 4, 70, 20);
        Add(*state, WC_EDITW, L"2160", ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
            HeightEdit, 382, y, 130, 26);
        y += 38;
        Add(*state, WC_STATICW, L"DPI", SS_LEFT, 0, 18, y + 4, 140, 20);
        Add(*state, WC_EDITW, L"300", ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
            DpiEdit, 160, y, 130, 26);
        Add(*state, WC_STATICW, L"Output format", SS_LEFT, 0, 310, y + 4, 100, 20);
        HWND formatCombo = Add(*state, WC_COMBOBOXW, L"",
                               CBS_DROPDOWNLIST | WS_TABSTOP, FormatCombo,
                               414, y, 196, 150);
        for (const wchar_t* name : {L"PNG - lossless", L"TIFF - lossless", L"BMP - uncompressed"}) {
            SendMessageW(formatCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        }
        SendMessageW(formatCombo, CB_SETCURSEL, 0, 0);
        y += 38;
        Add(*state, WC_STATICW, L"Renderer", SS_LEFT, 0, 18, y + 4, 140, 20);
        HWND backendCombo = Add(*state, WC_COMBOBOXW, L"",
                                CBS_DROPDOWNLIST | WS_TABSTOP, BackendCombo,
                                160, y, 450, 180);
        for (const wchar_t* name : {L"GPU Direct3D 11 tiled - default, supports huge renders", L"GPU OpenGL tiled - compatibility fallback", L"CPU scanline tiled - compatibility fallback"}) {
            SendMessageW(backendCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(name));
        }
        SendMessageW(backendCombo, CB_SETCURSEL, 0, 0);
        y += 44;
        Add(*state, WC_STATICW,
            L"No arbitrary resolution or DPI cap is applied. Direct3D 11 and OpenGL GPU export are split into bounded overlapping tiles and encoded a band at a time, so output size is not limited to one GPU texture. Iteration depth is automatically raised from the preset minimum when the camera and output resolution reveal finer detail. Actual limits are the selected Windows encoder, disk space and available memory.",
            SS_LEFT, 0, 18, y, 592, 72);
        y += 80;
        Add(*state, PROGRESS_CLASSW, L"", PBS_SMOOTH, ProgressBar, 18, y, 592, 22);
        SendMessageW(GetDlgItem(window, ProgressBar), PBM_SETRANGE32, 0, 1000);
        y += 32;
        Add(*state, WC_STATICW,
            L"Adjust coordinates, resolution, DPI and format, then render a still frame using the current preview equation, palette and visual settings.",
            SS_LEFT, StatusLabel, 18, y, 592, 44);
        y += 54;
        Add(*state, WC_BUTTONW, L"&Render", BS_DEFPUSHBUTTON | WS_TABSTOP,
            RenderButton, 18, y, 105, 32);
        Add(*state, WC_BUTTONW, L"&Cancel Render", BS_PUSHBUTTON | WS_TABSTOP,
            CancelRenderButton, 131, y, 125, 32);
        Add(*state, WC_BUTTONW, L"&Preview", BS_PUSHBUTTON | WS_TABSTOP,
            PreviewButton, 264, y, 105, 32);
        Add(*state, WC_BUTTONW, L"&Save As...", BS_PUSHBUTTON | WS_TABSTOP,
            SaveAsButton, 377, y, 105, 32);
        Add(*state, WC_BUTTONW, L"&Close", BS_PUSHBUTTON | WS_TABSTOP,
            CloseButton, 505, y, 105, 32);
        SendMessageW(window, DM_SETDEFID, RenderButton, 0);
        state->layout.Initialise(window, state->dpi, state->font, 640, y + 70);
        state->layout.Focus(GetDlgItem(window, CoordinatesEdit));
        SetInputEnabled(*state, true);
        return 0;
    }
    if (message == WM_GETMINMAXINFO) {
        state->layout.ApplyMinimumTrackSize(*reinterpret_cast<MINMAXINFO*>(lParam));
        return 0;
    }
    if (message == WM_SIZE) {
        state->layout.OnSize();
        return 0;
    }
    if ((message == WM_VSCROLL || message == WM_HSCROLL) && lParam == 0) {
        if (state->layout.OnScroll(message, wParam)) return 0;
    }
    if (message == WM_MOUSEWHEEL && state->layout.OnMouseWheel(wParam)) return 0;
    if (message == WM_DPICHANGED) {
        const UINT newDpi = HIWORD(wParam);
        HFONT newFont = CreateResponsiveDialogFont(newDpi);
        if (!newFont) newFont = state->font;
        const RECT suggested = *reinterpret_cast<const RECT*>(lParam);
        state->layout.OnDpiChanged(newDpi, suggested, newFont);
        if (newFont != state->font && state->font) DeleteObject(state->font);
        state->font = newFont;
        state->dpi = newDpi;
        return 0;
    }
    if (message == kRenderProgressMessage) {
        const auto permille = static_cast<unsigned>(wParam);
        SendMessageW(GetDlgItem(window, ProgressBar), PBM_SETPOS,
                     std::min(permille, 1000U), 0);
        std::wostringstream status;
        status << L"Rendering in the background: "
               << std::fixed << std::setprecision(1)
               << static_cast<double>(std::min(permille, 1000U)) / 10.0 << L"%.";
        SetStatus(*state, status.str());
        return 0;
    }
    if (message == kRenderCompleteMessage) {
        FinishRender(*state);
        return 0;
    }
    if (message == WM_COMMAND) {
        switch (LOWORD(wParam)) {
        case RenderButton:
        case IDOK:
            StartRender(*state);
            return 0;
        case CancelRenderButton:
            CancelRender(*state);
            return 0;
        case PreviewButton:
            ShowPreviewWindow(*state);
            return 0;
        case SaveAsButton:
            SaveCompletedOutput(*state);
            return 0;
        case CloseButton:
        case IDCANCEL:
            CloseDialog(*state);
            return 0;
        }
    }
    if (message == WM_CLOSE) {
        CloseDialog(*state);
        return 0;
    }
    if (message == WM_DESTROY) {
        state->cancelRequested.store(true);
        JoinWorker(*state);
        DeleteTemporaryOutput(*state);
        RememberDialogPlacement(window, kClassName, state->dpi);
        state->layout.Shutdown();
        if (state->font) DeleteObject(state->font);
        state->font = nullptr;
        state->done = true;
        EnableWindow(state->owner, TRUE);
        SetForegroundWindow(state->owner);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

} // namespace

void HighResRenderDialog::Show(HWND owner, HINSTANCE instance, const Preset& snapshot, const PerformanceSettings& performance) {
    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = Procedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kClassName;
    RegisterClassExW(&windowClass);

    State state;
    state.owner = owner;
    state.instance = instance;
    state.snapshot = snapshot;
    state.performance = performance;
    state.dpi = DialogDpi(owner);
    const RECT dialogRect = ResponsiveDialogRect(owner, 680, 500, state.dpi, kClassName);
    HWND window = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
        kClassName, L"Render Hi-Res and Save",
        WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MAXIMIZEBOX |
            WS_POPUP | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
        dialogRect.left, dialogRect.top,
        dialogRect.right - dialogRect.left, dialogRect.bottom - dialogRect.top,
        owner, nullptr, instance, &state);
    if (!window) return;

    EnableWindow(owner, FALSE);
    MSG message{};
    while (!state.done && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!ProcessModalDialogMessage(window, CloseButton, message, &state.layout)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
}
#endif
} // namespace mw
