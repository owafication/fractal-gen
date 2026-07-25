# Verification record for 1.7.0

## Implemented

- Added platform-independent sustained overload and bounded resume logic.
- Added process CPU and working-set sampling on Windows.
- Added configurable adaptive resource-protection dialog.
- Added preview and wallpaper visual-change suppression before shader execution.
- Added settings schema version 7 persistence and migration from version 6.
- Added adaptive state, resource samples and skipped-frame counts to diagnostics.
- Updated build, installer and resource metadata to version 1.7.0.

## Checks available in the build environment

- GCC core build with warnings treated as errors.
- Clang core build with warnings treated as errors.
- Core tests for low-FPS pause, CPU pause, stable resume, visual-idle detection and accumulated visible movement.
- AddressSanitizer and UndefinedBehaviorSanitizer core run.
- Source structure, offline-policy and package integrity checks.

## Unverified here

- Win32 process sampling values on the target machine.
- WGL frame pacing under the user's AMD driver.
- UI layout and modal dialog operation on Windows 10/11.
- Real-world threshold tuning across different CPUs and GPUs.
