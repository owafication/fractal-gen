#include "Infrastructure/Paths.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void Check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

bool SetAppDataOverride(const std::filesystem::path& path) {
#ifdef _WIN32
    return _wputenv_s(L"MW_APPDATA_DIR", path.wstring().c_str()) == 0;
#else
    return setenv("MW_APPDATA_DIR", path.string().c_str(), 1) == 0;
#endif
}

bool ClearAppDataOverride() {
#ifdef _WIN32
    return _wputenv_s(L"MW_APPDATA_DIR", L"") == 0;
#else
    return unsetenv("MW_APPDATA_DIR") == 0;
#endif
}

} // namespace

int main() {
    const std::filesystem::path overridePath =
        std::filesystem::temp_directory_path() / "MandelbrotLiveWallpaper-PathOverride-Test";

    Check(SetAppDataOverride(overridePath), "The test should set MW_APPDATA_DIR.");
    Check(mw::Paths::AppDataDirectory() == overridePath,
          "MW_APPDATA_DIR should override the platform application-data directory.");
    Check(mw::Paths::SettingsPath() == overridePath / "settings.json",
          "SettingsPath should remain below the isolated application-data root.");
    Check(mw::Paths::LogDirectory() == overridePath / "logs",
          "LogDirectory should remain below the isolated application-data root.");
    Check(mw::Paths::StaticRenderDirectory() == overridePath / "static-renders",
          "StaticRenderDirectory should remain below the isolated application-data root.");
    Check(ClearAppDataOverride(), "The test should clear MW_APPDATA_DIR.");

    if (failures == 0) {
        std::cout << "All path isolation tests passed.\n";
        return 0;
    }
    std::cerr << failures << " path isolation test(s) failed.\n";
    return 1;
}
