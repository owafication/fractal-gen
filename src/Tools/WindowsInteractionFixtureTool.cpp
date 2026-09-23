#ifdef _WIN32

#include "Core/ExternalVideoExport.h"
#include "Core/SettingsStore.h"

#include <windows.h>
#include <commctrl.h>
#include <psapi.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {

constexpr wchar_t kMainWindowClass[] = L"MandelbrotLiveWallpaperControl";
constexpr wchar_t kPaletteWindowClass[] = L"MandelbrotPaletteEditorDialog";
constexpr wchar_t kEquationWindowClass[] = L"MandelbrotAdvancedEquationEditor";
constexpr wchar_t kSettingsWindowClass[] = L"MandelbrotLiveWallpaperSettingsDialog";
constexpr wchar_t kJourneyWindowClass[] = L"MandelbrotJourneySettingsDialog";
constexpr wchar_t kScoutWindowClass[] = L"MandelbrotFractalScoutDialog";
constexpr wchar_t kTimelineWindowClass[] = L"MandelbrotGeneralAnimationEditorDialog";
constexpr wchar_t kFrameExportWindowClass[] = L"MandelbrotFrameSequenceExportDialog";
constexpr wchar_t kVideoExportWindowClass[] = L"MandelbrotExternalVideoExportDialog";
constexpr wchar_t kCommonDialogClass[] = L"#32770";
constexpr wchar_t kWallpaperWindowClass[] = L"MandelbrotLiveWallpaperHost";

constexpr int kMainNavigationPreview = 2002;
constexpr int kMainNavigationWallpaper = 2003;
constexpr int kMainNavigationDiagnostics = 2004;
constexpr int kMainNavigationSettings = 2005;
constexpr int kMainOpenPalette = 2006;
constexpr int kMainOpenEquation = 2008;
constexpr int kMainPresetLibrary = 2009;
constexpr int kMainCoordinates = 2010;
constexpr int kMainRotation = 2011;
constexpr int kMainPresetCombo = 2014;
constexpr int kMainDesktopModeCombo = 2018;
constexpr int kMainApplyDesktopMode = 2019;
constexpr int kMainSetVideoWallpaper = 2048;
constexpr int kMainOpenJourney = 2021;
constexpr int kMainOpenTimeline = 2022;
constexpr int kMainUndo = 2054;
constexpr int kMainRedo = 2055;
constexpr int kMainStopWallpaper = 2053;
constexpr int kMainPauseWallpaper = 2052;
constexpr int kMainOpenSettings = 2060;
constexpr int kMainImportPreset = 2063;
constexpr int kMainPrecisionLabel = 2075;
constexpr int kMainOpenScout = 2080;
constexpr int kMainOpenFrameExport = 2082;
constexpr int kMainOpenVideoExport = 2083;
constexpr int kPaletteFrequency = 6015;
constexpr int kPaletteOk = 6023;
constexpr int kEquationPower = 7009;
constexpr int kEquationOk = 7059;
constexpr int kEquationCancel = 7060;
constexpr int kSettingsRotation = 5002;
constexpr int kSettingsOk = 5033;
constexpr int kSettingsCancel = 5034;
constexpr int kJourneyWaypoints = 9701;
constexpr int kJourneyOk = 9703;
constexpr int kJourneyCancel = 9704;
constexpr int kScoutResultList = 9606;
constexpr int kScoutUse = 9610;
constexpr int kScoutClose = 9612;
constexpr int kTimelineTrackList = 9801;
constexpr int kTimelineTarget = 9802;
constexpr int kTimelineAddTrack = 9803;
constexpr int kTimelineKeyframeList = 9806;
constexpr int kTimelineAddCurrent = 9809;
constexpr int kTimelineDuration = 9811;
constexpr int kTimelineScrubber = 9813;
constexpr int kTimelineTimeLabel = 9814;
constexpr int kTimelinePlayPause = 9816;
constexpr int kTimelineStop = 9817;
constexpr int kTimelineJourneyToTracks = 9818;
constexpr int kTimelineTracksToJourney = 9819;
constexpr int kTimelineOk = 9821;
constexpr int kTimelineCancel = 9822;
constexpr int kFrameExportOutput = 9901;
constexpr int kFrameExportWidth = 9903;
constexpr int kFrameExportHeight = 9904;
constexpr int kFrameExportDpi = 9905;
constexpr int kFrameExportRate = 9906;
constexpr int kFrameExportIncludeEnd = 9908;
constexpr int kFrameExportPrefix = 9909;
constexpr int kFrameExportSummary = 9911;
constexpr int kFrameExportStatus = 9913;
constexpr int kFrameExportStart = 9914;
constexpr int kFrameExportCancel = 9915;
constexpr int kFrameExportOpenFolder = 9916;
constexpr int kFrameExportClose = 9917;
constexpr int kVideoExportSequence = 10101;
constexpr int kVideoExportFfmpeg = 10103;
constexpr int kVideoExportOutput = 10106;
constexpr int kVideoExportPreset = 10108;
constexpr int kVideoExportCrf = 10109;
constexpr int kVideoExportSummary = 10111;
constexpr int kVideoExportStatus = 10113;
constexpr int kVideoExportStart = 10114;
constexpr int kVideoExportCancel = 10115;
constexpr int kVideoExportOpenOutput = 10116;
constexpr int kVideoExportClose = 10117;
constexpr int kExitCommand = 41008;
constexpr UINT kVideoPlaybackFailedMessage = WM_APP + 41U;

constexpr auto kWindowTimeout = std::chrono::seconds(20);
constexpr auto kRenderTimeout = std::chrono::seconds(15);

struct ProcessHandles {
    HANDLE process{nullptr};
    HANDLE thread{nullptr};
    DWORD id{0};

    ~ProcessHandles() {
        if (thread) CloseHandle(thread);
        if (process) CloseHandle(process);
    }
};

struct WindowSearch {
    DWORD processId{0};
    std::wstring_view className;
    HWND result{nullptr};
};

BOOL CALLBACK FindWindowCallback(HWND window, LPARAM parameter) {
    auto& search = *reinterpret_cast<WindowSearch*>(parameter);
    DWORD owner = 0;
    GetWindowThreadProcessId(window, &owner);
    if (owner != search.processId || !IsWindowVisible(window)) return TRUE;

    wchar_t className[256]{};
    if (GetClassNameW(window, className, static_cast<int>(std::size(className))) <= 0) {
        return TRUE;
    }
    if (search.className == className) {
        search.result = window;
        return FALSE;
    }
    return TRUE;
}

HWND FindProcessWindow(DWORD processId, std::wstring_view className) {
    WindowSearch search{processId, className, nullptr};
    EnumWindows(FindWindowCallback, reinterpret_cast<LPARAM>(&search));
    return search.result;
}

HWND FindProcessWindowInDesktopTree(DWORD processId, std::wstring_view className) {
    if (HWND topLevel = FindProcessWindow(processId, className)) return topLevel;
    WindowSearch search{processId, className, nullptr};
    EnumChildWindows(GetDesktopWindow(), FindWindowCallback,
                     reinterpret_cast<LPARAM>(&search));
    return search.result;
}

struct EditableChildSearch {
    HWND dialog{nullptr};
    HWND preferred{nullptr};
    HWND fallback{nullptr};
    std::vector<HWND> candidates;
};

BOOL CALLBACK FindEditableChildCallback(HWND child, LPARAM parameter) {
    auto& search = *reinterpret_cast<EditableChildSearch*>(parameter);
    wchar_t className[32]{};
    if (GetClassNameW(child, className, static_cast<int>(std::size(className))) <= 0 ||
        std::wstring_view(className) != L"Edit" ||
        (GetWindowLongPtrW(child, GWL_STYLE) & ES_READONLY) != 0 ||
        !IsWindowEnabled(child)) {
        return TRUE;
    }
    if (!search.fallback) search.fallback = child;
    search.candidates.push_back(child);
    if (GetDlgCtrlID(child) == 0x0480) {
        if (!search.preferred) search.preferred = child;
        return TRUE;
    }
    for (HWND ancestor = GetParent(child);
         ancestor && ancestor != search.dialog;
         ancestor = GetParent(ancestor)) {
        if (GetDlgCtrlID(ancestor) == 0x047c) {
            if (!search.preferred) search.preferred = child;
            return TRUE;
        }
    }
    return TRUE;
}

HWND FindCommonDialogFileNameEdit(HWND dialog, std::vector<HWND>* candidates = nullptr) {
    EditableChildSearch search{dialog};
    EnumChildWindows(dialog, FindEditableChildCallback,
                     reinterpret_cast<LPARAM>(&search));
    if (candidates) *candidates = search.candidates;
    return search.preferred ? search.preferred : search.fallback;
}

bool ProcessExited(HANDLE process) {
    return WaitForSingleObject(process, 0) == WAIT_OBJECT_0;
}

template <typename Predicate>
bool WaitUntil(std::chrono::steady_clock::duration timeout, Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    do {
        if (predicate()) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (std::chrono::steady_clock::now() < deadline);
    return predicate();
}

HWND WaitForProcessWindow(const ProcessHandles& process, std::wstring_view className) {
    HWND result = nullptr;
    const bool found = WaitUntil(kWindowTimeout, [&] {
        if (ProcessExited(process.process)) return true;
        result = FindProcessWindow(process.id, className);
        return result != nullptr;
    });
    return found && !ProcessExited(process.process) ? result : nullptr;
}

bool SendMessageBounded(HWND window, UINT message, WPARAM wParam, LPARAM lParam,
                        LRESULT* result = nullptr) {
    DWORD_PTR value = 0;
    if (!SendMessageTimeoutW(window, message, wParam, lParam,
                             SMTO_ABORTIFHUNG | SMTO_BLOCK, 5000, &value)) {
        return false;
    }
    if (result) *result = static_cast<LRESULT>(value);
    return true;
}

std::optional<std::wstring> ReadWindowTextBounded(HWND window) {
    LRESULT length = 0;
    if (!SendMessageBounded(window, WM_GETTEXTLENGTH, 0, 0, &length) || length < 0 ||
        length > 65535) {
        return std::nullopt;
    }
    std::vector<wchar_t> buffer(static_cast<std::size_t>(length) + 1U, L'\0');
    LRESULT copied = 0;
    if (!SendMessageBounded(window, WM_GETTEXT, buffer.size(),
                            reinterpret_cast<LPARAM>(buffer.data()), &copied) || copied < 0) {
        return std::nullopt;
    }
    return std::wstring(buffer.data(), static_cast<std::size_t>(copied));
}

struct WindowTextSearch {
    std::wstring_view needle;
    bool found{false};
};

BOOL CALLBACK FindWindowTextCallback(HWND child, LPARAM parameter) {
    auto& search = *reinterpret_cast<WindowTextSearch*>(parameter);
    const auto text = ReadWindowTextBounded(child);
    if (text && text->find(search.needle) != std::wstring::npos) {
        search.found = true;
        return FALSE;
    }
    return TRUE;
}

bool WindowTreeContainsText(HWND window, std::wstring_view needle) {
    const auto title = ReadWindowTextBounded(window);
    if (title && title->find(needle) != std::wstring::npos) return true;
    WindowTextSearch search{needle};
    EnumChildWindows(window, FindWindowTextCallback,
                      reinterpret_cast<LPARAM>(&search));
    return search.found;
}

bool SetWindowTextBounded(HWND window, std::wstring_view text) {
    std::wstring owned(text);
    return SendMessageBounded(window, WM_SETTEXT, 0,
                              reinterpret_cast<LPARAM>(owned.c_str()));
}

bool SendCommand(HWND window, int id, int notification = BN_CLICKED, HWND control = nullptr) {
    return SendMessageBounded(
        window, WM_COMMAND, MAKEWPARAM(id, notification), reinterpret_cast<LPARAM>(control));
}

bool PostCommand(HWND window, int id) {
    return PostMessageW(window, WM_COMMAND, MAKEWPARAM(id, BN_CLICKED), 0) != FALSE;
}

std::optional<std::wstring> ControlText(HWND parent, int id) {
    const HWND control = GetDlgItem(parent, id);
    if (!control) return std::nullopt;
    return ReadWindowTextBounded(control);
}

std::optional<std::vector<std::wstring>> ComboItems(HWND parent, int id) {
    const HWND control = GetDlgItem(parent, id);
    LRESULT count = 0;
    if (!control || !SendMessageBounded(control, CB_GETCOUNT, 0, 0, &count) || count < 0 || count > 64) {
        return std::nullopt;
    }
    std::vector<std::wstring> items;
    items.reserve(static_cast<std::size_t>(count));
    for (LRESULT index = 0; index < count; ++index) {
        LRESULT length = 0;
        if (!SendMessageBounded(control, CB_GETLBTEXTLEN, static_cast<WPARAM>(index), 0, &length) ||
            length < 0 || length > 256) {
            return std::nullopt;
        }
        std::vector<wchar_t> buffer(static_cast<std::size_t>(length) + 1U, L'\0');
        LRESULT copied = 0;
        if (!SendMessageBounded(control, CB_GETLBTEXT, static_cast<WPARAM>(index),
                                reinterpret_cast<LPARAM>(buffer.data()), &copied) || copied < 0) {
            return std::nullopt;
        }
        items.emplace_back(buffer.data(), static_cast<std::size_t>(copied));
    }
    return items;
}

bool SetControlTextAndNotify(HWND parent, int id, std::wstring_view text, int notification) {
    const HWND control = GetDlgItem(parent, id);
    return control && SetWindowTextBounded(control, text) &&
           SendCommand(parent, id, notification, control);
}

bool SelectComboItemAndNotify(HWND parent, int id, int index) {
    const HWND control = GetDlgItem(parent, id);
    LRESULT selected = CB_ERR;
    return control && SendMessageBounded(control, CB_SETCURSEL,
                                         static_cast<WPARAM>(index), 0, &selected) &&
           selected == index && SendCommand(parent, id, CBN_SELCHANGE, control);
}

bool ReadPngDimensions(const std::filesystem::path& path,
                       std::uint32_t& width, std::uint32_t& height) {
    std::ifstream input(path, std::ios::binary);
    std::array<unsigned char, 24> header{};
    if (!input.read(reinterpret_cast<char*>(header.data()),
                    static_cast<std::streamsize>(header.size()))) {
        return false;
    }
    constexpr std::array<unsigned char, 8> signature{
        0x89U, 0x50U, 0x4eU, 0x47U, 0x0dU, 0x0aU, 0x1aU, 0x0aU,
    };
    if (!std::equal(signature.begin(), signature.end(), header.begin()) ||
        header[12] != 'I' || header[13] != 'H' ||
        header[14] != 'D' || header[15] != 'R') {
        return false;
    }
    const auto bigEndian = [&header](std::size_t offset) {
        return (static_cast<std::uint32_t>(header[offset]) << 24U) |
               (static_cast<std::uint32_t>(header[offset + 1U]) << 16U) |
               (static_cast<std::uint32_t>(header[offset + 2U]) << 8U) |
               static_cast<std::uint32_t>(header[offset + 3U]);
    };
    width = bigEndian(16U);
    height = bigEndian(20U);
    return width > 0U && height > 0U;
}

bool ContainsPartialFile(const std::filesystem::path& directory) {
    std::error_code error;
    if (!std::filesystem::exists(directory, error)) return false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, error)) {
        if (error) return true;
        if (entry.is_regular_file(error) && entry.path().extension() == L".part") return true;
        if (error) return true;
    }
    return false;
}

bool ContainsTemporaryVideo(const std::filesystem::path& directory) {
    std::error_code error;
    if (!std::filesystem::exists(directory, error)) return false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(directory, error)) {
        if (error) return true;
        if (entry.is_regular_file(error) &&
            entry.path().filename().wstring().find(L".part.mp4") != std::wstring::npos) {
            return true;
        }
        if (error) return true;
    }
    return false;
}

