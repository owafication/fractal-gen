# Validation and Evidence

**Status:** Current canonical validation policy and scoped evidence ledger  
**Purpose:** Own checks, evidence levels, reports, environment metadata and claim boundaries  
**Owner:** Quality and release engineering  
**Reading trigger:** Before reporting completion, fixing regressions or changing rendering/persistence/release behaviour  
**Update trigger:** Test surface, release matrix, threshold or evidence change

## Exclusive editor-window state — BR-20260924-02

- Direct MSVC `/c` compilation passed for `src/App/AppWindow.cpp`, `src/App/SettingsDialog.cpp` and `src/Tools/WindowsInteractionFixtureTool.cpp`; a second `/W4 /WX` pass also passed. The newly compiled AppWindow and fixture objects were manually linked with the existing Release objects and core library; the Settings dialog was compile-checked separately.
- The editor-only native fixture passed in the manually linked binaries: Palette, Equation and Settings each disabled the main window, blocked a competing production route for 750 ms, then restored the owner after close. The fixture process shut down cleanly and used isolated application data.
- The normal CMake/MSBuild build was attempted and failed before compilation with Visual Studio FileTracker `E_ACCESSDENIED`; the prior native CTest result is not reused as proof for this source change. The broad desktop fixture was also not promoted because the restricted host could not expose Progman.
- This proves the three named editor routes and their owner gate only. Journey, Timeline, Scout, preset, render/export, file-picker and Quick Controller combinations remain source-reviewed or unproven in this batch. No full CTest, release, accessibility, physical lifecycle or package claim is made.
- [Batch ledger](../artifacts/ph11-20260924-02/report.md) records the exact commands, reports and rollback material.

## File-backed desktop export pause — BR-20260924-01

- Native MSVC Release build and CTest **8/8** passed in **154.86 seconds**; the interaction fixture passed in **138.97 seconds**, including all 24 modal pause cases and the existing WIC output/refusal/cancellation matrix.
- Initial regression failed because synthetic suspend/resume released the running static image's export hold. The first corrected run passed the 24 cases but failed on rapid frame-dialog reopen; the final correction defers that queued reopen until the previous modal loop returns.
- Evidence: [ledger](../artifacts/ph11-20260924-01/report.md), [final interaction report](../artifacts/ph11-20260924-01/interaction-final/report.md), [CTest](../artifacts/ph11-20260924-01/ctest.log).
- Tests use owned processes and isolated application data. Power events are process-scoped messages, not physical host suspension. Native status/host checks do not prove visual timing or video position. Active export jobs with a running desktop, physical lifecycle, broader DPI/accessibility, external FFmpeg encode/cancel opt-ins, package/installer and soak were not validated in this batch. No phase or release gate is declared complete.

## PH-11 desktop and installer hardening — BR-20260923-03

- [Evidence ledger](../artifacts/ph11-20260923-03/report.md), [native interaction/resource report](../artifacts/ph11-20260923-03/interaction-soak/report.md), [installer report](../artifacts/ph11-20260923-03/installer/report.md), and [Release CTest log](../artifacts/ph11-20260923-03/ctest.log).
- Native MSVC Release build and cumulative CTest 8/8 passed; interaction used the existing verified MP4. Static presentation, ten-second slideshow advancement, all-files exhaustion, invalid MP4, MFPlay mute state, completed loop and injected asynchronous failure stop/detach passed. Video readiness now gates repaint and pause/resume; a loop marker follows completed restart rather than seek submission.
- The production `WM_DISPLAYCHANGE` handler rebuilt layout on two 100%-scaled monitors, one with negative X coordinates. Reparenting only the owned wallpaper window caused production host reattachment. No physical reconnect or Explorer process restart was performed.
- Native tab-stop/button-name inspection is bounded structural runtime evidence, not screen-reader, high-contrast or complete keyboard-traversal proof.
- Requested static resource interval: 60 seconds; total fixture duration: 109.091635 seconds. Working-set delta: -7,614,464 bytes; handles: -111; GDI: 0; USER: -2. Endpoint bounds are 64 MiB, 64 handles, 32 GDI and 32 USER objects. This is neither peak-resource profiling nor long-duration leak proof.
- An isolated test AppId/name/directory passed install, production launch/Exit, running-application uninstall refusal, same-version 1.13.1 reinstall and uninstall. Settings and an untracked output survived; the executable, registration and test shortcut were removed. Normal application data retained all 132 file hashes, and wallpaper registry configuration was unchanged from the pre-installer snapshot through final tests.
- Initial restricted build/GPU queries lacked Windows access; restricted desktop testing could not find Progman. Two normal-desktop runs reproduced a valid-video startup failure before the readiness fix. These failed attempts remain in the evidence directory; final runs passed. Expected injected failures are successful containment tests, not unresolved production-test failures.
- Windows 11, other GPUs/drivers, mixed DPI, physical reconnect, actual Explorer restart, lock/sleep/RDP/device-loss lifecycle, full accessibility, preceding-release upgrade, normal-AppId install, signing and long-duration soak remain unproven. Real FFmpeg encode/cancel opt-ins were not rerun; their historical evidence remains separately scoped. PH-11 is not complete.

## Evidence vocabulary

- **Observation:** directly inspected source/file content.
- **Ran:** a command executed with recorded environment and output.
- **Passed/Failed:** only the named check and scope.
- **Verified:** multiple appropriate checks jointly support the stated claim.
- **Unproven:** required evidence has not been produced.
- **Historical report:** user-supplied prior record; do not treat as rerun evidence.

## Evidence inventory for this pack

### Inspected and changed in BR-20260727-02

- Version surfaces in CMake, manifest, Win32 resource, release script and installer.
- CMake test registration and Windows release packaging flow.
- `resources.rc` was aligned from 1.11.5 to 1.13.1.
- A reusable CMake consistency check was added and a deliberate 1.13.2 mismatch was confirmed to fail.

### Ran and passed in the portable environment

- `cmake -DROOT_DIR=<root> -DEXPECTED_VERSION=1.13.1 -P cmake/VerifyVersionConsistency.cmake`.
- `python3 scripts/verify-source.py`.
- GNU C++ 14.2.0 configuration and warnings-as-errors build through CMake 3.31.6.
- CTest: `MandelbrotCoreTests` and `VersionConsistency`, 2/2 passed.

### Additional BR-20260727-03 evidence

- Initialised the local working repository on `main` with baseline commit `8f82119` and no remote.
- Added and parsed the canonical Visual Studio 2022 configure/build/test presets.
- Added `MW_APPDATA_DIR` isolation and a dedicated path test.
- Added the report-producing Windows validation workflow and manual matrix.
- Reconfigured and rebuilt the portable warnings-as-errors targets; CTest passed 3/3 (`MandelbrotCoreTests`, `MandelbrotPathTests`, `VersionConsistency`).

### Additional BR-20260727-05 evidence

- Added platform-neutral `ProjectState` descriptors, snapshots, apply adapters and canonical fingerprint implementation.
- SHA-256 matched the standard empty-string, `abc` and multi-block vectors.
- Camera/palette round-trip tests preserved compensated low components and selected palette/post fields exactly.
- Canonical-state tests proved excluded metadata stability, render-affecting sensitivity, output-context sensitivity, negative-zero normalisation, invalid-enum/non-finite rejection and preservation of the authoritative 4,096-stop palette boundary.
- CPU visual fixtures emitted `mw-render-state-v1` canonical bytes and SHA-256 metadata; the deliberate mutation changed both fingerprint and pixels.
- Added transactional parameter-mutation tests covering model normalisation, invalidation aggregation, no-op batches, duplicate/type/non-finite rejection and unrelated-authority preservation.
- Migrated main-window brightness, contrast, saturation and colour offset; source verification rejects direct `workingPreset_` assignments for those fields.
- Migrated built-in palette combo selection through a discrete transaction with unsupported-value rejection, actual-change-only custom-stop clearing, palette invalidation, explicit origin/future-history eligibility and scalar-history deferral when a custom-stop subtree is removed.
- GNU 14.2 and Clang 17 warnings-as-errors builds each passed CTest 4/4 for the final PH-03 implementation, including the visual fixture self-check. A bounded Clang AddressSanitizer/UndefinedBehaviorSanitizer run passed core, path and version tests 3/3 with visual fixtures disabled. Native MSVC evidence for this exact final archive remains open.
- User-reported evidence: the preceding PH-03 palette/origin archive compiled, built and passed GPU tests on Windows. Commands, logs, compiler identity, adapter details and runtime output were not supplied, so this does not independently verify the final gesture/replacement archive.
- All four fixture `render-state.canonical` artifacts were byte-identical between GNU and Clang; their SHA-256 digests matched the recorded fingerprint values.
- AddressSanitizer/UndefinedBehaviorSanitizer core and path tests passed, and a bounded standard-fixture repeatability run passed without reported findings; the full sanitizer visual-fixture CTest exceeded the available timeout and is not claimed.
- Native MSVC and pending PH-02 GPU/baseline checks remain unproven and non-blocking only for implementation sequencing under DEC-017.
- BR-20260728-03 portable evidence: GNU and Clang warnings-as-errors builds each passed CTest 4/4; the bounded Clang AddressSanitizer/UndefinedBehaviorSanitizer suite passed 3/3. Native Windows verification for this exact palette/origin slice remains open.
- BR-20260728-04 portable evidence: gesture-token, 250 ms coalescing-window, replacement no-op/normalisation/rollback and origin-kind tests passed; GNU and Clang warnings-as-errors CTest passed 4/4, the bounded sanitizer suite passed 3/3, and source-audit/fresh-package results are recorded in the delivery report. Native Windows verification for this exact final slice remains open.
- BR-20260728-05 PH-04 portable evidence: typed camera/palette history, exact undo/redo, camera metadata restoration, drag and palette-thumb coalescing, rotation replay, redo-branch truncation, runtime exclusion, custom-stop partial-history prevention and configured bounds pass in core tests. GNU and Clang warnings-as-errors CTest pass 4/4; a bounded Clang AddressSanitizer/UndefinedBehaviorSanitizer run passes 3/3 with visual fixtures disabled. Native Windows compilation and UI interaction for this archive remain open.
- BR-20260728-06 PH-05 portable evidence: atomic palette-stop/equation/journey/preset replacements, scalar-completeness fallback, post-processing inclusion, structural runtime exclusion, oversized-entry rejection and one-entry Scout Apply exact replay pass in core tests. Fresh GNU 14.2 and Clang 17 warnings-as-errors builds each pass CTest 4/4 including `VisualFixtureSelfCheck`; a bounded Clang AddressSanitizer/UndefinedBehaviorSanitizer build passes 3/3 with visual fixtures disabled. `python3 scripts/verify-source.py` passes all historical gates through PH-05. Native MSVC compilation and Windows UI interaction for this archive remain open.
- BR-20260728-06 package evidence: the generated source ZIP was extracted into a fresh directory; source verification passed and a clean GNU warnings-as-errors CTest run passed 4/4.

