#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <vector>
#include <functional>

namespace mw {

class SettingsDialog {
public:
#ifdef _WIN32
    static bool Show(HWND owner, HINSTANCE instance, AppSettings& settings, Preset& preset,
                     const std::vector<Preset>& presets, std::function<void()> onChanged = {});
#endif
};

} // namespace mw
