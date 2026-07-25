#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <vector>

namespace mw {

class PaletteEditorDialog {
public:
#ifdef _WIN32
    static bool Show(HWND owner, HINSTANCE instance, Preset& preset,
                     std::vector<PalettePreset>& savedPalettes);
#endif
};

} // namespace mw
