#include "Infrastructure/Logger.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <chrono>
#include <iomanip>
#include <sstream>
#include <vector>

namespace mw {
namespace {
constexpr std::uintmax_t kMaximumLogBytes = 2 * 1024 * 1024;
constexpr std::size_t kMaximumLogFiles = 5;
}

Logger& Logger::Instance() {
    static Logger logger;
    return logger;
}

void Logger::Initialise(const std::filesystem::path& directory) {
    std::lock_guard lock(mutex_);
    directory_ = directory;
    std::error_code ec;
    std::filesystem::create_directories(directory_, ec);
    currentPath_ = directory_ / "mandelbrot-wallpaper.log";
    RotateLocked();
    stream_.open(currentPath_, std::ios::app | std::ios::binary);
}

void Logger::Write(LogLevel level, const std::string& message) {
    std::lock_guard lock(mutex_);
    if (directory_.empty()) return;
    if (!stream_.is_open()) stream_.open(currentPath_, std::ios::app | std::ios::binary);
    const auto now = std::chrono::system_clock::now();
    const std::time_t timestamp = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &timestamp);
#else
    localtime_r(&timestamp, &local);
#endif
    stream_ << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << " [" << LevelName(level) << "] " << message << '\n';
    stream_.flush();
    std::error_code ec;
    if (std::filesystem::file_size(currentPath_, ec) > kMaximumLogBytes) {
        stream_.close();
        RotateLocked();
        stream_.open(currentPath_, std::ios::app | std::ios::binary);
    }
}

void Logger::Clear() {
    std::lock_guard lock(mutex_);
    if (stream_.is_open()) stream_.close();
    std::error_code ec;
    if (!directory_.empty()) {
        for (const auto& entry : std::filesystem::directory_iterator(directory_, ec)) {
            if (entry.is_regular_file()) std::filesystem::remove(entry.path(), ec);
        }
    }
    if (!currentPath_.empty()) stream_.open(currentPath_, std::ios::trunc | std::ios::binary);
}

std::filesystem::path Logger::CurrentLogPath() const {
    std::lock_guard lock(mutex_);
    return currentPath_;
}

std::string Logger::DiagnosticSummary() const {
    std::ostringstream stream;
    stream << "Mandelbrot Live Wallpaper diagnostics\n";
#ifdef _WIN32
    OSVERSIONINFOW version{};
    version.dwOSVersionInfoSize = sizeof(version);
#pragma warning(push)
#pragma warning(disable : 4996)
    GetVersionExW(&version);
#pragma warning(pop)
    stream << "Windows version: " << version.dwMajorVersion << '.' << version.dwMinorVersion << " build " << version.dwBuildNumber << '\n';
    stream << "Remote session: " << (GetSystemMetrics(SM_REMOTESESSION) ? "yes" : "no") << '\n';
#endif
    stream << "Log path: " << currentPath_.string() << '\n';
    return stream.str();
}

void Logger::RotateLocked() {
    if (currentPath_.empty()) return;
    std::error_code ec;
    if (!std::filesystem::exists(currentPath_, ec) || std::filesystem::file_size(currentPath_, ec) <= kMaximumLogBytes) return;
    for (std::size_t index = kMaximumLogFiles - 1; index > 0; --index) {
        const auto source = directory_ / ("mandelbrot-wallpaper.log." + std::to_string(index));
        const auto target = directory_ / ("mandelbrot-wallpaper.log." + std::to_string(index + 1));
        if (std::filesystem::exists(source, ec)) std::filesystem::rename(source, target, ec);
    }
    const auto first = directory_ / "mandelbrot-wallpaper.log.1";
    std::filesystem::rename(currentPath_, first, ec);
}

std::string Logger::LevelName(LogLevel level) {
    switch (level) {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "INFO";
}

} // namespace mw
