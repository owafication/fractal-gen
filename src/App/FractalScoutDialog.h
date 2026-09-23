#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace mw {

enum class FractalScoutAction {
    None,
    UseInPreview,
    UseAndSaveAsNew,
};

class FractalScoutDialog {
public:
#ifdef _WIN32
    static FractalScoutAction Show(HWND owner, HINSTANCE instance,
                                   const Preset& sourcePreset,
                                   const PerformanceSettings& performance,
                                   Preset& selectedCandidate);
#endif
};

} // namespace mw