### Still not run

- Native MSVC resource compilation/linking.
- Built Windows executable metadata verification.
- Shader compilation/runtime rendering.
- Windows UI/desktop integration.
- Portable ZIP or installer generation and upgrade checks.

### Historical records

`docs/VERIFICATION-1.13.0.md` and `docs/VERIFICATION-1.13.1.md` report GCC/Clang/sanitizer checks in their prior environment and explicitly leave native MSVC unproven. This pack preserves that distinction.

## Validation layers

1. **Static structure:** file presence, symbols, banned patterns, version scan. Does not prove compilation.
2. **Portable core:** GCC/Clang build, warnings, CTest, sanitizers. Does not prove Win32/UI/GPU.
3. **Native Windows build:** MSVC resources, compile and link. Does not prove runtime behaviour.
4. **Runtime smoke:** launch, dialogs, renderer initialisation and shutdown.
5. **Functional integration:** desktop modes, persistence, multi-monitor, system lifecycle, jobs and cancellation.
6. **Visual evidence:** deterministic fixtures, backend metrics, seam strips and approved changes.
7. **Packaging/release:** package contents, integrity, install/upgrade/uninstall and version metadata.

## Required baseline commands

Exact commands may be adapted to the environment, but reports must record the resolved command and tool versions.

```powershell
python .\scripts\verify-source.py
.\scripts\validate-windows-release.ps1
```

The validation workflow resolves to the `windows-msvc-release` configure/build/test presets, executable metadata checks, portable package inspection and isolated startup/shutdown smoke. Its report still requires the manual matrix in `docs/testing/WINDOWS-RELEASE-VALIDATION.md` for complete PH-01 evidence.

The release script removes and recreates `build/`; do not run it with uncommitted generated artifacts stored there.

## Windows runtime matrix

Minimum release-facing matrix:

- Windows 10 22H2 and supported Windows 11 environment;
- integrated and dedicated GPU where available;
- D3D11 default, OpenGL fallback and explicit backend selection for high-resolution rendering;
- 100%, 125%, 150% and mixed DPI;
- one monitor plus representative Mirror/Span arrangements, including negative coordinates; Independent was removed by DEC-040;
- Explorer restart, display reconnect, lock/unlock, sleep/wake and Remote Desktop;
- static/slideshow/exported-MP4 start, pause, resume and stop; verify no desktop action or failure path enters fractal rendering;
- Settings, Equation, Palette, Journey, Quick Controller, Scout, Slideshow and High-Resolution dialogs;
- installer/portable package and upgrade path.

Current scope authority is DEC-039/REQ-041/VAL-061. Fractal renderer and animation validation applies to preview and explicit output jobs only. Desktop validation covers WIC-decoded static/slideshow files and Media Foundation playback of an already-exported local MP4, including proof that failure and lifecycle routes never start a fractal renderer. Earlier batch evidence below that names wallpaper GPU release records the implementation at that historical checkpoint and does not define the current desktop contract.

## Visual evidence

Owned by `docs/testing/VISUAL-REGRESSION.md`. Every fixture artifact identifies:

- application/source revision;
- backend/device/driver or WARP identity;
- dimensions and tile layout;
- precision policy and selected strategy;
- anti-aliasing and post-processing;
- fixed time and seed;
- canonical render fingerprint;
- metrics and baseline identity.

## Report format

```text
Report ID
Objective and affected IDs
Environment/toolchain
Inspected
Changed
Commands run
Passed
Failed
Skipped
Visual/migration artifacts
Known limitations
Unproven items
Rollback point
Next decision
```

## Audit levels

- **Batch:** current delivery paths, links, ownership, IDs and contradictions.
- **Cumulative:** all visible accepted material to date.
- **Final:** complete visible pack and required evidence.

BR-20260727-02 adds a repository-execution batch audit for the version fix and portable checks. BR-20260727-03 adds the Git baseline and repeatable Windows evidence workflow. BR-20260727-04 adds the portable CPU production-renderer fixture command and recorded VAL-009/VAL-012/VAL-013 evidence within the same-process GNU scope. BR-20260727-05 adds portable VAL-014 evidence and canonical-fingerprint checks. BR-20260728-01 adds bounded VAL-016 evidence for the migrated main-window scalar domain. BR-20260728-02 extends that bounded VAL-016 evidence to deliberate camera edits. BR-20260728-03 extends it to built-in palette selection and explicit mutation-origin/history-eligibility reporting. BR-20260728-04 completes the inspected PH-03 mutation routes with preview gesture coalescing and classified whole-preset replacement. BR-20260728-05 adds bounded PH-04 camera/palette history evidence. BR-20260728-06 adds PH-05 structural history and Scout Apply evidence while leaving native Windows verification and PH-02 baseline approval open. No cumulative repository audit or final product verification is claimed.

## PH-06 portable evaluator evidence — BR-20260728-07

- Source audit checks the platform-neutral timeline model, stable target registry, immutable evaluation contract, compensated/log/angle interpolation, loop modes, bounded IDs/resources, independent clocks and documentation boundary.
- GNU warnings-as-errors Release build and CTest cover the evaluator through `MandelbrotCoreTests`; the production CPU visual fixture self-check remains unchanged and passing.
- Clang warnings-as-errors and sanitizer evidence are recorded in the delivery report when run for the final archive.
- AC-014 and VAL-023–VAL-026 pass in the recorded portable/core/structural scope. Native MSVC compilation and Windows timeline-dialog interaction for the exact PH-07 archive remain pending.

## PH-07 validation boundary

- Portable core tests prove strict Journey parsing, deterministic one-pass camera-track construction, exact hold evaluation, supported round trips, explicit unsupported/lossy refusal, authoritative Add Current Value and duplicate-time rollback.
- Source audit proves the editor source is linked, exposes track/keyframe/duration/loop/scrub/playback/conversion controls, uses `AnimationClockDomain::Preview`, renders the evaluator's frame-local `Preset`, does not add a schema field and applies accepted Journey conversion through the existing atomic replacement boundary.
- These checks do not prove MSVC compilation, dialog creation, DPI layout, keyboard navigation, visual preview correctness or user interaction on Windows. Those remain native smoke requirements.

## PH-08 deterministic frame-sequence evidence — BR-20260729-02

- Portable core tests exercise direct rational frame timing/counts, immutable fingerprint stability, early cancellation, verified-output preservation, manifest resume, full typed mismatch refusal, malformed-entry rejection and deterministic selected production CPU frames.
- The source audit checks that the Win32 PNG route is linked, uses the production tiled renderer with adaptive quality disabled, writes through a reusable WIC row encoder, validates dimensions, joins its worker, exposes progress/cancel/resume controls, and pauses/releases active wallpaper GPU resources around the modal route.
- GNU and Clang warnings-as-errors builds, production visual fixture self-check, and Clang ASan/UBSan results are recorded in BR-20260729-02 for the final package.
- AC-015, AC-016 and VAL-027–VAL-030 pass only in the recorded portable/core/structural scope. Native Visual Studio 18 2026 compilation, WIC PNG output, Win32 dialog interaction and wallpaper lifecycle behaviour remain unproven until run on Windows.
- User-reported evidence: the preceding PH-07 archive builds and runs. This does not verify PH-08 Windows-specific changes.

## PH-08 native compile correction — BR-20260729-03

- User-reported native evidence: Visual Studio 18 2026 / MSVC 19.51.36248.0 configured successfully and built the portable/core targets, then failed compiling `src/App/HighResRenderDialog.cpp` because `COINIT_MULTITHREADED`, `CoInitializeEx` and `CoUninitialize` were undeclared.
- Implemented correction: `HighResRenderDialog.cpp` now explicitly includes `<objbase.h>` under `_WIN32`; `ole32` was already linked and was not changed.
- Source audit verifies the explicit declaration dependency. Portable builds cannot prove the Win32 compile correction; a user-side native rebuild is still required before the exact PH-08 package can be called natively compiled.

## BR-20260729-04 — PH-09 external video export evidence

### Inspected and changed

- Added platform-neutral external-video job/capability/cleanup contracts and Win32 owned-process execution.
- Added a modal MP4 export route over verified PH-08 sequences with explicit executable selection or `PATH` discovery.
- Accepted DEC-009 and DEC-027 without changing settings schema 9 or bundling/downloading/persisting FFmpeg.
- Recorded the user's report that the corrected PH-08 archive builds and runs on Windows as baseline evidence only.

### Portable evidence

- `scripts/verify-source.py` passes all historical source gates through PH-09 and requires capability checks, fixed-vector no-shell invocation, owned cancellation, verified promotion and manifest-scoped cleanup markers.
- Core tests cover capability parsing, missing-encoder rejection, complete digest-matched source validation, fixed encode/decode vectors, successful promotion, process failure, cancellation and optional cleanup containment.
- GNU 14.2 Release warnings-as-errors build with production visual fixtures passes CTest 4/4.
- Clang 17 Release warnings-as-errors build with production visual fixtures passes CTest 4/4 after correcting one explicit `wchar_t`/`wint_t` conversion.
- Clang 17 AddressSanitizer/UndefinedBehaviorSanitizer Debug build with visual fixtures disabled passes CTest 3/3.
- Final clean-package rebuild evidence is recorded in the delivery report for the exact PH-09 archive.

