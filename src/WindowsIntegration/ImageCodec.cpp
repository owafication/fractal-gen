#include "WindowsIntegration/ImageCodec.h"

#ifdef _WIN32
#include <wincodec.h>
#include <wrl/client.h>
#endif

#include <algorithm>
#include <limits>
#include <vector>

namespace mw {

#ifdef _WIN32
namespace {
using Microsoft::WRL::ComPtr;

class ComInitialisation {
public:
    ComInitialisation() : result_(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {}
    ~ComInitialisation() { if (SUCCEEDED(result_)) CoUninitialize(); }
    [[nodiscard]] bool Available() const noexcept {
        return SUCCEEDED(result_) || result_ == RPC_E_CHANGED_MODE;
    }
private:
    HRESULT result_{E_FAIL};
};

const GUID& ContainerGuid(SavedImageFormat format) {
    switch (format) {
    case SavedImageFormat::Png: return GUID_ContainerFormatPng;
    case SavedImageFormat::Jpeg: return GUID_ContainerFormatJpeg;
    case SavedImageFormat::Tiff: return GUID_ContainerFormatTiff;
    case SavedImageFormat::Bmp: return GUID_ContainerFormatBmp;
    }
    return GUID_ContainerFormatPng;
}

void ConfigureEncoderOptions(IPropertyBag2* options,
                             SavedImageFormat format,
                             int compressionQuality) {
    if (!options) return;
    const int quality = std::clamp(compressionQuality, 1, 100);
    if (format == SavedImageFormat::Jpeg) {
        PROPBAG2 property{};
        property.pstrName = const_cast<LPOLESTR>(L"ImageQuality");
        VARIANT value{};
        VariantInit(&value);
        value.vt = VT_R4;
        value.fltVal = static_cast<float>(quality) / 100.0F;
        (void)options->Write(1, &property, &value);
        VariantClear(&value);
    } else if (format == SavedImageFormat::Tiff) {
        PROPBAG2 property{};
        property.pstrName = const_cast<LPOLESTR>(L"TiffCompressionMethod");
        VARIANT value{};
        VariantInit(&value);
        value.vt = VT_UI1;
        value.bVal = static_cast<BYTE>(quality >= 65 ? WICTiffCompressionZIP :
                     quality >= 30 ? WICTiffCompressionLZW : WICTiffCompressionNone);
        (void)options->Write(1, &property, &value);
        VariantClear(&value);
    } else if (format == SavedImageFormat::Png) {
        PROPBAG2 property{};
        property.pstrName = const_cast<LPOLESTR>(L"FilterOption");
        VARIANT value{};
        VariantInit(&value);
        value.vt = VT_UI1;
        value.bVal = static_cast<BYTE>(quality >= 70 ? WICPngFilterAdaptive :
                     quality >= 35 ? WICPngFilterPaeth : WICPngFilterNone);
        (void)options->Write(1, &property, &value);
        VariantClear(&value);
    }
}

} // namespace

struct WicRowEncoder::Impl {
    ComPtr<IWICImagingFactory> factory;
    ComPtr<IWICStream> stream;
    ComPtr<IWICBitmapEncoder> encoder;
    ComPtr<IWICBitmapFrameEncode> frame;
    std::vector<BYTE> bgrRow;
    std::uint32_t width{0U};
    UINT stride{0U};
};

WicRowEncoder::WicRowEncoder() : impl_(std::make_unique<Impl>()) {}
WicRowEncoder::~WicRowEncoder() = default;

bool WicRowEncoder::Initialise(const std::filesystem::path& path,
                               SavedImageFormat format,
                               int compressionQuality,
                               std::uint32_t width,
                               std::uint32_t height,
                               std::uint32_t dpi,
                               std::string& error) {
    error.clear();
    if (!impl_) impl_ = std::make_unique<Impl>();
    impl_ = std::make_unique<Impl>();
    if (width == 0U || height == 0U || dpi == 0U ||
        width > std::numeric_limits<UINT>::max() / 3U) {
        error = "The requested image dimensions or DPI exceed the encoder limits.";
        return false;
    }
    std::error_code directoryError;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), directoryError);
    }
    if (directoryError) {
        error = "The image output directory could not be created.";
        return false;
    }
    HRESULT result = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(impl_->factory.ReleaseAndGetAddressOf()));
    if (FAILED(result)) {
        error = "Windows Imaging Component could not start.";
        return false;
    }
    result = impl_->factory->CreateStream(impl_->stream.ReleaseAndGetAddressOf());
    if (FAILED(result) ||
        FAILED(impl_->stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))) {
        error = "The temporary image file could not be opened for writing.";
        return false;
    }
    result = impl_->factory->CreateEncoder(ContainerGuid(format), nullptr,
                                            impl_->encoder.ReleaseAndGetAddressOf());
    if (FAILED(result) ||
        FAILED(impl_->encoder->Initialize(impl_->stream.Get(), WICBitmapEncoderNoCache))) {
        error = "The selected Windows image encoder could not initialise.";
        return false;
    }
    ComPtr<IPropertyBag2> options;
    result = impl_->encoder->CreateNewFrame(impl_->frame.ReleaseAndGetAddressOf(),
                                             options.ReleaseAndGetAddressOf());
    if (FAILED(result)) {
        error = "The encoded image frame could not be created.";
        return false;
    }
    ConfigureEncoderOptions(options.Get(), format, compressionQuality);
    if (FAILED(impl_->frame->Initialize(options.Get())) ||
        FAILED(impl_->frame->SetSize(width, height)) ||
        FAILED(impl_->frame->SetResolution(static_cast<double>(dpi),
                                           static_cast<double>(dpi)))) {
        error = "The selected encoder rejected the requested image metadata.";
        return false;
    }
    WICPixelFormatGUID pixelFormat = GUID_WICPixelFormat24bppBGR;
    if (FAILED(impl_->frame->SetPixelFormat(&pixelFormat)) ||
        !IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppBGR)) {
        error = "The selected encoder does not support the required BGR pixel format.";
        return false;
    }
    impl_->width = width;
    impl_->stride = width * 3U;
    impl_->bgrRow.resize(static_cast<std::size_t>(impl_->stride));
    return true;
}

