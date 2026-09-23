# Windows Release Validation

**Status:** Current PH-01/PH-11 validation matrix; bounded desktop, 60-second resource and isolated installer evidence recorded in BR-20260923-03; release gates remain open  
**Purpose:** Execute and record PH-01/PH-11 native evidence without touching normal user settings
**Owner:** Release engineering
**Reading trigger:** Native Windows build, smoke, package or release work
**Update trigger:** Toolchain, runtime matrix, report schema or release workflow change
**Linked IDs:** PH-01, AC-003–AC-006, VAL-003–VAL-008, RISK-002, RISK-003

## Automated entry point

From a clean local repository in Windows PowerShell:

```powershell
.\scripts\validate-windows-release.ps1
```

Optional controls:

```powershell
.\scripts\validate-windows-release.ps1 -SkipInstaller
.\scripts\validate-windows-release.ps1 -SkipRuntimeSmoke
.\scripts\validate-windows-release.ps1 -ReportDirectory C:\Temp\MLW-validation
```

The workflow:

1. requires native Windows, CMake, Git and Python 3;
2. records whether the Git working tree is clean;
3. runs the source/offline-policy verifier;
4. configures the canonical native MSVC x64 Release generator from `CMakePresets.json`;
5. builds with warnings as errors;
6. runs CTest, including core, path-isolation, production-renderer, version-consistency and serial native Palette/Equation/preset-history interaction tests;
7. verifies embedded executable versions;
8. builds and inspects the portable ZIP;
9. launches the application with `MW_APPDATA_DIR` redirected into the report directory;
10. waits for the production main window, allows the normal render loop to run briefly, sends the real application Exit command and requires clean startup/shutdown log markers;
11. discovers machine-wide, per-user or `PATH`-provided Inno Setup, records installer production, and verifies the artifact is non-empty with matching file version, SHA-256 and explicit Authenticode status;
12. writes `report.md`, `report.json`, `validation.log` and isolated app data under `artifacts/windows-validation/<timestamp>/` by default.

The current bounded installer-production evidence is `artifacts/windows-validation/20260909-ph11-installer-final/`. Its machine-readable report owns the exact artifact size and SHA-256. The validator requires version 1.13.1 and records Authenticode status explicitly; signing requires separate certificate authority and is not implied by this build.

`WindowsInteractionFixture` launches only the just-built application with a unique isolated app-data directory. It drives the real Palette/Equation command routes, changes the exact camera while each editor remains open, verifies Automatic preview precision changes with the live camera, accepts an editor setting, checks camera retention and structural history labels, exercises Equation Undo/Redo, verifies built-in preset Load/Undo/Redo with exact camera and selected-identity checks, verifies Settings accept/Undo/Redo with main-window camera-control synchronisation, verifies exact Journey waypoint accept/Undo/Redo by reopening the real dialog, submits a production-serialised preset through the native Open dialog before exact Import/Undo/Redo camera and identity checks, runs production Fractal Scout candidate-selection/Close isolation before exact camera-only Apply/Undo/Redo checks, and drives Animation Timeline add/current-value, scrub/play/stop, Cancel rollback and runtime-only OK/reopen state. It waits for complete dialog construction and the post-dialog main-window commit signal before asserting state. It also applies fixture-owned static/slideshow images, checks exhaustion and invalid MP4 rejection, and optionally verifies supplied MP4 mute/loop and injected failure stop/detach. It exercises the production display-change handler and simulated owned-host loss, checks native tab stops/button names, and optionally runs a bounded resource check. It requires normal shutdown; failure cleanup may terminate only its own child process. Foreground/desktop occlusion and adaptive pausing are disabled in fixture settings for deterministic media timing.

The isolated startup smoke does not load or overwrite the normal `%LOCALAPPDATA%\MandelbrotLiveWallpaper\settings.json` file. Window discovery enumerates top-level windows and filters by the launched process ID and exact class name; the global `FindWindowW` lookup is not reliable evidence on every Windows host.

## Manual release matrix