### Unproven for PH-09

- Visual Studio 18 2026 compile/link of the new Win32 dialog and process runner.
- Real FFmpeg version/encoder/muxer capability probes.
- H.264/MP4 encode and decode-probe behaviour with a real executable.
- Win32 progress, cancellation, DPI/keyboard interaction and wallpaper pause/resume around the video route.
- Behaviour across different third-party FFmpeg builds and their attached licence configurations.

## Current native Release build evidence — BR-20260729-05

- `build/CMakeCache.txt` records the Visual Studio 18 2026 generator, x64 platform, `MW_BUILD_TESTS=ON` and `MW_WARNINGS_AS_ERRORS=ON`.
- The current `build/Release` contains `MandelbrotWallpaper.exe`, `MandelbrotCoreTests.exe`, `MandelbrotPathTests.exe` and `MandelbrotVisualFixtures.exe`, timestamped 2026-07-29.
- `build/Testing/Temporary/LastTest.log` records native Release CTest 4/4 passed: `MandelbrotCoreTests`, `MandelbrotPathTests`, `VisualFixtureSelfCheck` and `VersionConsistency`.
- This is direct evidence that the current project build compiled and linked the affected native targets and passed the recorded automated tests through PH-09. It does not prove manual Windows runtime behaviour, GPU backend coverage, dialog/DPI/keyboard interaction, cancellation, wallpaper lifecycle, installer/upgrade behaviour or real FFmpeg execution.

## PH-01 runtime smoke attempt — BR-20260729-06

- Ran `scripts/run-current-build-runtime-smoke.ps1` against the existing `build/Release/MandelbrotWallpaper.exe` with isolated `MW_APPDATA_DIR`.
- The application started and logged `Application startup.` followed by `Direct3D 11 renderer started: AMD Radeon RX 7900 XT / Direct3D 11.1`.
- The canonical `MandelbrotLiveWallpaperControl` window was not discoverable within 30 seconds, so the real Exit command could not be delivered and clean shutdown was not observed.
- The isolated log contains startup and D3D11 initialization entries but no `Application shutdown.` entry. PH-01 runtime smoke is therefore failed/unproven and later canonical runtime gates are not started.

## PH-01 corrected isolated runtime smoke — BR-20260729-07

- The diagnostic run enumerated a visible `MandelbrotLiveWallpaperControl` titled `Mandelbrot Live Wallpaper`, owned by the launched process, after the D3D11 start log.
- The earlier failure was caused by the helper's global `FindWindowW` lookup returning zero despite the process-owned top-level window. It was not a Direct3D 11 initialization failure.
- The helper now resolves the expected class through `EnumWindows` filtered to the launched process ID. The corrected smoke passed with exit code 0 and one isolated log containing both `Application startup.` and `Application shutdown.` markers.
- PH-01 remains incomplete pending its manual Windows, package, installer and upgrade matrix; the isolated startup/shutdown/D3D11 gate is passed.

## PH-01 portable package smoke and PH-02 native CPU fixtures — BR-20260729-08

- Inspected `dist/Mandelbrot-Live-Wallpaper-1.13.1-win-x64.zip`; it contains the executable, README and license, and no settings, logs, build, dist, PDB, ILK, object or user artifacts. SHA-256: `31fe3c222c81740ec6a500e4597143a15485a4ff27bf2dcb27ffe8a9d173f622`.
- Extracted the portable ZIP into a fresh temporary directory and ran the isolated process-scoped startup/window/D3D11/clean-shutdown smoke against its executable. It passed with exit code 0 and one isolated log containing both startup and shutdown markers.
- Ran `build/Release/MandelbrotVisualFixtures.exe` with a fresh temporary artifact directory. All six native CPU production-renderer checks passed: four exact repeatability fixtures, tiled/full seam equality and deliberate-mutation detection.
- PH-02 remains incomplete because approved CPU baselines, D3D11 WARP/offscreen fixtures, supported OpenGL offscreen fixtures and backend-specific bloom/perturbation validation are not implemented. Fresh MSVC evidence is recorded separately in BR-20260729-09; no GPU fixture implementation or compile claim is made.

## Fresh MSVC Release rebuild and CTest — BR-20260729-09

- Activated Visual Studio 18 Build Tools x64 through the discovered `vcvars64.bat`, then rebuilt the current `codex-msvc-release` preset outside the Codex filesystem sandbox. The sandbox-only attempt failed before compilation in MSBuild FileTracker with `E_ACCESSDENIED`; it is not a project build failure.
- The fresh build compiled and linked `MandelbrotWallpaper.exe`, `MandelbrotCoreTests.exe`, `MandelbrotPathTests.exe` and `MandelbrotVisualFixtures.exe`.
- `ctest --test-dir build-codex-runtime -C Release --output-on-failure` passed 4/4: `MandelbrotCoreTests`, `MandelbrotPathTests`, `VisualFixtureSelfCheck` and `VersionConsistency` (22.80 seconds total).
- This closes PH-02's native-MSVC-evidence sub-gate. It does not prove D3D11 WARP/offscreen, OpenGL offscreen, GPU bloom/perturbation, approved visual baselines or any manual phase gate.

## D3D11 WARP production readback fixture — BR-20260729-10

- Added a test-only WARP option to the production `Direct3D11Renderer`; application callers retain the hardware default.
- Added `MandelbrotD3D11WarpFixtures`, which creates a hidden `CS_OWNDC` window, initializes the production Direct3D 11 renderer with WARP, renders a fixed 256x144 default scene twice at time zero, reads back the final output texture, requires correct dimensions and non-uniform pixels, and requires exact repeatability.
- Fresh Visual Studio 18 x64 Release compilation linked the fixture. Direct execution passed. Isolated CTest `D3D11WarpFixtureSelfCheck` passed 1/1 in 9.19 seconds.
- This is bounded same-process WARP evidence only; it does not establish a reviewed baseline, hardware D3D11 equivalence, OpenGL support, or bloom/perturbation coverage.

## D3D11 WARP bloom and perturbation expansion — BR-20260729-11

- Expanded `MandelbrotD3D11WarpFixtures` with fixed bloom-enabled and explicit-perturbation scenes. The bloom scene enables the production bright-pass blur; the perturbation scene uses a fixed deep camera and `PrecisionMode::Perturbation`.
- Direct execution passed with exact, non-uniform repeated readbacks for standard, bloom and perturbation scenes at 256x144.
- Isolated `D3D11WarpFixtureSelfCheck` passed 1/1 in 9.32 seconds.
- This confirms execution and deterministic readback of those WARP production paths only. Reviewed image baselines, hardware D3D11 and OpenGL fixtures remain separate gates.

## D3D11 WARP reference-orbit texture transport — BR-20260731-01

- The deterministic WARP fixture now reads the production immutable real/imaginary `R32G32B32A32_FLOAT` reference-orbit textures through staging resources after the fixed perturbation render.
- It constructs the canonical CPU `BuildReferenceOrbitDouble` result for that scene and requires every uploaded four-float real/imaginary component to round-trip bit-for-bit. The isolated CTest `D3D11WarpFixtureSelfCheck` passed 1/1 in 9.51 seconds.
- This is a transport-retention measurement. It deliberately does **not** establish a numerical orbit-encoding ceiling, shader reconstruction error, correction validity, hardware equivalence, plan-metadata binding or the end-to-end direct-reference comparison required by VAL-051.

## Independent float4 reference-orbit payload comparison — BR-20260731-02

- Core coverage now compares every real and imaginary component of the four-float payload, rather than collapsing the expansion to `double`, at selected Mandelbrot and exact power-2 Tricorn reference-orbit iterations.
- The existing fixed-point arbitrary route and the independent Boost.Multiprecision 512-bit route produced identical IEEE-754 float payloads and matching escape metadata for the selected bounded fixtures. `MandelbrotCoreTests` passed in 2.30 seconds.
- This is VAL-048 selected-sample evidence for the current `mw-orbit-float4-expansion/v1` producer. It is not a maximum-error/precision-ceiling measurement, GPU shader validation, a different backend implementation, arbitrary-camera proof or a correction-validity result.

## Reference-orbit cache boundary enforcement — BR-20260731-03

- Core coverage now creates a cache sized for one 64-iteration reference orbit, verifies deterministic eviction on a distinct exact-camera key, verifies that the evicted key rebuilds rather than reports a stale cache hit, and verifies refusal with no cache publication when the byte budget is one byte too small.
- `MandelbrotCoreTests` passed in 2.28 seconds.
- This proves byte/entry-bound enforcement for the service contract. It does not calibrate default limits against real workloads, measure worker queue pressure, establish long-session memory behaviour or prove renderer upload lifetime.

## Reference-orbit worker shutdown and queue boundary — BR-20260803-07

- `ReferenceOrbitWorker::Shutdown` now refuses future enqueues, cancels current subscribers, joins its owned thread, and clears active/pending work before it returns. A second caller waits for the same stopped state; completion callbacks are explicitly prohibited from synchronously shutting down their own worker thread.
- Core coverage proves equal-key coalescing/generation delivery, stopped-worker enqueue refusal without callback delivery, zero-capacity independent-key refusal without retention, and zero pending keys after shutdown. Native Release `MandelbrotCoreTests` passed 1/1 in 2.70 seconds.
- This is a bounded worker ownership result. It does not measure pressure at the default queue depth, prove cancellation timing under a long numerical computation, establish allocation-failure behaviour, renderer upload lifetime, GPU resource teardown, or long-session resource use.

## Reference-orbit immutable provenance binding — BR-20260803-08

- `PrecisionPlan` now carries the stable `mw-precision-plan-v1` identity. Every reference-orbit request identifies the current named float4 encoding; the service validates both identities, includes them in cache/coalescing keys, and returns them with the formula capability in every result.
- Core coverage proves successful result provenance and fail-closed rejection of substituted plan or encoding versions without cache mutation. Native Release `MandelbrotCoreTests` passed 1/1 in 2.80 seconds.
- This is request/cache provenance evidence only. It does not bind an orbit result to a live renderer upload, validate GPU transport/execution, establish a future-version migration policy, or make the current float4 encoding safe for deep GPU output.

