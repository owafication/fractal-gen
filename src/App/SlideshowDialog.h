#pragma once

#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace mw {

enum class SlideshowDialogResult {
    Cancelled,
    Saved,
    StartSelected,
};

class SlideshowDialog {
public:
#ifdef _WIN32
    static SlideshowDialogResult Show(HWND owner, HINSTANCE instance, StaticWallpaperSettings& settings);
#endif
};

} // namespace mw
