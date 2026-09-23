#pragma once

#ifdef _WIN32
#include <windows.h>
#endif

namespace mw {

class VideoExportDialog {
public:
#ifdef _WIN32
    static void Show(HWND owner, HINSTANCE instance);
#endif
};

} // namespace mw