bool WriteSolidBmp(const std::filesystem::path& path,
                   unsigned char red, unsigned char green, unsigned char blue) {
    constexpr LONG width = 8;
    constexpr LONG height = 8;
    constexpr DWORD rowBytes = ((width * 3U + 3U) / 4U) * 4U;
    constexpr DWORD pixelBytes = rowBytes * height;
    BITMAPFILEHEADER fileHeader{};
    fileHeader.bfType = 0x4d42U;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    fileHeader.bfSize = fileHeader.bfOffBits + pixelBytes;
    BITMAPINFOHEADER infoHeader{};
    infoHeader.biSize = sizeof(BITMAPINFOHEADER);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 24;
    infoHeader.biCompression = BI_RGB;
    infoHeader.biSizeImage = pixelBytes;
    std::array<unsigned char, rowBytes> row{};
    for (LONG x = 0; x < width; ++x) {
        row[static_cast<std::size_t>(x) * 3U] = blue;
        row[static_cast<std::size_t>(x) * 3U + 1U] = green;
        row[static_cast<std::size_t>(x) * 3U + 2U] = red;
    }
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(&fileHeader), sizeof(fileHeader));
    output.write(reinterpret_cast<const char*>(&infoHeader), sizeof(infoHeader));
    for (LONG y = 0; y < height; ++y) {
        output.write(reinterpret_cast<const char*>(row.data()), row.size());
    }
    return output.good();
}

bool LogContainsText(const std::filesystem::path& appData, std::string_view needle) {
    std::error_code error;
    const auto logDirectory = appData / L"logs";
    if (!std::filesystem::exists(logDirectory, error)) return false;
    for (const auto& entry : std::filesystem::directory_iterator(logDirectory, error)) {
        if (error) return false;
        if (!entry.is_regular_file(error)) continue;
        std::ifstream input(entry.path(), std::ios::binary);
        std::ostringstream text;
        text << input.rdbuf();
        if (text.str().find(needle) != std::string::npos) return true;
    }
    return false;
}

bool SubmitCommonDialogPath(HWND dialog, const std::filesystem::path& path) {
    HWND fileNameEdit = nullptr;
    if (!WaitUntil(kWindowTimeout, [&] {
            fileNameEdit = FindCommonDialogFileNameEdit(dialog);
            return fileNameEdit && GetDlgItem(dialog, IDOK);
        })) {
        return false;
    }
    const HWND fileNameCombo = GetParent(fileNameEdit);
    const HWND openButton = GetDlgItem(dialog, IDOK);
    if (!fileNameCombo || !openButton ||
        !SetWindowTextBounded(fileNameCombo, path.wstring()) ||
        !SetWindowTextBounded(fileNameEdit, path.wstring()) ||
        !SendMessageBounded(dialog, WM_COMMAND,
                            MAKEWPARAM(0x047c, CBN_EDITCHANGE),
                            reinterpret_cast<LPARAM>(fileNameCombo)) ||
        !SendMessageBounded(openButton, BM_CLICK, 0, 0)) {
        return false;
    }
    if (!IsWindow(dialog)) return true;
    if (!WaitUntil(std::chrono::seconds(5), [&] {
            return !IsWindow(dialog) ||
                   (ReadWindowTextBounded(fileNameEdit) == path.filename().wstring());
        })) {
        return false;
    }
    if (!IsWindow(dialog)) return true;
    const HWND currentOpenButton = GetDlgItem(dialog, IDOK);
    return currentOpenButton && SendMessageBounded(currentOpenButton, BM_CLICK, 0, 0);
}

bool HasBoundedAccessibleActionSurface(HWND mainWindow, std::string& error) {
    constexpr std::array<int, 10> actions{
        kMainNavigationPreview, kMainNavigationWallpaper,
        kMainNavigationDiagnostics, kMainNavigationSettings,
        kMainOpenPalette, kMainOpenEquation, kMainDesktopModeCombo,
        kMainApplyDesktopMode, kMainStopWallpaper, kMainOpenSettings,
    };
    for (const int id : actions) {
        const HWND control = GetDlgItem(mainWindow, id);
        if (!control) {
            error = "missing control " + std::to_string(id);
            return false;
        }
        if ((GetWindowLongPtrW(control, GWL_STYLE) & WS_TABSTOP) == 0) {
            error = "control " + std::to_string(id) + " is not keyboard reachable";
            return false;
        }
        wchar_t className[32]{};
        GetClassNameW(control, className, static_cast<int>(std::size(className)));
        if (std::wstring_view(className) == L"Button") {
            const auto name = ReadWindowTextBounded(control);
            if (!name || name->empty()) {
                error = "button " + std::to_string(id) + " has no native accessible name";
                return false;
            }
        }
    }
    return true;
}

struct ResourceSnapshot {
    SIZE_T workingSetBytes{0};
    DWORD handleCount{0};
    DWORD gdiObjects{0};
    DWORD userObjects{0};
};

std::optional<ResourceSnapshot> ReadResourceSnapshot(HANDLE process) {
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    ResourceSnapshot snapshot;
    if (!GetProcessMemoryInfo(process, &counters, sizeof(counters)) ||
        !GetProcessHandleCount(process, &snapshot.handleCount)) {
        return std::nullopt;
    }
    snapshot.workingSetBytes = counters.WorkingSetSize;
    snapshot.gdiObjects = GetGuiResources(process, GR_GDIOBJECTS);
    snapshot.userObjects = GetGuiResources(process, GR_USEROBJECTS);
    return snapshot;
}

std::optional<std::wstring> PreviewPrecision(std::wstring_view label) {
    constexpr std::wstring_view prefix = L"| preview ";
    constexpr std::wstring_view suffix = L" | desktop";
    const auto startMarker = label.find(prefix);
    if (startMarker == std::wstring_view::npos) return std::nullopt;
    const auto start = startMarker + prefix.size();
    const auto end = label.find(suffix, start);
    if (end == std::wstring_view::npos || end <= start) return std::nullopt;
    return std::wstring(label.substr(start, end - start));
}

std::string NarrowAscii(std::wstring_view text) {
    std::string result;
    result.reserve(text.size());
    for (wchar_t value : text) {
        result.push_back(value >= 32 && value <= 126 ? static_cast<char>(value) : '?');
    }
    return result;
}

std::string ControlPath(HWND child, HWND root) {
    std::ostringstream path;
    for (HWND current = child; current && current != root; current = GetParent(current)) {
        wchar_t className[64]{};
        GetClassNameW(current, className, static_cast<int>(std::size(className)));
        if (path.tellp() > 0) path << " <- ";
        path << NarrowAscii(className) << "#" << GetDlgCtrlID(current);
    }
    return path.str();
}

std::wstring QuoteArgument(const std::filesystem::path& path) {
    return L"\"" + path.wstring() + L"\"";
}

void RestoreEnvironment(const std::optional<std::wstring>& previous) {
    SetEnvironmentVariableW(L"MW_APPDATA_DIR", previous ? previous->c_str() : nullptr);
}

std::optional<std::wstring> CurrentEnvironmentValue(const wchar_t* name) {
    const DWORD required = GetEnvironmentVariableW(name, nullptr, 0);
    if (required == 0) return std::nullopt;
    std::wstring value(static_cast<std::size_t>(required), L'\0');
    const DWORD copied = GetEnvironmentVariableW(name, value.data(), required);
    if (copied == 0 || copied >= required) return std::nullopt;
    value.resize(copied);
    return value;
}

bool StartApplication(const std::filesystem::path& executable,
                      const std::filesystem::path& appData,
                      ProcessHandles& process, std::string& error) {
    if (FindWindowW(kMainWindowClass, nullptr)) {
        error = "another Mandelbrot Live Wallpaper main window is already open";
        return false;
    }
    const auto previous = CurrentEnvironmentValue(L"MW_APPDATA_DIR");
    if (!SetEnvironmentVariableW(L"MW_APPDATA_DIR", appData.wstring().c_str())) {
        error = "could not set the process-scoped application-data override";
        return false;
    }

    std::wstring commandLine = QuoteArgument(executable);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION information{};
    const BOOL started = CreateProcessW(
        executable.wstring().c_str(), commandLine.data(), nullptr, nullptr, FALSE, 0, nullptr,
        executable.parent_path().wstring().c_str(), &startup, &information);
    RestoreEnvironment(previous);
    if (!started) {
        error = "CreateProcessW failed with error " + std::to_string(GetLastError());
        return false;
    }
    process.process = information.hProcess;
    process.thread = information.hThread;
    process.id = information.dwProcessId;
    return true;
}

void StopOwnedApplication(const ProcessHandles& process, HWND mainWindow) {
    if (!process.process || ProcessExited(process.process)) return;
    if (mainWindow && IsWindow(mainWindow)) PostCommand(mainWindow, kExitCommand);
    if (WaitForSingleObject(process.process, 5000) != WAIT_OBJECT_0) {
        TerminateProcess(process.process, 2);
        WaitForSingleObject(process.process, 5000);
    }
}

bool LogContainsShutdown(const std::filesystem::path& appData) {
    std::error_code error;
    if (!std::filesystem::exists(appData, error)) return false;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(appData, error)) {
        if (error) return false;
        if (!entry.is_regular_file(error)) continue;
        std::ifstream input(entry.path(), std::ios::binary);
        std::ostringstream text;
        text << input.rdbuf();
        if (text.str().find("Application shutdown.") != std::string::npos) return true;
    }
    return false;
}

class Report {
public:
    void Note(std::string text) {
        notes_.push_back(std::move(text));
    }

    void Fail(std::string text) {
        if (failure_.empty()) failure_ = std::move(text);
    }

    [[nodiscard]] bool Passed() const noexcept { return failure_.empty(); }

    void Write(const std::filesystem::path& path) const {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "# Native Windows interaction fixture\n\n";
        output << "Status: " << (Passed() ? "Passed" : "Failed") << "\n\n";
        for (const auto& note : notes_) output << "- " << note << "\n";
        if (!failure_.empty()) output << "\nFailure: " << failure_ << "\n";
    }

private:
    std::vector<std::string> notes_;
    std::string failure_;
};

bool CheckExclusiveEditorSession(const ProcessHandles& process, HWND mainWindow,
                                 Report& report) {
    struct Case {
        int openCommand;
        std::wstring_view dialogClass;
        int competingCommand;
        std::wstring_view competingClass;
        const char* name;
    };
    const std::array cases{
        Case{kMainOpenPalette, kPaletteWindowClass, kMainOpenEquation,
             kEquationWindowClass, "Palette Editor"},
        Case{kMainOpenEquation, kEquationWindowClass, kMainOpenSettings,
             kSettingsWindowClass, "Equation Editor"},
        Case{kMainOpenSettings, kSettingsWindowClass, kMainOpenPalette,
             kPaletteWindowClass, "Settings"},
    };
    for (const auto& item : cases) {
        if (!PostCommand(mainWindow, item.openCommand)) return false;
        HWND dialog = WaitForProcessWindow(process, item.dialogClass);
        if (!dialog || !WaitUntil(kWindowTimeout, [&] {
                return IsWindow(dialog) && !IsWindowEnabled(mainWindow);
            })) {
            report.Fail(std::string(item.name) + " did not disable the main window for its edit session");
            return false;
        }
        if (!PostCommand(mainWindow, item.competingCommand)) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(750));
        if (FindProcessWindow(process.id, item.competingClass)) {
            report.Fail(std::string(item.name) + " allowed a competing editor to open");
            return false;
        }
        PostMessageW(dialog, WM_CLOSE, 0, 0);
        if (!WaitUntil(kWindowTimeout, [&] {
                return !IsWindow(dialog) && IsWindowEnabled(mainWindow);
            })) {
            report.Fail(std::string(item.name) + " did not restore the main window after close");
            return false;
        }
        report.Note(std::string(item.name) +
                    " exclusively owned the edit session; its competing window route stayed blocked until close.");
    }
    return true;
}

bool CheckExportPresentationPause(const ProcessHandles& process, HWND mainWindow,
                                  const char* mode, int openCommand,
                                  std::wstring_view dialogClass, int closeCommand,
                                  Report& report) {
    const HWND host = FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass);
    const auto coordinates = ControlText(mainWindow, kMainCoordinates);
    const auto undo = ControlText(mainWindow, kMainUndo);
    if (!host || !coordinates || !undo) return false;
    enum class PauseCase { Running, User, Automatic, StillSuspended };
    for (const auto pauseCase : {PauseCase::Running, PauseCase::User,
                                PauseCase::Automatic, PauseCase::StillSuspended}) {
        const bool alreadyPaused = pauseCase == PauseCase::User;
        if (alreadyPaused &&
            (!SendCommand(mainWindow, kMainPauseWallpaper) ||
             !WaitUntil(kWindowTimeout, [&] {
                 return WindowTreeContainsText(mainWindow, L"Status: paused — Paused by user");
             }))) return false;
        if (pauseCase == PauseCase::Automatic &&
            (!SendMessageBounded(mainWindow, WM_POWERBROADCAST, PBT_APMSUSPEND, 0) ||
             !WaitUntil(kWindowTimeout, [&] {
                 return WindowTreeContainsText(mainWindow, L"Status: paused — System is suspended");
             }))) return false;
        if (!PostCommand(mainWindow, openCommand)) return false;
        HWND dialog = WaitForProcessWindow(process, dialogClass);
        bool ok = dialog && WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(dialog, closeCommand) && !IsWindowEnabled(mainWindow) &&
                   WindowTreeContainsText(mainWindow, L"Status: paused") &&
                   FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass) == host;
        });
        if (ok && !alreadyPaused) {
            // Exercise only the owned application's event path, never suspend the host.
            ok = SendMessageBounded(mainWindow, WM_POWERBROADCAST, PBT_APMSUSPEND, 0);
            if (ok) {
                (void)WaitUntil(std::chrono::milliseconds(750), [&] {
                    return ProcessExited(process.process);
                });
                ok = (pauseCase == PauseCase::StillSuspended ||
                      SendMessageBounded(mainWindow, WM_POWERBROADCAST, PBT_APMRESUMEAUTOMATIC, 0)) &&
                     SendCommand(mainWindow, 41003) && // Tray Resume cannot release the export hold.
                     !WaitUntil(std::chrono::seconds(3), [&] {
                         return ProcessExited(process.process) ||
                                !WindowTreeContainsText(mainWindow, L"Status: paused");
                     });
            }
            if (!ok) report.Fail(std::string(mode) +
                " presentation resumed inside a modal export after simulated suspend/resume");
        }
        if (dialog && IsWindow(dialog)) {
            ok = SendMessageBounded(GetDlgItem(dialog, closeCommand), BM_CLICK, 0, 0) && ok;
        }
        ok = WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(dialog) && IsWindowEnabled(mainWindow) &&
                   WindowTreeContainsText(mainWindow, alreadyPaused
                       ? L"Status: paused — Paused by user" :
                       pauseCase == PauseCase::StillSuspended
                           ? L"Status: paused — System is suspended" : L"Status: running") &&
                   FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass) == host &&
                   ControlText(mainWindow, kMainCoordinates) == coordinates &&
                   ControlText(mainWindow, kMainUndo) == undo;
        }) && ok;
        if (!ok) return false;
        const char* result = alreadyPaused
            ? ": a pre-existing user pause survived modal open/close."
            : pauseCase == PauseCase::Automatic
                ? ": a pre-existing automatic pause cleared only after modal close."
                : pauseCase == PauseCase::StillSuspended
                    ? ": modal close preserved the reported suspension; a later resume released it."
                    : ": a running background stayed paused through suspend/resume and Tray Resume, then resumed on modal close.";
        if (alreadyPaused && !SendCommand(mainWindow, kMainPauseWallpaper)) return false;
        if (pauseCase == PauseCase::StillSuspended &&
            !SendMessageBounded(mainWindow, WM_POWERBROADCAST, PBT_APMRESUMEAUTOMATIC, 0)) return false;
        if (!WaitUntil(kWindowTimeout, [&] {
                return WindowTreeContainsText(mainWindow, L"Status: running");
            })) return false;
        report.Note(std::string(mode) + (openCommand == kMainOpenFrameExport ? " / frame export" : " / video export") +
                    result + " Desktop host, camera and undo label were preserved; power events were synthetic and process-scoped.");
    }
    return true;
}

} // namespace

