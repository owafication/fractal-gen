#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace mw {

class HighResRenderDialog {
public:
#ifdef _WIN32
    static void Show(HWND owner, HINSTANCE instance, const Preset& snapshot, const PerformanceSettings& performance);
#endif
};

} // namespace mw
