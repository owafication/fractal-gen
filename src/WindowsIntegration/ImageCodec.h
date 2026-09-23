#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

namespace mw {

#ifdef _WIN32
class WicRowEncoder {
public:
    WicRowEncoder();
    ~WicRowEncoder();
    WicRowEncoder(const WicRowEncoder&) = delete;
    WicRowEncoder& operator=(const WicRowEncoder&) = delete;

    bool Initialise(const std::filesystem::path& path,
                    SavedImageFormat format,
                    int compressionQuality,
                    std::uint32_t width,
                    std::uint32_t height,
                    std::uint32_t dpi,
                    std::string& error);
    bool WriteRow(std::span<const std::uint32_t> pixels, std::string& error);
    bool Commit(std::string& error);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

bool ValidateImageDimensionsWithWic(const std::filesystem::path& path,
                                    std::uint32_t expectedWidth,
                                    std::uint32_t expectedHeight,
                                    std::string& error);

[[nodiscard]] const wchar_t* SavedImageExtension(SavedImageFormat format) noexcept;
[[nodiscard]] const wchar_t* SavedImageDisplayName(SavedImageFormat format) noexcept;

bool SavePixelsWithWic(const std::filesystem::path& path,
                       SavedImageFormat format,
                       int compressionQuality,
                       std::span<const std::uint32_t> bgraPixels,
                       std::uint32_t width,
                       std::uint32_t height,
                       double dpi,
                       std::string& error);

bool LoadPixelsWithWic(const std::filesystem::path& path,
                       std::vector<std::uint32_t>& bgraPixels,
                       int& width,
                       int& height,
                       std::string& error);
#endif

} // namespace mw
