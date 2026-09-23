# Windows Desktop Integration

**Status:** Current compatibility summary. Release evidence is owned by `project_docs/VALIDATION_AND_EVIDENCE.md`.

## Mechanism

The application locates Progman/WorkerW, requests the desktop hierarchy with a bounded timeout, finds the host behind `SHELLDLL_DefView`, and attaches an application-owned non-activating child window behind desktop icons. `DesktopHost` isolates this undocumented mechanism.

## Reversibility

The application does not use `SystemParametersInfo` to replace the configured wallpaper image. Stop/shutdown detaches and destroys the app-owned host, revealing the prior wallpaper.

## Recovery and displays

- Explorer attachment is periodically revalidated and reattached without an uncontrolled loop.
- Display/DPI changes rebuild virtual bounds and rendering regions.
- Host-relative mapping handles monitors left/above the primary display.
- Mirror, Span and Independent modes use one virtual-desktop host with appropriate regions/assignments.

## Compatibility risk

WorkerW/Progman is undocumented and may vary across Explorer versions, shell replacements and managed environments. Source inspection cannot prove compatibility. Native Windows 10/11 tests must cover startup, icon interaction, Explorer restart, mixed DPI/negative coordinates, display reconnect, lock, sleep, Remote Desktop, pause/resume and stop.

## Release gate

Windows-facing completion requires native MSVC resource/compile/link evidence plus the applicable runtime matrix. Run `scripts/validate-windows-release.ps1`, then complete `docs/testing/WINDOWS-RELEASE-VALIDATION.md`. Portable checks are not sufficient.