bool WicRowEncoder::WriteRow(std::span<const std::uint32_t> pixels,
                             std::string& error) {
    if (!impl_ || !impl_->frame || pixels.size() != impl_->width) {
        error = "The encoder received an invalid scanline.";
        return false;
    }
    for (std::size_t index = 0U; index < pixels.size(); ++index) {
        const std::uint32_t pixel = pixels[index];
        const std::size_t offset = index * 3U;
        impl_->bgrRow[offset] = static_cast<BYTE>(pixel & 0xFFU);
        impl_->bgrRow[offset + 1U] = static_cast<BYTE>((pixel >> 8U) & 0xFFU);
        impl_->bgrRow[offset + 2U] = static_cast<BYTE>((pixel >> 16U) & 0xFFU);
    }
    if (FAILED(impl_->frame->WritePixels(1U, impl_->stride, impl_->stride,
                                         impl_->bgrRow.data()))) {
        error = "The image encoder could not write an output scanline.";
        return false;
    }
    return true;
}

bool WicRowEncoder::Commit(std::string& error) {
    if (!impl_ || !impl_->frame || !impl_->encoder) {
        error = "The image encoder was not ready to commit.";
        return false;
    }
    if (FAILED(impl_->frame->Commit()) || FAILED(impl_->encoder->Commit())) {
        error = "The encoded image file could not be finalised.";
        return false;
    }
    return true;
}

bool ValidateImageDimensionsWithWic(const std::filesystem::path& path,
                                    std::uint32_t expectedWidth,
                                    std::uint32_t expectedHeight,
                                    std::string& error) {
    error.clear();
    ComInitialisation com;
    if (!com.Available()) {
        error = "Windows Imaging Component could not initialise COM.";
        return false;
    }
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(factory.GetAddressOf())))) {
        error = "Windows Imaging Component could not start.";
        return false;
    }
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                  WICDecodeMetadataCacheOnDemand,
                                                  decoder.GetAddressOf()))) {
        error = "The rendered image could not be reopened for validation.";
        return false;
    }
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0U, frame.GetAddressOf()))) {
        error = "The rendered image does not contain a readable frame.";
        return false;
    }
    UINT width = 0U;
    UINT height = 0U;
    if (FAILED(frame->GetSize(&width, &height)) || width != expectedWidth ||
        height != expectedHeight) {
        error = "The rendered image dimensions do not match the export job.";
        return false;
    }
    return true;
}