## Reference-orbit per-subscriber plan delivery — BR-20260803-09

- `ReferenceOrbitResult` now carries the complete immutable `PrecisionPlan`. Coalesced worker requests share their numerical orbit only; on delivery, each result is restored to that subscriber's plan before its generation is stamped.
- Core coverage coalesces two numerical requests with distinct `requiresDirectCorrection` values and verifies that each generation receives its own policy. Native Release `MandelbrotCoreTests` passed 1/1 in 2.71 seconds.
- This prevents policy conflation at the service boundary. It does not perform direct correction, bind an accepted result to a renderer generation/upload, validate GPU execution, or establish correction thresholds.

## Reference-orbit subscriber resource bound — BR-20260803-10

- `ReferenceOrbitWorkerLimits` now caps subscribers retained by one coalesced numerical key (64 by default), closing the unbounded callback-vector path that remained after key-queue bounding. A zero or exhausted cap refuses the request before a key or callback is retained.
- Core coverage proves zero-cap rejection and no pending-key retention. Native Release `MandelbrotCoreTests` passed 1/1 in 2.71 seconds.
- This proves configuration-bound refusal only. It does not calibrate the default limit under interactive pressure, exercise an exhausted non-zero cap while work is active, prove cancellation latency, or establish long-session memory behaviour.

## Ultra-deep schema-3 exact-camera reload — BR-20260803-11

- The one-way exact-to-legacy adapter now maps a positive half-height that underflows double to `denorm_min` and marks loss, while retaining the canonical exact text. Model normalisation recognises that a schema-3 exact camera is authoritative and does not reject the derived small legacy scale.
- Core coverage directly checks the marked `1e-1000` adapter result and serialises/reloads a schema-3 preset at that depth with exact-camera equality intact. Native Release `MandelbrotCoreTests` passed 1/1 in 2.69 seconds.
- This establishes bounded persistence/adaptation only. It does not make the legacy renderer capable of that scale, implement exact camera edits/animation authority everywhere, validate a deep GPU path, or prove still/frame/video output beyond the existing direct CPU route.

## Exact-camera transaction boundary — BR-20260803-12

- Added `ApplyProjectExactCameraMutation`, which accepts canonical exact camera values, derives the legacy adapter only after that authority is fixed, and commits one camera invalidation/history-eligible transaction. Invalid exact camera input leaves the preset unchanged.
- The native main-window camera route now uses this core transaction whenever a caller supplies parsed exact coordinates; only legacy-only gestures retain the scalar compatibility transaction. Core coverage commits `1e-1000` exact text unchanged and rejects a negative half-height transaction atomically. The Release core and Win32 application targets compiled; `MandelbrotCoreTests` passed 1/1 in 2.80 seconds.
- This proves the core/UI compile boundary. It does not prove manual coordinate-editor interaction, deep live preview, exact animation/Journey authority, or renderer/GPU correctness.

## Exact-camera history replay — BR-20260803-13

- Added core coverage for a `1e-1000` exact camera mutation recorded through `ProjectHistory`. The scalar history replay cannot reproduce the exact authority, so the existing coordinator correctly records an atomic full-preset replacement.
- Undo and redo both restore the exact pre/post presets and report a full render; the redo assertion checks the canonical exact camera text, not merely its legacy double adapter.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 14.12 seconds.
- This is core/native-build evidence only. Manual undo/redo UI interaction, exact animation/Journey authority, deep live preview, and renderer/GPU numerical correctness remain unproven.

## Immutable exact frame-plan hand-off — BR-20260803-14

- `FrameSequenceFrameRequest` now carries the plan produced after its exact frame camera and selected renderer pass planner validation. The Windows direct CPU frame callback consumes `selectedBits` from that request instead of independently mapping renderer text to a tier.
- Core coverage runs a `1e-1000` exact frame job and asserts that its callback receives `cpu-boost-8192-direct`, 8,192 selected bits and the current precision-plan version.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 15.81 seconds.
- This proves a core-to-Windows callback contract and a fixture encoder path only. It does not prove real frame/video output, exact animation authority, deep tiling, correction, GPU equivalence or runtime dialog interaction.

## Exact direct row-memory admission — BR-20260803-15

- `RenderExactDirectStillImage` now refuses an output row above 64 MiB before allocating it. This bounds its row-vector working allocation for direct exact still and frame rendering.
- Core coverage passes a width one pixel above the bound and confirms rejection before row allocation.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 15.93 seconds.
- This is an allocation admission boundary only; it does not establish an acceptable render duration, total output-disk budget, WIC runtime behavior or a measured deep support limit.

## Explicit perturbation sample validity — BR-20260803-16

- The platform-neutral perturbation result now classifies each sample as `Stable`, `Rebased`, or `Unresolved`. A successful reference refresh is `Rebased`; an instability with refresh disabled is `Unresolved`, so a future renderer cannot mistake it for a normal escape result.
- Existing stable and rebase fixtures now assert their classifications, and a no-refresh instability fixture asserts `Unresolved`.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 16.16 seconds.
- This is PH-14 core groundwork. It does not connect validity to production GPU colouring, implement direct correction, establish rebase thresholds, or prove a GPU deep frame.

## Exact direct global tile mapping — BR-20260803-17

- The direct exact CPU request can now carry full-frame dimensions and a tile origin. It constructs samples at global coordinates, so a crop has the same exact camera mapping as its corresponding full-frame pixels; an out-of-frame crop is refused.
- Core coverage renders a full 8×6 exact image and verifies each row of a 4×3 crop at origin (2,1) matches its non-contiguous global source rows exactly.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 15.57 seconds.
- This is a mapping contract only. No UI tile scheduler, large-image WIC streaming integration, rotation, correction or GPU equivalence is claimed.

## Deep exact cancellation before row publication — BR-20260803-18

- Core coverage cancels a `1e-1000`, 8,192-bit direct exact render during its first high-precision sample. The renderer returns cancellation and the row writer records zero output rows.
- This proves cancellation reaches the selected exact evaluator before a partially computed row is published.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 14.28 seconds.
- This does not prove WIC temporary-file cleanup, cancellation after a committed row, long-running resource behavior, tiled scheduling, or video output cancellation.

## Frame renderer route refusal — BR-20260803-19

- Immutable frame-job creation and manifest save/load now admit only the registered production CPU renderer and the three implemented exact CPU Boost tiers. Any other ID, including a prospective GPU identifier, is rejected before a manifest, output directory, or renderer callback can be created.
- The Windows frame callback also accepts the production Float64 CPU path explicitly and refuses any other non-exact ID, defending against a future validation regression instead of silently invoking the CPU renderer.
- Core coverage attempts `gpu-d3d11-validated-v1` and verifies both job construction and manifest rewriting fail with the deterministic-route refusal.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 15.56 seconds. `C:\Python314\python.exe .\scripts\verify-source.py` passed, including Boost.Multiprecision 1.83.0 integrity (178 files).
- This preserves the DEC-036 no-silent-fallback boundary. It does not implement a GPU frame renderer, expose a GPU choice in the UI, validate GPU equivalence, or prove runtime dialog/export behavior.

## High-resolution exact renderer status — BR-20260803-20

- The high-resolution dialog already routes a planner-selected direct exact CPU request away from legacy Float64/GPU execution. Its progress status now names the exact CPU direct evaluator instead of the compatibility tiled CPU renderer.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 13.75 seconds.
- This is native compile and regression-suite evidence for a Windows UI text branch. It does not prove manual dialog behavior, deep still output, encoder behavior, GPU parity, or performance.

## Deep camera animation refusal — BR-20260803-21

- Camera animation tracks currently contain double-valued keyframes. When a frame evaluation would change camera coordinates, it now first checks the immutable base exact camera adapter; any centre or half-height precision loss refuses evaluation rather than clearing exact authority and rebuilding from doubles.
- Core coverage evaluates a camera-track timeline from a `1e-1000` exact base and verifies deterministic refusal naming the missing exact-keyframe capability.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 15.63 seconds.
- This protects exact authority only. It does not implement exact keyframe storage/interpolation, deep Journey promotion, deep Scout search, GPU rendering, or runtime editor behavior.

## Deep Scout source refusal — BR-20260803-22

- Fractal Scout’s bounded candidate search currently derives every coordinate from `CameraState`. It now inspects a supplied exact source before work begins and refuses if adapting its centre or half-height loses precision, so approximate candidates cannot be promoted as a deep-exact result.
- Core coverage supplies a `1e-1000` exact source and verifies it fails before candidate search with the explicit missing exact-Scout-coordinate boundary.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 15.81 seconds.
- This retains ordinary bounded Scout behavior. It does not implement exact-coordinate candidate search/promotion, runtime UI behavior, GPU deep rendering, or measured support limits.

## Boost 16,384-bit direct exact tier — BR-20260803-23

