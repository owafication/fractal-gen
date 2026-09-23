#pragma once

#include "Core/GeneralAnimation.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <functional>
#include <optional>
#include <string>

namespace mw {

struct GeneralAnimationEditorResult {
    bool accepted{false};
    std::optional<std::string> journeyScript;
};

class GeneralAnimationEditorDialog {
public:
#ifdef _WIN32
    using PreviewCallback = std::function<void(const AnimationTimeline&, double, bool)>;

    static GeneralAnimationEditorResult Show(HWND owner,
                                             HINSTANCE instance,
                                             const Preset& basePreset,
                                             AnimationTimeline& timeline,
                                             AnimationClockBank& clocks,
                                             PreviewCallback previewCallback);
#endif
};

} // namespace mw
