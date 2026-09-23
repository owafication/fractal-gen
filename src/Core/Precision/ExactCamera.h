#pragma once

#include "Core/Precision/ExactDecimal.h"

namespace mw {

struct ExactCamera {
    ExactDecimal centreX;
    ExactDecimal centreY;
    ExactDecimal halfHeight;

    bool operator==(const ExactCamera&) const = default;
};

} // namespace mw
