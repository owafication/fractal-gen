#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace mw {

class AdaptivePerformanceDialog {
public:
#ifdef _WIN32
    static bool Show(HWND owner, HINSTANCE instance, AdaptivePerformanceSettings& settings);
#endif
};

} // namespace mw
