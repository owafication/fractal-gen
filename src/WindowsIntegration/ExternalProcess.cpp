#include "WindowsIntegration/ExternalProcess.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <string>
#include <thread>
#include <utility>

namespace mw {
#ifdef _WIN32
namespace {

constexpr std::size_t kMaximumCapturedBytes = 1024U * 1024U;

class UniqueHandle {
public:
    UniqueHandle() = default;
    explicit UniqueHandle(HANDLE value) noexcept : value_(value) {}
    ~UniqueHandle() { Reset(); }
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;
    UniqueHandle(UniqueHandle&& other) noexcept : value_(other.Release()) {}
    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) Reset(other.Release());
        return *this;
    }
    [[nodiscard]] HANDLE Get() const noexcept { return value_; }
    [[nodiscard]] HANDLE Release() noexcept {
        HANDLE value = value_;
        value_ = nullptr;
        return value;
    }
    void Reset(HANDLE value = nullptr) noexcept {
        if (value_ && value_ != INVALID_HANDLE_VALUE) CloseHandle(value_);
        value_ = value;
    }
    explicit operator bool() const noexcept {
        return value_ && value_ != INVALID_HANDLE_VALUE;
    }

private:
    HANDLE value_{nullptr};
};

std::string WindowsError(DWORD value) {
    wchar_t* message = nullptr;
    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS;
    const DWORD count = FormatMessageW(flags, nullptr, value, 0,
                                       reinterpret_cast<wchar_t*>(&message), 0, nullptr);
    std::string result = "Windows error " + std::to_string(value);
    if (count > 0U && message) {
        const int utf8Count = WideCharToMultiByte(CP_UTF8, 0, message,
                                                   static_cast<int>(count), nullptr, 0,
                                                   nullptr, nullptr);
        if (utf8Count > 0) {
            result.resize(static_cast<std::size_t>(utf8Count));
            WideCharToMultiByte(CP_UTF8, 0, message, static_cast<int>(count),
                                result.data(), utf8Count, nullptr, nullptr);
            while (!result.empty() &&
                   (result.back() == '\r' || result.back() == '\n' || result.back() == ' ')) {
                result.pop_back();
            }
        }
    }
    if (message) LocalFree(message);
    return result;
}

bool CreateOutputPipe(UniqueHandle& readHandle,
                      UniqueHandle& writeHandle,
                      std::string& error) {
    SECURITY_ATTRIBUTES attributes{};
    attributes.nLength = sizeof(attributes);
    attributes.bInheritHandle = TRUE;
    HANDLE readValue = nullptr;
    HANDLE writeValue = nullptr;
    if (!CreatePipe(&readValue, &writeValue, &attributes, 0U)) {
        error = "A process output pipe could not be created: " + WindowsError(GetLastError());
        return false;
    }
    readHandle.Reset(readValue);
    writeHandle.Reset(writeValue);
    if (!SetHandleInformation(readHandle.Get(), HANDLE_FLAG_INHERIT, 0U)) {
        error = "A process output pipe could not be isolated: " + WindowsError(GetLastError());
        return false;
    }
    return true;
}

void AppendBounded(std::string& destination, const char* data, std::size_t size) {
    if (destination.size() >= kMaximumCapturedBytes || size == 0U) return;
    const std::size_t count = std::min(size, kMaximumCapturedBytes - destination.size());
    destination.append(data, count);
}

void ReadPipe(HANDLE pipe,
              std::string& output,
              const std::function<void(std::string_view)>& chunkCallback) {
    std::array<char, 8192U> buffer{};
    for (;;) {
        DWORD count = 0U;
        const BOOL read = ReadFile(pipe, buffer.data(), static_cast<DWORD>(buffer.size()),
                                   &count, nullptr);
        if (!read || count == 0U) break;
        AppendBounded(output, buffer.data(), static_cast<std::size_t>(count));
        if (chunkCallback) {
            try {
                chunkCallback(std::string_view(buffer.data(), static_cast<std::size_t>(count)));
            } catch (...) {
                // External progress callbacks may not break pipe drainage.
            }
        }
    }
}

} // namespace

