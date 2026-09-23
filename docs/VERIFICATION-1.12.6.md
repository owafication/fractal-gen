# Verification — 1.12.6

## Reported failure

MSVC failed while compiling `src/App/FractalScoutDialog.cpp` because `CameraCentreX` and `CameraCentreY` were undeclared. The later ambiguous wide-stream error was a cascade from those unresolved expressions.

## Implemented correction

- Added a direct `#include "Core/DeepZoom.h"` to `src/App/FractalScoutDialog.cpp`.
- Added a source regression check requiring the declaration header before the helper calls.
- Updated release metadata to 1.12.6.

## Available verification

- Offline source verifier.
- GCC Release core build and tests with warnings treated as errors.
- Clang Release core build and tests with warnings treated as errors.
- Clang AddressSanitizer and UndefinedBehaviorSanitizer core tests.
- Clean packaged-tree verification and ZIP integrity validation.

## Verification limit

This environment does not include the Windows SDK/MSBuild runtime, so the corrected native MSVC build must still be rerun on Windows. The change directly supplies the missing declarations reported by MSVC.
