#pragma once

#include "Core/Models.h"

#include <filesystem>
#include <optional>
#include <string>

namespace mw {

struct LoadSettingsResult {
    AppSettings settings;
    bool usedDefaults{false};
    std::string warning;
};

class SettingsStore {
public:
    explicit SettingsStore(std::filesystem::path settingsPath);

    [[nodiscard]] const std::filesystem::path& Path() const noexcept { return settingsPath_; }
    LoadSettingsResult Load() const;
    bool Save(const AppSettings& settings, std::string& error) const;
    bool Reset(std::string& error) const;

    static std::string SerialisePreset(const Preset& preset);
    static std::optional<Preset> DeserialisePreset(const std::string& text, std::string& error);
    static std::string SerialiseSettings(const AppSettings& settings);
    static std::optional<AppSettings> DeserialiseSettings(const std::string& text, std::string& error);

private:
    std::filesystem::path settingsPath_;
};

} // namespace mw