Automated startup smoke is necessary but not sufficient. Record each case as Passed, Failed, Skipped or Unproven with OS, GPU, driver, display topology and observed result.

### Windows and renderer

- [ ] Windows 10 22H2, D3D11 default.
- [ ] Supported Windows 11 environment, D3D11 default.
- [ ] OpenGL fallback or explicit OpenGL path.
- [ ] Integrated GPU where available.
- [ ] Dedicated GPU where available.
- [ ] Runtime D3D11 and OpenGL shader compilation produces no unexplained error.

### Main workflows

- [ ] Startup, visible preview and clean exit.
- [ ] Settings dialog.
- [ ] Equation editor.
- [ ] Palette editor.
- [ ] Journey settings.
- [ ] Quick Controller.
- [ ] Fractal Scout search, cancel, preview and non-mutating close.
- [ ] Slideshow editor.
- [ ] High-resolution render dialog and cancellation.
- [ ] Static capture and configured image encoding.

### Desktop modes and reversibility

Before testing, record the existing Windows wallpaper configuration. After each mode is stopped and after application exit, confirm that configuration is still present.

- [ ] None.
- [ ] Static.
- [ ] Slideshow, advancement and all-files failure.
- [ ] Exported local MP4, invalid-file rejection, verified mute/loop and asynchronous failure stop/detach.
- [ ] Pause/resume/stop from main window, Quick Controller and tray where applicable.

### Display and lifecycle

- [ ] 100%, 125% and 150% DPI.
- [ ] Mixed DPI.
- [ ] Monitor left/above primary, including negative coordinates.
- [ ] Mirror and Span modes; Independent was removed by DEC-040.
- [ ] Explorer restart.
- [ ] Display disconnect/reconnect.
- [ ] Lock/unlock.
- [ ] Sleep/wake.
- [ ] Remote Desktop connect/disconnect.

### Packaging and migration

- [ ] Portable ZIP starts from a fresh directory.
- [ ] Installer installs for the current user.
- [ ] Upgrade from the preceding accepted release preserves settings and presets.
- [ ] Uninstall removes installed files without deleting user-created outputs.
- [ ] A malformed settings file is quarantined/recovered according to the persistence contract.

## Bounded PH-11 evidence and reproduction

BR-20260923-03 records Windows 10 22H2 build 19045.6466, AMD Radeon RX 7900 XT, and two 100%-scaled monitors including negative X coordinates. A synthetic `WM_DISPLAYCHANGE` and simulated owned-window host loss pass; physical reconnect and Explorer process restart remain unproven. Native tab-stop/name checks do not prove screen-reader, high-contrast or full keyboard operation.

```powershell
$env:MW_TEST_VIDEO_WALLPAPER_PATH=(Resolve-Path artifacts/p9-v7/verified-fixture.mp4).Path
.\build\Release\MandelbrotWindowsInteractionFixture.exe .\build\Release\MandelbrotWallpaper.exe .\artifacts\ph11-normal
$env:MW_TEST_SOAK_SECONDS='60'
.\build\Release\MandelbrotWindowsInteractionFixture.exe .\build\Release\MandelbrotWallpaper.exe .\artifacts\ph11-soak
Remove-Item Env:MW_TEST_SOAK_SECONDS
.\scripts\validate-isolated-installer.ps1 -ReportDirectory artifacts/ph11-installer-fresh
```

Run fixtures serially on the interactive desktop. The resource duration accepts 0–3600 seconds; longer runs use direct execution because CTest has a 150-second interaction timeout. Do not close pre-existing user applications automatically. The installer validator uses a new AppId/name/directory and isolated settings; it proves install, launch, running-app refusal, same-version reinstall and uninstall with preservation of an untracked output. It does not prove upgrade from the preceding accepted release.

## Completion boundary

PH-01 remains incomplete until AC-003–AC-006 have appropriate native evidence. The automated report proves only its named build, test, metadata, package and isolated startup/shutdown checks. It does not prove every dialog, desktop mode, GPU, display, lifecycle, installer or upgrade case.
