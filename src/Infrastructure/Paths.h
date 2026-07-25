#pragma once

#include <filesystem>

namespace mw {

class Paths {
public:
    static std::filesystem::path ExecutablePath();
    static std::filesystem::path ExecutableDirectory();
    static std::filesystem::path AppDataDirectory();
    static std::filesystem::path SettingsPath();
    static std::filesystem::path LogDirectory();
    static std::filesystem::path StaticRenderDirectory();
};

} // namespace mw
