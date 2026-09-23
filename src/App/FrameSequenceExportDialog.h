#pragma once

#include "Core/GeneralAnimation.h"
#include "Core/Models.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace mw {

class FrameSequenceExportDialog {
public:
#ifdef _WIN32
    static void Show(HWND owner,
                     HINSTANCE instance,
                     const Preset& baseSnapshot,
                     const AnimationTimeline& timelineSnapshot,
                     const StaticWallpaperSettings& outputSettings);
#endif
};

} // namespace mw