- Added the bounded `CpuBoost16384Direct` planner tier and fixed 16,384-bit Boost.Multiprecision evaluator. The tier preserves canonical exact camera text through CPU still and immutable frame-export planning; it does not adapt deep coordinates through `CameraState`.
- High-resolution CPU and frame-export selection now identify `cpu-exact-boost16384-v1`. The route rejects unsupported rotation, animated coefficients and out-of-range anti-aliasing rather than falling back to a different renderer.
- Core coverage renders a `1e-3000` exact still, checks planner selection beyond 8,192 bits, and proves the frame callback receives the immutable 16,384-bit plan. `cmake --build build --config Release --parallel` passed; `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 18.69 seconds; `C:\Python314\python.exe .\scripts\verify-source.py` passed, including the pinned Boost package-integrity check.
- This adds another bounded direct CPU tier; it does not prove runtime UI flow, broad performance/resource thresholds, perturbation correction, GPU equivalence, exact animated keyframes, exact Scout, or completed video output.

## Planner-matched high-precision reference orbits — BR-20260804-01

- `ReferenceOrbitService` now admits only a matching immutable Boost CPU plan/tier: 512, 2,048, 8,192 or 16,384 bits. The numerical reference builder receives that tier rather than defaulting every exact request to 512 bits.
- Core coverage supplies a `1e-3000` exact camera with a planner-selected 16,384-bit service request and verifies that the result retains the matching tier and bounded 64-point orbit.
- `cmake --build build --config Release --parallel` passed. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 18.85 seconds.
- This preserves reference-generation precision only. The float4 transport has no validated deep GPU envelope, and no CPU/GPU perturbation or correction renderer consumes the service yet.

## Fixture-specific float4 orbit reconstruction measurement — BR-20260803-01

- Added `MandelbrotOrbitEncodingMeasurement`, a platform-neutral CTest fixture. It models the current D3D11/OpenGL perturbation coordinate reconstruction exactly as ordered float additions of the uploaded four components, then compares each real/imaginary coordinate with the independent Boost-512 producer.
- At camera `(-0.743643887037151 + 1e-24, 0.131825904205330 - 2e-24)`, 128 requested iterations produced: analytic quadratic Mandelbrot, 256 coordinates, maximum absolute error `3.4413387754277605e-08`, maximum relative error `5.7453155647240968e-08`; exact power-2 Tricorn, 28 coordinates before escape at iteration 13, maximum absolute error `1.3645269757469331e-06`, maximum relative error `5.3683366176295286e-08`.
- `MandelbrotCoreTests` and `OrbitEncodingMeasurementSelfCheck` passed 2/2 in 2.15 seconds.
- These are fixture-local producer/reconstruction observations. They do **not** validate device shader execution, compiler arithmetic reassociation, reference-times-delta arithmetic, correction, hardware drivers, arbitrary exact cameras or a supported encoding/deep-zoom ceiling.

## Unvalidated GPU orbit transport planner refusal — BR-20260803-02

- Core coverage now supplies a present GPU float4 transport with `gpuOrbitFloat4ValidatedBits == 0` to the platform-neutral planner. The planner selects the bounded CPU reference route, sets `requiresDirectCorrection`, and reports that the GPU orbit encoding has no validated precision envelope.
- `MandelbrotCoreTests` passed 1/1 in 2.37 seconds.
- This proves planner policy only. It does not add a GPU execution backend, enable correction, validate a transport envelope or prove any GPU frame/output.

## Exact direct CPU anti-aliasing — BR-20260803-03

- Added exact rational subpixel mapping for a regular 1–4 samples-per-axis grid. The prior centre-pixel API delegates to the same AA-1 mapping; a 2× fixture proves expected rational factors without binary coordinate conversion.
- `RenderExactDirectStillImage` now averages independently evaluated exact samples for AA 1–4. The high-resolution and frame-sequence UI labels/validation accept that bounded range, retaining CPU-only and zero-rotation requirements.
- `MandelbrotCoreTests` passed AA-2 deterministic repeated output and AA-4 upper-bound rendering. The native Release `MandelbrotWallpaper` target compiled and linked successfully.
- This does not validate deep tiled output, rotation, arbitrary AA grids beyond 4, GPU equivalence, correction, frame/video output or runtime dialog interaction.

## Exact frame-render preflight boundary — BR-20260803-04

- Core immutable job creation now rejects exact CPU renderer selection when the base preset has non-zero rotation or AA outside its bounded 1–4 range. The identical guard runs after each timeline frame evaluation before the renderer callback, so a later animated rotation cannot create that frame's temporary output.
- `MandelbrotCoreTests` passed the rotated exact-frame job refusal and a timeline-driven rotation fixture; the latter rejects frame 0 with zero renderer-callback invocations, in 2.81 seconds.
- This is a validation-boundary result only. It does not provide rotated exact rendering, pre-scan every future timeline frame, validate encoder/runtime files or prove video output.

## Central GPU compatibility precision policy — BR-20260803-05

- Moved the legacy GPU automatic threshold/fallback policy from `Direct3D11Renderer` and `OpenGLRenderer` to platform-neutral `PrecisionPlanner`. Both backends now provide only their Float64/split/perturbation/arbitrary-reference capability report.
- Core regression covers split-float selection without Float64, Float64 preference when reported, arbitrary-reference selection beyond split range, unsupported-formula Float64 fallback, and explicit incompatible perturbation refusal. A source audit confirms thresholds `1e6`, `1e13` and `1e14` exist only in `PrecisionPlanner.cpp`.
- `MandelbrotCoreTests` passed 1/1 in 2.68 seconds. D3D11 WARP and OpenGL production fixtures passed 2/2 in 9.97 seconds.
- This retains legacy preview/still compatibility behavior only. It does not make legacy GPU execution exact-camera authoritative, enable a validated GPU deep backend, validate float4 precision, or provide correction.

## Exact direct coefficient-animation refusal — BR-20260803-06

- The exact direct evaluator has no coefficient-time input. It now rejects `animateCoefficients` explicitly; `RenderExactDirectStillImage`, high-resolution exact CPU preflight, and exact frame job/per-frame validation expose the same boundary.
- Core coverage proves evaluator, direct still and immutable exact frame job refusal. `MandelbrotCoreTests` passed 1/1 in 3.03 seconds. The native Release `MandelbrotWallpaper` target compiled and linked successfully.
- This prevents silent static rendering of time-varying equations. It does not implement deterministic exact coefficient-time evaluation, animated exact still/frame output, GPU equivalence or video validation.

## Hardware D3D11 production readback fixture — BR-20260729-12

- Added an explicit `--hardware` mode to the same production fixture; the CTest default remains WARP.
- Native direct execution on this host passed exact, non-uniform standard, bloom and explicit-perturbation readbacks at 256x144 on `AMD Radeon RX 7900 XT / Direct3D 11.1`.
- This is adapter-specific same-process evidence, not an approved hardware baseline or a cross-adapter equivalence claim.

## OpenGL production readback fixture — BR-20260729-13

- Added `MandelbrotOpenGLFixtures`, using `GpuRenderer` with explicit OpenGL preference and a hidden production context.
- Direct and isolated CTest execution passed exact standard, bloom and perturbation readbacks at 256x144 on ATI Technologies Inc. / AMD Radeon RX 7900 XT / OpenGL 4.6.0 Compatibility Profile Context 26.6.4.260624; CTest passed 1/1 in 0.37 seconds.
- Reviewed image baselines and threshold calibration remain the only PH-02 completion gates.

## PH-02 approved CPU baselines — BR-20260729-14

- User explicitly approved PH-02 baseline promotion.
- Promoted the four canonical CPU candidate images to `tests/baselines/visual/<fixture>/cpu/baseline.ppm` and recorded approval metadata with strict thresholds: maximum channel error 0, maximum differing ratio 0 and minimum SSIM 1.
- Ran `MandelbrotVisualFixtures` against those baselines with the recorded strict thresholds; all four baseline comparisons passed, as did tiled/full seams and deliberate-mutation detection.
- PH-02 is complete at its documented scope. Backend evidence remains same-process and environment-specific; no cross-backend pixel-equivalence claim is made.

## PH-03 native core verification — BR-20260729-15

- Ran the current native Release `MandelbrotCoreTests.exe`; it passed.
- The current MSVC Release build also compiles and links the complete Windows application and PH-03 implementation.
- This confirms native compilation and core-contract execution, not Windows interaction for gesture coalescing, dialog replacement, or preview behaviour. PH-03 remains incomplete pending that interaction evidence.

## PH-03 preview-navigation history correction — BR-20260729-16

- Corrected preview-navigation coalescing: successive events under one token may now merge the union of changed camera fields, retaining each field's earliest before value and latest after value.
- Added a core regression test for a single navigation token whose events change different camera-field subsets; exact undo and redo endpoints pass.
- Native MSVC Release build and `MandelbrotCoreTests` passed. Runtime interaction retest remains required.

## PH-03 live dialog preview and candidate rollback — BR-20260729-17

- Palette, Equation, Settings and Journey now edit candidate state, render that candidate immediately in the preview, and discard it on Cancel.
- Palette and Equation libraries are candidates too; their changes are committed only after the corresponding dialog is accepted.
- The Settings candidate supplies preview render options without changing the persisted desktop/runtime settings before acceptance.
- The native MSVC Release application compiled and linked. The complete six-target Release CTest set passed, including core/path, CPU visual, D3D11 WARP, OpenGL and version checks.
- Manual Windows interaction evidence remains required for live update, Cancel rollback and Settings/Journey behaviour.

## PH-03 runtime interaction confirmation — BR-20260729-18

- The user confirmed that preview navigation undo now works and that live preview with Cancel rollback works for Palette, Equation, Settings and Journey.
- Together with BR-20260729-16/17 native Release build and CTest evidence, this completes PH-03 at its documented parameter-adapter and render-fingerprint scope.
- The next canonical unfinished task is PH-04 Windows interaction validation for camera/palette undo/redo and redo-branch behaviour.

## PH-04 runtime interaction confirmation — BR-20260729-19

- The user confirmed camera and palette/post-control undo/redo and redo-branch truncation after a divergent new edit.
- Together with the native Release build/link and portable core regressions, this completes PH-04 at its documented bounded camera/palette history scope.
- The next canonical unfinished task is PH-05 Windows interaction validation for atomic structural replacements.

## PH-05 palette replacement interaction — BR-20260729-20

- The user clarified that the initial Palette Editor failure report came from a different Android-source build. In this Windows native build, accepted Palette Editor changes Undo and Redo correctly.
- The Palette Editor commit-boundary correction remains in place: only OK commits the candidate palette/library, while Cancel cannot create a phantom accepted replacement.
- Remaining PH-05 interaction paths: Equation, Settings, Journey, preset/import and Scout Apply atomic replacement undo/redo.

## Deep-zoom pack integration and baseline reconciliation — BR-20260730-01

### Inspected

- Complete `Mandelbrot-Deep-Zoom-Upgrade-Pack-1.13.1-v2-COMBINED.md`.
- Canonical requirement, phase, validation, decision, risk, route, persistence, architecture, feature and delivery owners.
- Live AppWindow gesture/dialog-preview paths, ProjectHistory navigation merge behaviour, core tests and `scripts/verify-source.py`.
- Proposed ID ranges PH-12–PH-15, REQ/AC-021–040, VAL-037–060, DEC-028–037, RISK-013–030 and ROUTE-015–018; no prior canonical use was found.
- Checkout provenance: `.git` and a Git bundle are absent; the normal Release executable exists.

### Failed, corrected and passed

The first exact source-audit run failed at the stale expectation `ParameterGestureKind::PreviewPan`. After reconciling the verifier and evidence text to the accepted shared `PreviewNavigation` implementation, the next run exposed a second stale PH-05 documentation marker and then a PH-07 marker that predated dialog candidate previews. Each was narrowed to the live accepted source contract.

The final `C:\Python314\python.exe scripts\verify-source.py` run passed all checks through PH-09 on this source state. This is VAL-037 structural evidence only. It does not prove PH-12 exact parsing/migration, a native rebuild after documentation/verifier edits, deeper rendering, GPU numerical correctness, UI behaviour, resource limits or release suitability.

The first sandboxed `cmake --build build --config Release --parallel` attempt failed because Visual Studio `FileTracker` returned `E_ACCESSDENIED`. The permitted native retry succeeded and linked `MandelbrotWallpaper.exe`, core/path tests and CPU/D3D11/OpenGL fixture executables. `ctest --test-dir build -C Release --output-on-failure` then passed 6/6 in 14.66 seconds.

The native result proves this unchanged C++ application state still compiles/links and its existing six-test suite passes. It does not prove PH-12 exact parsing/migration, deeper numerical correctness, manual UI/desktop behaviour, hardware-GPU deep equivalence, resource limits, real FFmpeg deep parity or release suitability.

### Proposed evidence catalogue

- VAL-038–VAL-044: bounded exact syntax, exact camera UI/clipboard/settings/preset/job round trip, legacy migration/original preservation, single authority, fingerprint v2, preset ownership and exact Journey/timeline/Scout adapters.
- VAL-045–VAL-050: deterministic plan matrix, backend policy removal, formula fingerprints, independent orbit comparison, cancellation/generation races and byte-accounted cache/resource behaviour.
- VAL-051–VAL-056: D3D11/OpenGL direct-reference comparisons, injected validity faults, rebase/correction continuity, multi-reference determinism and enforced termination.
- VAL-057–VAL-060: exact global mapping/seams, deterministic motion/timing, lifecycle/failure matrix and real still/frame/FFmpeg support evidence.

Numerical thresholds must be fixture-specific and measured. Deep evidence records exact camera and digit count, formula/plan/fingerprint versions, camera/reference/upload/delta precision, classification/correction/unresolved counts, backend/device/driver, global mapping, CPU/RAM/VRAM/cache/workers/disk/cancellation and artifact provenance. Portable checks remain separate from native MSVC, hardware GPU, Win32 UI, WIC, real FFmpeg, device-loss and soak evidence.

### Deep-programme decision status

- DEC-015 split precision-intent ownership is accepted.
- DEC-029 accepts the Boost.Multiprecision 1.83.0 standalone source subset; package-integrity/build/test evidence is owned by BR-20260730-03. The current `ReferenceOrbitService` has portable/native core coverage for Boost-512 plan validation, cache reuse, caller-generation stamping and cooperative cancellation; production deep-coordinate rendering, worker coalescing and measured resource evidence remain required.
- Settings schema 12, preset schema 3 and forward-only migration are accepted; schema-10 Live/Journey defaults migrate to None, schema-11 Independent monitor state migrates to Mirror without assignment data, and implementation evidence remains required.
- DEC-036 accepts explicit user selection of CPU-reference or validated-GPU export with manifest classification and no silent fallback.
- Measured planner, validity, correction and resource thresholds.
- PH-12 may precede PH-10/PH-11 with only direct dependencies incorporated and no predecessor-completion claim.
- Exact parser/library behaviour, depth beyond the current cap, orbit-encoding ceiling, backend parity, direct high-precision performance, multi-reference termination, long-session resources, UI responsiveness, device loss, extreme WIC output, real deep FFmpeg parity, installer impact and release suitability.

## Windows UX regression correction — BR-20260805-01

- Fresh native Release build passed and the complete Release CTest suite passed 7/7 in 16.93 seconds. The governed source/offline verifier also passed after the final product-source edit.
- Isolated Win32 window capture/inventory exercised three main pages and eleven tool/dialog entry points at the current 96-DPI desktop. It confirmed owner enablement by route, disabled built-in preset Update/Delete, disabled empty animation keyframe actions, and visually removed the audited default clipping/unwanted-scrollbar regressions.
- Focused process evidence confirmed Exit with Settings open within five seconds with no remaining Settings window; exact-camera Copy Coordinates returned canonical `-5e-1,0,1.5e0`; nested Journey Apply → Cancel followed by Settings OK preserved the original Journey value.
- Evidence artifacts are under `build/ux-audit-runtime/verified-final-all`, `verified-final-exit`, `verified-final-copy-alone` and `verified-final-nested-journey` and use isolated `MW_APPDATA_DIR` state.
- No destructive custom-preset/log action or real output/export was executed. Mixed-DPI, keyboard, accessibility, live colour/precision/adaptive preview and the remaining PH-05 atomic undo/redo matrix are not proven by this bounded runtime pass.

## Modeless editor camera retention and automatic preview precision — BR-20260907-01

- Core regression proves Palette and Equation candidates merge only their owned domains over the latest authoritative preset, preserving camera, exact camera, zoom, rotation and unrelated project fields. Both live-preview and accepted AppWindow routes use those merge functions.
- The native MSVC Release build passed and CTest passed 7/7 in 17.54 seconds.
- The production D3D11 fixture rendered in Automatic mode at three camera scales and reported `GPU float32` at 1.5, `Split high/low float` at `1e-7`, and `GPU perturbation / arbitrary-precision reference` at `1e-14`; the complete direct fixture passed on both Microsoft Basic Render Driver / Direct3D 11.1 and AMD Radeon RX 7900 XT / Direct3D 11.1.
- The production OpenGL fixture explicitly rendered native Float64 and passed on AMD Radeon RX 7900 XT / OpenGL 4.6.0 Compatibility Profile Context 26.6.4.260624. This confirms shader compile/render/readback on that adapter/driver, not full-double transcendental accuracy or a validated deep-GPU envelope.
- Source inspection confirms changed precision settings and a dead preview renderer use one bounded reinitialisation path, and coordinate admission uses the central legacy GPU policy with the active renderer capability report.
- End-to-end native pointer interaction remains unproven because the screen-capture API failed with `0x80004002`. No desktop, export, package, mixed-DPI, accessibility, device-loss, soak or performance claim is made.

## PH-10 production Scout thumbnails — BR-20260908-01

- **VAL-034 passed in the bounded native fixture scope:** `MandelbrotVisualFixtures` invokes production `RunFractalScout` and `RenderStillImageTiled` twice with one fixed request. All three retained candidates must preserve exact per-rank identity, score and pixels; each thumbnail must be non-uniform; and the three image hashes must not collapse to one image.
- The fixture writes rank-specific current/repeat/diff PPM images and metrics plus a shared state record below `build/test_artifacts/visual/fractal-scout-thumbnails/cpu/candidate/`. The recorded image hashes are `cb70154358f0b7f3`, `2ebbb4fb9d317c5d` and `bf7f3dc9c0ba27f1`; each comparison reports exact equality and SSIM 1.
- Native Visual Studio 18 2026 Release compilation passed. The focused `VisualFixtureSelfCheck` passed 1/1 in 2.63 seconds, and the final complete Release suite passed 7/7 in 17.35 seconds. The governed source/offline verifier also passed after the fixture and documentation updates.
- A contact sheet generated from the fixture PPMs was inspected and showed three distinct, non-uniform Mandelbrot thumbnails. Candidate generation remains separate from baseline approval; no Scout baseline was promoted.
- **VAL-035 was partial in this delivery:** candidate-copy source inspection, core tests and production thumbnails supported isolation; BR-20260910-03 later added native dialog interaction.
- **VAL-036 was partial at the UI boundary in this delivery:** core history proved one reversible labelled Scout Apply transaction; BR-20260910-02/03 later added native Apply/Undo/Redo interaction.
- A fresh isolated application run reached the production D3D11 preview and opened the Advanced Equation Editor. The available automation host exposed the accessibility tree but could not provide screenshot pixels or reliable dialog input geometry (`0x80004002`), so no PH-05 or Scout interaction claim is made from that attempt.

## PH-11 process-scoped automated Windows release validation — BR-20260908-02

- The canonical validator now enumerates top-level windows and filters by the exact launched process ID and `MandelbrotLiveWallpaperControl` class. A governed source check rejects reintroduction of the unreliable global `FindWindowW` lookup.
- The corrected workflow passed on Windows 10 build 19045 with PowerShell 7.6.5 and Visual Studio 18 2026/MSVC 19.51.36252.0: source/offline policy; clean x64 Release configure/build; CTest 7/7 in 18.92 seconds; embedded file/product version 1.13.1; and portable ZIP inspection of 87 entries with required files present and forbidden local/build artifacts absent.
- The isolated runtime used a dedicated `MW_APPDATA_DIR`. Its process-owned main window appeared, the normal loop ran for two seconds, D3D11 initialized on AMD Radeon RX 7900 XT / Direct3D 11.1, the normal Exit command completed, and one log contains startup and shutdown markers.
- The machine-readable report is `artifacts/windows-validation/20260908-ph11-process-scoped/report.json`; status is `Automated checks passed`. Repository cleanliness is explicitly unproven because this supplied checkout has no `.git` directory. Installer creation was deliberately skipped.
- The first attempt was blocked by a stale isolated application process holding the generated executable. After that exact process was terminated, the second attempt completed build/test/package but reproduced the known false-negative global-window lookup. Both failures are retained in separate report directories.
- This evidence does not complete PH-01 or PH-11. Windows 11, other GPU/driver configurations, manual dialogs/desktop modes, mixed-DPI/multi-monitor, Explorer/session/power/device-loss lifecycle, settings migration, real FFmpeg, cancellation, resource soak, installer and upgrade/uninstall remain open.

## PH-09 real FFmpeg production-path fixture — BR-20260909-01

- Added the Windows-only, opt-in `MandelbrotExternalVideoFixture` target. It accepts an explicit external FFmpeg path and a new artifact directory, is intentionally absent from default CTest, and persists neither a configured path nor an encoder binary.
- The fixture invokes production frame-sequence construction/export, CPU tiled rendering, WIC PNG encoding/dimension validation, complete manifest/receipt verification, external-video job construction, owned `CreateProcessW` execution and final video-export verification/promotion.
- Native MSVC Release compilation of the target passed with warnings-as-errors. The first sandboxed rebuild attempt failed inside Visual Studio FileTracker with `E_ACCESSDENIED`; the permitted native retry passed.
- The privacy-corrected fixture passed with `ffmpeg version 8.1.1-full_build-www.gyan.dev`: version, `libx264` and MP4 muxer probes; three verified 96x64 production PNGs at 3/1 fps; fixed H.264/yuv420p encoding; machine-progress frame 3; decode of one output frame; atomic promotion of a 3,426-byte MP4; removal of the owned temporary MP4; and preservation/revalidation of all source frames and receipts.
- `artifacts/p9-v7/report.json` records application version 1.13.1, executable filename only, `ffmpegPathPersisted=false`, the version/capabilities, dimensions/count, full immutable job fingerprint, output/log filenames, success-boundary results, both cancellation timings and failure-containment fields. Inspection confirmed no local user/install directory is present in the report.
- The artifact directory also retains the schema-3 manifest, three receipts, three source PNGs, bounded stdout/stderr logs and verified MP4. The stdout log records the version/capability header, `frame=3`, `progress=end` and the later verification section; stderr contains only the verification section marker.
- After promotion, the fixture replayed the verified MP4 in a fixture-owned indefinite real-time loop and requested cancellation after 200 ms. `RunOwnedProcess` terminated only that child, returned `cancelled=true` with Windows exit code 1223 in 257 ms, and inspection found zero surviving FFmpeg processes and no additional output.
- The integrated cancellation run constructed 120 verified 640x360 source frames from one production-rendered seed using same-volume hard links, selected the production `veryslow` job, and armed cancellation only after the actual process runner began. The child and `RunExternalVideoExport` both reported cancellation with exit code 1223 in 116 ms; no final or temporary MP4 and no FFmpeg process remained, both bounded logs existed, and all 120 frames, receipts and the manifest revalidated.
- A second valid 96x64 production sequence passed job construction. The fixture then moved only that fixture-owned sequence directory between production preflight and the exact FFmpeg call. FFmpeg failed with its real missing-sequence diagnostic; the production export boundary retained bounded logs and removed the partial/final MP4. After fixture restoration, all three PNGs, receipts and the manifest revalidated, no held directory remained and no FFmpeg process survived.
- A new core regression rejects odd source dimensions before launching the fixed yuv420p encoder. This preserves valid odd-dimension PH-08 PNG export while preventing a predictable video conversion failure.
- A read-only `ffprobe.exe` inspection of the promoted file independently reported H.264, yuv420p, 96x64, 3/1 fps, three frames, 1.000000-second duration and 3,426-byte size; `artifacts/p9-v7/media-inspection.json` preserves those fields without a local installation path.
- The final full MSVC Release build linked the application and every test/fixture target, including `MandelbrotExternalVideoFixture`. Release CTest passed 7/7 in 17.90 seconds. The final governed source/offline audit passed after the implementation and evidence integration.
- This is direct success-path, owned-child termination, integrated real `RunExternalVideoExport` cancellation cleanup and real exact-argument encoder-failure containment evidence for VAL-031–VAL-033 on one concrete build. Win32 dialog/progress/keyboard/DPI behavior, wallpaper pause/release/resume, other FFmpeg builds and licence suitability remain unproven.
- An earlier run below the repository's longer `artifacts/external-video/...` path encoded the video but failed opening its 265-character diagnostic log path because the standalone fixture does not embed the application's long-path-aware manifest. The production application does embed that manifest. The final dated evidence run used a fresh shorter artifact root and passed; no production path contract was changed based on the harness-only limit.

## PH-01 native installer artifact validation — BR-20260909-02

- `scripts/build-release.ps1` now retains its scalar-safe machine-wide Inno Setup lookup and falls back to a per-user or `PATH`-provided `ISCC.exe`; this closes the discovery gap on hosts where Inno Setup is installed for the current user.
- `scripts/validate-windows-release.ps1` now requires the installer artifact to be a non-empty file, verifies its embedded file version is 1.13.1, records SHA-256 and records Authenticode status rather than treating existence alone as sufficient artifact evidence.
- The final canonical run at `artifacts/windows-validation/20260909-ph11-installer-final/report.json` passed source policy, clean native Release configure/build, CTest 7/7, executable metadata, 87-entry portable ZIP inspection, process-scoped isolated D3D11 startup/render/clean shutdown and Inno Setup 6.7.3 installer production.
- This is installer-production and artifact-integrity evidence only. The installer was not installed, upgraded or uninstalled; Authenticode is `NotSigned`; Windows 11, manual dialogs/desktop modes, display/lifecycle cases and soak remain open. Repository cleanliness is unproven because this source copy has no `.git` directory.

## Native Palette/Equation interaction regression — BR-20260909-03

- Added `MandelbrotWindowsInteractionFixture` as a Windows-only, serial CTest gate. It launches only the freshly built application, redirects `MW_APPDATA_DIR` to a unique fixture-owned directory, uses process/class/control identities from the real Win32 routes, applies no desktop mode, sends the real Exit command and may terminate only its owned process on bounded cleanup failure.
- The focused native run passed: initial Automatic preview resolved to `GPU float32`; changing the live exact camera while Palette remained open produced canonical `1.25e-1, -2.5e-1, 1e-8` and switched the preview to `Split high/low float`; accepting frequency `1.75` preserved that camera and exposed the `Edit Palette` history entry.
- With Equation open, changing the live camera to `-5e-1, 0, 2.5e-1` returned the preview to `GPU float32`; accepting power 3 preserved the camera and exposed `Edit Equation`; native Undo/Redo restored powers 2/3 and retained the camera. The isolated log records clean shutdown and no fixture-owned process remained.
- The focused CTest artifact is `build/test_artifacts/windows-interaction/report.md`. The cumulative release workflow reruns the same gate. This proves ROUTE-002/005/006/016 and bounded VAL-016/VAL-021 interaction behaviour, not the remaining PH-05 dialogs, keyboard/DPI/accessibility, desktop modes, Windows 11 or lifecycle matrix.

## Native preset load history interaction — BR-20260909-04

- The process-scoped `WindowsInteractionFixture` now waits for the Palette/Equation action controls before editing them and waits for the main-window structural history label before asserting an accepted dialog commit. This removes two reproduced harness races: observing a top-level dialog before its final population and observing window destruction before the nested dialog loop returned to `AppWindow`.
- The extended fixture selects the second built-in preset through the production combo notification and requires the `Load Preset` structural history label. Native Undo restores the exact pre-load camera text and working preset; Redo restores the loaded camera and visible `Seahorse Valley` identity.
- The warnings-as-errors MSVC fixture/application build passed. The focused native test passed in 14.48 seconds. The subsequent complete Release CTest run and source verifier are recorded in BR-20260909-04.
- The fixture continues to use unique isolated application data, applies no desktop mode, exits through the production command and may terminate only its owned process after bounded cleanup failure.
- This is additional ROUTE-002 and VAL-016/VAL-021 evidence. Settings, Journey, import and Scout Apply interaction remain pending; keyboard, DPI, accessibility, desktop modes, installer runtime, lifecycle and soak remain unproven.

## Native Settings history and main-window synchronisation — BR-20260909-05

- The extended process-scoped fixture opens the real Settings window, edits project rotation to `17.5`, accepts it, requires the `Edit Project Settings` history label, and checks the separate main-window rotation field.
- The first focused run failed because the authoritative Settings candidate and preview changed while the main-window field retained its old value. Source inspection located the accepted path calling only the legacy coordinate-triplet refresh.
- The accepted Settings path now calls the complete main-window camera-control synchronisation routine. The subsequent focused native fixture passed and verified Undo/Redo restores rotation `0`/`17.5` through one structural history entry.
- Palette/Equation live-camera retention and Automatic preview precision switching continued to pass in the same native process: `GPU float32` → `Split high/low float` → `GPU float32` as the live camera scale changed.
- The complete warnings-as-errors MSVC Release build passed for every target. Release CTest passed 8/8 in 31.81 seconds, including the 14.32-second Windows interaction fixture, and the governed source/offline verifier passed after the final evidence update.
- This is bounded ROUTE-002/006/016 and VAL-016/VAL-021 evidence. Journey, import and Scout Apply interaction remain pending; keyboard, DPI, accessibility, desktop modes, installer runtime, lifecycle and soak remain unproven.

## Native Journey history interaction — BR-20260909-06

- The process-scoped fixture opens the production Journey Settings dialog, waits for its actual controls, replaces the waypoint editor with a valid two-row route, and accepts it through the real OK command.
- The accepted route exposes one `Edit Journey` history label. Reopening the dialog proves the accepted text is retained exactly; reopening again after native Undo and Redo proves the original and accepted strings are restored without flattening the structural edit.
- The focused warnings-as-errors native application/fixture build passed. The focused Windows interaction CTest passed in 14.96 seconds and its report records clean shutdown with no desktop mode applied.
- The subsequent complete warnings-as-errors MSVC Release build passed, and Release CTest passed 8/8 in 32.37 seconds; the repeated Windows interaction fixture passed in 14.66 seconds. The governed source/offline verifier passed after the final evidence update.
- This is bounded ROUTE-002 and VAL-016/VAL-021 interaction evidence. It does not execute Journey wallpaper playback or prove timing, transition rendering, keyboard, DPI, accessibility, desktop integration, lifecycle or soak behavior. Import and Scout Apply interaction remain pending.

## Native preset Import history interaction — BR-20260910-01

- The process-scoped fixture creates a bounded preset with the production `SettingsStore::SerialisePreset` path, requires production deserialisation to round-trip its identity and camera, and writes it only below the fixture-owned isolated application-data directory.
- The real Import command opens the native common Open dialog. The fixture identifies its filename editor by Win32 hierarchy, submits the isolated absolute JSON path and waits for the dialog to close and the `Import Preset` history label plus `Fixture Imported` selection to appear.
- Native Undo restores the exact pre-import canonical camera text and selected preset identity; Redo restores the imported camera and imported selected identity. The fixture never applies a desktop mode and exits its owned process cleanly.
- The focused warnings-as-errors native application/fixture build passed. Two consecutive focused Windows interaction CTest runs passed in 16.10 and 15.52 seconds.
- The subsequent complete warnings-as-errors MSVC Release build passed, and Release CTest passed 8/8 in 31.84 seconds; the Windows interaction fixture passed in 15.61 seconds. The governed source/offline verifier passed before and after the complete run.
- This is bounded ROUTE-002 and VAL-016/VAL-021 interaction evidence. It does not prove malicious/invalid-file dialog handling, keyboard, DPI, accessibility, desktop integration, lifecycle or soak behavior. Scout Apply interaction remains pending.

## Native Fractal Scout history interaction — BR-20260910-02

- The process-scoped fixture opens the production Fractal Scout dialog, lets its automatically started bounded search finish, and waits for a selected result and enabled `Use in Preview` action.
- Invoking that action closes the modal dialog, changes only the working camera, preserves the selected preset identity and exposes one `Apply Scout Camera` history entry.
- Native Undo restores the exact pre-Scout canonical camera and identity; Redo restores the exact applied camera while preserving the same identity. Two consecutive focused runs passed in 16.92 and 16.45 seconds with clean shutdown and no desktop mode applied.
- The subsequent complete warnings-as-errors MSVC Release build passed, and Release CTest passed 8/8 in 32.62 seconds; the Windows interaction fixture passed in 16.51 seconds. The governed source/offline verifier passed before and after the complete run.
- This completes the automated native PH-05 atomic replacement interaction matrix and provides VAL-036 UI-boundary evidence. It does not prove Scout cancellation/refinement/save-as-new, keyboard, DPI, accessibility, visual layout, desktop integration, lifecycle or soak behavior.

## Native Fractal Scout isolation and PH-10 completion — BR-20260910-03

- The process-scoped fixture waits for a completed production Scout candidate and enabled `Use in Preview`, then invokes Close instead. The exact main-window camera, selected preset identity and Undo label remain unchanged.
- It reopens Scout, completes a second production search and retains the exact Apply/Undo/Redo checks from BR-20260910-02. Two consecutive focused runs passed in 17.64 and 17.47 seconds with isolated application data and clean shutdown.
- The complete warnings-as-errors MSVC Release build passed and Release CTest passed 8/8 in 33.40 seconds, including the 17.43-second Windows interaction fixture. The governed source/offline verifier passed before and after the complete run.
- Combined with the existing deterministic candidate identity/core coverage and native production-thumbnail fixture, this passes VAL-034–VAL-036 and completes PH-10 at the documented bounded/native scope.
- Optional style/session extensions, Scout cancellation/refinement/save-as-new UI interaction and deep exact Scout execution remain outside this completion claim.

## Native Animation Timeline clock and candidate interaction — BR-20260910-04

- The process-scoped fixture opens the production General Animation Timeline, adds a camera-centre track and its authoritative current-value keyframe, scrubs the preview clock to the midpoint, adds a second current-value keyframe, enters Play and invokes Stop.
- Stop returns the control label to `Play` and resets the displayed preview time to zero. Cancel closes the dialog and preserves the exact project camera, selected preset identity and Undo label; reopening proves the discarded track did not leak into runtime state.
- A second edit adds one track and accepts with OK. Main project state and history remain unchanged, while reopening the editor proves the accepted runtime-only track remains present in the current session.
- The focused warnings-as-errors native fixture build passed. Two consecutive focused interaction runs passed in 18.26 and 17.99 seconds with isolated application data, no desktop mode applied and clean shutdown.
- The complete warnings-as-errors MSVC Release build passed and Release CTest passed 8/8 in 34.06 seconds, including the 17.97-second Windows interaction fixture. The governed source/offline verifier passed before and after the complete run.
- This is bounded VAL-023/VAL-025 and ROUTE-013 native control/state evidence. It does not prove rendered pixels changed during playback, Journey conversion interaction, wallpaper/export clock behavior, keyboard, DPI, accessibility, lifecycle or soak behavior.

## Native Journey and Timeline conversion interaction — BR-20260910-05

- The fixture-owned imported preset contains a valid bounded two-row Journey. The production Timeline's Journey → Tracks action replaces its candidate with exactly three camera tracks.
- Tracks → Journey exposes the production `Tracks to Journey` successful-preparation confirmation. The fixture closes that modal confirmation through its standard `IDOK` command without assuming a direct-child button hierarchy.
- Cancelling the conversion candidate leaves exact project camera text, selected preset identity and the Undo label unchanged. Two consecutive focused runs passed in 18.39 and 18.11 seconds with isolated application data and clean shutdown.
- The complete warnings-as-errors MSVC Release build passed and Release CTest passed 8/8 in 34.17 seconds, including the 18.10-second Windows interaction fixture. The governed source/offline verifier passed before and after the complete run.
- This adds bounded native VAL-026/ROUTE-013 interaction evidence. It does not prove accepted reverse conversion changes a different Journey, rendered visual playback, DPI, keyboard, accessibility, lifecycle or soak behavior.

## Native frame-sequence export and cancellation interaction — BR-20260910-06

- The process-scoped fixture opens the production Frame Sequence Export dialog from the main window after accepting a runtime-only 0.04-second palette-offset timeline. The fixture creates output only below its unique isolated application-data directory and never applies a desktop mode.
- A Start action produces one production WIC-encoded 32x24 PNG, the `.mw-frame-sequence` manifest and no `.part` file. The fixture reads the PNG signature/IHDR dimensions independently after the production route has reopened and WIC-validated the image before promotion.
- Reopening the same dialog with identical inputs and matching-manifest resume enabled completes as `0 rendered, 1 resumed`; the existing verified PNG remains and no partial file is introduced.
- A mismatched width against the completed manifest exposes the production mismatch error and preserves the verified 32x24 PNG. A separate fixture-owned sentinel at the expected final PNG path exposes the untracked-output error and remains byte-for-byte unchanged; neither refusal leaves a `.part` file.
- A separate 1024x1024 run is started and immediately cancelled through the visible Cancel button. The dialog reports cancellation, its worker joins and re-enables the controls, resumable metadata remains, and no `.part` file survives.
- A second 1024x1024 run receives the native window close action while active. It cancels and joins the worker, retains the dialog with its cancellation result visible, preserves resumable metadata and leaves no partial file; a subsequent Close destroys the dialog and restores its owner.
- Both completed/cancelled dialog closes restore the enabled main owner and preserve exact camera text, selected preset identity and Undo label. The application then accepts its normal Exit command and logs clean shutdown.
- The warnings-as-errors fixture target build passed. Two consecutive focused native runs with the active-close check passed in 21.06 and 21.04 seconds.
- The complete warnings-as-errors MSVC Release build passed and Release CTest passed 8/8 in 37.84 seconds, including the 21.20-second cumulative Windows interaction fixture.
- The governed source/offline verifier passed after the fixture and canonical evidence updates.
- This adds bounded native VAL-027–VAL-029 and ROUTE-014 evidence. It does not prove Open Folder launch, a long/mid-frame cancellation with verified frames, selected-frame visual approval, wallpaper pause/GPU-release/resume, DPI, keyboard, accessibility, lifecycle or soak behavior.

## Native Video Export validation interaction — BR-20260918-01

- The process-scoped fixture opens the production External FFmpeg H.264 / MP4 Export dialog against its verified one-frame PH-08 sequence and fixture-owned output directory.
- Typing the sequence path now refreshes the manifest summary to one 32x24 frame. This corrects the reproduced inconsistency where Browse refreshed the summary but direct edit/paste did not.
- Missing required paths and CRF 52 expose and dismiss their production validation messages. A fixture-owned nonexistent FFmpeg path is rejected by executable preflight; the dialog reports failure, re-enables Start and remains usable.
- No final MP4 or `.part.mp4` is created. Closing restores the enabled main owner while preserving exact camera text, selected preset identity and Undo label; the application exits cleanly.
- The warnings-as-errors fixture target build passed. Two consecutive focused native runs passed in 21.41 and 21.38 seconds.
- The opt-in route was then run twice with the explicitly resolved FFmpeg 8.1.1 executable. Both production-dialog runs passed in 22.82 and 22.09 seconds: capability checks, encoding, decode verification, final promotion, enabled Open Output, source-PNG preservation, no temporary MP4, owner/project isolation and clean shutdown. The promoted MP4 is 1,650 bytes in the latest run.
- A second opt-in input points only to the existing fixture-owned verified 120-frame cancellation sequence. Two combined success/cancellation runs passed in 22.85 and 23.08 seconds. Each reached the owned encoding stage, invoked the visible Cancel action, revalidated the unchanged sequence fingerprint and all 120 completed frames, and left neither final nor temporary cancellation output.
- The complete warnings-as-errors MSVC Release build passed and Release CTest passed 8/8 in 37.82 seconds, including the 21.52-second cumulative Windows interaction fixture.
- The governed source/offline verifier passed after the product, fixture and canonical evidence updates.
- This adds bounded native VAL-031–VAL-033 dialog evidence. It does not prove Open Output launch through the dialog, wallpaper pause/GPU-release/resume, DPI, keyboard, accessibility, other FFmpeg builds, licensing suitability, lifecycle or soak behavior.
