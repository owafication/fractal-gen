# Verification — 1.13.1

## Reported failure

MSVC rejected `JourneySettingsDialog.cpp` because it called the private `AnimationController::ParseJourneyScript` member.

## Implemented correction

- Added public `AnimationController::HasValidJourneyScriptTargets(const std::string&)`.
- Kept `ParseJourneyScript` and `JourneyPoint` private.
- Updated Journey Settings to call only the public validation contract.
- Added core tests for valid, malformed and empty scripts.
- Added source-regression checks that prohibit future dialog calls to the private parser.

## Verification limits

Passed in this environment: GCC Release with warnings as errors, Clang Release with warnings as errors, and Clang AddressSanitizer/UndefinedBehaviorSanitizer core tests. Native MSVC must be rerun on Windows to directly verify the original failing target.
