#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <functional>

namespace mw {

class JourneySettingsDialog {
public:
#ifdef _WIN32
    static bool Show(HWND owner, HINSTANCE instance, Preset& preset,
                     std::function<void()> onChanged = {});
#endif
};

} // namespace mw
