# Verification record for 1.5.0

## Covered in the source package environment

- Core C++ compilation with GCC and Clang using warnings as errors.
- Core tests for fractal math, settings validation, equation security, palette persistence, journey pan/zoom phases, journey target count, boundary-rich target selection and deep-zoom reference-orbit math.
- AddressSanitizer and UndefinedBehaviorSanitizer core test execution.
- Static source checks for offline operation, one embedded manifest, staged tabbed UI controls, numeric fields, precision descriptions, WorkerW coordinate mapping and independent assignment feedback.
- ZIP integrity and exclusion of build output.

## Requires Windows runtime verification

- Win32 tab layout at Windows scaling values such as 100%, 125%, 150% and 200%.
- WorkerW/Progman placement across the user's exact three-monitor topology.
- Independent preset assignment on each physical monitor.
- WGL renderer and deep-zoom modes on the installed AMD driver.
- Explorer restart, sleep/wake and display reconnect behaviour.

Passing the platform-independent checks does not prove these Windows-specific behaviours. The copied diagnostics now include virtual-desktop and per-monitor rectangles to make any remaining topology issue inspectable.
