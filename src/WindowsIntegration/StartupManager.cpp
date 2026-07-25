#include "WindowsIntegration/StartupManager.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace mw {

bool StartupManager::IsEnabled() {
#ifdef _WIN32
    HKEY key = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) return false;
    DWORD type = 0;
    DWORD bytes = 0;
    const LONG result = RegQueryValueExW(key, L"MandelbrotLiveWallpaper", nullptr, &type, nullptr, &bytes);
    RegCloseKey(key);
    return result == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ) && bytes > sizeof(wchar_t);
#else
    return false;
#endif
}

bool StartupManager::SetEnabled(bool enabled, const std::filesystem::path& executablePath, std::string& error) {
#ifdef _WIN32
    HKEY key = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        error = "Could not open the current-user startup registry key.";
        return false;
    }
    LONG result = ERROR_SUCCESS;
    if (enabled) {
        const std::wstring command = L"\"" + executablePath.wstring() + L"\" --tray";
        result = RegSetValueExW(key, L"MandelbrotLiveWallpaper", 0, REG_SZ,
                                reinterpret_cast<const BYTE*>(command.c_str()),
                                static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    } else {
        result = RegDeleteValueW(key, L"MandelbrotLiveWallpaper");
        if (result == ERROR_FILE_NOT_FOUND) result = ERROR_SUCCESS;
    }
    RegCloseKey(key);
    if (result != ERROR_SUCCESS) {
        error = enabled ? "Could not enable Start with Windows." : "Could not disable Start with Windows.";
        return false;
    }
    return true;
#else
    (void)enabled;
    (void)executablePath;
    error = "Start with Windows is only supported on Windows.";
    return false;
#endif
}

} // namespace mw
