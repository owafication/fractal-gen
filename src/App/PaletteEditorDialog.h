#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <functional>
#include <vector>

namespace mw {

class PaletteEditorDialog {
public:
#ifdef _WIN32
    static bool Show(HWND owner, HINSTANCE instance, Preset& preset,
                     std::vector<PalettePreset>& savedPalettes,
                     std::function<void()> onChanged = {});
#endif
};

} // namespace mw
