#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>
#include <vector>

namespace mw {

enum class PresetManagerAction {
    Cancelled,
    Load,
    SaveNew,
    Update,
    Delete,
    RestoreBuiltIns,
    Import,
    Export,
};

class PresetManagerDialog {
public:
#ifdef _WIN32
    static PresetManagerAction Show(HWND owner, HINSTANCE instance,
                                    const std::vector<Preset>& presets,
                                    int currentIndex,
                                    int& selectedIndex,
                                    std::string& saveAsName);
#endif
};

} // namespace mw
