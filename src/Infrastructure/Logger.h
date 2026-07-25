#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>

namespace mw {

enum class LogLevel { Debug, Info, Warning, Error };

class Logger {
public:
    static Logger& Instance();

    void Initialise(const std::filesystem::path& directory);
    void Write(LogLevel level, const std::string& message);
    void Clear();
    [[nodiscard]] std::filesystem::path CurrentLogPath() const;
    [[nodiscard]] std::string DiagnosticSummary() const;

private:
    Logger() = default;
    void RotateLocked();
    static std::string LevelName(LogLevel level);

    mutable std::mutex mutex_;
    std::filesystem::path directory_;
    std::filesystem::path currentPath_;
    std::ofstream stream_;
};

inline void LogDebug(const std::string& message) { Logger::Instance().Write(LogLevel::Debug, message); }
inline void LogInfo(const std::string& message) { Logger::Instance().Write(LogLevel::Info, message); }
inline void LogWarning(const std::string& message) { Logger::Instance().Write(LogLevel::Warning, message); }
inline void LogError(const std::string& message) { Logger::Instance().Write(LogLevel::Error, message); }

} // namespace mw