const wchar_t* SavedImageExtension(SavedImageFormat format) noexcept {
    switch (format) {
    case SavedImageFormat::Png: return L"png";
    case SavedImageFormat::Jpeg: return L"jpg";
    case SavedImageFormat::Tiff: return L"tiff";
    case SavedImageFormat::Bmp: return L"bmp";
    }
    return L"png";
}

const wchar_t* SavedImageDisplayName(SavedImageFormat format) noexcept {
    switch (format) {
    case SavedImageFormat::Png: return L"PNG";
    case SavedImageFormat::Jpeg: return L"JPEG";
    case SavedImageFormat::Tiff: return L"TIFF";
    case SavedImageFormat::Bmp: return L"BMP";
    }
    return L"PNG";
}

bool SavePixelsWithWic(const std::filesystem::path& path,
                       SavedImageFormat format,
                       int compressionQuality,
                       std::span<const std::uint32_t> bgraPixels,
                       std::uint32_t width,
                       std::uint32_t height,
                       double dpi,
                       std::string& error) {
    error.clear();
    ComInitialisation com;
    if (!com.Available()) {
        error = "Windows Imaging Component could not initialise COM.";
        return false;
    }
    if (width == 0U || height == 0U ||
        bgraPixels.size() != static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        error = "The image dimensions or pixel buffer are invalid.";
        return false;
    }
    std::error_code directoryError;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), directoryError);
    if (directoryError) {
        error = "The image output directory could not be created.";
        return false;
    }

    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(factory.GetAddressOf())))) {
        error = "Windows Imaging Component could not start.";
        return false;
    }
    ComPtr<IWICStream> stream;
    if (FAILED(factory->CreateStream(stream.GetAddressOf())) ||
        FAILED(stream->InitializeFromFilename(path.c_str(), GENERIC_WRITE))) {
        error = "The image file could not be created.";
        return false;
    }
    ComPtr<IWICBitmapEncoder> encoder;
    if (FAILED(factory->CreateEncoder(ContainerGuid(format), nullptr, encoder.GetAddressOf())) ||
        FAILED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache))) {
        error = "The selected image encoder could not start.";
        return false;
    }
    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> options;
    if (FAILED(encoder->CreateNewFrame(frame.GetAddressOf(), options.GetAddressOf()))) {
        error = "The image frame could not be created.";
        return false;
    }
    ConfigureEncoderOptions(options.Get(), format, compressionQuality);
    if (FAILED(frame->Initialize(options.Get())) ||
        FAILED(frame->SetSize(width, height)) ||
        FAILED(frame->SetResolution(dpi, dpi))) {
        error = "The image frame could not be initialised.";
        return false;
    }

    WICPixelFormatGUID pixelFormat = format == SavedImageFormat::Jpeg
        ? GUID_WICPixelFormat24bppBGR : GUID_WICPixelFormat32bppBGRA;
    if (FAILED(frame->SetPixelFormat(&pixelFormat))) {
        error = "The image encoder rejected the pixel format.";
        return false;
    }

    if (IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGRA)) {
        if (width > std::numeric_limits<UINT>::max() / 4U) {
            error = "The image scanline is too wide for the encoder.";
            return false;
        }
        const UINT stride = width * 4U;
        const std::uint64_t byteCount = static_cast<std::uint64_t>(stride) * height;
        if (byteCount > std::numeric_limits<UINT>::max()) {
            error = "The complete image is too large for a single WIC write.";
            return false;
        }
        if (FAILED(frame->WritePixels(height, stride, static_cast<UINT>(byteCount),
                                      reinterpret_cast<BYTE*>(const_cast<std::uint32_t*>(bgraPixels.data()))))) {
            error = "The image pixels could not be encoded.";
            return false;
        }
    } else {
        if (width > std::numeric_limits<UINT>::max() / 3U) {
            error = "The image scanline is too wide for the encoder.";
            return false;
        }
        const UINT stride = width * 3U;
        std::vector<BYTE> bgr(static_cast<std::size_t>(stride) * height);
        for (std::size_t index = 0; index < bgraPixels.size(); ++index) {
            const std::uint32_t pixel = bgraPixels[index];
            bgr[index * 3U + 0U] = static_cast<BYTE>(pixel & 0xFFU);
            bgr[index * 3U + 1U] = static_cast<BYTE>((pixel >> 8U) & 0xFFU);
            bgr[index * 3U + 2U] = static_cast<BYTE>((pixel >> 16U) & 0xFFU);
        }
        if (bgr.size() > std::numeric_limits<UINT>::max() ||
            FAILED(frame->WritePixels(height, stride, static_cast<UINT>(bgr.size()), bgr.data()))) {
            error = "The image pixels could not be encoded.";
            return false;
        }
    }

    if (FAILED(frame->Commit()) || FAILED(encoder->Commit())) {
        error = "The image file could not be finalised.";
        return false;
    }
    return true;
}

