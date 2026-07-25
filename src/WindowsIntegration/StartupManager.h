#pragma once

#include <filesystem>
#include <string>

namespace mw {

class StartupManager {
public:
    static bool IsEnabled();
    static bool SetEnabled(bool enabled, const std::filesystem::path& executablePath, std::string& error);
};

} // namespace mw