bool RunOwnedProcess(const std::filesystem::path& executable,
                     const std::vector<std::wstring>& arguments,
                     const std::function<bool()>& cancellationCallback,
                     const std::function<void(std::string_view)>& stdoutChunkCallback,
                     ExternalProcessResult& result,
                     std::string& error,
                     std::uint32_t timeoutMilliseconds) {
    result = {};
    error.clear();
    std::error_code fileError;
    if (!std::filesystem::is_regular_file(executable, fileError) || fileError) {
        error = "The external executable does not exist: " + executable.string();
        return false;
    }

    UniqueHandle stdoutRead;
    UniqueHandle stdoutWrite;
    UniqueHandle stderrRead;
    UniqueHandle stderrWrite;
    if (!CreateOutputPipe(stdoutRead, stdoutWrite, error) ||
        !CreateOutputPipe(stderrRead, stderrWrite, error)) {
        return false;
    }
    UniqueHandle nullInput(CreateFileW(L"NUL", GENERIC_READ,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (!nullInput) {
        error = "The null input device could not be opened: " + WindowsError(GetLastError());
        return false;
    }
    if (!SetHandleInformation(nullInput.Get(), HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT)) {
        error = "The null input device could not be inherited: " + WindowsError(GetLastError());
        return false;
    }

    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESTDHANDLES;
    startup.hStdInput = nullInput.Get();
    startup.hStdOutput = stdoutWrite.Get();
    startup.hStdError = stderrWrite.Get();

    std::vector<std::wstring> fullArguments;
    fullArguments.reserve(arguments.size() + 1U);
    fullArguments.push_back(executable.wstring());
    fullArguments.insert(fullArguments.end(), arguments.begin(), arguments.end());
    std::wstring commandLine = BuildWindowsCommandLine(fullArguments);
    commandLine.push_back(L'\0');

    PROCESS_INFORMATION processInfo{};
    if (!CreateProcessW(executable.c_str(), commandLine.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &processInfo)) {
        error = "The external process could not be started: " + WindowsError(GetLastError());
        return false;
    }
    UniqueHandle process(processInfo.hProcess);
    UniqueHandle thread(processInfo.hThread);
    stdoutWrite.Reset();
    stderrWrite.Reset();
    nullInput.Reset();

    std::thread stdoutReader(ReadPipe, stdoutRead.Get(), std::ref(result.standardOutput),
                             stdoutChunkCallback);
    std::thread stderrReader(ReadPipe, stderrRead.Get(), std::ref(result.standardError),
                             std::function<void(std::string_view)>{});

    const ULONGLONG started = GetTickCount64();
    bool terminated = false;
    for (;;) {
        const DWORD waitResult = WaitForSingleObject(process.Get(), 50U);
        if (waitResult == WAIT_OBJECT_0) break;
        if (waitResult == WAIT_FAILED) {
            error = "Waiting for the external process failed: " + WindowsError(GetLastError());
            TerminateProcess(process.Get(), ERROR_PROCESS_ABORTED);
            terminated = true;
            break;
        }
        const bool cancelled = cancellationCallback && cancellationCallback();
        const bool timedOut = timeoutMilliseconds > 0U &&
            GetTickCount64() - started >= static_cast<ULONGLONG>(timeoutMilliseconds);
        if (cancelled || timedOut) {
            TerminateProcess(process.Get(), cancelled ? ERROR_CANCELLED : WAIT_TIMEOUT);
            WaitForSingleObject(process.Get(), 5000U);
            result.cancelled = cancelled;
            if (timedOut) error = "The external process exceeded its time limit.";
            terminated = true;
            break;
        }
    }

    DWORD exitCode = static_cast<DWORD>(-1);
    if (!GetExitCodeProcess(process.Get(), &exitCode)) {
        if (error.empty()) {
            error = "The external process exit code could not be read: " +
                    WindowsError(GetLastError());
        }
    }
    result.exitCode = exitCode <= static_cast<DWORD>(std::numeric_limits<int>::max())
        ? static_cast<int>(exitCode) : -1;
    process.Reset();
    if (stdoutReader.joinable()) stdoutReader.join();
    if (stderrReader.joinable()) stderrReader.join();
    stdoutRead.Reset();
    stderrRead.Reset();

    if (!error.empty() && !result.cancelled) return false;
    if (terminated && !result.cancelled) return false;
    return true;
}

#endif
} // namespace mw