bool LoadPixelsWithWic(const std::filesystem::path& path,
                       std::vector<std::uint32_t>& bgraPixels,
                       int& width,
                       int& height,
                       std::string& error) {
    error.clear();
    ComInitialisation com;
    if (!com.Available()) {
        error = "Windows Imaging Component could not initialise COM.";
        return false;
    }
    bgraPixels.clear();
    width = 0;
    height = 0;
    ComPtr<IWICImagingFactory> factory;
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(factory.GetAddressOf())))) {
        error = "Windows Imaging Component could not start.";
        return false;
    }
    ComPtr<IWICBitmapDecoder> decoder;
    if (FAILED(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                                  WICDecodeMetadataCacheOnLoad,
                                                  decoder.GetAddressOf()))) {
        error = "The saved image could not be opened.";
        return false;
    }
    ComPtr<IWICBitmapFrameDecode> frame;
    if (FAILED(decoder->GetFrame(0, frame.GetAddressOf()))) {
        error = "The saved image has no readable frame.";
        return false;
    }
    UINT decodedWidth = 0U;
    UINT decodedHeight = 0U;
    if (FAILED(frame->GetSize(&decodedWidth, &decodedHeight)) || decodedWidth == 0U || decodedHeight == 0U) {
        error = "The saved image dimensions are invalid.";
        return false;
    }
    const std::uint64_t pixelCount = static_cast<std::uint64_t>(decodedWidth) * decodedHeight;
    if (pixelCount > 100000000ULL ||
        pixelCount > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t))) {
        error = "The saved image dimensions exceed the safe decode limit.";
        return false;
    }
    ComPtr<IWICFormatConverter> converter;
    if (FAILED(factory->CreateFormatConverter(converter.GetAddressOf())) ||
        FAILED(converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppBGRA,
                                     WICBitmapDitherTypeNone, nullptr, 0.0,
                                     WICBitmapPaletteTypeCustom))) {
        error = "The saved image pixel format could not be converted.";
        return false;
    }
    if (decodedWidth > std::numeric_limits<UINT>::max() / 4U) {
        error = "The saved image scanline is too wide.";
        return false;
    }
    const UINT stride = decodedWidth * 4U;
    const std::uint64_t byteCount = static_cast<std::uint64_t>(stride) * decodedHeight;
    if (byteCount > std::numeric_limits<UINT>::max()) {
        error = "The saved image is too large for the decoder buffer.";
        return false;
    }
    bgraPixels.resize(static_cast<std::size_t>(pixelCount));
    if (FAILED(converter->CopyPixels(nullptr, stride, static_cast<UINT>(byteCount),
                                     reinterpret_cast<BYTE*>(bgraPixels.data())))) {
        bgraPixels.clear();
        error = "The saved image pixels could not be decoded.";
        return false;
    }
    width = static_cast<int>(decodedWidth);
    height = static_cast<int>(decodedHeight);
    return true;
}
#endif

} // namespace mw
