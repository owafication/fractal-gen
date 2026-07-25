#include "Infrastructure/Paths.h"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#endif

#include <array>
#include <cstdlib>

namespace mw {

std::filesystem::path Paths::ExecutablePath() {
#ifdef _WIN32
    std::array<wchar_t, 32768> buffer{};
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return std::filesystem::current_path();
    return std::filesystem::path(std::wstring(buffer.data(), length));
#else
    return std::filesystem::current_path();
#endif
}

std::filesystem::path Paths::ExecutableDirectory() {
    const auto path = ExecutablePath();
    return path.has_parent_path() ? path.parent_path() : std::filesystem::current_path();
}

std::filesystem::path Paths::AppDataDirectory() {
#ifdef _WIN32
    PWSTR rawPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &rawPath)) && rawPath) {
        std::filesystem::path path(rawPath);
        CoTaskMemFree(rawPath);
        return path / L"MandelbrotLiveWallpaper";
    }
    wchar_t* local = nullptr;
    std::size_t length = 0;
    if (_wdupenv_s(&local, &length, L"LOCALAPPDATA") == 0 && local != nullptr) {
        const std::filesystem::path path(local);
        std::free(local);
        return path / L"MandelbrotLiveWallpaper";
    }
    std::free(local);
#endif
    return std::filesystem::temp_directory_path() / "MandelbrotLiveWallpaper";
}

std::filesystem::path Paths::SettingsPath() { return AppDataDirectory() / "settings.json"; }
std::filesystem::path Paths::LogDirectory() { return AppDataDirectory() / "logs"; }
std::filesystem::path Paths::StaticRenderDirectory() { return AppDataDirectory() / "static-renders"; }

} // namespace mw