int wmain(int argc, wchar_t** argv) {
    if (argc != 3) {
        std::cerr << "Usage: MandelbrotWindowsInteractionFixture <application.exe> <report-directory>\n";
        return 2;
    }

    const std::filesystem::path executable = std::filesystem::absolute(argv[1]);
    const std::filesystem::path reportDirectory = std::filesystem::absolute(argv[2]);
    const std::filesystem::path appData = reportDirectory /
        (L"appdata-" + std::to_wstring(GetCurrentProcessId()) + L"-" +
         std::to_wstring(GetTickCount64()));
    std::error_code filesystemError;
    std::optional<std::filesystem::path> optInFfmpeg;
    std::optional<std::filesystem::path> optInVideoCancelSequence;
    std::optional<std::filesystem::path> optInVideoWallpaper;
    unsigned long soakSeconds = 0;
    const bool editorExclusivityOnly = GetEnvironmentVariableW(
        L"MW_TEST_EDITOR_EXCLUSIVITY_ONLY", nullptr, 0) > 0;
    std::array<wchar_t, 32768U> ffmpegPathBuffer{};
    const DWORD ffmpegPathLength = GetEnvironmentVariableW(
        L"MW_TEST_FFMPEG_PATH", ffmpegPathBuffer.data(),
        static_cast<DWORD>(ffmpegPathBuffer.size()));
    if (ffmpegPathLength > 0U && ffmpegPathLength < ffmpegPathBuffer.size()) {
        optInFfmpeg = std::filesystem::path(ffmpegPathBuffer.data());
    }
    std::array<wchar_t, 32768U> cancelSequenceBuffer{};
    const DWORD cancelSequenceLength = GetEnvironmentVariableW(
        L"MW_TEST_VIDEO_CANCEL_SEQUENCE", cancelSequenceBuffer.data(),
        static_cast<DWORD>(cancelSequenceBuffer.size()));
    if (cancelSequenceLength > 0U && cancelSequenceLength < cancelSequenceBuffer.size()) {
        optInVideoCancelSequence = std::filesystem::path(cancelSequenceBuffer.data());
    }
    std::array<wchar_t, 32768U> videoWallpaperBuffer{};
    const DWORD videoWallpaperLength = GetEnvironmentVariableW(
        L"MW_TEST_VIDEO_WALLPAPER_PATH", videoWallpaperBuffer.data(),
        static_cast<DWORD>(videoWallpaperBuffer.size()));
    if (videoWallpaperLength > 0U && videoWallpaperLength < videoWallpaperBuffer.size()) {
        optInVideoWallpaper = std::filesystem::path(videoWallpaperBuffer.data());
    }
    std::array<wchar_t, 32U> soakSecondsBuffer{};
    const DWORD soakSecondsLength = GetEnvironmentVariableW(
        L"MW_TEST_SOAK_SECONDS", soakSecondsBuffer.data(),
        static_cast<DWORD>(soakSecondsBuffer.size()));
    if (soakSecondsLength > 0U && soakSecondsLength < soakSecondsBuffer.size()) {
        wchar_t* end = nullptr;
        soakSeconds = std::wcstoul(soakSecondsBuffer.data(), &end, 10);
        if (!end || *end != L'\0' || soakSeconds > 3600UL) {
            std::cerr << "MW_TEST_SOAK_SECONDS must be an integer from 0 to 3600.\n";
            return 2;
        }
    }
    std::filesystem::create_directories(appData, filesystemError);
    if (filesystemError || !std::filesystem::is_regular_file(executable) ||
        (optInFfmpeg && !std::filesystem::is_regular_file(*optInFfmpeg)) ||
        (optInVideoWallpaper && !std::filesystem::is_regular_file(*optInVideoWallpaper)) ||
        (optInVideoCancelSequence &&
         !std::filesystem::is_directory(*optInVideoCancelSequence))) {
        std::cerr << "Fixture preflight failed.\n";
        return 2;
    }
    const std::filesystem::path firstImagePath = appData / L"fixture-red.bmp";
    const std::filesystem::path secondImagePath = appData / L"fixture-blue.bmp";
    const std::filesystem::path invalidVideoPath = appData / L"fixture-invalid.mp4";
    if (!WriteSolidBmp(firstImagePath, 255U, 0U, 0U) ||
        !WriteSolidBmp(secondImagePath, 0U, 0U, 255U)) {
        std::cerr << "Fixture static-image setup failed.\n";
        return 2;
    }
    {
        std::ofstream invalidVideo(invalidVideoPath, std::ios::binary | std::ios::trunc);
        invalidVideo << "not an mp4";
        if (!invalidVideo) {
            std::cerr << "Fixture invalid-video setup failed.\n";
            return 2;
        }
    }
    {
        mw::AppSettings fixtureSettings;
        fixtureSettings.staticWallpaper.imagePaths = {
            firstImagePath.string(), secondImagePath.string()};
        fixtureSettings.staticWallpaper.currentIndex = 0;
        fixtureSettings.staticWallpaper.cycleSeconds = 10;
        fixtureSettings.staticWallpaper.order = mw::StaticSlideshowOrder::Sequential;
        // Foreground/desktop occlusion is external to these deterministic media checks.
        fixtureSettings.performance.pauseWhenFullscreen = false;
        fixtureSettings.performance.pauseWhenDesktopHidden = false;
        fixtureSettings.performance.adaptive.enabled = false;
        fixtureSettings.videoWallpaper.filePath = invalidVideoPath.string();
        mw::SettingsStore fixtureStore(appData / L"settings.json");
        std::string settingsError;
        if (!fixtureStore.Save(fixtureSettings, settingsError)) {
            std::cerr << "Fixture desktop settings setup failed: " << settingsError << "\n";
            return 2;
        }
    }
    const std::filesystem::path importPresetPath = appData / L"fixture-import.json";
    {
        mw::Preset fixturePreset;
        fixturePreset.id = "fixture-import";
        fixturePreset.name = "Fixture Imported";
        fixturePreset.builtIn = false;
        fixturePreset.camera.centreX = -0.25;
        fixturePreset.camera.centreY = 0.125;
        fixturePreset.camera.scale = 0.75;
        fixturePreset.startingScale = fixturePreset.camera.scale;
        fixturePreset.rotationDegrees = 23.5;
        fixturePreset.palette = mw::Palette::Fire;
        fixturePreset.automaticJourneyWaypoints =
            "-0.1,0.65,0.1,3,4\n-0.75,0,0.5,1,2";
        fixturePreset.animationMode = mw::AnimationMode::ManualView;
        const std::string importText =
            mw::SettingsStore::SerialisePreset(fixturePreset);
        std::string importError;
        const auto checkedImport =
            mw::SettingsStore::DeserialisePreset(importText, importError);
        if (!checkedImport || checkedImport->name != fixturePreset.name ||
            checkedImport->camera.centreX != fixturePreset.camera.centreX ||
            checkedImport->camera.centreY != fixturePreset.camera.centreY ||
            checkedImport->camera.scale != fixturePreset.camera.scale) {
            std::cerr << "Fixture import-preset validation failed: "
                      << importError << "\n";
            return 2;
        }
        std::ofstream importPreset(importPresetPath, std::ios::binary | std::ios::trunc);
        importPreset << importText;
        if (!importPreset) {
            std::cerr << "Fixture import-preset setup failed.\n";
            return 2;
        }
    }

    Report report;
    report.Note("Application: " + executable.string());
    report.Note("Application data: " + appData.string());
    report.Note(optInVideoWallpaper
        ? "The fixture launches and terminates only its own process; file-backed desktop tests include fixture images and the explicitly supplied MP4."
        : "The fixture launches and terminates only its own process; file-backed desktop tests use fixture-owned images and invalid media only.");
    report.Note(optInFfmpeg
        ? "An explicitly supplied external encoder is available for the opt-in native Video Export success route."
        : "The opt-in native Video Export success route is skipped because MW_TEST_FFMPEG_PATH is not set.");
    report.Note(optInFfmpeg && optInVideoCancelSequence
        ? "An explicitly supplied verified sequence is available for the opt-in native Video Export cancellation route."
        : "The opt-in native Video Export cancellation route is skipped unless both encoder and sequence environment inputs are set.");
    report.Note(optInVideoWallpaper
        ? "An explicitly supplied verified MP4 is available for the opt-in file-backed desktop playback route."
        : "The opt-in file-backed desktop playback route is skipped because MW_TEST_VIDEO_WALLPAPER_PATH is not set.");
    report.Note(soakSeconds > 0
        ? "A bounded resource soak was requested for " + std::to_string(soakSeconds) + " seconds."
        : "The bounded resource soak is skipped because MW_TEST_SOAK_SECONDS is not set.");

    ProcessHandles process;
    HWND mainWindow = nullptr;
    std::string startupError;
    if (!StartApplication(executable, appData, process, startupError)) {
        report.Fail(startupError);
        report.Write(reportDirectory / L"report.md");
        std::cerr << startupError << '\n';
        return 1;
    }

    const auto fail = [&](std::string message) {
        report.Fail(std::move(message));
    };

    mainWindow = WaitForProcessWindow(process, kMainWindowClass);
    if (!mainWindow) {
        fail("the fixture-owned process did not expose its main window");
    }

    if (editorExclusivityOnly && report.Passed()) {
        if (!CheckExclusiveEditorSession(process, mainWindow, report)) {
            fail("the exclusive editor-session check failed");
        }
        StopOwnedApplication(process, mainWindow);
        if (!ProcessExited(process.process)) fail("the editor-only fixture process did not terminate");
        else if (!LogContainsShutdown(appData)) fail("the editor-only fixture did not record a clean shutdown");
        report.Note("Editor-only mode skipped desktop-host/media checks; all tested windows were process-scoped.");
        report.Write(reportDirectory / L"report.md");
        if (!report.Passed()) {
            std::cerr << "Windows interaction fixture failed; see "
                      << (reportDirectory / L"report.md").string() << '\n';
            return 1;
        }
        return 0;
    }

    if (report.Passed()) {
        const std::vector<std::wstring> expected{L"None", L"Static image", L"Slide show", L"Video file"};
        std::optional<std::vector<std::wstring>> modes;
        const bool ready = WaitUntil(kWindowTimeout, [&] {
            modes = ComboItems(mainWindow, kMainDesktopModeCombo);
            return modes && *modes == expected;
        });
        if (!ready) {
            std::string observed = modes ? std::string{} : std::string("<unavailable>");
            if (modes) {
                for (const auto& mode : *modes) {
                    if (!observed.empty()) observed += " | ";
                    observed += NarrowAscii(mode);
                }
            } else {
                observed += "; immediate children:";
                for (HWND child = GetWindow(mainWindow, GW_CHILD); child; child = GetWindow(child, GW_HWNDNEXT)) {
                    observed += " " + ControlPath(child, mainWindow);
                }
            }
            fail("the desktop-mode list did not contain exactly None, Static image, Slide show and Video file; observed: " + observed);
        } else {
            report.Note("Desktop modes are file-backed: None, Static image, Slide show and Video file; Live/Journey are absent.");
        }
    }
    if (report.Passed()) {
        std::string accessibilityError;
        if (!HasBoundedAccessibleActionSurface(mainWindow, accessibilityError)) {
            fail("the bounded native accessibility surface failed: " + accessibilityError);
        } else {
            report.Note("Bounded accessibility check passed: primary native action controls are keyboard tab stops and buttons expose non-empty native names.");
        }
    }

    HWND wallpaperWindow = nullptr;
    if (report.Passed() &&
        (!SelectComboItemAndNotify(mainWindow, kMainDesktopModeCombo, 1) ||
         !PostCommand(mainWindow, kMainApplyDesktopMode) ||
         !WaitUntil(kWindowTimeout, [&] {
             wallpaperWindow = FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass);
             return wallpaperWindow &&
                    WindowTreeContainsText(mainWindow, L"saved image/slideshow");
         }))) {
        fail("the fixture-owned static image did not start through the production Apply route");
    } else if (report.Passed()) {
        report.Note("Static image mode decoded and presented a fixture-owned BMP through the production file-backed route.");
    }
    if (report.Passed()) {
        if (!SendMessageBounded(wallpaperWindow, WM_DISPLAYCHANGE, 32U,
                                MAKELPARAM(GetSystemMetrics(SM_CXSCREEN),
                                           GetSystemMetrics(SM_CYSCREEN))) ||
            !WaitUntil(std::chrono::seconds(5), [&] {
                return LogContainsText(appData,
                    "Display configuration changed; wallpaper layout rebuilt.");
            })) {
            fail("the production display-change handler did not rebuild the file-backed wallpaper layout");
        } else {
            report.Note("The active static wallpaper handled WM_DISPLAYCHANGE and rebuilt its layout on the current display topology.");
        }
    }
    if (report.Passed()) {
        const HWND originalParent = GetParent(wallpaperWindow);
        SetLastError(ERROR_SUCCESS);
        SetParent(wallpaperWindow, GetDesktopWindow());
        if (GetParent(wallpaperWindow) != GetDesktopWindow() ||
            !WaitUntil(std::chrono::seconds(7), [&] {
                return GetParent(wallpaperWindow) == originalParent &&
                       LogContainsText(appData,
                           "Wallpaper reattached after Explorer desktop host change.");
            })) {
            fail("the production desktop-host recovery path did not reattach after simulated Explorer-host loss");
        } else {
            report.Note("The wallpaper reattached after a fixture-induced loss of its Explorer desktop host; no Explorer process was terminated.");
        }
    }
    if (report.Passed() &&
        (!CheckExportPresentationPause(process, mainWindow, "Static image", kMainOpenFrameExport,
                                       kFrameExportWindowClass, kFrameExportClose, report) ||
         !CheckExportPresentationPause(process, mainWindow, "Static image", kMainOpenVideoExport,
                                       kVideoExportWindowClass, kVideoExportClose, report))) {
        fail("static image export pause/restoration did not preserve the desktop state");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainStopWallpaper) ||
        !WaitUntil(kWindowTimeout, [&] {
            return WindowTreeContainsText(mainWindow, L"Status: stopped") &&
                   !FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass);
        }))) {
        fail("static image mode did not stop and detach cleanly");
    }

    if (report.Passed() &&
        (!SelectComboItemAndNotify(mainWindow, kMainDesktopModeCombo, 2) ||
         !PostCommand(mainWindow, kMainApplyDesktopMode) ||
         !WaitUntil(kWindowTimeout, [&] {
             return WindowTreeContainsText(mainWindow, L"saved image/slideshow");
         }) ||
         !WaitUntil(std::chrono::seconds(30), [&] {
             return LogContainsText(appData,
                 "Slideshow advanced to the next saved image.");
         }))) {
        fail("slideshow mode did not start and advance through the production file-backed route");
    } else if (report.Passed()) {
        report.Note("Slideshow mode decoded fixture-owned images and advanced after its bounded ten-second interval.");
    }
    if (report.Passed() &&
        (!CheckExportPresentationPause(process, mainWindow, "Slideshow", kMainOpenFrameExport,
                                       kFrameExportWindowClass, kFrameExportClose, report) ||
         !CheckExportPresentationPause(process, mainWindow, "Slideshow", kMainOpenVideoExport,
                                       kVideoExportWindowClass, kVideoExportClose, report))) {
        fail("slideshow export pause/restoration did not preserve the desktop state");
    }
    if (report.Passed()) {
        std::filesystem::remove(firstImagePath, filesystemError);
        filesystemError.clear();
        std::filesystem::remove(secondImagePath, filesystemError);
        filesystemError.clear();
        HWND stoppedMessage = nullptr;
        if (!WaitUntil(std::chrono::seconds(30), [&] {
                stoppedMessage = FindProcessWindow(process.id, kCommonDialogClass);
                const auto title = stoppedMessage
                    ? ReadWindowTextBounded(stoppedMessage) : std::optional<std::wstring>{};
                return title && *title == L"Desktop Background Stopped";
            }) ||
            !WindowTreeContainsText(stoppedMessage,
                L"none of its configured image files could be loaded") ||
            !PostMessageW(stoppedMessage, WM_CLOSE, 0, 0) ||
            !WaitUntil(kWindowTimeout, [&] {
                return !IsWindow(stoppedMessage) &&
                       WindowTreeContainsText(mainWindow, L"Status: stopped") &&
                       !FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass);
            })) {
            fail("slideshow exhaustion did not fail closed with a visible stopped state");
        } else {
            report.Note("Deleting every fixture-owned slideshow image caused a visible fail-closed stop with no renderer fallback.");
        }
    }

    if (report.Passed()) {
        HWND invalidVideoMessage = nullptr;
        if (!SelectComboItemAndNotify(mainWindow, kMainDesktopModeCombo, 3) ||
            !PostCommand(mainWindow, kMainApplyDesktopMode) ||
            !WaitUntil(kWindowTimeout, [&] {
                invalidVideoMessage = FindProcessWindow(process.id, kCommonDialogClass);
                const auto title = invalidVideoMessage
                    ? ReadWindowTextBounded(invalidVideoMessage) : std::optional<std::wstring>{};
                return title && *title == L"Video Wallpaper";
            }) ||
            !WindowTreeContainsText(invalidVideoMessage,
                L"Windows Media Foundation could not open") ||
            !PostMessageW(invalidVideoMessage, WM_CLOSE, 0, 0) ||
            !WaitUntil(kWindowTimeout, [&] {
                return !IsWindow(invalidVideoMessage) &&
                       WindowTreeContainsText(mainWindow, L"Status: stopped") &&
                       !FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass);
            })) {
            fail("invalid MP4 playback did not fail visibly and remain stopped");
        } else {
            report.Note("A fixture-owned invalid MP4 was rejected visibly and left desktop presentation stopped.");
        }
    }

    if (report.Passed() && optInVideoWallpaper) {
        HWND videoFileDialog = nullptr;
        if (!PostCommand(mainWindow, kMainSetVideoWallpaper) ||
            !(videoFileDialog = WaitForProcessWindow(process, kCommonDialogClass)) ||
            !SubmitCommonDialogPath(videoFileDialog, *optInVideoWallpaper) ||
            !WaitUntil(kWindowTimeout, [&] {
                return !IsWindow(videoFileDialog) &&
                       WindowTreeContainsText(mainWindow, L"exported video");
            })) {
            fail("the verified MP4 did not start through the production file-selection route");
        } else if (!WaitUntil(std::chrono::seconds(15), [&] {
                       return LogContainsText(appData,
                                  "Video wallpaper mute state verified") &&
                              LogContainsText(appData,
                                  "restarted from position zero");
                   })) {
            fail("verified MP4 playback did not prove both mute and loop behavior");
        } else {
            report.Note("Media Foundation confirmed mute state and the verified MP4 reached end-of-file and looped from zero.");
        }
        if (report.Passed() &&
            (!CheckExportPresentationPause(process, mainWindow, "Video", kMainOpenFrameExport,
                                           kFrameExportWindowClass, kFrameExportClose, report) ||
             !CheckExportPresentationPause(process, mainWindow, "Video", kMainOpenVideoExport,
                                           kVideoExportWindowClass, kVideoExportClose, report))) {
            fail("video export pause/restoration did not preserve the desktop state");
        }
        wallpaperWindow = FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass);
        HWND asynchronousFailureMessage = nullptr;
        if (report.Passed() &&
            (!wallpaperWindow ||
             !PostMessageW(wallpaperWindow, kVideoPlaybackFailedMessage, 0,
                           static_cast<LPARAM>(E_FAIL)) ||
             !WaitUntil(kWindowTimeout, [&] {
                 asynchronousFailureMessage = FindProcessWindow(process.id, kCommonDialogClass);
                 const auto title = asynchronousFailureMessage
                     ? ReadWindowTextBounded(asynchronousFailureMessage)
                     : std::optional<std::wstring>{};
                 return title && *title == L"Desktop Background Stopped";
             }) ||
             !WindowTreeContainsText(asynchronousFailureMessage,
                 L"Windows Media Foundation reported a failure") ||
             !PostMessageW(asynchronousFailureMessage, WM_CLOSE, 0, 0) ||
             !WaitUntil(kWindowTimeout, [&] {
                 return !IsWindow(asynchronousFailureMessage) &&
                        WindowTreeContainsText(mainWindow, L"Status: stopped") &&
                        !FindProcessWindowInDesktopTree(process.id, kWallpaperWindowClass);
             }))) {
            fail("an asynchronous video playback failure did not stop, detach and notify visibly");
        } else if (report.Passed()) {
            report.Note("An injected asynchronous playback-failure window message stopped and detached video with a visible error; this does not reproduce a decoder-originated fault.");
        }
    }

    std::optional<std::wstring> ordinaryPrecision;
    if (report.Passed()) {
        const bool rendered = WaitUntil(kRenderTimeout, [&] {
            const auto label = ControlText(mainWindow, kMainPrecisionLabel);
            if (!label) return false;
            const auto precision = PreviewPrecision(*label);
            if (!precision || *precision == L"Not rendered yet") return false;
            ordinaryPrecision = precision;
            return true;
        });
        if (!rendered) fail("the preview did not report an initial resolved precision");
    }
    if (report.Passed()) {
        report.Note("Initial automatic preview precision: " + NarrowAscii(*ordinaryPrecision));
    }

    HWND paletteWindow = nullptr;
    std::optional<std::wstring> paletteCoordinates;
    std::optional<std::wstring> deepPrecision;
    if (report.Passed()) {
        const HWND coordinates = GetDlgItem(mainWindow, kMainCoordinates);
        if (!coordinates || !SetWindowTextBounded(coordinates, L"0.125, -0.25, 0.00000001") ||
            !SendCommand(mainWindow, kMainCoordinates, EN_KILLFOCUS, coordinates)) {
            fail("the pre-Palette deep camera edit could not be applied");
        } else {
            paletteCoordinates = ControlText(mainWindow, kMainCoordinates);
            if (!paletteCoordinates || *paletteCoordinates != L"1.25e-1, -2.5e-1, 1e-8") {
                fail("the pre-Palette deep camera edit was rejected or not reflected in the main window");
            }
        }
    }
    if (report.Passed()) {
        const bool switched = WaitUntil(kRenderTimeout, [&] {
            const auto label = ControlText(mainWindow, kMainPrecisionLabel);
            if (!label) return false;
            const auto precision = PreviewPrecision(*label);
            if (!precision || *precision == L"Not rendered yet" || *precision == *ordinaryPrecision) return false;
            deepPrecision = precision;
            return true;
        });
        if (!switched) fail("Automatic preview precision did not change before opening Palette Editor");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenPalette) ||
                           !(paletteWindow = WaitForProcessWindow(process, kPaletteWindowClass)))) {
        fail("the Palette Editor did not open through the real main-window command route");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(paletteWindow, kPaletteFrequency) &&
                   GetDlgItem(paletteWindow, kPaletteOk);
        })) {
        fail("the Palette Editor did not finish constructing its controls");
    }
    if (report.Passed()) {
        if (IsWindowEnabled(mainWindow) ||
            !PostCommand(mainWindow, kMainOpenEquation) ||
            (std::this_thread::sleep_for(std::chrono::milliseconds(750)),
             FindProcessWindow(process.id, kEquationWindowClass) != nullptr)) {
            fail("Palette Editor did not disable the main window or block a competing Equation Editor");
        } else {
            report.Note("Palette Editor exclusively owned the edit session; the main window and competing Equation route stayed blocked.");
        }
    }
    if (report.Passed() &&
        !SetControlTextAndNotify(paletteWindow, kPaletteFrequency, L"1.75", EN_CHANGE)) {
        fail("the Palette Editor setting change could not be delivered");
    }
    if (report.Passed() && !SendCommand(paletteWindow, kPaletteOk)) {
        fail("the Palette Editor OK command could not be delivered");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] { return !IsWindow(paletteWindow); })) {
        fail("the Palette Editor did not close after accepting the setting change");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto undo = ControlText(mainWindow, kMainUndo);
            return undo && undo->find(L"Edit Palette") != std::wstring::npos;
        })) {
        fail("the accepted palette change did not create its structural history entry");
    }
    if (report.Passed()) {
        const auto retained = ControlText(mainWindow, kMainCoordinates);
        if (!retained || *retained != *paletteCoordinates) {
            fail("accepting Palette Editor restored the camera captured when the editor opened");
        } else {
            report.Note("Palette accept retained the camera selected before the exclusive editor opened: " + NarrowAscii(*retained));
            report.Note("Automatic preview precision changed to: " + NarrowAscii(*deepPrecision));
            report.Note("Palette accept created the native Edit Palette history entry.");
        }
    }

    HWND equationWindow = nullptr;
    std::optional<std::wstring> equationCoordinates;
    std::optional<std::wstring> finalPrecision;
    std::optional<std::wstring> initialEquationPower;
    std::optional<std::wstring> editedEquationPower;
    if (report.Passed()) {
        const HWND coordinates = GetDlgItem(mainWindow, kMainCoordinates);
        if (!coordinates || !SetWindowTextBounded(coordinates, L"-0.5, 0, 0.25") ||
            !SendCommand(mainWindow, kMainCoordinates, EN_KILLFOCUS, coordinates)) {
            fail("the pre-Equation ordinary camera edit could not be applied");
        } else {
            equationCoordinates = ControlText(mainWindow, kMainCoordinates);
            if (!equationCoordinates || *equationCoordinates != L"-5e-1, 0, 2.5e-1") {
                fail("the pre-Equation ordinary camera edit was rejected or not reflected in the main window");
            }
        }
    }
    if (report.Passed()) {
        const bool switched = WaitUntil(kRenderTimeout, [&] {
            const auto label = ControlText(mainWindow, kMainPrecisionLabel);
            if (!label) return false;
            const auto precision = PreviewPrecision(*label);
            if (!precision || *precision == L"Not rendered yet" || *precision == *deepPrecision) return false;
            finalPrecision = precision;
            return true;
        });
        if (!switched) fail("Automatic preview precision did not return before opening Equation Editor");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenEquation) ||
                           !(equationWindow = WaitForProcessWindow(process, kEquationWindowClass)))) {
        fail("the Equation Editor did not open through the real main-window command route");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(equationWindow, kEquationPower) &&
                   GetDlgItem(equationWindow, kEquationOk) &&
                   GetDlgItem(equationWindow, kEquationCancel);
        })) {
        fail("the Equation Editor did not finish constructing its controls");
    }
    if (report.Passed()) {
        const auto okText = ControlText(equationWindow, kEquationOk);
        const auto cancelText = ControlText(equationWindow, kEquationCancel);
        report.Note("Equation command controls: " +
                    (okText ? NarrowAscii(*okText) : std::string("missing")) + "/" +
                    (cancelText ? NarrowAscii(*cancelText) : std::string("missing")));
        initialEquationPower = ControlText(equationWindow, kEquationPower);
        if (!initialEquationPower || initialEquationPower->empty()) {
            fail("the Equation Editor did not expose its initial power");
        } else {
            editedEquationPower = *initialEquationPower == L"3" ? L"4" : L"3";
            report.Note("Equation power edit: " + NarrowAscii(*initialEquationPower) +
                        " -> " + NarrowAscii(*editedEquationPower));
        }
    }
    if (report.Passed()) {
        if (IsWindowEnabled(mainWindow) ||
            !PostCommand(mainWindow, kMainOpenSettings) ||
            (std::this_thread::sleep_for(std::chrono::milliseconds(750)),
             FindProcessWindow(process.id, kSettingsWindowClass) != nullptr)) {
            fail("Equation Editor did not disable the main window or block a competing Settings dialog");
        } else {
            report.Note("Equation Editor exclusively owned the edit session; the main window and competing Settings route stayed blocked.");
        }
    }
    if (report.Passed() &&
        !SetControlTextAndNotify(equationWindow, kEquationPower, *editedEquationPower, EN_CHANGE)) {
        fail("the Equation Editor setting change could not be delivered");
    }
    if (report.Passed()) {
        const auto changedPower = ControlText(equationWindow, kEquationPower);
        if (!changedPower || *changedPower != *editedEquationPower) {
            fail("the Equation Editor did not retain the delivered power before acceptance");
        }
    }
    if (report.Passed() && !SendCommand(equationWindow, kEquationOk)) {
        fail("the Equation Editor OK command could not be delivered");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] { return !IsWindow(equationWindow); })) {
        fail("the Equation Editor did not close after accepting the setting change");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto undo = ControlText(mainWindow, kMainUndo);
            return undo && undo->find(L"Edit Equation") != std::wstring::npos;
        })) {
        const auto undo = ControlText(mainWindow, kMainUndo);
        fail("the accepted equation change did not create its structural history entry; Undo text was " +
             (undo ? NarrowAscii(*undo) : std::string("unavailable")));
    }
    if (report.Passed()) {
        const auto retained = ControlText(mainWindow, kMainCoordinates);
        if (!retained || *retained != *equationCoordinates) {
            fail("accepting Equation Editor restored the camera captured when the editor opened");
        } else {
            report.Note("Equation accept retained the camera selected before the exclusive editor opened: " + NarrowAscii(*retained));
            report.Note("Automatic preview precision returned to: " + NarrowAscii(*finalPrecision));
            report.Note("Equation accept created the native Edit Equation history entry.");
        }
    }

    if (report.Passed() && !SendCommand(mainWindow, kMainUndo)) {
        fail("the native Undo command could not be delivered after the equation change");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenEquation) ||
                           !(equationWindow = WaitForProcessWindow(process, kEquationWindowClass)))) {
        fail("the Equation Editor did not reopen after Undo");
    }
    if (report.Passed()) {
        ShowWindow(equationWindow, SW_HIDE);
        const auto power = ControlText(equationWindow, kEquationPower);
        if (!power || *power != *initialEquationPower) {
            fail("native Undo did not restore the pre-edit equation power");
        }
    }
    if (equationWindow && IsWindow(equationWindow)) {
        if (!SendCommand(equationWindow, kEquationCancel) ||
            !WaitUntil(kWindowTimeout, [&] { return !IsWindow(equationWindow); })) {
            fail("the post-Undo Equation Editor could not be closed safely");
        }
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainRedo)) {
        fail("the native Redo command could not be delivered for the equation change");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenEquation) ||
                           !(equationWindow = WaitForProcessWindow(process, kEquationWindowClass)))) {
        fail("the Equation Editor did not reopen after Redo");
    }
    if (report.Passed()) {
        ShowWindow(equationWindow, SW_HIDE);
        const auto power = ControlText(equationWindow, kEquationPower);
        const auto retained = ControlText(mainWindow, kMainCoordinates);
        if (!power || *power != *editedEquationPower) {
            fail("native Redo did not restore the accepted equation power");
        } else if (!retained || *retained != *equationCoordinates) {
            fail("native equation Undo/Redo changed the newer camera state");
        } else {
            report.Note("Native Equation Undo/Redo restored powers " +
                        NarrowAscii(*initialEquationPower) + "/" +
                        NarrowAscii(*editedEquationPower) +
                        " and preserved the live camera.");
        }
    }
    if (equationWindow && IsWindow(equationWindow)) {
        if (!SendCommand(equationWindow, kEquationCancel) ||
            !WaitUntil(kWindowTimeout, [&] { return !IsWindow(equationWindow); })) {
            fail("the post-Redo Equation Editor could not be closed safely");
        }
    }

    std::optional<std::wstring> beforePresetCoordinates;
    std::optional<std::wstring> loadedPresetCoordinates;
    if (report.Passed()) {
        beforePresetCoordinates = ControlText(mainWindow, kMainCoordinates);
        const HWND presetCombo = GetDlgItem(mainWindow, kMainPresetCombo);
        LRESULT presetCount = 0;
        if (!beforePresetCoordinates || !presetCombo ||
            !SendMessageBounded(presetCombo, CB_GETCOUNT, 0, 0, &presetCount) ||
            presetCount < 2 ||
            !SendMessageBounded(presetCombo, CB_SETCURSEL, 1, 0) ||
            !SendCommand(mainWindow, kMainPresetCombo, CBN_SELCHANGE, presetCombo)) {
            fail("the second built-in preset could not be selected through the real combo route");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto undo = ControlText(mainWindow, kMainUndo);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            return undo && undo->find(L"Load Preset") != std::wstring::npos &&
                   preset && preset->find(L"Seahorse Valley") != std::wstring::npos;
        })) {
        fail("the built-in preset load did not expose its selected state and history label");
    }
    if (report.Passed()) {
        loadedPresetCoordinates = ControlText(mainWindow, kMainCoordinates);
        if (!loadedPresetCoordinates || *loadedPresetCoordinates == *beforePresetCoordinates) {
            fail("the built-in preset load did not replace the working camera");
        }
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainUndo)) {
        fail("the native Undo command could not be delivered after preset load");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto redo = ControlText(mainWindow, kMainRedo);
            return coordinates && *coordinates == *beforePresetCoordinates &&
                   redo && redo->find(L"Load Preset") != std::wstring::npos;
        })) {
        fail("native Undo did not restore the complete pre-load preset state");
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainRedo)) {
        fail("the native Redo command could not be delivered after preset load Undo");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            return coordinates && *coordinates == *loadedPresetCoordinates &&
                   preset && preset->find(L"Seahorse Valley") != std::wstring::npos;
        })) {
        fail("native Redo did not restore the loaded built-in preset state");
    }
    if (report.Passed()) {
        report.Note("Native preset Load/Undo/Redo restored the exact before/after cameras and selected preset identity.");
    }

    HWND settingsWindow = nullptr;
    std::optional<std::wstring> initialRotation;
    constexpr std::wstring_view editedRotation = L"17.5";
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenSettings) ||
                           !(settingsWindow = WaitForProcessWindow(process, kSettingsWindowClass)))) {
        fail("Settings did not open through the real main-window command route");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(settingsWindow, kSettingsRotation) &&
                   GetDlgItem(settingsWindow, kSettingsOk) &&
                   GetDlgItem(settingsWindow, kSettingsCancel);
        })) {
        fail("Settings did not finish constructing its controls");
    }
    if (report.Passed()) {
        if (IsWindowEnabled(mainWindow) ||
            !PostCommand(mainWindow, kMainOpenPalette) ||
            (std::this_thread::sleep_for(std::chrono::milliseconds(750)),
             FindProcessWindow(process.id, kPaletteWindowClass) != nullptr)) {
            fail("Settings did not disable the main window or block a competing Palette Editor");
        } else {
            report.Note("Settings exclusively owned the edit session; the main window and competing Palette route stayed blocked.");
        }
    }
    if (report.Passed()) {
        initialRotation = ControlText(mainWindow, kMainRotation);
        if (!initialRotation ||
            !SetControlTextAndNotify(settingsWindow, kSettingsRotation,
                                     editedRotation, EN_CHANGE)) {
            fail("the Settings rotation change could not be delivered");
        }
    }
    if (report.Passed() && !SendCommand(settingsWindow, kSettingsOk)) {
        fail("the Settings OK command could not be delivered");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] { return !IsWindow(settingsWindow); })) {
        fail("Settings did not close after accepting the project change");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto undo = ControlText(mainWindow, kMainUndo);
            return undo && undo->find(L"Edit Project Settings") != std::wstring::npos;
        })) {
        fail("the accepted Settings project change did not create its structural history entry");
    }
    if (report.Passed()) {
        const auto rotation = ControlText(mainWindow, kMainRotation);
        if (!rotation || *rotation != editedRotation) {
            fail("the accepted Settings rotation was not reflected in the main-window control");
        }
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainUndo)) {
        fail("the native Undo command could not be delivered after the Settings change");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto rotation = ControlText(mainWindow, kMainRotation);
            const auto redo = ControlText(mainWindow, kMainRedo);
            return rotation && *rotation == *initialRotation &&
                   redo && redo->find(L"Edit Project Settings") != std::wstring::npos;
        })) {
        fail("native Undo did not restore the pre-Settings project rotation");
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainRedo)) {
        fail("the native Redo command could not be delivered after the Settings Undo");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto rotation = ControlText(mainWindow, kMainRotation);
            return rotation && *rotation == editedRotation;
        })) {
        fail("native Redo did not restore the accepted Settings project rotation");
    }
    if (report.Passed()) {
        report.Note("Native Settings accept/Undo/Redo restored project rotation " +
                    NarrowAscii(*initialRotation) + "/" + NarrowAscii(editedRotation) +
                    " through one Edit Project Settings entry.");
    }

    HWND journeyWindow = nullptr;
    std::optional<std::wstring> initialJourney;
    constexpr std::wstring_view editedJourney =
        L"-0.75,0,0.5,1,2\r\n-0.1,0.65,0.1,3,4";
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenJourney) ||
                           !(journeyWindow = WaitForProcessWindow(process, kJourneyWindowClass)))) {
        fail("Journey Settings did not open through the real main-window command route");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(journeyWindow, kJourneyWaypoints) &&
                   GetDlgItem(journeyWindow, kJourneyOk) &&
                   GetDlgItem(journeyWindow, kJourneyCancel);
        })) {
        fail("Journey Settings did not finish constructing its controls");
    }
    if (report.Passed()) {
        ShowWindow(journeyWindow, SW_HIDE);
        initialJourney = ControlText(journeyWindow, kJourneyWaypoints);
        if (!initialJourney ||
            !SetControlTextAndNotify(journeyWindow, kJourneyWaypoints,
                                     editedJourney, EN_CHANGE)) {
            fail("the Journey waypoint change could not be delivered");
        }
    }
    if (report.Passed() && !SendCommand(journeyWindow, kJourneyOk)) {
        fail("the Journey Settings OK command could not be delivered");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(journeyWindow) && undo &&
                   undo->find(L"Edit Journey") != std::wstring::npos;
        })) {
        fail("the accepted Journey change did not commit one structural history entry");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenJourney) ||
                           !(journeyWindow = WaitForProcessWindow(process, kJourneyWindowClass)))) {
        fail("Journey Settings did not reopen after acceptance");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto journey = ControlText(journeyWindow, kJourneyWaypoints);
            return journey && *journey == editedJourney;
        })) {
        fail("the accepted Journey waypoint text was not retained exactly");
    }
    if (journeyWindow && IsWindow(journeyWindow)) {
        if (!SendCommand(journeyWindow, kJourneyCancel) ||
            !WaitUntil(kWindowTimeout, [&] { return !IsWindow(journeyWindow); })) {
            fail("the accepted Journey inspection dialog could not be closed safely");
        }
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainUndo)) {
        fail("the native Undo command could not be delivered after the Journey change");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenJourney) ||
                           !(journeyWindow = WaitForProcessWindow(process, kJourneyWindowClass)))) {
        fail("Journey Settings did not reopen after Undo");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto journey = ControlText(journeyWindow, kJourneyWaypoints);
            const auto redo = ControlText(mainWindow, kMainRedo);
            return journey && *journey == *initialJourney && redo &&
                   redo->find(L"Edit Journey") != std::wstring::npos;
        })) {
        fail("native Undo did not restore the original Journey waypoint text");
    }
    if (journeyWindow && IsWindow(journeyWindow)) {
        if (!SendCommand(journeyWindow, kJourneyCancel) ||
            !WaitUntil(kWindowTimeout, [&] { return !IsWindow(journeyWindow); })) {
            fail("the post-Undo Journey inspection dialog could not be closed safely");
        }
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainRedo)) {
        fail("the native Redo command could not be delivered after Journey Undo");
    }
    if (report.Passed() && (!PostCommand(mainWindow, kMainOpenJourney) ||
                           !(journeyWindow = WaitForProcessWindow(process, kJourneyWindowClass)))) {
        fail("Journey Settings did not reopen after Redo");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto journey = ControlText(journeyWindow, kJourneyWaypoints);
            return journey && *journey == editedJourney;
        })) {
        fail("native Redo did not restore the accepted Journey waypoint text");
    }
    if (report.Passed()) {
        report.Note("Native Journey accept/Undo/Redo restored the exact before/after waypoint text through one Edit Journey entry.");
    }
    if (journeyWindow && IsWindow(journeyWindow)) {
        if (!SendCommand(journeyWindow, kJourneyCancel) ||
            !WaitUntil(kWindowTimeout, [&] { return !IsWindow(journeyWindow); })) {
            fail("the post-Redo Journey inspection dialog could not be closed safely");
        }
    }

    HWND fileDialog = nullptr;
    std::optional<std::wstring> beforeImportCoordinates;
    std::optional<std::wstring> importedCoordinates;
    std::optional<std::wstring> beforeImportPreset;
    if (report.Passed()) {
        beforeImportCoordinates = ControlText(mainWindow, kMainCoordinates);
        beforeImportPreset = ControlText(mainWindow, kMainPresetLibrary);
        if (!beforeImportCoordinates || !beforeImportPreset ||
            !PostCommand(mainWindow, kMainImportPreset) ||
            !(fileDialog = WaitForProcessWindow(process, kCommonDialogClass))) {
            fail("the preset Import command did not open its native file dialog");
        }
    }
    HWND fileNameEdit = nullptr;
    std::vector<HWND> fileDialogEdits;
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            fileNameEdit = FindCommonDialogFileNameEdit(fileDialog, &fileDialogEdits);
            return fileNameEdit != nullptr && GetDlgItem(fileDialog, IDOK) != nullptr;
        })) {
        fail("the preset Import file dialog did not expose its filename and Open controls");
    }
    if (report.Passed()) {
        for (HWND candidate : fileDialogEdits) {
            report.Note("Import dialog editable control: " +
                        ControlPath(candidate, fileDialog) +
                        (candidate == fileNameEdit ? " (selected)" : ""));
        }
    }
    if (report.Passed()) {
        const HWND fileNameCombo = GetParent(fileNameEdit);
        const HWND openButton = GetDlgItem(fileDialog, IDOK);
        const bool submitted =
            fileNameCombo && openButton &&
            SetWindowTextBounded(fileNameCombo, importPresetPath.wstring()) &&
            SetWindowTextBounded(fileNameEdit, importPresetPath.wstring()) &&
            SendMessageBounded(fileDialog, WM_COMMAND,
                               MAKEWPARAM(0x047c, CBN_EDITCHANGE),
                               reinterpret_cast<LPARAM>(fileNameCombo)) &&
            SendMessageBounded(openButton, BM_CLICK, 0, 0);
        if (!submitted) {
            fail("the fixture-owned preset path could not be submitted to the Import dialog");
        } else if (IsWindow(fileDialog)) {
            const bool submissionComplete = WaitUntil(std::chrono::seconds(5), [&] {
                if (!IsWindow(fileDialog)) return true;
                const auto text = ReadWindowTextBounded(fileNameEdit);
                return text && *text == importPresetPath.filename().wstring();
            });
            if (!submissionComplete) {
                const auto currentText = ReadWindowTextBounded(fileNameEdit);
                const auto currentTitle = ReadWindowTextBounded(fileDialog);
                report.Note("Import navigation diagnostic: title=" +
                            (currentTitle ? NarrowAscii(*currentTitle) : std::string("<none>")) +
                            ", filename=" +
                            (currentText ? NarrowAscii(*currentText) : std::string("<none>")));
                fail("the Import dialog did not accept the fixture-owned preset path");
            } else if (IsWindow(fileDialog)) {
                HWND navigatedOpenButton = nullptr;
                std::optional<std::wstring> navigatedTitle;
                const bool controlsReady = WaitUntil(std::chrono::seconds(5), [&] {
                    fileDialog = FindProcessWindow(process.id, kCommonDialogClass);
                    navigatedTitle = fileDialog
                        ? ReadWindowTextBounded(fileDialog)
                        : std::optional<std::wstring>{};
                    navigatedOpenButton =
                        fileDialog ? GetDlgItem(fileDialog, IDOK) : nullptr;
                    return navigatedTitle && *navigatedTitle != L"Open" ||
                           (navigatedTitle && *navigatedTitle == L"Open" &&
                            navigatedOpenButton != nullptr);
                });
                if (!controlsReady || !navigatedTitle || *navigatedTitle != L"Open") {
                    fail("the navigated Import dialog did not expose a current Open button");
                } else if (!SendMessageBounded(navigatedOpenButton, BM_CLICK, 0, 0)) {
                    fail("the current Import Open button rejected the fixture-owned preset file");
                }
            }
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto undo = ControlText(mainWindow, kMainUndo);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            return !IsWindow(fileDialog) && undo &&
                   undo->find(L"Import Preset") != std::wstring::npos &&
                   preset && preset->find(L"Fixture Imported") != std::wstring::npos;
        })) {
        const HWND remainingDialog = FindProcessWindow(process.id, kCommonDialogClass);
        const auto dialogTitle = remainingDialog
            ? ReadWindowTextBounded(remainingDialog) : std::optional<std::wstring>{};
        const auto submittedText = fileNameEdit && IsWindow(fileNameEdit)
            ? ReadWindowTextBounded(fileNameEdit) : std::optional<std::wstring>{};
        report.Note("Import completion diagnostic: original-dialog=" +
                    std::string(IsWindow(fileDialog) ? "open" : "closed") +
                    ", current-dialog-title=" +
                    (dialogTitle ? NarrowAscii(*dialogTitle) : std::string("<none>")) +
                    ", filename=" +
                    (submittedText ? NarrowAscii(*submittedText) : std::string("<unavailable>")));
        fail("the imported preset did not expose its selected state and history label");
    }
    if (report.Passed()) {
        importedCoordinates = ControlText(mainWindow, kMainCoordinates);
        if (!importedCoordinates || *importedCoordinates == *beforeImportCoordinates) {
            fail("the imported preset did not replace the working camera");
        }
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainUndo)) {
        fail("the native Undo command could not be delivered after preset import");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto redo = ControlText(mainWindow, kMainRedo);
            return coordinates && *coordinates == *beforeImportCoordinates &&
                   preset && *preset == *beforeImportPreset && redo &&
                   redo->find(L"Import Preset") != std::wstring::npos;
        })) {
        fail("native Undo did not restore the complete pre-import preset state");
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainRedo)) {
        fail("the native Redo command could not be delivered after preset import Undo");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            return coordinates && *coordinates == *importedCoordinates && preset &&
                   preset->find(L"Fixture Imported") != std::wstring::npos;
        })) {
        fail("native Redo did not restore the complete imported preset state");
    }
    if (report.Passed()) {
        report.Note("Native preset Import/Undo/Redo restored the exact before/after cameras and selected preset identity.");
    }

    HWND scoutWindow = nullptr;
    std::optional<std::wstring> beforeScoutCoordinates;
    std::optional<std::wstring> afterScoutCoordinates;
    std::optional<std::wstring> beforeScoutPreset;
    std::optional<std::wstring> beforeScoutUndo;
    if (report.Passed()) {
        beforeScoutCoordinates = ControlText(mainWindow, kMainCoordinates);
        beforeScoutPreset = ControlText(mainWindow, kMainPresetLibrary);
        beforeScoutUndo = ControlText(mainWindow, kMainUndo);
        if (!beforeScoutCoordinates || !beforeScoutPreset || !beforeScoutUndo ||
            !PostCommand(mainWindow, kMainOpenScout) ||
            !(scoutWindow = WaitForProcessWindow(process, kScoutWindowClass))) {
            fail("the Fractal Scout command did not open its production dialog");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT resultCount = 0;
            const HWND resultList = GetDlgItem(scoutWindow, kScoutResultList);
            const HWND useButton = GetDlgItem(scoutWindow, kScoutUse);
            return resultList && useButton && IsWindowEnabled(useButton) &&
                   SendMessageBounded(resultList, LB_GETCOUNT, 0, 0, &resultCount) &&
                   resultCount > 0;
        })) {
        fail("the Fractal Scout dialog did not expose a completed selectable candidate");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(scoutWindow, kScoutClose), BM_CLICK, 0, 0)) {
        fail("the completed Fractal Scout dialog could not be closed without applying");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(scoutWindow) && coordinates &&
                   *coordinates == *beforeScoutCoordinates && preset &&
                   *preset == *beforeScoutPreset && undo && *undo == *beforeScoutUndo;
        })) {
        fail("closing Fractal Scout after candidate selection mutated project state or history");
    }
    if (report.Passed()) {
        report.Note("Native Fractal Scout candidate selection and Close preserved the camera, preset identity and history.");
        if (!PostCommand(mainWindow, kMainOpenScout) ||
            !(scoutWindow = WaitForProcessWindow(process, kScoutWindowClass))) {
            fail("the Fractal Scout command did not reopen its production dialog");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT resultCount = 0;
            const HWND resultList = GetDlgItem(scoutWindow, kScoutResultList);
            const HWND useButton = GetDlgItem(scoutWindow, kScoutUse);
            return resultList && useButton && IsWindowEnabled(useButton) &&
                   SendMessageBounded(resultList, LB_GETCOUNT, 0, 0, &resultCount) &&
                   resultCount > 0;
        })) {
        fail("the reopened Fractal Scout dialog did not expose a completed selectable candidate");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(scoutWindow, kScoutUse), BM_CLICK, 0, 0)) {
        fail("the Fractal Scout Use in Preview action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(scoutWindow) && coordinates &&
                   *coordinates != *beforeScoutCoordinates && preset &&
                   *preset == *beforeScoutPreset && undo &&
                   undo->find(L"Apply Scout Camera") != std::wstring::npos;
        })) {
        fail("Fractal Scout Use in Preview did not apply one camera-only history entry");
    }
    if (report.Passed()) afterScoutCoordinates = ControlText(mainWindow, kMainCoordinates);
    if (report.Passed() && !SendCommand(mainWindow, kMainUndo)) {
        fail("the native Undo command could not be delivered after Scout Apply");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto redo = ControlText(mainWindow, kMainRedo);
            return coordinates && *coordinates == *beforeScoutCoordinates && preset &&
                   *preset == *beforeScoutPreset && redo &&
                   redo->find(L"Apply Scout Camera") != std::wstring::npos;
        })) {
        fail("native Undo did not restore the exact pre-Scout camera and preset identity");
    }
    if (report.Passed() && !SendCommand(mainWindow, kMainRedo)) {
        fail("the native Redo command could not be delivered after Scout Apply Undo");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            return coordinates && *coordinates == *afterScoutCoordinates && preset &&
                   *preset == *beforeScoutPreset;
        })) {
        fail("native Redo did not restore the exact applied Scout camera and preset identity");
    }
    if (report.Passed()) {
        report.Note("Native Fractal Scout Apply/Undo/Redo restored the exact before/after cameras while preserving preset identity.");
    }

    HWND timelineWindow = nullptr;
    const auto beforeTimelineCoordinates = report.Passed()
        ? ControlText(mainWindow, kMainCoordinates) : std::optional<std::wstring>{};
    const auto beforeTimelinePreset = report.Passed()
        ? ControlText(mainWindow, kMainPresetLibrary) : std::optional<std::wstring>{};
    const auto beforeTimelineUndo = report.Passed()
        ? ControlText(mainWindow, kMainUndo) : std::optional<std::wstring>{};
    if (report.Passed() &&
        (!beforeTimelineCoordinates || !beforeTimelinePreset || !beforeTimelineUndo ||
         !PostCommand(mainWindow, kMainOpenTimeline) ||
         !(timelineWindow = WaitForProcessWindow(process, kTimelineWindowClass)))) {
        fail("the Animation Timeline command did not open its production dialog");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(timelineWindow, kTimelineTrackList) &&
                   GetDlgItem(timelineWindow, kTimelineScrubber) &&
                   GetDlgItem(timelineWindow, kTimelineCancel);
        })) {
        fail("the Animation Timeline dialog did not finish constructing its controls");
    }
    if (report.Passed() &&
        !SelectComboItemAndNotify(timelineWindow, kTimelineTarget, 4)) {
        fail("the Animation Timeline could not select its palette-offset target");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineAddTrack), BM_CLICK, 0, 0)) {
        fail("the Animation Timeline Add Track action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT tracks = 0;
            LRESULT keys = 0;
            return SendMessageBounded(GetDlgItem(timelineWindow, kTimelineTrackList),
                                      LB_GETCOUNT, 0, 0, &tracks) &&
                   SendMessageBounded(GetDlgItem(timelineWindow, kTimelineKeyframeList),
                                      LB_GETCOUNT, 0, 0, &keys) &&
                   tracks == 1 && keys == 1;
        })) {
        fail("Animation Timeline did not create one track with its authoritative initial current-value keyframe");
    }
    if (report.Passed()) {
        const HWND scrubber = GetDlgItem(timelineWindow, kTimelineScrubber);
        if (!SendMessageBounded(scrubber, TBM_SETPOS, TRUE, 5000) ||
            !SendMessageBounded(timelineWindow, WM_HSCROLL,
                                MAKEWPARAM(TB_THUMBPOSITION, 5000),
                                reinterpret_cast<LPARAM>(scrubber))) {
            fail("the Animation Timeline scrubber could not seek the preview clock");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto time = ControlText(timelineWindow, kTimelineTimeLabel);
            return time && time->find(L"5") != std::wstring::npos;
        })) {
        fail("the Animation Timeline preview clock did not reach the scrubbed midpoint");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineAddCurrent), BM_CLICK, 0, 0)) {
        fail("the Animation Timeline Add Current Value action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT keys = 0;
            return SendMessageBounded(GetDlgItem(timelineWindow, kTimelineKeyframeList),
                                      LB_GETCOUNT, 0, 0, &keys) && keys == 2;
        })) {
        fail("Animation Timeline did not retain the midpoint current-value keyframe");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelinePlayPause), BM_CLICK, 0, 0)) {
        fail("the Animation Timeline Play action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(std::chrono::seconds(2), [&] {
            const auto label = ControlText(timelineWindow, kTimelinePlayPause);
            return label && *label == L"Pause";
        })) {
        fail("the Animation Timeline preview clock did not enter playback");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineStop), BM_CLICK, 0, 0)) {
        fail("the Animation Timeline Stop action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(std::chrono::seconds(2), [&] {
            const auto play = ControlText(timelineWindow, kTimelinePlayPause);
            const auto time = ControlText(timelineWindow, kTimelineTimeLabel);
            return play && *play == L"Play" && time &&
                   time->find(L"Time: 0") != std::wstring::npos;
        })) {
        fail("the Animation Timeline Stop action did not reset the preview clock");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineCancel), BM_CLICK, 0, 0)) {
        fail("the Animation Timeline Cancel action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(timelineWindow) && coordinates &&
                   *coordinates == *beforeTimelineCoordinates && preset &&
                   *preset == *beforeTimelinePreset && undo && *undo == *beforeTimelineUndo;
        })) {
        fail("Animation Timeline Cancel did not restore project and history isolation");
    }
    if (report.Passed()) {
        report.Note("Native Animation Timeline scrub/play/stop and Cancel preserved project state and history.");
        if (!PostCommand(mainWindow, kMainOpenTimeline) ||
            !(timelineWindow = WaitForProcessWindow(process, kTimelineWindowClass))) {
            fail("the Animation Timeline command did not reopen after Cancel");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT tracks = -1;
            return GetDlgItem(timelineWindow, kTimelineOk) &&
                   SendMessageBounded(GetDlgItem(timelineWindow, kTimelineTrackList),
                                      LB_GETCOUNT, 0, 0, &tracks) && tracks == 0;
    })) {
        fail("Animation Timeline Cancel retained its discarded candidate track");
    }
    if (report.Passed() &&
        !SelectComboItemAndNotify(timelineWindow, kTimelineTarget, 4)) {
        fail("the reopened Animation Timeline could not select its palette-offset target");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineAddTrack), BM_CLICK, 0, 0)) {
        fail("the reopened Animation Timeline Add Track action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT tracks = 0;
            return SendMessageBounded(GetDlgItem(timelineWindow, kTimelineTrackList),
                                      LB_GETCOUNT, 0, 0, &tracks) && tracks == 1;
        })) {
        fail("the reopened Animation Timeline did not create its accepted track");
    }
    if (report.Passed() &&
        !SetControlTextAndNotify(timelineWindow, kTimelineDuration, L"0.04", EN_KILLFOCUS)) {
        fail("the reopened Animation Timeline duration could not be shortened for bounded export coverage");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineOk), BM_CLICK, 0, 0)) {
        fail("the Animation Timeline OK action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(timelineWindow) && coordinates &&
                   *coordinates == *beforeTimelineCoordinates && preset &&
                   *preset == *beforeTimelinePreset && undo && *undo == *beforeTimelineUndo;
        })) {
        fail("Animation Timeline OK entered runtime-only timeline state into project history");
    }
    if (report.Passed() &&
        (!PostCommand(mainWindow, kMainOpenTimeline) ||
         !(timelineWindow = WaitForProcessWindow(process, kTimelineWindowClass)))) {
        fail("the Animation Timeline command did not reopen after OK");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT tracks = 0;
            return SendMessageBounded(GetDlgItem(timelineWindow, kTimelineTrackList),
                                      LB_GETCOUNT, 0, 0, &tracks) && tracks == 1;
        })) {
        fail("Animation Timeline OK did not retain the accepted runtime track");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineCancel), BM_CLICK, 0, 0)) {
        fail("the post-accept Animation Timeline dialog could not be closed safely");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout,
                                     [&] { return !IsWindow(timelineWindow); })) {
        fail("the post-accept Animation Timeline dialog did not close");
    }
    if (report.Passed()) {
        report.Note("Native Animation Timeline OK retained one runtime track without changing project state or history.");
    }
    HWND conversionMessage = nullptr;
    if (report.Passed() &&
        (!PostCommand(mainWindow, kMainOpenTimeline) ||
         !(timelineWindow = WaitForProcessWindow(process, kTimelineWindowClass)))) {
        fail("the Animation Timeline command did not reopen for Journey conversion");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineJourneyToTracks),
                            BM_CLICK, 0, 0)) {
        fail("the Animation Timeline Journey to Tracks action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            LRESULT tracks = 0;
            return SendMessageBounded(GetDlgItem(timelineWindow, kTimelineTrackList),
                                      LB_GETCOUNT, 0, 0, &tracks) && tracks == 3;
        })) {
        fail("Journey to Tracks did not produce the three camera tracks");
    }
    if (report.Passed() && !PostCommand(timelineWindow, kTimelineTracksToJourney)) {
        fail("the Animation Timeline Tracks to Journey action could not be invoked");
    }
    if (report.Passed()) {
        conversionMessage = WaitForProcessWindow(process, kCommonDialogClass);
        const auto title = conversionMessage
            ? ReadWindowTextBounded(conversionMessage) : std::optional<std::wstring>{};
        report.Note("Tracks to Journey confirmation diagnostic: title=" +
                    (title ? NarrowAscii(*title) : std::string("<none>")));
        if (!conversionMessage || !title || *title != L"Tracks to Journey" ||
            !SendMessageBounded(conversionMessage, WM_COMMAND,
                                MAKEWPARAM(IDOK, BN_CLICKED), 0)) {
            fail("Tracks to Journey did not expose its successful preparation confirmation");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(timelineWindow, kTimelineCancel), BM_CLICK, 0, 0)) {
        fail("the Journey-conversion Timeline candidate could not be cancelled");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(timelineWindow) && coordinates &&
                   *coordinates == *beforeTimelineCoordinates && preset &&
                   *preset == *beforeTimelinePreset && undo && *undo == *beforeTimelineUndo;
        })) {
        fail("cancelling the prepared Journey conversion mutated project state or history");
    }
    if (report.Passed()) {
        report.Note("Native Journey to Tracks and Tracks to Journey preparation passed; Cancel preserved project state and history.");
    }

    if (report.Passed()) {
        const HWND coordinates = GetDlgItem(mainWindow, kMainCoordinates);
        if (!coordinates ||
            !SetWindowTextBounded(coordinates, L"-0.5, 0, 1.5") ||
            !SendCommand(mainWindow, kMainCoordinates, EN_KILLFOCUS, coordinates) ||
            !WaitUntil(kWindowTimeout, [&] {
                const auto value = ControlText(mainWindow, kMainCoordinates);
                return value && *value == L"-5e-1, 0, 1.5e0";
            })) {
            fail("the frame-export preflight camera could not be set to an ordinary exact value");
        }
    }

    HWND frameExportWindow = nullptr;
    HWND frameExportMessage = nullptr;
    const std::filesystem::path completedExportDirectory = appData / L"frame-export-complete";
    const std::filesystem::path cancelledExportDirectory = appData / L"frame-export-cancelled";
    const std::filesystem::path closeCancelledExportDirectory = appData / L"frame-export-close-cancelled";
    const std::filesystem::path untrackedExportDirectory = appData / L"frame-export-untracked";
    const auto beforeExportCoordinates = report.Passed()
        ? ControlText(mainWindow, kMainCoordinates) : std::optional<std::wstring>{};
    const auto beforeExportPreset = report.Passed()
        ? ControlText(mainWindow, kMainPresetLibrary) : std::optional<std::wstring>{};
    const auto beforeExportUndo = report.Passed()
        ? ControlText(mainWindow, kMainUndo) : std::optional<std::wstring>{};
    if (report.Passed()) {
        filesystemError.clear();
        std::filesystem::create_directories(completedExportDirectory, filesystemError);
        if (!filesystemError) {
            std::filesystem::create_directories(cancelledExportDirectory, filesystemError);
        }
        if (!filesystemError) {
            std::filesystem::create_directories(closeCancelledExportDirectory, filesystemError);
        }
        if (!filesystemError) {
            std::filesystem::create_directories(untrackedExportDirectory, filesystemError);
        }
        if (filesystemError || !beforeExportCoordinates || !beforeExportPreset ||
            !beforeExportUndo || !PostCommand(mainWindow, kMainOpenFrameExport) ||
            !(frameExportWindow = WaitForProcessWindow(process, kFrameExportWindowClass))) {
            fail("the Frame Sequence Export route could not create its isolated output folders or open its production dialog");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(frameExportWindow, kFrameExportOutput) &&
                   GetDlgItem(frameExportWindow, kFrameExportStart) &&
                   GetDlgItem(frameExportWindow, kFrameExportCancel) &&
                   GetDlgItem(frameExportWindow, kFrameExportClose);
        })) {
        fail("the Frame Sequence Export dialog did not finish constructing its controls");
    }
    if (report.Passed()) {
        const HWND includeEnd = GetDlgItem(frameExportWindow, kFrameExportIncludeEnd);
        if (!SetControlTextAndNotify(frameExportWindow, kFrameExportOutput,
                                     completedExportDirectory.wstring(), EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportWidth, L"32", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportHeight, L"24", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportDpi, L"96", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportPrefix,
                                     L"fixture_complete", EN_CHANGE) ||
            !SelectComboItemAndNotify(frameExportWindow, kFrameExportRate, 0) ||
            !includeEnd ||
            !SendMessageBounded(includeEnd, BM_SETCHECK, BST_CHECKED, 0)) {
            fail("the bounded successful frame-export controls could not be configured");
        }
    }
    if (report.Passed()) {
        const auto summary = ControlText(frameExportWindow, kFrameExportSummary);
        if (!summary || summary->find(L"1 PNG frame") == std::wstring::npos) {
            fail("the bounded frame-export duration did not resolve to one frame");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportStart),
                            BM_CLICK, 0, 0)) {
        fail("the Frame Sequence Export Start action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto status = ControlText(frameExportWindow, kFrameExportStatus);
            return status && status->find(L"Complete:") == 0 &&
                   IsWindowEnabled(GetDlgItem(frameExportWindow, kFrameExportOpenFolder));
        })) {
        fail("the production WIC frame export did not complete through the native dialog");
    }
    if (report.Passed()) {
        const std::filesystem::path framePath =
            completedExportDirectory / L"fixture_complete-000000.png";
        const std::filesystem::path manifestPath =
            completedExportDirectory / L".mw-frame-sequence" / L"manifest.json";
        std::uint32_t pngWidth = 0U;
        std::uint32_t pngHeight = 0U;
        if (!std::filesystem::is_regular_file(framePath) ||
            !std::filesystem::is_regular_file(manifestPath) ||
            !ReadPngDimensions(framePath, pngWidth, pngHeight) ||
            pngWidth != 32U || pngHeight != 24U ||
            ContainsPartialFile(completedExportDirectory)) {
            fail("the completed native frame export did not leave one verified 32x24 PNG and manifest without partial files");
        } else {
            report.Note("Native Frame Sequence Export completed one production WIC 32x24 PNG with a manifest and no partial file.");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the completed Frame Sequence Export dialog could not be closed");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(frameExportWindow) && IsWindowEnabled(mainWindow) &&
                   coordinates && *coordinates == *beforeExportCoordinates &&
                   preset && *preset == *beforeExportPreset &&
                   undo && *undo == *beforeExportUndo;
    })) {
        fail("the completed frame export changed project state/history or did not restore its owner window");
    }
    if (report.Passed() &&
        (!PostCommand(mainWindow, kMainOpenFrameExport) ||
         !(frameExportWindow = WaitForProcessWindow(process, kFrameExportWindowClass)))) {
        fail("the Frame Sequence Export command did not reopen for matching-manifest resume");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(frameExportWindow, kFrameExportOutput) &&
                   GetDlgItem(frameExportWindow, kFrameExportStart) &&
                   GetDlgItem(frameExportWindow, kFrameExportClose);
        })) {
        fail("the resume Frame Sequence Export dialog did not finish constructing its controls");
    }
    if (report.Passed()) {
        const HWND includeEnd = GetDlgItem(frameExportWindow, kFrameExportIncludeEnd);
        if (!SetControlTextAndNotify(frameExportWindow, kFrameExportOutput,
                                     completedExportDirectory.wstring(), EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportWidth, L"32", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportHeight, L"24", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportDpi, L"96", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportPrefix,
                                     L"fixture_complete", EN_CHANGE) ||
            !SelectComboItemAndNotify(frameExportWindow, kFrameExportRate, 0) ||
            !includeEnd ||
            !SendMessageBounded(includeEnd, BM_SETCHECK, BST_CHECKED, 0)) {
            fail("the matching-manifest resume controls could not be configured");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportStart),
                            BM_CLICK, 0, 0)) {
        fail("the matching-manifest frame resume action could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto status = ControlText(frameExportWindow, kFrameExportStatus);
            return status && status->find(L"Complete: 0 rendered, 1 resumed.") == 0;
        })) {
        fail("the native frame-export resume did not reuse its one verified frame");
    }
    if (report.Passed() && ContainsPartialFile(completedExportDirectory)) {
        fail("the resumed native frame export left an incomplete .part file");
    }
    if (report.Passed()) {
        report.Note("Native Frame Sequence Export resumed its matching manifest with 0 rendered and 1 verified frame reused.");
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the resumed Frame Sequence Export dialog could not be closed");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(frameExportWindow) && IsWindowEnabled(mainWindow);
        })) {
        fail("the resumed Frame Sequence Export dialog did not restore its owner window");
    }
    if (report.Passed() &&
        (!PostCommand(mainWindow, kMainOpenFrameExport) ||
         !(frameExportWindow = WaitForProcessWindow(process, kFrameExportWindowClass)))) {
        fail("the Frame Sequence Export command did not reopen for manifest-mismatch refusal");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(frameExportWindow, kFrameExportOutput) &&
                   GetDlgItem(frameExportWindow, kFrameExportStart) &&
                   GetDlgItem(frameExportWindow, kFrameExportClose);
        })) {
        fail("the manifest-mismatch Frame Sequence Export dialog did not finish constructing its controls");
    }
    if (report.Passed()) {
        const HWND includeEnd = GetDlgItem(frameExportWindow, kFrameExportIncludeEnd);
        if (!SetControlTextAndNotify(frameExportWindow, kFrameExportOutput,
                                     completedExportDirectory.wstring(), EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportWidth, L"33", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportHeight, L"24", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportDpi, L"96", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportPrefix,
                                     L"fixture_complete", EN_CHANGE) ||
            !SelectComboItemAndNotify(frameExportWindow, kFrameExportRate, 0) ||
            !includeEnd ||
            !SendMessageBounded(includeEnd, BM_SETCHECK, BST_CHECKED, 0) ||
            !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportStart),
                                BM_CLICK, 0, 0)) {
            fail("the manifest-mismatch frame-export action could not be configured or started");
        }
    }
    if (report.Passed()) {
        frameExportMessage = WaitForProcessWindow(process, kCommonDialogClass);
        const auto title = frameExportMessage
            ? ReadWindowTextBounded(frameExportMessage) : std::optional<std::wstring>{};
        if (!frameExportMessage || !title || *title != L"Frame Sequence Export" ||
            !WindowTreeContainsText(frameExportMessage, L"does not match") ||
            !SendMessageBounded(frameExportMessage, WM_COMMAND,
                                MAKEWPARAM(IDOK, BN_CLICKED), 0)) {
            fail("the production frame-export manifest-mismatch refusal was not exposed");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto status = ControlText(frameExportWindow, kFrameExportStatus);
            return status && *status == L"Export failed." &&
                   IsWindowEnabled(GetDlgItem(frameExportWindow, kFrameExportStart));
        })) {
        fail("the manifest-mismatch refusal did not return control to the export dialog");
    }
    if (report.Passed()) {
        const std::filesystem::path framePath =
            completedExportDirectory / L"fixture_complete-000000.png";
        std::uint32_t pngWidth = 0U;
        std::uint32_t pngHeight = 0U;
        if (!ReadPngDimensions(framePath, pngWidth, pngHeight) ||
            pngWidth != 32U || pngHeight != 24U ||
            ContainsPartialFile(completedExportDirectory)) {
            fail("the manifest-mismatch refusal changed the verified frame or left a partial file");
        } else {
            report.Note("Native Frame Sequence Export refused a mismatched manifest and preserved the verified PNG.");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the manifest-mismatch Frame Sequence Export dialog could not be closed");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(frameExportWindow) && IsWindowEnabled(mainWindow);
        })) {
        fail("the manifest-mismatch Frame Sequence Export dialog did not restore its owner window");
    }
    frameExportMessage = nullptr;
    const std::filesystem::path untrackedFramePath =
        untrackedExportDirectory / L"fixture_untracked-000000.png";
    const std::string untrackedContents = "fixture-owned untracked output";
    if (report.Passed()) {
        std::ofstream untracked(untrackedFramePath, std::ios::binary | std::ios::trunc);
        untracked << untrackedContents;
        if (!untracked) fail("the fixture-owned untracked frame could not be prepared");
    }
    if (report.Passed() &&
        (!PostCommand(mainWindow, kMainOpenFrameExport) ||
         !(frameExportWindow = WaitForProcessWindow(process, kFrameExportWindowClass)))) {
        fail("the Frame Sequence Export command did not reopen for untracked-output refusal");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(frameExportWindow, kFrameExportOutput) &&
                   GetDlgItem(frameExportWindow, kFrameExportStart) &&
                   GetDlgItem(frameExportWindow, kFrameExportClose);
        })) {
        fail("the untracked-output Frame Sequence Export dialog did not finish constructing its controls");
    }
    if (report.Passed()) {
        const HWND includeEnd = GetDlgItem(frameExportWindow, kFrameExportIncludeEnd);
        if (!SetControlTextAndNotify(frameExportWindow, kFrameExportOutput,
                                     untrackedExportDirectory.wstring(), EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportWidth, L"32", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportHeight, L"24", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportDpi, L"96", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportPrefix,
                                     L"fixture_untracked", EN_CHANGE) ||
            !SelectComboItemAndNotify(frameExportWindow, kFrameExportRate, 0) ||
            !includeEnd ||
            !SendMessageBounded(includeEnd, BM_SETCHECK, BST_CHECKED, 0) ||
            !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportStart),
                                BM_CLICK, 0, 0)) {
            fail("the untracked-output frame-export action could not be configured or started");
        }
    }
    if (report.Passed()) {
        frameExportMessage = WaitForProcessWindow(process, kCommonDialogClass);
        const auto title = frameExportMessage
            ? ReadWindowTextBounded(frameExportMessage) : std::optional<std::wstring>{};
        if (!frameExportMessage || !title || *title != L"Frame Sequence Export" ||
            !WindowTreeContainsText(frameExportMessage, L"untracked final frame already exists") ||
            !SendMessageBounded(frameExportMessage, WM_COMMAND,
                                MAKEWPARAM(IDOK, BN_CLICKED), 0)) {
            fail("the production frame-export untracked-output refusal was not exposed");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto status = ControlText(frameExportWindow, kFrameExportStatus);
            return status && *status == L"Export failed." &&
                   IsWindowEnabled(GetDlgItem(frameExportWindow, kFrameExportStart));
        })) {
        fail("the untracked-output refusal did not return control to the export dialog");
    }
    if (report.Passed()) {
        std::ifstream untracked(untrackedFramePath, std::ios::binary);
        std::ostringstream contents;
        contents << untracked.rdbuf();
        if (contents.str() != untrackedContents ||
            ContainsPartialFile(untrackedExportDirectory)) {
            fail("the untracked-output refusal changed the existing file or left a partial file");
        } else {
            report.Note("Native Frame Sequence Export refused an untracked final PNG and preserved its exact contents.");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the untracked-output Frame Sequence Export dialog could not be closed");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(frameExportWindow) && IsWindowEnabled(mainWindow);
        })) {
        fail("the untracked-output Frame Sequence Export dialog did not restore its owner window");
    }
    frameExportMessage = nullptr;
    if (report.Passed() &&
        (!PostCommand(mainWindow, kMainOpenFrameExport) ||
         !(frameExportWindow = WaitForProcessWindow(process, kFrameExportWindowClass)))) {
        fail("the Frame Sequence Export command did not reopen for cancellation");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(frameExportWindow, kFrameExportOutput) &&
                   GetDlgItem(frameExportWindow, kFrameExportStart) &&
                   GetDlgItem(frameExportWindow, kFrameExportCancel);
        })) {
        fail("the reopened Frame Sequence Export dialog did not finish constructing its controls");
    }
    if (report.Passed()) {
        if (!SetControlTextAndNotify(frameExportWindow, kFrameExportOutput,
                                     cancelledExportDirectory.wstring(), EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportWidth, L"1024", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportHeight, L"1024", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportDpi, L"96", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportPrefix,
                                     L"fixture_cancelled", EN_CHANGE) ||
            !SelectComboItemAndNotify(frameExportWindow, kFrameExportRate, 0)) {
            fail("the cancellable frame-export controls could not be configured");
        }
    }
    if (report.Passed() &&
        (!SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportStart),
                             BM_CLICK, 0, 0) ||
         !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportCancel),
                             BM_CLICK, 0, 0))) {
        fail("the native frame-export Start/Cancel actions could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto status = ControlText(frameExportWindow, kFrameExportStatus);
            return status && status->find(L"Cancelled. Preserved ") == 0 &&
                   IsWindowEnabled(GetDlgItem(frameExportWindow, kFrameExportStart));
        })) {
        fail("the native frame-export cancellation did not complete and re-enable the dialog");
    }
    if (report.Passed()) {
        if (ContainsPartialFile(cancelledExportDirectory)) {
            fail("the cancelled native frame export left an incomplete .part file");
        } else {
            report.Note("Native Frame Sequence Export cancellation joined its worker, preserved resumable metadata and left no partial file.");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the cancelled Frame Sequence Export dialog could not be closed");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(frameExportWindow) && IsWindowEnabled(mainWindow) &&
                   coordinates && *coordinates == *beforeExportCoordinates &&
                   preset && *preset == *beforeExportPreset &&
                   undo && *undo == *beforeExportUndo;
    })) {
        fail("the cancelled frame export changed project state/history or did not restore its owner window");
    }

    if (report.Passed() &&
        (!PostCommand(mainWindow, kMainOpenFrameExport) ||
         !(frameExportWindow = WaitForProcessWindow(process, kFrameExportWindowClass)))) {
        fail("the Frame Sequence Export command did not reopen for active-close cancellation");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(frameExportWindow, kFrameExportOutput) &&
                   GetDlgItem(frameExportWindow, kFrameExportStart) &&
                   GetDlgItem(frameExportWindow, kFrameExportClose);
        })) {
        fail("the active-close Frame Sequence Export dialog did not finish constructing its controls");
    }
    if (report.Passed()) {
        if (!SetControlTextAndNotify(frameExportWindow, kFrameExportOutput,
                                     closeCancelledExportDirectory.wstring(), EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportWidth, L"1024", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportHeight, L"1024", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportDpi, L"96", EN_CHANGE) ||
            !SetControlTextAndNotify(frameExportWindow, kFrameExportPrefix,
                                     L"fixture_close_cancelled", EN_CHANGE) ||
            !SelectComboItemAndNotify(frameExportWindow, kFrameExportRate, 0)) {
            fail("the active-close frame-export controls could not be configured");
        }
    }
    if (report.Passed() &&
        (!SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportStart),
                             BM_CLICK, 0, 0) ||
         !PostMessageW(frameExportWindow, WM_CLOSE, 0, 0))) {
        fail("the native frame-export Start/close actions could not be invoked");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto status = ControlText(frameExportWindow, kFrameExportStatus);
            return IsWindow(frameExportWindow) && status &&
                   status->find(L"Cancelled. Preserved ") == 0 &&
                   IsWindowEnabled(GetDlgItem(frameExportWindow, kFrameExportStart));
        })) {
        fail("closing an active native frame export did not cancel and re-enable the dialog");
    }
    if (report.Passed()) {
        const std::filesystem::path manifestPath =
            closeCancelledExportDirectory / L".mw-frame-sequence" / L"manifest.json";
        if (!std::filesystem::is_regular_file(manifestPath) ||
            ContainsPartialFile(closeCancelledExportDirectory)) {
            fail("closing an active native frame export did not preserve resumable metadata cleanly");
        } else {
            report.Note("Closing an active Frame Sequence Export cancelled and joined its worker, retained its dialog and resumable metadata, and left no partial file.");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the active-close Frame Sequence Export dialog could not be closed after cancellation");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(frameExportWindow) && IsWindowEnabled(mainWindow) &&
                   coordinates && *coordinates == *beforeExportCoordinates &&
                   preset && *preset == *beforeExportPreset &&
                   undo && *undo == *beforeExportUndo;
    })) {
        fail("active-close cancellation changed project state/history or did not restore its owner window");
    }

    HWND videoExportWindow = nullptr;
    HWND videoExportMessage = nullptr;
    const std::filesystem::path videoExportDirectory = appData / L"video-export";
    const std::filesystem::path videoOutputPath = videoExportDirectory / L"fixture.mp4";
    const std::filesystem::path missingFfmpegPath =
        videoExportDirectory / L"missing-ffmpeg.exe";
    if (report.Passed()) {
        filesystemError.clear();
        std::filesystem::create_directories(videoExportDirectory, filesystemError);
        if (filesystemError || !PostCommand(mainWindow, kMainOpenVideoExport) ||
            !(videoExportWindow = WaitForProcessWindow(process, kVideoExportWindowClass))) {
            fail("the Video Export route could not create its isolated output folder or open its production dialog");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(videoExportWindow, kVideoExportSequence) &&
                   GetDlgItem(videoExportWindow, kVideoExportFfmpeg) &&
                   GetDlgItem(videoExportWindow, kVideoExportOutput) &&
                   GetDlgItem(videoExportWindow, kVideoExportCrf) &&
                   GetDlgItem(videoExportWindow, kVideoExportStart) &&
                   GetDlgItem(videoExportWindow, kVideoExportClose);
        })) {
        fail("the Video Export dialog did not finish constructing its controls");
    }
    if (report.Passed() &&
        (!SetControlTextAndNotify(videoExportWindow, kVideoExportSequence,
                                  completedExportDirectory.wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportFfmpeg, L"", EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportOutput, L"", EN_CHANGE))) {
        fail("the Video Export validation controls could not be configured");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto summary = ControlText(videoExportWindow, kVideoExportSummary);
            return summary && summary->find(L"Sequence: 1 frames") != std::wstring::npos &&
                   summary->find(L"32x24") != std::wstring::npos;
        })) {
        fail("typing a verified sequence path did not refresh the Video Export summary");
    }
    if (report.Passed() && !PostCommand(videoExportWindow, kVideoExportStart)) {
        fail("the incomplete Video Export action could not be invoked");
    }
    if (report.Passed()) {
        videoExportMessage = WaitForProcessWindow(process, kCommonDialogClass);
        const auto title = videoExportMessage
            ? ReadWindowTextBounded(videoExportMessage) : std::optional<std::wstring>{};
        if (!videoExportMessage || !title || *title != L"External Video Export" ||
            !WindowTreeContainsText(videoExportMessage,
                                    L"Choose a verified frame-sequence folder") ||
            !PostMessageW(videoExportMessage, WM_CLOSE, 0, 0)) {
            fail("the Video Export missing-input validation was not exposed");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(videoExportMessage) && IsWindowEnabled(videoExportWindow);
        })) {
        fail("the Video Export missing-input message did not return control to the dialog");
    }
    videoExportMessage = nullptr;
    if (report.Passed() &&
        (!SetControlTextAndNotify(videoExportWindow, kVideoExportFfmpeg,
                                  missingFfmpegPath.wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportOutput,
                                  videoOutputPath.wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportCrf, L"52", EN_CHANGE) ||
         !PostCommand(videoExportWindow, kVideoExportStart))) {
        fail("the invalid-CRF Video Export action could not be configured or invoked");
    }
    if (report.Passed()) {
        videoExportMessage = WaitForProcessWindow(process, kCommonDialogClass);
        const auto title = videoExportMessage
            ? ReadWindowTextBounded(videoExportMessage) : std::optional<std::wstring>{};
        if (!videoExportMessage || !title || *title != L"External Video Export" ||
            !WindowTreeContainsText(videoExportMessage,
                                    L"CRF must be a whole number from 0 to 51") ||
            !PostMessageW(videoExportMessage, WM_CLOSE, 0, 0)) {
            fail("the Video Export CRF validation was not exposed");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(videoExportMessage) && IsWindowEnabled(videoExportWindow);
        })) {
        fail("the Video Export CRF message did not return control to the dialog");
    }
    videoExportMessage = nullptr;
    if (report.Passed() &&
        (!SetControlTextAndNotify(videoExportWindow, kVideoExportCrf, L"18", EN_CHANGE) ||
         !PostCommand(videoExportWindow, kVideoExportStart))) {
        fail("the missing-executable Video Export action could not be invoked");
    }
    if (report.Passed()) {
        videoExportMessage = WaitForProcessWindow(process, kCommonDialogClass);
        const auto title = videoExportMessage
            ? ReadWindowTextBounded(videoExportMessage) : std::optional<std::wstring>{};
        if (!videoExportMessage || !title || *title != L"External Video Export" ||
            !WindowTreeContainsText(videoExportMessage,
                                    L"external executable does not exist") ||
            !PostMessageW(videoExportMessage, WM_CLOSE, 0, 0)) {
            fail("the Video Export missing-executable failure was not exposed");
        }
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(videoExportMessage) && IsWindowEnabled(videoExportWindow);
        })) {
        fail("the Video Export missing-executable message did not return control to the dialog");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto status = ControlText(videoExportWindow, kVideoExportStatus);
            return status && *status == L"Video encoding failed." &&
                   IsWindowEnabled(GetDlgItem(videoExportWindow, kVideoExportStart));
        })) {
        fail("the missing-executable Video Export failure did not return control to the dialog");
    }
    if (report.Passed()) {
        if (std::filesystem::exists(videoOutputPath) ||
            ContainsTemporaryVideo(videoExportDirectory)) {
            fail("the Video Export validation/failure route left final or temporary MP4 output");
        } else {
            report.Note("Native Video Export refreshed its typed-sequence summary, exposed missing-input and CRF validation, and contained a missing-executable failure without MP4 output.");
        }
    }
    if (report.Passed() &&
        !SendMessageBounded(GetDlgItem(videoExportWindow, kVideoExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the Video Export dialog could not be closed after validation");
    }
    if (report.Passed() && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(videoExportWindow) && IsWindowEnabled(mainWindow) &&
                   coordinates && *coordinates == *beforeExportCoordinates &&
                   preset && *preset == *beforeExportPreset &&
                   undo && *undo == *beforeExportUndo;
    })) {
        fail("Video Export validation changed project state/history or did not restore its owner window");
    }

    const std::filesystem::path verifiedVideoOutputPath =
        videoExportDirectory / L"verified-dialog.mp4";
    if (report.Passed() && optInFfmpeg &&
        (!PostCommand(mainWindow, kMainOpenVideoExport) ||
         !(videoExportWindow = WaitForProcessWindow(process, kVideoExportWindowClass)))) {
        fail("the Video Export command did not reopen for the opt-in real encode");
    }
    if (report.Passed() && optInFfmpeg && !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(videoExportWindow, kVideoExportSequence) &&
                   GetDlgItem(videoExportWindow, kVideoExportFfmpeg) &&
                   GetDlgItem(videoExportWindow, kVideoExportOutput) &&
                   GetDlgItem(videoExportWindow, kVideoExportStart) &&
                   GetDlgItem(videoExportWindow, kVideoExportOpenOutput) &&
                   GetDlgItem(videoExportWindow, kVideoExportClose);
        })) {
        fail("the opt-in real Video Export dialog did not finish constructing its controls");
    }
    if (report.Passed() && optInFfmpeg &&
        (!SetControlTextAndNotify(videoExportWindow, kVideoExportSequence,
                                  completedExportDirectory.wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportFfmpeg,
                                  optInFfmpeg->wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportOutput,
                                  verifiedVideoOutputPath.wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportCrf, L"28", EN_CHANGE) ||
         !SelectComboItemAndNotify(videoExportWindow, kVideoExportPreset, 0) ||
         !PostCommand(videoExportWindow, kVideoExportStart))) {
        fail("the opt-in real Video Export action could not be configured or started");
    }
    if (report.Passed() && optInFfmpeg &&
        !WaitUntil(std::chrono::seconds(45), [&] {
            const auto status = ControlText(videoExportWindow, kVideoExportStatus);
            return status && status->find(L"Verified MP4 complete:") == 0 &&
                   IsWindowEnabled(GetDlgItem(videoExportWindow, kVideoExportOpenOutput));
        })) {
        fail("the opt-in real Video Export did not reach verified MP4 completion");
    }
    if (report.Passed() && optInFfmpeg) {
        filesystemError.clear();
        const auto outputSize = std::filesystem::file_size(
            verifiedVideoOutputPath, filesystemError);
        const auto sourceFramePath =
            completedExportDirectory / L"fixture_complete-000000.png";
        if (filesystemError || outputSize == 0U ||
            !std::filesystem::is_regular_file(sourceFramePath) ||
            ContainsTemporaryVideo(videoExportDirectory)) {
            fail("the opt-in real Video Export did not preserve a verified final-only MP4 boundary");
        } else {
            report.Note("Native Video Export completed and verified one MP4 through the explicitly supplied external encoder, preserved its source PNG and left no temporary video.");
        }
    }
    if (report.Passed() && optInFfmpeg &&
        !SendMessageBounded(GetDlgItem(videoExportWindow, kVideoExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the completed opt-in Video Export dialog could not be closed");
    }
    if (report.Passed() && optInFfmpeg && !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(videoExportWindow) && IsWindowEnabled(mainWindow) &&
                   coordinates && *coordinates == *beforeExportCoordinates &&
                   preset && *preset == *beforeExportPreset &&
                   undo && *undo == *beforeExportUndo;
    })) {
        fail("the completed opt-in Video Export changed project state/history or did not restore its owner window");
    }

    const bool runOptInVideoCancellation =
        optInFfmpeg.has_value() && optInVideoCancelSequence.has_value();
    const std::filesystem::path cancelledVideoOutputPath =
        videoExportDirectory / L"cancelled-dialog.mp4";
    mw::FrameSequenceManifest cancellationManifestBefore;
    if (report.Passed() && runOptInVideoCancellation) {
        std::string verificationError;
        if (!mw::LoadVerifiedFrameSequence(*optInVideoCancelSequence,
                                           cancellationManifestBefore,
                                           verificationError) ||
            cancellationManifestBefore.completedFrames.size() < 2U) {
            fail("the opt-in Video Export cancellation sequence was not complete and verified");
        }
    }
    if (report.Passed() && runOptInVideoCancellation &&
        (!PostCommand(mainWindow, kMainOpenVideoExport) ||
         !(videoExportWindow = WaitForProcessWindow(process, kVideoExportWindowClass)))) {
        fail("the Video Export command did not reopen for opt-in cancellation");
    }
    if (report.Passed() && runOptInVideoCancellation &&
        !WaitUntil(kWindowTimeout, [&] {
            return GetDlgItem(videoExportWindow, kVideoExportSequence) &&
                   GetDlgItem(videoExportWindow, kVideoExportCancel) &&
                   GetDlgItem(videoExportWindow, kVideoExportClose);
        })) {
        fail("the opt-in cancellation Video Export dialog did not finish constructing its controls");
    }
    if (report.Passed() && runOptInVideoCancellation &&
        (!SetControlTextAndNotify(videoExportWindow, kVideoExportSequence,
                                  optInVideoCancelSequence->wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportFfmpeg,
                                  optInFfmpeg->wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportOutput,
                                  cancelledVideoOutputPath.wstring(), EN_CHANGE) ||
         !SetControlTextAndNotify(videoExportWindow, kVideoExportCrf, L"18", EN_CHANGE) ||
         !SelectComboItemAndNotify(videoExportWindow, kVideoExportPreset, 8) ||
         !PostCommand(videoExportWindow, kVideoExportStart))) {
        fail("the opt-in Video Export cancellation action could not be configured or started");
    }
    if (report.Passed() && runOptInVideoCancellation &&
        !WaitUntil(std::chrono::seconds(45), [&] {
            const auto status = ControlText(videoExportWindow, kVideoExportStatus);
            return status &&
                   (status->find(L"Encoding to an owned temporary MP4") == 0 ||
                    status->find(L"FFmpeg encoded approximately") == 0) &&
                   IsWindowEnabled(GetDlgItem(videoExportWindow, kVideoExportCancel));
        })) {
        fail("the opt-in Video Export did not reach its owned encoding stage for cancellation");
    }
    if (report.Passed() && runOptInVideoCancellation &&
        !SendMessageBounded(GetDlgItem(videoExportWindow, kVideoExportCancel),
                            BM_CLICK, 0, 0)) {
        fail("the opt-in Video Export Cancel action could not be invoked");
    }
    if (report.Passed() && runOptInVideoCancellation &&
        !WaitUntil(std::chrono::seconds(45), [&] {
            const auto status = ControlText(videoExportWindow, kVideoExportStatus);
            return status && status->find(L"Cancelled. Source PNG frames") == 0 &&
                   IsWindowEnabled(GetDlgItem(videoExportWindow, kVideoExportStart));
        })) {
        fail("the opt-in Video Export cancellation did not return control to the dialog");
    }
    if (report.Passed() && runOptInVideoCancellation) {
        mw::FrameSequenceManifest cancellationManifestAfter;
        std::string verificationError;
        if (std::filesystem::exists(cancelledVideoOutputPath) ||
            ContainsTemporaryVideo(videoExportDirectory) ||
            !mw::LoadVerifiedFrameSequence(*optInVideoCancelSequence,
                                           cancellationManifestAfter,
                                           verificationError) ||
            cancellationManifestAfter.jobFingerprint !=
                cancellationManifestBefore.jobFingerprint ||
            cancellationManifestAfter.completedFrames.size() !=
                cancellationManifestBefore.completedFrames.size()) {
            fail("the opt-in Video Export cancellation did not preserve its verified source/final boundary");
        } else {
            report.Note("Native Video Export cancellation terminated the owned FFmpeg encode, preserved the complete verified source sequence and left no final or temporary MP4.");
        }
    }
    if (report.Passed() && runOptInVideoCancellation &&
        !SendMessageBounded(GetDlgItem(videoExportWindow, kVideoExportClose),
                            BM_CLICK, 0, 0)) {
        fail("the cancelled opt-in Video Export dialog could not be closed");
    }
    if (report.Passed() && runOptInVideoCancellation &&
        !WaitUntil(kWindowTimeout, [&] {
            const auto coordinates = ControlText(mainWindow, kMainCoordinates);
            const auto preset = ControlText(mainWindow, kMainPresetLibrary);
            const auto undo = ControlText(mainWindow, kMainUndo);
            return !IsWindow(videoExportWindow) && IsWindowEnabled(mainWindow) &&
                   coordinates && *coordinates == *beforeExportCoordinates &&
                   preset && *preset == *beforeExportPreset &&
                   undo && *undo == *beforeExportUndo;
        })) {
        fail("the cancelled opt-in Video Export changed project state/history or did not restore its owner window");
    }

    if (paletteWindow && IsWindow(paletteWindow)) PostMessageW(paletteWindow, WM_CLOSE, 0, 0);
    if (equationWindow && IsWindow(equationWindow)) PostMessageW(equationWindow, WM_CLOSE, 0, 0);
    if (settingsWindow && IsWindow(settingsWindow)) PostMessageW(settingsWindow, WM_CLOSE, 0, 0);
    if (journeyWindow && IsWindow(journeyWindow)) PostMessageW(journeyWindow, WM_CLOSE, 0, 0);
    if (fileDialog && IsWindow(fileDialog)) PostMessageW(fileDialog, WM_CLOSE, 0, 0);
    if (scoutWindow && IsWindow(scoutWindow)) {
        PostMessageW(scoutWindow, WM_COMMAND, MAKEWPARAM(kScoutClose, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(scoutWindow, kScoutClose)));
    }
    if (timelineWindow && IsWindow(timelineWindow)) {
        PostMessageW(timelineWindow, WM_COMMAND, MAKEWPARAM(kTimelineCancel, BN_CLICKED),
                     reinterpret_cast<LPARAM>(GetDlgItem(timelineWindow, kTimelineCancel)));
    }
    if (frameExportWindow && IsWindow(frameExportWindow)) {
        const HWND cancel = GetDlgItem(frameExportWindow, kFrameExportCancel);
        if (cancel && IsWindowEnabled(cancel)) {
            SendMessageBounded(cancel, BM_CLICK, 0, 0);
        }
        WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(frameExportWindow) ||
                   IsWindowEnabled(GetDlgItem(frameExportWindow, kFrameExportStart));
        });
        if (IsWindow(frameExportWindow)) {
            SendMessageBounded(GetDlgItem(frameExportWindow, kFrameExportClose),
                               BM_CLICK, 0, 0);
        }
    }
    if (videoExportMessage && IsWindow(videoExportMessage)) {
        PostMessageW(videoExportMessage, WM_CLOSE, 0, 0);
    }
    if (videoExportWindow && IsWindow(videoExportWindow)) {
        const HWND cancel = GetDlgItem(videoExportWindow, kVideoExportCancel);
        if (cancel && IsWindowEnabled(cancel)) {
            SendMessageBounded(cancel, BM_CLICK, 0, 0);
        }
        WaitUntil(kWindowTimeout, [&] {
            return !IsWindow(videoExportWindow) ||
                   IsWindowEnabled(GetDlgItem(videoExportWindow, kVideoExportStart));
        });
        if (IsWindow(videoExportWindow)) {
            SendMessageBounded(GetDlgItem(videoExportWindow, kVideoExportClose),
                               BM_CLICK, 0, 0);
        }
    }
    if (conversionMessage && IsWindow(conversionMessage)) {
        PostMessageW(conversionMessage, WM_CLOSE, 0, 0);
    }
    if (report.Passed() && soakSeconds > 0) {
        if (!WriteSolidBmp(firstImagePath, 255U, 0U, 0U) ||
            !SelectComboItemAndNotify(mainWindow, kMainDesktopModeCombo, 1) ||
            !PostCommand(mainWindow, kMainApplyDesktopMode) ||
            !WaitUntil(kWindowTimeout, [&] {
                return WindowTreeContainsText(mainWindow, L"saved image/slideshow");
            })) {
            fail("the bounded resource soak could not start static file-backed presentation");
        } else {
            const auto before = ReadResourceSnapshot(process.process);
            const auto deadline = std::chrono::steady_clock::now() +
                std::chrono::seconds(soakSeconds);
            while (report.Passed() && std::chrono::steady_clock::now() < deadline) {
                if (ProcessExited(process.process)) {
                    fail("the fixture-owned application exited during the bounded resource soak");
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
            const auto after = ReadResourceSnapshot(process.process);
            constexpr SIZE_T maximumWorkingSetGrowth = 64U * 1024U * 1024U;
            if (report.Passed() && (!before || !after)) {
                fail("resource counters were unavailable during the bounded soak");
            } else if (report.Passed() &&
                       (after->workingSetBytes > before->workingSetBytes + maximumWorkingSetGrowth ||
                        after->handleCount > before->handleCount + 64U ||
                        after->gdiObjects > before->gdiObjects + 32U ||
                        after->userObjects > before->userObjects + 32U)) {
                fail("the bounded resource soak exceeded its working-set or handle-growth allowance");
            } else if (report.Passed()) {
                report.Note("Bounded " + std::to_string(soakSeconds) +
                    "-second static-presentation soak passed: working-set delta " +
                    std::to_string(static_cast<long long>(after->workingSetBytes) -
                                   static_cast<long long>(before->workingSetBytes)) +
                    " bytes, handle delta " +
                    std::to_string(static_cast<long long>(after->handleCount) -
                                   static_cast<long long>(before->handleCount)) +
                    ", GDI delta " +
                    std::to_string(static_cast<long long>(after->gdiObjects) -
                                   static_cast<long long>(before->gdiObjects)) +
                    ", USER delta " +
                    std::to_string(static_cast<long long>(after->userObjects) -
                                   static_cast<long long>(before->userObjects)) + ".");
            }
            if (!PostCommand(mainWindow, kMainStopWallpaper) ||
                !WaitUntil(kWindowTimeout, [&] {
                    return WindowTreeContainsText(mainWindow, L"Status: stopped");
                })) {
                fail("static presentation did not stop after the bounded resource soak");
            }
        }
    }
    StopOwnedApplication(process, mainWindow);
    if (!ProcessExited(process.process)) {
        fail("the fixture-owned application process did not terminate");
    } else if (!LogContainsShutdown(appData)) {
        fail("the fixture-owned application did not record a clean shutdown");
    } else {
        report.Note("The Exit command completed and the isolated log records clean shutdown.");
    }

    report.Write(reportDirectory / L"report.md");
    if (!report.Passed()) {
        std::cerr << "Windows interaction fixture failed; see "
                  << (reportDirectory / L"report.md").string() << '\n';
        return 1;
    }
    std::cout << "Windows interaction fixture passed; report: "
              << (reportDirectory / L"report.md").string() << '\n';
    return 0;
}

#else

int main() { return 0; }

#endif
