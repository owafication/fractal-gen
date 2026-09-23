#pragma once

#include "Core/Models.h"
#include "Core/Precision/ExactCamera.h"

#include <string>

namespace mw {

struct LegacyCameraAdaptation {
    CameraState camera;
    bool centreXLoss{false};
    bool centreYLoss{false};
    bool halfHeightLoss{false};
};

// Used only to reconstruct the exact mathematical value of legacy persisted
// high/low camera components during forward migration.
[[nodiscard]] bool BuildExactCameraFromLegacy(const CameraState& legacy,
                                              ExactCamera& result,
                                              std::string& error);

// This is intentionally one-way: exact text stays authoritative and the
// derived CameraState cannot be written back as a replacement for it. A
// positive half-height below double range maps to denorm_min with loss set,
// allowing schema-3 exact state to load while legacy rendering remains an
// explicitly lossy compatibility path.
[[nodiscard]] bool AdaptExactCameraToLegacy(const ExactCamera& exact,
                                             LegacyCameraAdaptation& result,
                                             std::string& error);

} // namespace mw
