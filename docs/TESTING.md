# Testing

## Automated core tests

`tests/CoreTests.cpp` covers:

- Interior points: `0 + 0i`, `-1 + 0i`.
- Exterior points: `2 + 2i`, `0.5 + 0.5i`.
- Finite smooth-colouring values.
- Preset numeric clamping and practical zoom floor.
- Versioned settings JSON round-trip.
- Custom monitor assignment persistence.
- Rejection of malformed JSON and unsupported palette values.
- Data-only handling of unknown imported fields.
- Continuous zoom progression.
- Built-in preset count and performance-profile defaults.
- Compensated sub-ULP camera movement.
- Double and arbitrary-precision reference-orbit construction.
- Precision strategy persistence and validation.
- Perturbation rejection for non-analytic absolute-value recurrences.

Run with:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

## Required Windows runtime matrix

The following cannot be proven by the Linux build environment used to produce the source archive and must be run on physical or virtual Windows systems:

- Windows 10 22H2 and current Windows 11.
- Integrated and dedicated graphics.
- 1080p, 1440p and 4K.
- 100%, 125%, 150% and mixed DPI.
- One monitor, two-monitor mirror/span/independent, portrait and negative-coordinate layouts.
- Explorer restart, display reconnect and primary-monitor change.
- Lock/unlock, sleep/wake and Remote Desktop.
- Full-screen game/application pause and resume delay.
- Driver reset/device loss where a test harness can induce it safely.
- Installer install, upgrade and uninstall.

## Manual acceptance checklist

1. Preview renders and resizes.
2. Wheel zoom follows the cursor and drag pans.
3. Palette, iteration, FPS and render-scale changes are visible.
4. Wallpaper attaches behind icons and does not intercept icon interaction.
5. Pause, resume and stop work from both UI and tray.
6. Explorer restart causes reattachment.
7. Full-screen applications pause rendering when enabled.
8. Settings and monitor assignments survive restart.
9. Invalid preset files show a user-facing error without a crash.
10. Stopping or exiting reveals the previous wallpaper.
11. Mirror, span and independent modes display the expected view per monitor.
12. CPU, GPU and memory remain stable during an extended run.

## Equation editor tests

- Confirm **Edit Equation** opens and the main preview updates while coefficients change.
- Confirm Cancel restores the prior equation and OK retains the new equation.
- Verify `A=0, B=1, C=1.2, D=0` renders `z = z + 1.2c`.
- Verify `A=0, B=1, C=1, D=1.2` renders `z = z + c + 1.2`.
- Confirm real and imaginary coefficient values outside -8 to 8 are rejected.
- Confirm preset save, export, import, static capture, mirror, span, and independent modes retain the equation.
- Confirm older presets without an `equation` object load as classic `z = z² + c`.
- Confirm an `equation` string or executable field is rejected or ignored as data and is never compiled or executed.

## Deep-zoom runtime checks

1. Open **Configure precision…** and verify each strategy can be selected directly.
2. Toggle every Automatic candidate independently and restart the app to confirm persistence.
3. On a GPU with float64 support, confirm diagnostics report `GPU float64`; on unsupported hardware, confirm safe fallback or a clear error when fallback is disabled.
4. At progressively deeper scales, confirm diagnostics transition from float32 to the enabled deep-zoom strategy without block-shaped coordinate collapse.
5. Test perturbation at a fixed deep centre, then pan slightly and confirm the reference orbit is rebuilt without a crash.
6. Test 128-, 256-, and 512-bit reference settings and observe CPU cost without uncontrolled retry loops.
7. Select a Burning Ship absolute-value equation and confirm perturbation is rejected or falls back as configured.
8. Verify preview, live wallpaper, independent monitor regions, and static capture use the same selected precision settings.
