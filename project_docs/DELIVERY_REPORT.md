# Delivery Report

Current delivery: [BR-20260924-02 — Exclusive editor-window state hardening](#br-20260924-02--exclusive-editor-window-state-hardening).

## BR-20260727-01 — Markdown Foundation Generation

**Status:** Proposed / File generation
**Date:** 2026-07-27
**Objective:** Generate a complete evidence-grounded Markdown foundation for the supplied Mandelbrot Live Wallpaper 1.13.1 source and roadmap recommendations.

## Inspected

- Archive layout, source tree, tests, existing Markdown set and absence of Git metadata.
- CMake project/version and target structure.
- Current models, schema version, settings/preset serialisation, bounded JSON parser and failure-safe save/corrupt-file behaviour.
- Animation, Fractal Scout and still-render public contracts.
- Renderer types and wallpaper controller surface.
- AppWindow state/member layout and user workflows.
- Paths, local log rotation, application manifest, Win32 resource, release script and installer version.
- Active architecture/testing/privacy/Windows docs and 1.13.0/1.13.1 feature/verification records.
- User-supplied roadmap recommendations.

## Generated or updated

- Root routing/governance: `AGENTS.md`, `CONTRIBUTING.md`, `CHANGELOG.md`, README governance notice.
- Canonical `project_docs/` foundation, traceability, plan, repository/versioning, UI, data, validation, security/risk, decisions, debugging, project settings and this report.
- Canonical roadmap, state/parameter, rendering/export, visual regression, undo/redo, animation, offline export and Scout integration documents.
- Compatibility entry points for active architecture, testing, privacy/security and Windows integration docs.
- Historical completion/roadmap banners without rewriting historical release evidence.

## Findings

1. **Failed by inspection:** `src/App/resources.rc` declares 1.11.5 while CMake, manifest, release script and installer declare 1.13.1.
2. **Unproven:** native MSVC compilation and Windows runtime for the supplied archive. Historical 1.13.1 verification explicitly requires a Windows rerun.
3. **Observed:** current settings schema is 9, so active architecture text naming schema 8 was stale.
4. **Observed:** current `EquationSettings` is broader than the old privacy summary; imported content remains a bounded data model, not shader/script source.
5. **Observed:** Fractal Scout is already present; future work should integrate it with shared state/history/fixtures, not recreate it.
6. **Observed:** no Git metadata is included in the archive.

## Commands run

File listing, source text inspection, searches, copy and Markdown/ZIP generation only.

## Not run

- Source verifier.
- CMake configuration/build.
- CTest or sanitizers.
- Native MSVC, shader or Windows runtime checks.
- Visual fixtures.
- Installer or package build.
- Git operations.

## Batch audit

- New IDs use the required immutable formats.
- Canonical ownership and aliases are registered in `PROJECT_INDEX.md`.
- Historical records are preserved rather than converted into current claims.
- `AGENTS.md` is under 3,500 characters.
- `PROJECT_SETTINGS.md` is under 6,000 characters.
- Internal Markdown links and generated-path existence are checked by the packaging script.

## Rollback

Delete the generated `AGENTS.md`, `project_docs/`, new canonical subdirectories/files and README/active-doc edits, or restore the original uploaded archive. No application source code was changed.

## Unproven items and next decisions

- DEC-011 was open at this delivery and was resolved by BR-20260727-02.
- DEC-012: local Git root/branch/baseline commit.
- DEC-014: future timeline persistence location.
- DEC-015: precision policy ownership after state separation.
- Native Windows release baseline and visual fixture thresholds.

## BR-20260727-02 — Version Consistency and Release-Gate Hardening

**Status:** Implemented / partly verified
**Date:** 2026-07-27
**Mode:** Repository execution
**Objective:** Resolve RISK-001 and continue PH-00/PH-01 without claiming unavailable Windows evidence.

### Inspected

- `src/App/resources.rc`, `src/App/app.manifest`, `CMakeLists.txt`, `scripts/build-release.ps1` and `installer/MandelbrotWallpaper.iss`.
- CMake test registration, portable core target and existing source verifier.
- Canonical version, validation, decision, risk, traceability and roadmap owners.

### Changed

- Corrected Win32 `FILEVERSION`, `PRODUCTVERSION`, `FileVersion` and `ProductVersion` from 1.11.5 to 1.13.1.
- Added `cmake/VerifyVersionConsistency.cmake`.
- Made CMake configuration fail when any release-facing source version drifts from `PROJECT_VERSION`.
- Added CTest `VersionConsistency`.
- Added a release-script precheck and built-executable file/product version validation before packaging.
- Updated canonical governance and roadmap status.

### Environment

- Linux 6.12.13 x86_64.
- CMake 3.31.6.
- GNU C++ 14.2.0.
- Python 3.13.5.

### Ran

```text
cmake -DROOT_DIR=<root> -DEXPECTED_VERSION=1.13.1 -P cmake/VerifyVersionConsistency.cmake
cmake -DROOT_DIR=<root> -DEXPECTED_VERSION=1.13.2 -P cmake/VerifyVersionConsistency.cmake
python3 scripts/verify-source.py
cmake -S . -B build-linux-audit -DMW_BUILD_TESTS=ON -DMW_WARNINGS_AS_ERRORS=ON
cmake --build build-linux-audit --parallel 2
ctest --test-dir build-linux-audit --output-on-failure
```

### Passed

- VAL-002 source version consistency at 1.13.1.
- Deliberate 1.13.2 mismatch was rejected and listed every inconsistent surface.
- Existing source structure/offline-policy verifier.
- Portable warnings-as-errors core build.
- CTest: 2/2 passed (`MandelbrotCoreTests`, `VersionConsistency`).

### Failed

- None within the checks run.

### Skipped / unproven

- Native MSVC compilation, resource compilation and Windows linking.
- Execution of the new built-EXE metadata check.
- Application startup, dialogs, GPU shader paths, desktop modes, packaging, installer and upgrade.
- Git branch, remotes, working-tree cleanliness and baseline commit because the distributed archive contains no `.git` metadata and no repository initialisation was requested.

### Roadmap result

- PH-00 advanced: DEC-011 and DEC-013 are accepted; RISK-001 is mitigated; the Git baseline remains open under DEC-012.
- PH-01 version prerequisite is satisfied. The phase remains blocked on the native Windows execution matrix.
- PH-02 was not started because its accepted prerequisite, PH-01, has not passed.

### Rollback

Restore the prior `resources.rc`, remove `cmake/VerifyVersionConsistency.cmake`, remove its CMake/CTest registration and revert the release-script metadata checks.

## BR-20260727-03 — Repository Baseline and Windows Validation Workflow

**Status:** Implemented / partly verified
**Date:** 2026-07-27
**Mode:** Repository execution
**Objective:** Complete PH-00 in the working repository and continue PH-01 with a repeatable, failure-bounded native Windows evidence workflow without claiming unavailable Windows results.

### Inspected

- Repository root, ignored/generated paths, branch/remotes and imported historical records.
- CMake target/test structure, release script, executable/package locations and version checks.
- Application data path resolution and startup/shutdown behaviour relevant to safe smoke testing.
- Main window class, tray Exit command, log markers and default desktop-mode behaviour.
- PH-00/PH-01 requirements, stop conditions, acceptance criteria, validation IDs and risks.

### Implemented

- Initialised the confirmed root as a local Git repository on branch `main` with no remote.
- Added a bounded `.gitignore` and created baseline commit `8f82119`, including imported historical evidence records.
- Added `CMakePresets.json` with one host-gated Visual Studio 2022 x64 Release configure/build/test preset set.
- Updated `scripts/build-release.ps1` to use the canonical presets.
- Added `MW_APPDATA_DIR` as an explicit process-level application-data override.
- Added `MandelbrotPathTests` to prove settings, logs and static-render paths remain under the isolated root.
- Added `scripts/validate-windows-release.ps1` and CMD wrapper.
- The Windows workflow records tool/repository state, source verification, native build/CTest, executable metadata, package inspection, isolated startup/shutdown smoke, installer status, logs and remaining manual checks.
- Added `docs/testing/WINDOWS-RELEASE-VALIDATION.md` with the required dialog, desktop-mode, GPU, display, lifecycle, packaging and migration matrix.
- Updated README, contributor guidance and canonical governance owners.

### Environment

- Linux 6.12.13 x86_64.
- CMake 3.31.6.
- GNU C++ 14.2.0.
- Python 3.13.5.
- Git 2.47.3.
- Native Windows, Visual Studio, Windows SDK, PowerShell parser/runtime, GPU and Inno Setup were unavailable.

### Ran

```text
git init -b main
git add -A
git commit -m "baseline: import governed 1.13.1 source"
cmake --list-presets=all
python3 scripts/verify-source.py
./scripts/run-core-tests.sh
cmake -S . -B build-linux-continue -DMW_BUILD_TESTS=ON -DMW_WARNINGS_AS_ERRORS=ON
cmake --build build-linux-continue --parallel 2
ctest --test-dir build-linux-continue --output-on-failure
ad hoc JSON, Markdown-link, character-limit and PowerShell-delimiter audit
git diff --check
```

### Passed

- PH-00 repository root, `main` branch, baseline commit and no-remote policy in the working copy.
- CMake preset JSON/schema parsing; the native presets are correctly host-gated and therefore hidden on Linux.
- Existing source/offline-policy verifier plus new PH-01 workflow markers.
- GNU warnings-as-errors compilation of `MandelbrotCoreTests` and `MandelbrotPathTests`.
- CTest: 3/3 passed (`MandelbrotCoreTests`, `MandelbrotPathTests`, `VersionConsistency`) in both portable build runs.
- `MW_APPDATA_DIR` path isolation for settings, logs and static renders.
- JSON syntax, internal Markdown links, governance character limits and diff whitespace checks.
- Lightweight PowerShell delimiter/here-string structural audit.

### Failed

- None within the checks run.

### Skipped / unproven

- Native PowerShell parsing and execution of the Windows validation workflow.
- Visual Studio 2022/Windows SDK configuration, resource compilation, MSVC compile/link and native CTest.
- Built executable metadata and portable-package checks on Windows.
- Automated startup/shutdown smoke against the native executable.
- D3D11/OpenGL runtime rendering, dialogs, desktop modes, display/lifecycle matrix, installer and upgrade/uninstall behaviour.
- PH-02 visual fixtures, because PH-01 has not passed.

### Roadmap result

- PH-00 is passed in the working repository. Source ZIPs intentionally omit `.git`; a Git bundle is the transfer artifact for repository history.
- PH-01 now has a canonical, report-producing execution path and a safe isolated runtime state boundary.
- PH-01 remains in progress and blocked on running the workflow plus manual matrix on Windows.
- PH-02 remains stopped at its accepted prerequisite.

### Rollback

Revert the PH-01 workflow commit. This removes the preset, validation scripts, path override/test and governance updates while preserving baseline commit `8f82119`. No remote or external service was added.

## BR-20260727-04 — Portable Production-Renderer Visual Fixture Foundation

**Status:** Implemented / partly verified
**Date:** 2026-07-27
**Mode:** Source archive continuation
**Objective:** Begin PH-02 with the smallest bounded visual-regression foundation that can be verified outside Windows, while preserving PH-01 and GPU evidence limits.

### Starting evidence

- The supplied PH-01 source archive SHA-256 matched its manifest: `b1a7ba6fea25a8c84c67752003a6c8537c8215a85258168388ef8256d93cb0f0`.
- The archive records PH-01 as still awaiting native/manual runtime evidence.
- The user directed PH-02 continuation after reporting a successful native build. The exact native logs and manual matrix were not contained in this archive, so they are not reclassified as verified here.

### Inspected

- PH-02 programme, visual-regression, rendering/export, traceability and validation contracts.
- `StillImageRenderer` request, global sample mapping, quality, tile and row-writer boundaries.
- Built-in scene definitions for standard Mandelbrot, Tricorn Cyan Fire Ring and reference scenes.
- Existing CMake, CTest, source verifier, release scripts and contributor workflow.

### Implemented

- Added platform-neutral `VisualRegression` fixture definitions, production-render collection, image comparison, structural similarity, diagnostic difference images and fixture-local hashes.
- Added `MandelbrotVisualFixtures`, which invokes `RenderStillImageTiled` rather than a duplicate renderer.
- Added four bounded canonical CPU fixtures: standard Mandelbrot, Tricorn Cyan Fire Ring, rotated bloom-state and deep Mandelbrot perturbation-state.
- Added exact same-process repeatability for every fixture.
- Added full-width versus tiled comparison and dedicated vertical seam-strip artifacts.
- Added an expected-failure mutation check using palette offset and depth shading.
- Added deterministic PPM and JSON artifacts with state, environment, renderer statistics and metrics.
- Added explicit baseline comparison arguments; no command promotes or auto-approves baselines.
- Added CMake/CTest integration plus shell, PowerShell and CMD runner scripts.
- Ignored generated `test_artifacts/` and updated canonical roadmap, validation, traceability and contributor documentation.

### Environment

- Linux 6.12.13 x86_64.
- CMake 3.31.6.
- GNU C++ 14.2.0.
- Python 3.13.5 for archive inspection and artifact review only.
- Native Windows, MSVC, D3D11, OpenGL hardware contexts and WARP were not available in this environment.

### Ran

```text
sha256sum Mandelbrot-Live-Wallpaper-1.13.1-ph01-validation-roadmap-progress-source.zip
cmake -S . -B build-ph02 -DMW_BUILD_TESTS=ON -DMW_BUILD_VISUAL_FIXTURES=ON -DMW_WARNINGS_AS_ERRORS=ON
cmake --build build-ph02 --parallel 2
ctest --test-dir build-ph02 --output-on-failure
CC=clang CXX=clang++ cmake -S . -B build-ph02-clang -DMW_BUILD_TESTS=ON -DMW_BUILD_VISUAL_FIXTURES=ON -DMW_WARNINGS_AS_ERRORS=ON
cmake --build build-ph02-clang --parallel 2
ctest --test-dir build-ph02-clang --output-on-failure
Clang AddressSanitizer/UndefinedBehaviorSanitizer build and bounded standard-fixture run
MandelbrotVisualFixtures exact-baseline pass and deliberately changed-baseline failure checks
MandelbrotVisualFixtures fixture-scale inspection runs
manual review of generated standard, Tricorn, rotated, deep and mutation-diff images
```

### Passed

- GNU warnings-as-errors compilation of the production core, core/path tests and visual fixture executable.
- GNU CTest: 4/4 passed in the final clean recorded run in 12.41 seconds; `VisualFixtureSelfCheck` completed in 9.82 seconds.
- Clang 17 warnings-as-errors compilation and CTest: 4/4 passed in 15.07 seconds; `VisualFixtureSelfCheck` completed in 11.44 seconds.
- AddressSanitizer/UndefinedBehaviorSanitizer core and path tests completed before the bounded full-suite timeout; a separate standard fixture repeatability run passed without reported sanitizer findings.
- Four repeated CPU production renders were exact within the same executable/process environment.
- Full-width and 29-pixel tiled rotated output matched exactly across 9,216 pixels.
- Dedicated seam strips matched exactly across 864 pixels.
- The deliberate mutation changed 18,764 of 20,736 pixels (90.49%), with maximum channel error 168, RMS channel error 73.76 and structural similarity 0.7895.
- Generated images were manually inspected for non-empty representative output; the deep fixture scale was adjusted to `1e-9` after an earlier candidate rendered black.
- An exact copied baseline passed; changing one baseline pixel caused exit code 1 and wrote a failed summary, proving the explicit comparison path rejects drift.

### Failed during implementation and corrected

- The first deep fixture candidate at scale `1e-12` rendered black and was rejected. The retained `1e-9` candidate has representative non-empty detail.
- The first Clang warnings-as-errors build found one signed-character conversion in the fixture-state hash loop. The conversion was made explicit; the clean Clang rebuild and 4/4 CTest run then passed.

### Skipped / unproven

- Native MSVC compilation and CTest for the new files.
- Approved CPU baselines across separate runs/toolchains.
- D3D11 WARP and hardware fixture paths.
- OpenGL offscreen fixture path.
- Actual GPU bloom blur and perturbation shader execution.
- Full sanitizer execution of all four visual fixtures and seam/mutation checks; the bounded sanitizer CTest attempt timed out during `VisualFixtureSelfCheck`, so only core/path and the separately run standard fixture have sanitizer evidence.
- Final AC-007–AC-009 and VAL-009–VAL-013 acceptance beyond the recorded portable scope.
- PH-01 manual Windows runtime, desktop, display, lifecycle, installer and upgrade matrix.

### Roadmap result

- PH-02 is now in progress, not complete.
- VAL-009, VAL-012 and VAL-013 have direct evidence only for the recorded same-process GNU CPU scope.
- VAL-010 and VAL-011 remain unproven.
- PH-02 verification remains required for PH-02/release-completion claims but is non-blocking for later implementation under DEC-017.

### Rollback

Remove `src/Core/VisualRegression.*`, `src/Tools/VisualFixtureTool.cpp`, the three visual-fixture runner scripts and their CMake entries, then revert the related governance/documentation updates. No application settings, persisted schemas, desktop behaviour or release package contents were changed.


## BR-20260727-05 — PH-03 parameter adapter and render fingerprint foundation

**Status:** Implemented / partly verified
**Date:** 2026-07-27
**Scope:** Bounded PH-03 foundation only; PH-02 native verification remains pending and non-blocking under DEC-017.

### Implemented

- Added a stable 13-key camera, palette and post-processing parameter descriptor registry.
- Added immutable camera and palette/post snapshots plus explicit apply adapters over caller-owned authoritative `Preset` values.
- Preserved compensated camera high/low components without flattening them to one ordinary double.
- Added versioned `mw-render-state-v1` canonical serialisation using stable field order, typed encodings, finite IEEE-754 bit patterns, canonical positive zero and length-prefixed strings.
- Added a dependency-free SHA-256 digest implementation and standard reference-vector tests.
- Added canonical render fingerprints to PH-02 CPU fixture evidence, including exact canonical-byte artifacts and deliberate mutation detection.
- Audited current direct mutation categories and retained mutation-coordinator migration as explicit remaining PH-03 work.
- Recorded DEC-017 so pending PH-02 verification does not prohibit later implementation and does not count as PH-02 completion.

### Verification

- GNU 14.2 warnings-as-errors build passed.
- GNU CTest passed 4/4, including adapter/fingerprint unit tests and the PH-02 visual fixture self-check.
- Clang 17 warnings-as-errors compilation and CTest passed 4/4.
- All four canonical fixture-state artifacts were byte-identical between GNU and Clang, and their file SHA-256 values matched the embedded fingerprints.
- AddressSanitizer/UndefinedBehaviorSanitizer core and path tests passed; a bounded standard-fixture repeatability run passed without reported findings. The complete sanitizer fixture CTest exceeded the available timeout and is not claimed.
- `scripts/verify-source.py` passed all legacy, PH-01, PH-02 and PH-03 structural checks.
- SHA-256 empty-string, `abc` and multi-block reference vectors passed.
- Camera/palette adapter round trips passed, including exact compensated low-component preservation and unrelated-authority retention.
- Canonical fingerprints changed for compensated-coordinate, palette and output-size changes; excluded metadata and negative-zero representation did not alter the digest.
- Non-finite render state and unsupported enum values were rejected; the fingerprint boundary preserves the authoritative 4,096-stop custom-palette limit.
- The deliberate visual mutation changed both its canonical render fingerprint and rendered pixels.

### Remaining / unproven

- Native MSVC compilation and Windows runtime evidence for the new PH-03 files.
- Cross-process approved PH-02 baselines and D3D11/OpenGL fixture verification.
- One interactive camera or palette domain routed through a single mutation coordinator.
- VAL-016 and final AC-011 completion evidence.
- Full project-state replacement, persistence-schema migration, undo/redo or animation integration; none were claimed or introduced in this slice.

### Rollback

Remove `src/Core/ProjectState.*`, its CMake/test/fixture integrations, and the BR-20260727-05 documentation updates. Existing persisted settings, UI behaviour and production ownership remain unchanged.


## BR-20260728-01 — PH-03 main-window scalar mutation coordinator

**Status:** Implemented / partly verified
**Date:** 2026-07-28
**Scope:** First bounded PH-03 mutation-path migration. PH-02 native visual verification remains pending and non-blocking under DEC-017.

### Implemented

- Expanded the stable descriptor registry from 13 to 16 keys with brightness, contrast and saturation.
- Added transactional `ApplyProjectParameterMutations` batches over the caller-owned authoritative `Preset`.
- Added descriptor type checks, finite-real rejection, duplicate-key rejection and all-or-nothing failure behaviour.
- Reused `ValidateAndNormalise` as the range authority, then committed only requested normalised fields so unrelated preset state is not replaced.
- Aggregated camera, palette and post-processing invalidation classes from the actual changed keys; no-op batches emit no invalidation.
- Routed the main-window brightness, contrast, saturation and colour-offset sliders/text edits through one `ApplyMainWindowPalettePostControls` path.
- Removed direct `workingPreset_` assignments for those four migrated fields. Palette enum selection, camera interaction, palette-dialog whole-preset edits and broad preset replacement remain explicit separate paths.
- Added core tests and source-audit gates for transactional rejection, normalisation, unrelated-authority preservation, no-op behaviour, type mismatch, duplicates and invalidation mapping.

### Verification

- GNU 14.2 warnings-as-errors compilation passed.
- GNU CTest passed 4/4, including the new coordinator tests and existing visual-fixture self-check.
- Clang 17 warnings-as-errors compilation passed and CTest passed 4/4.
- A bounded Clang AddressSanitizer/UndefinedBehaviorSanitizer build passed core, path and version tests 3/3 with visual fixtures disabled.
- The user reported that the preceding PH-03 release archive built and passed on Windows. The exact command, compiler output and runtime checks were not supplied, and the current coordinator slice has not been independently built on Windows in this environment.

### Remaining / unproven

- Native MSVC build and Windows runtime interaction checks for this coordinator slice.
- Bounded camera interaction migration, including coordinate edits, jump/reset, Scout Apply and preview pan/zoom decisions.
- Explicit mutation-origin/history metadata required by PH-04.
- Palette enum and palette-dialog broad-operation transaction boundaries.
- PH-02 D3D11/OpenGL/approved-baseline verification.

### Rollback

Revert the BR-20260728-01 changes in `src/Core/ProjectState.*`, `src/App/AppWindow.*`, `tests/CoreTests.cpp`, `scripts/verify-source.py` and the associated documentation. Persisted schemas and saved presets are unchanged.


## BR-20260728-02 — PH-03 deliberate camera mutation route

**Status:** Implemented / partly verified
**Date:** 2026-07-28
**Scope:** Second bounded PH-03 mutation-path migration. PH-02 native visual verification remains pending and non-blocking under DEC-017.

### Implemented

- Added one `ApplyMainWindowCameraMutation` path over the existing transactional coordinator for five compensated camera fields: X high/low, Y high/low and scale.
- Routed coordinate-triplet edits, separate centre/scale controls, Jump to Coordinates, Reset View and Fractal Scout Apply through that path.
- Centralised camera-control synchronisation after accepted, normalised or rejected mutations.
- Preserved compensated low components from camera-valued Scout and Reset sources; ordinary double text entry continues to reset low components explicitly at the existing conversion boundary.
- Preserved existing route-specific behaviour for `startingScale` and Manual View selection.
- Kept deliberate camera reapply actions capable of resetting the preview animation even when authoritative camera values are unchanged.
- Added finite/positive validation to the Jump prompt before coordinator submission.
- Left the two preview drag/wheel copy-backs as explicit runtime/gesture routes pending PH-04 coalescing and history design.
- Added camera-specific coordinator tests and strengthened the source audit so no other direct `workingPreset_.camera` replacement is permitted.

### Verification

- `scripts/verify-source.py` passed all legacy feature gates plus PH-01, PH-02 and the expanded PH-03 route audit.
- GNU 14.2 warnings-as-errors compilation passed and CTest passed 4/4.
- Clang 17 warnings-as-errors compilation passed and CTest passed 4/4.
- A Clang AddressSanitizer/UndefinedBehaviorSanitizer build passed the three enabled core/path/version tests.
- Camera tests passed for authoritative clamping, compensated-low preservation, camera-only invalidation, unrelated-state preservation and all-or-nothing rejection rollback.
- The user-reported Windows release pass applies to the preceding mutation-coordinator archive. This exact deliberate-camera archive has not been compiled or exercised on native Windows in this environment.

### Remaining / unproven

- Native MSVC build and Windows interaction checks for coordinate edits, Jump, Reset and Scout Apply in this exact slice.
- Preview pan/zoom gesture transaction and coalescing boundary.
- Palette enum and palette-dialog broad-operation transaction boundaries.
- Explicit mutation-origin/history metadata required before PH-04 history is introduced.
- PH-02 D3D11/OpenGL/approved-baseline verification.

### Rollback

Revert the BR-20260728-02 changes in `src/App/AppWindow.*`, `tests/CoreTests.cpp`, `scripts/verify-source.py` and the associated documentation. Core parameter keys, persisted schemas and saved presets are unchanged.

## BR-20260728-03 — PH-03 palette selection and mutation-origin route

**Status:** Implemented / partly verified
**Date:** 2026-07-28
**Scope:** Third bounded PH-03 mutation-path migration. PH-02 native visual verification remains pending and non-blocking under DEC-017.

### Implemented

- Expanded the stable descriptor registry from 16 to 17 keys with discrete `palette.selection` identity.
- Added bounded integer validation for supported built-in palette values; unsupported enum values fail transactionally.
- Added `ApplyProjectPaletteSelection`, which uses the shared coordinator and atomically clears custom stops only when the selected built-in palette actually changes.
- Routed the main-window palette combo through `ApplyMainWindowPaletteSelection`; removed its direct `workingPreset_.palette` write and stopped using broad control synchronisation for a palette-only selection event.
- Added `ParameterMutationOrigin`, `ParameterMutationContext` and result-level origin/future-history-eligibility reporting; palette changes that implicitly remove custom stops are withheld from scalar history until a subtree operation exists.
- Classified user controls, user gestures and Scout Apply as future-history eligible; preset load/import, undo/redo replay, animation/export evaluation, migration and system/runtime origins are explicitly ineligible.
- Applied explicit origins to the migrated main-window scalar, camera, Scout and palette-selection paths without introducing undo history or persistence changes.
- Added core tests and source-audit gates for palette no-op behaviour, custom-stop clearing, invalid palette rollback, selective invalidation and origin eligibility.

### Verification

- `scripts/verify-source.py` passed all historical feature gates plus the expanded PH-03 palette/origin audit.
- GNU 14.2 warnings-as-errors compilation passed and CTest passed 4/4.
- Clang 17 warnings-as-errors compilation passed and CTest passed 4/4.
- A Clang AddressSanitizer/UndefinedBehaviorSanitizer build passed the three enabled core/path/version tests.
- Palette-selection and origin/history-eligibility unit tests passed in the portable core test target.
- Native Windows/MSVC compilation and UI interaction verification for this exact slice remain pending.

### Remaining / unproven

- Native MSVC build and Windows interaction checks for the exact palette-selection/origin slice.
- Preview pan/zoom gesture transaction and coalescing boundary.
- Broad palette/equation dialog and preset load/import replacement-origin classification.
- PH-02 D3D11/OpenGL/approved-baseline verification.
- PH-04 undo history; no history stack, persistence or commands were introduced.

### Rollback

Revert the BR-20260728-03 changes in `src/Core/ProjectState.*`, `src/App/AppWindow.*`, `tests/CoreTests.cpp`, `scripts/verify-source.py` and the associated documentation. Persisted schemas and saved presets are unchanged.


## BR-20260728-04 — PH-03 gesture and broad-replacement completion

**Status:** Implemented / partly verified
**Date:** 2026-07-28
**Scope:** Complete the remaining inspected PH-03 implementation without introducing undo history or changing persistence.

### Implemented

- Added `ParameterGestureCoalescer` and explicit gesture kind/token metadata to project parameter mutations.
- Routed preview pan and wheel zoom through the existing camera coordinator with `UserGesture` origin.
- Defined one token per drag from mouse-down to release/capture loss and a 250 ms monotonic wheel coalescing window.
- Added `ApplyProjectPresetReplacement` with classified preset-load, import, Palette Editor, Equation Editor, Settings and Journey Settings contexts.
- Changed project dialogs to edit candidate copies and commit only after acceptance; invalid/mismatched replacement contexts roll back transactionally.
- Removed the remaining inspected direct whole-working-preset and working-camera replacement writes.
- Kept broad replacement history ineligible and explicitly deferred its reversible representation to PH-05.
- Added value equality needed for reliable no-op replacement detection, unit tests, source-audit gates and documentation updates.
- No project/settings schema, saved-preset format, renderer contract or desktop-runtime ownership changed.

### Verification

- GNU 14.2 and Clang 17 warnings-as-errors builds each passed CTest 4/4, including `VisualFixtureSelfCheck`.
- A bounded Clang AddressSanitizer/UndefinedBehaviorSanitizer build passed the core, path and version tests 3/3 with visual fixtures disabled.
- Unit coverage includes gesture token lifetime, 250 ms wheel policy, non-monotonic timestamp handling, gesture metadata validation, preset replacement normalisation/no-op/rollback and replacement-origin classification.
- `scripts/verify-source.py` checks that inspected direct whole-preset/camera replacements are absent and all PH-03 routes/decision markers remain present.
- The user reported that the preceding palette/origin archive compiled, built and passed GPU tests on Windows. No command transcript or logs were supplied; native verification of this exact final archive remains pending.

### Remaining / unproven

- Native MSVC compilation and Windows interaction tests for this exact final PH-03 archive.
- PH-02 approved visual baselines and any backend-specific evidence not covered by the user's report.
- PH-04/PH-05 undo history; this batch introduces grouping/classification metadata only.

### Rollback

Revert BR-20260728-04 changes in `src/Core/Models.h`, `src/Core/ProjectState.*`, `src/App/AppWindow.*`, `tests/CoreTests.cpp`, `scripts/verify-source.py` and associated documentation. Persisted schemas and saved presets are unchanged.

## BR-20260728-05 — PH-04 bounded camera and palette undo/redo

**Status:** Implemented / partly verified
**Date:** 2026-07-28
**Scope:** Complete PH-04 over the registered camera/palette parameter domain without adding structural project history or persistence.

### Implemented

- Added `ProjectHistory` with typed stable-key before/after parameter deltas and atomic replay through the PH-03 coordinator.
- Included compensated camera coordinates, scale, rotation, camera-associated starting-scale/animation-mode metadata, built-in palette selection where no custom-stop subtree is removed, and palette offset.
- Added exact Undo/Redo cursor handling, redo-branch truncation, labels, invalidation aggregation, configurable 256-entry/4 MiB containment defaults and runtime-only storage.
- Coalesced preview pan, wheel zoom and palette-offset thumb gestures only when origin, gesture kind, token and target set match.
- Added a dedicated rotation edit route through the coordinator.
- Added labelled Preview-page Undo/Redo buttons and Ctrl+Z/Ctrl+Y accelerators with disabled states.
- Rejected replay during an active preview drag or palette-thumb gesture.
- Cleared PH-04 history after changed whole-preset replacements to prevent partial replay against incompatible broad state.
- Excluded runtime/evaluation/replay origins, custom-stop-deleting palette selection and mixed wider-domain mutations.
- Added unit tests, source-audit gates and canonical documentation updates.
- No settings/preset schema, project authority, renderer contract or desktop runtime ownership changed.

### Verification

- GNU 14.2 warnings-as-errors build and CTest 4/4 passed.
- Clang 17 warnings-as-errors build and CTest 4/4 passed.
- Unit coverage passes exact forward/undo/redo state equality, pan and palette-control coalescing, rotation round trip, redo branch truncation, runtime exclusion, custom-stop partial-history prevention, entry/memory bounds and clear/reset behaviour.
- `python3 scripts/verify-source.py` passed all historical gates through PH-04.
- A bounded Clang AddressSanitizer/UndefinedBehaviorSanitizer build passed the three enabled core/path/version tests 3/3 with visual fixtures disabled.
- Fresh-package and native Windows evidence are recorded separately when run for the final package.
- User-reported evidence: the preceding PH-03 complete archive builds and runs on Windows. No command transcript or logs were supplied; this does not verify the exact PH-04 archive.

### Remaining / unproven

- Native MSVC compilation and Windows interaction checks for the exact PH-04 archive.
- PH-02 approved visual baselines and any backend evidence not already supplied.
- PH-05 structural operations for custom palette stops, equations, post-processing, preset/import/dialog replacements and journey rows.

### Rollback

Remove `src/Core/ProjectHistory.*`, revert its CMake/tests/AppWindow integration and restore the PH-03 documentation state. Persisted files require no migration or rollback.


## BR-20260728-06 — PH-05 complete project undo/redo

**Status:** Implemented / partly verified
**Date:** 2026-07-28
**Scope:** Complete the documented PH-05 project-history slice over structural project edits, broad replacement transactions and Scout Apply without adding persisted history or a second project authority.

### Implemented

- Added a second history entry form, `PresetReplacementOperation`, with exact before/after authoritative `Preset` snapshots for structural or broad user transactions.
- Retained stable-key scalar/camera operations and PH-04 coalescing for complete registered parameter mutations.
- Added a scalar-completeness proof: when replaying recorded deltas cannot reproduce the exact committed state, the action is stored as one atomic project snapshot instead of partial history.
- Included post-processing scalar history, custom palette-stop insertion/removal/reorder/replacement, multi-field equations, journey rows/text, preset load, imported preset application and accepted Palette/Equation/Settings/Journey dialogs.
- Replaced the PH-04 broad-replacement history clear with labelled atomic replacement entries.
- Added full-project invalidation for structural replay and expanded main-window refresh after undo/redo across preset, camera, iterations, palette, post-processing, equation, animation and monitor/status surfaces.
- Marked startup preset population history-ineligible so application initialisation does not create an undo action.
- Added one-entry Scout Apply exact undo/redo coverage and bounded structural-memory/runtime-origin tests.
- Updated source-audit gates, roadmap status, traceability, architecture, persistence, risk and user-facing documentation.
- No settings/preset schema, project-file format, renderer contract, desktop runtime ownership or release version changed.

### Verification

- `python3 scripts/verify-source.py` passed all source structure and historical feature gates through PH-05.
- Fresh GNU 14.2 warnings-as-errors configuration/build passed CTest 4/4, including `MandelbrotCoreTests`, `MandelbrotPathTests`, `VisualFixtureSelfCheck` and `VersionConsistency`.
- Fresh Clang 17 warnings-as-errors Release configuration/build passed the same CTest 4/4 set.
- A bounded Clang 17 AddressSanitizer/UndefinedBehaviorSanitizer Debug build passed the enabled core, path and version tests 3/3 with visual fixtures disabled.
- The generated PH-05 source ZIP was extracted into a fresh directory; its source audit passed and a fresh GNU warnings-as-errors CTest run passed 4/4.
- Unit coverage passes exact structural round trips for custom palette stops, equation state, journey ordering/timing and preset application; mixed palette/post replay; implicit custom-stop deletion fallback; redo/undo labels; structural bounds; runtime exclusion; and Scout Apply.

### Remaining / unproven

- Native MSVC compilation/linking of the changed Win32 application integration for the exact PH-05 archive.
- Windows interaction checks for Undo/Redo buttons, Ctrl+Z/Ctrl+Y, preset/import/dialog transactions, control refresh and desktop-preview behaviour.
- PH-02 approved visual baselines and backend-specific GPU evidence remain pending under their existing gate.
- Persisted undo history and undo of user-settings/custom-asset-library mutations remain intentionally outside this phase.

### Rollback

Revert `src/Core/ProjectHistory.*`, the PH-05 context/eligibility additions in `src/Core/ProjectState.*`, broad-history integration and replay refresh in `src/App/AppWindow.cpp`, PH-05 tests/source-audit gates and the BR-20260728-06 documentation updates. Persisted schemas and saved files require no migration or rollback.

## BR-20260728-07 — PH-06 deterministic general animation evaluator

**Status:** Implemented / partly verified  
**Date:** 2026-07-28  
**Scope:** Complete the documented PH-06 runtime-only timeline model, validation, deterministic evaluator and independent clock foundation without adding timeline persistence or editor UI.

### Implemented

- Added `src/Core/GeneralAnimation.*` with bounded runtime-only timeline, track and keyframe models using caller-supplied stable IDs and stable target names.
- Added validated animation targets for compensated camera centre, logarithmic camera scale, wrapped rotation, palette controls, post-processing controls and selected equation parameters.
- Added strict timeline validation for ID and target syntax, unique IDs, strictly increasing keyframe times, target/value compatibility, interpolation support, duplicate enabled replacement targets, finite ranges and track/keyframe containment limits.
- Added deterministic evaluation from an immutable authoritative `Preset` snapshot into a frame-local result, with enabled tracks applied in stable track-ID order.
- Added step, linear and smoothstep interpolation; compensated high/low camera interpolation; logarithmic positive-scale interpolation; and shortest-path wrapped rotation interpolation.
- Added Clamp, Loop and PingPong time resolution, caller-provided deterministic seed carriage, aggregated invalidation classes and applied-track evidence.
- Added independent preview, wallpaper and export clocks with finite/non-negative validation and invalid-domain rejection.
- Added `EquationPrecision` invalidation so equation-track results are classified without reusing unrelated renderer domains.
- Kept timeline data out of settings/preset schemas, project history, startup state and Win32 UI under the open DEC-014 persistence decision.
- Updated unit tests, source-audit gates, roadmap, architecture, persistence, traceability, validation, risk, decisions, README and changelog.

### Verification

- `python3 scripts/verify-source.py` passed all historical source gates through PH-06.
- Fresh GNU 14.2 Release configuration/build with warnings as errors passed CTest 4/4: core tests, path tests, visual fixture self-check and version consistency.
- Fresh Clang 17 Release configuration/build with warnings as errors passed the same CTest 4/4 set.
- A fresh Clang 17 Debug build using explicit `-fsanitize=address,undefined -fno-omit-frame-pointer` compile flags and sanitizer linker flags passed the enabled core, path and version tests 3/3 with visual fixtures disabled.
- Unit coverage passes deterministic repeat evaluation, immutable base snapshots, stable track ordering, compensated-coordinate interpolation, logarithmic scale, shortest wrapped rotation, smoothstep palette evaluation, step equation integers, invalidation aggregation, Clamp/Loop/PingPong boundaries, duplicate target/time rejection, unsupported interpolation rejection, disabled unknown-target retention, history exclusion and independent clock behaviour.
- The user reported that the PH-05 archive builds and runs on Windows. That evidence confirms the preceding baseline only; native verification of this exact PH-06 archive remains pending.

### Remaining / unproven

- Native MSVC compilation/linking of the new PH-06 core files for the exact final archive.
- Windows runtime integration because PH-06 intentionally does not yet connect the evaluator to preview, wallpaper or export playback.
- PH-07 timeline editor, Journey adapter, authoring commands and user interaction evidence.
- Timeline persistence, migration/versioning and generated-ID policy remain deferred under DEC-014.
- PH-02 approved visual baselines and backend-specific GPU evidence remain pending under their existing gate.

### Rollback

Remove `src/Core/GeneralAnimation.*`, its CMake/test/source-audit integration and the `EquationPrecision` invalidation addition, then revert the BR-20260728-07 documentation updates. No persisted settings, saved presets or project files require migration or rollback.


## BR-20260729-01 — PH-07 basic animation editor and Journey adapter

**Status:** Implemented / partly verified  
**Date:** 2026-07-29  
**Scope:** Add a bounded runtime timeline editor and strict Journey adapter on the PH-06 deterministic evaluator without changing schema 9, project authority or wallpaper/export clocks.

### Implemented

- Added `ReadGeneralAnimationTargetValue`, strict Journey-to-timeline conversion, exact supported timeline-to-Journey conversion and transactional Add Current Value authoring in `src/Core/GeneralAnimation.*`.
- Added a modal `GeneralAnimationEditorDialog` with track creation/removal and enabled state, stable target selection, keyframe list/time/interpolation editing, duration and Clamp/Loop/PingPong controls, scrub/play/pause/stop, validation feedback and explicit Journey conversion commands.
- Kept the dialog candidate-owned: OK commits the runtime timeline, Cancel/close restores the prior preview time/state, and the candidate does not mutate `workingPreset_`.
- Integrated preview evaluation through `AnimationClockDomain::Preview` and a frame-local evaluated `Preset`; wallpaper and export clocks remain independent and unchanged.
- Routed an explicitly prepared lossless Tracks-to-Journey result through the existing atomic `JourneyDialog` project replacement/history boundary only after OK.
- Reset pending preview gesture coalescing before entering the modal editor so timeline preview cannot overlap a partially coalesced project gesture.
- Preserved settings/preset schema 9. General timelines remain process-runtime state under DEC-014; no timeline persistence or migration was introduced.
- Updated roadmap, architecture, persistence, UI workflow, decisions, traceability, validation, risk, README, changelog and source-audit ownership.

### Verification

- `python3 scripts/verify-source.py` passed all historical gates through PH-07.
- Fresh GNU 14.2 Release build with warnings as errors passed CTest 4/4: core tests, path tests, production visual fixture self-check and version consistency.
- Fresh Clang 17 Release build with warnings as errors passed the same CTest 4/4 set.
- Fresh Clang 17 Debug build using explicit AddressSanitizer/UndefinedBehaviorSanitizer compile/link flags passed the enabled core, path and version tests 3/3 with visual fixtures disabled.
- PH-07 unit coverage passes strict row rejection with unchanged output, one-pass Journey timing, exact holds, exact supported round trip, unsupported enabled-target refusal, compensated-low loss refusal, authoritative Add Current Value and duplicate-time rollback.
- Structural source checks confirm the Win32 editor is linked, exposes the bounded authoring/playback/conversion controls, uses only the preview clock, renders from a frame-local evaluated preset and applies accepted Journey text through the existing atomic replacement route.
- The configured Windows preset remains `Visual Studio 18 2026`.

### Remaining / unproven

- Native Visual Studio 18 2026/MSVC compilation and linking of `GeneralAnimationEditorDialog.cpp` and the AppWindow integration for this exact archive.
- Windows dialog creation, DPI/responsive layout, keyboard navigation, scrubbing/playback interaction and visual preview behaviour.
- Accepted general timelines are runtime-only and are lost on application exit by design until DEC-014 selects a persisted format and migration policy.
- Timeline integration with wallpaper playback and deterministic export remains future work; PH-08 is the next roadmap phase.
- PH-02 approved visual baselines and backend-specific native GPU evidence remain pending under their existing gates.

### Rollback

Remove `src/App/GeneralAnimationEditorDialog.*`, its CMake/AppWindow integration and the PH-07 adapter/authoring functions/tests, then restore the PH-06 render path and documentation state. No persisted settings or preset migration is required; a Journey string created through explicit reverse conversion can be undone through the existing project-history entry during the same session.

## BR-20260729-02 — PH-08 deterministic PNG frame-sequence export

**Status:** Implemented / partly verified  
**Date:** 2026-07-29  
**Baseline:** User-reported successful PH-07 Windows build/run; PH-07 VS2026 source archive used as the implementation baseline.

### Implemented

- Added platform-neutral `FrameSequenceExport` jobs with immutable normalised preset/timeline/settings snapshots, bounded rational frame rates/counts/naming, direct-index time mapping, stable timeline canonicalisation and SHA-256 project/timeline/job fingerprints.
- Added schema-1 manifest and per-frame prepared receipts under `.mw-frame-sequence`, temporary `.part` rendering, expected-dimension validation, byte-length/FNV-1a64 verification, no-overwrite final promotion, atomic manifest backup/recovery and exact typed/fingerprint resume refusal.
- Added deterministic cancellation/progress semantics that stop new work, remove only the current incomplete temporary frame and preserve verified promoted frames/metadata.
- Extracted the existing high-resolution WIC scanline encoder into reusable Windows integration and added metadata-only WIC dimension validation.
- Added the modal `Export PNG Frames...` route with output folder, dimensions, DPI, common rational frame rates, aligned-end policy, prefix, resume, progress, cancellation and Open Folder.
- Routed frames through `EvaluateGeneralAnimation` and production `RenderStillImageTiled` with fixed seed, preset iterations/AA and `scaleQualityToResolution=false`.
- Added AppWindow policy to pause and release active wallpaper GPU resources where possible around the modal export route, then resume.
- Preserved settings schema 9, runtime-only timeline policy and project-history exclusion.

### Ran and passed in the portable environment

- PH-08 source audit through all prior phase gates.
- GNU 14.2 Release warnings-as-errors build and CTest 4/4.
- Clang 17 Release warnings-as-errors build and CTest 4/4.
- Clang AddressSanitizer/UndefinedBehaviorSanitizer build and CTest 3/3.
- Final clean ZIP extraction, source audit, fresh GNU warnings-as-errors build, production visual fixture self-check and CTest 4/4.
- ZIP integrity and SHA-256 verification.

### Evidence boundary

- Portable tests cover rational frame timing/counts, immutable identity, cancellation preservation, matching resume, project/output and typed-manifest mismatch refusal, malformed metadata rejection, and deterministic start/middle/end production CPU frames.
- Source audit proves the intended Win32 wiring and lifetime policy only. Native Visual Studio 18 2026 compilation, WIC PNG creation/reopen, dialog/DPI/keyboard interaction, real cancellation and wallpaper pause/resume are pending for this exact archive.
- User-reported PH-07 build/run evidence applies only to the preceding archive and does not verify PH-08 Windows-specific code.

### Rollback

Return to the PH-07 archive. No settings or project schema migration is required; user-created PH-08 output directories can be retained or removed independently.

## BR-20260729-03 — PH-08 Visual Studio COM declaration build fix

**Status:** Implemented / source verified  
**Date:** 2026-07-29  
**Scope:** Correct the user-reported Visual Studio 18 2026 compilation failure in the PH-08 high-resolution render dialog without changing runtime behaviour or persisted contracts.

### Evidence

The user supplied a native build log from MSVC 19.51.36248.0 showing `HighResRenderDialog.cpp` failed at the render-worker COM initialisation calls because `COINIT_MULTITHREADED`, `CoInitializeEx` and `CoUninitialize` were undeclared. The failure occurred before application linking or runtime validation.

### Implemented

- Added the explicit Windows COM declaration header `<objbase.h>` to `src/App/HighResRenderDialog.cpp` under `_WIN32`.
- Preserved the existing `ole32` link dependency, COM apartment behaviour, render-thread lifetime and WIC/GPU/CPU output paths.
- Added a PH-08 source-verification rule that rejects this file when it uses `CoInitializeEx` without the explicit declaration header.
- No settings schema, project format, renderer algorithm, export manifest, UI flow or release version changed.

### Verification

- `python3 scripts/verify-source.py` passes through PH-08 after the fix.
- Portable warnings-as-errors core build and CTest are rerun for the corrected package; these checks do not compile the Win32 application target.
- The exact Visual Studio 18 2026 rebuild remains pending because the current execution environment cannot run MSVC.

### Rollback

Remove the `<objbase.h>` include, the associated source-verifier check and this delivery record. No data migration or output cleanup is required.

## BR-20260729-04 — PH-09 external FFmpeg H.264/MP4 export

**Status:** Implemented / partly verified  
**Date:** 2026-07-29  
**Baseline:** User-reported successful corrected PH-08 Windows build/run; corrected PH-08 source archive used as the implementation baseline.

**Objective:** Add optional encoded-video output over the verified PH-08 PNG sequence without shell interpolation, source-frame loss, bundling or persistence-schema changes.

### Implemented

- Added `ExternalVideoExport` capability parsing, immutable job identity, complete sequence/digest verification, fixed H.264/MP4 and decode-probe argument vectors, verified promotion and manifest-scoped cleanup.
- Added `ExternalProcess` direct `CreateProcessW` execution with independent bounded stdout/stderr capture, timeouts, progress delivery and owned-process cancellation.
- Added the modal `VideoExportDialog` and Preview-page **Encode MP4...** route with explicit executable selection/`PATH` discovery, bounded preset/CRF choices, cancellation and optional post-success cleanup.
- Preserved settings/preset schema 9, runtime-only timelines, project undo exclusions and the external user-supplied FFmpeg boundary. No encoder binary, download path or executable path persistence was added.
- Accepted DEC-009 and DEC-027 and updated architecture, security, persistence, UI, traceability and offline-export records.

### Ran and passed in the portable environment

- `python3 scripts/verify-source.py` through all PH-09 source and documentation gates.
- GNU 14.2 Release warnings-as-errors build with production visual fixtures; CTest 4/4.
- Clang 17 Release warnings-as-errors build with production visual fixtures; CTest 4/4.
- Clang 17 AddressSanitizer/UndefinedBehaviorSanitizer Debug build with visual fixtures disabled; CTest 3/3.
- The final packaged archive is additionally rebuilt from a clean extraction before handoff.

### Verification boundary

- Portable core/source checks cover capability rejection, complete source validation, fixed no-shell vectors, process-result handling, failure/cancellation frame preservation, decode-before-promotion and cleanup containment.
- The user reports the corrected PH-08 archive builds and runs on Windows; this confirms the baseline only.
- Native Visual Studio 18 2026 compilation of PH-09, real FFmpeg probing/encoding/decoding, Win32 progress/cancellation and wallpaper pause/resume are unproven until run against this exact archive.

### Rollback

Remove `src/Core/ExternalVideoExport.*`, `src/WindowsIntegration/ExternalProcess.*`, `src/App/VideoExportDialog.*`, their CMake/AppWindow/test/source-audit registrations and PH-09 documentation entries. The PH-08 PNG sequence workflow remains valid and no data migration is required.

## BR-20260729-05 — Current native Release build evidence

**Status:** Native Release compile/test evidence recorded; runtime gates remain open  
**Date:** 2026-07-29  
**Scope:** Reconcile the current project build with the PH-01–PH-09 evidence boundary.

### Evidence inspected

- `build/CMakeCache.txt` records Visual Studio 18 2026, x64, `MW_BUILD_TESTS=ON` and `MW_WARNINGS_AS_ERRORS=ON`.
- `build/Release` contains the application, core tests, path tests and visual-fixture executable.
- `build/Testing/Temporary/LastTest.log` records native Release CTest 4/4 passed: core, path isolation, visual fixture self-check and version consistency.
- `scripts/verify-source.py` passes through PH-09.

### Result

The current build is accepted as evidence of native MSVC Release compilation/linking and automated test execution for the current PH-01–PH-09 source. PH-03–PH-09 documentation now records native compile/test evidence rather than treating it as absent. PH-01 and PH-02 remain incomplete because runtime/package/manual and approved-baseline/GPU gates are not covered. PH-03–PH-09 remain incomplete as full release phases until their runtime-specific interaction, lifecycle, WIC, cancellation, GPU or real-FFmpeg evidence is run as applicable.

### Not claimed

No manual Windows matrix, installer/upgrade run, GPU WARP/hardware/OpenGL validation, dialog/DPI/keyboard test, wallpaper pause/resume test, or real FFmpeg probe/encode/decode was inferred from the build artifacts.

## BR-20260729-06 — PH-01 runtime smoke attempt

**Status:** Failed runtime smoke; later runtime gates stopped  
**Date:** 2026-07-29  
**Scope:** First non-destructive runtime check against the existing native Release build.

### Command and evidence

- Ran `powershell.exe -NoProfile -ExecutionPolicy Bypass -File scripts/run-current-build-runtime-smoke.ps1`.
- The application started with isolated app data and logged `Application startup.`.
- The application initialized `Direct3D 11 renderer started: AMD Radeon RX 7900 XT / Direct3D 11.1`.
- The canonical `MandelbrotLiveWallpaperControl` window was not found within the 30-second bound. A diagnostic fallback was tested once and identified `UAC_InputIndicatorOverlayWnd`, not the application window; it was removed from the helper.
- The application did not receive the real Exit command. No clean process exit or `Application shutdown.` log marker was observed.

### Result

PH-01 runtime smoke is failed/unproven. Verification stops here in canonical order; PH-02 and later runtime/UI/GPU/FFmpeg checks are not claimed.

## BR-20260729-07 — PH-01 corrected isolated runtime smoke

**Status:** Passed isolated runtime smoke; PH-01 manual matrix remains open  
**Date:** 2026-07-29  
**Scope:** Diagnose and correct the BR-20260729-06 false-negative window-discovery result.

### Root cause

The application was not failing to load with Direct3D 11. Its isolated log showed successful D3D11 initialization, and a process-scoped window enumeration found the visible `MandelbrotLiveWallpaperControl` window owned by the launched executable. The helper's global `FindWindowW` lookup returned zero despite that window and produced the false failure.

### Corrected evidence

- Updated the helper to enumerate top-level windows and select the expected class owned by the launched process ID.
- Re-ran the isolated smoke against `build/Release/MandelbrotWallpaper.exe`.
- The main window was found, the real tray Exit command was processed, the application exited with code 0, and the isolated log contained both startup and shutdown markers.

### Boundary

This passes the PH-01 isolated startup/window/D3D11/clean-shutdown check. It does not replace the remaining manual Windows, packaging, installer, upgrade, GPU-backend or lifecycle matrix.

## BR-20260729-08 — PH-01 portable package smoke and PH-02 native CPU fixtures

**Status:** Passed within automated local scope; PH-01 manual and PH-02 GPU gates remain open  
**Date:** 2026-07-29  
**Scope:** Continue canonical verification using current native artifacts without overwriting the protected existing build tree.

### Passed

- Inspected `dist/Mandelbrot-Live-Wallpaper-1.13.1-win-x64.zip`: required executable, README and license are present; no local settings, logs or build artifacts are packaged. SHA-256: `31fe3c222c81740ec6a500e4597143a15485a4ff27bf2dcb27ffe8a9d173f622`.
- Extracted that ZIP into a fresh temporary directory. Its executable passed the isolated process-scoped startup/window/D3D11/clean-exit smoke with exit code 0 and startup/shutdown log markers.
- Ran the current native `MandelbrotVisualFixtures.exe` suite in a fresh temporary artifact directory. All six CPU production-renderer checks passed.

### Blocked / unproven

- Inno Setup is not installed, so installer generation/install/upgrade/uninstall evidence cannot be run.
- PH-01 manual OS, display, lifecycle and desktop-mode matrix remains external runtime evidence.
- PH-02 still requires reviewed CPU baselines and new D3D11 WARP/offscreen, OpenGL offscreen, bloom and perturbation fixtures. The current host cannot configure a fresh MSVC build because no C++ compiler/toolchain is exposed, so that implementation cannot be compiled or verified here.

### Canonical handoff

Do not advance PH-02 to complete or claim PH-03–PH-09 full runtime completion until the PH-02 GPU/baseline work is built and its required evidence is recorded.

## BR-20260729-09 — Fresh MSVC Release rebuild and CTest

**Status:** Passed within automated native build/test scope; PH-02 remains incomplete  
**Date:** 2026-07-29  
**Scope:** Rebuild and test the current archive using the installed Visual Studio 18 Build Tools x64 environment.

### Run

- Activated `G:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Auxiliary\Build\vcvars64.bat` and rebuilt the `codex-msvc-release` preset.
- The sandboxed rebuild stopped in MSBuild FileTracker with `E_ACCESSDENIED` before project compilation. The same scoped command was rerun outside that sandbox.
- The native build compiled and linked `MandelbrotWallpaper`, `MandelbrotCoreTests`, `MandelbrotPathTests` and `MandelbrotVisualFixtures`.
- CTest passed 4/4: core tests, path tests, CPU production visual fixtures and version consistency (22.80 seconds).

### Result

Fresh native MSVC compile/link/test evidence is passed for the current archive. PH-02 remains in progress: approved CPU baselines, D3D11 WARP/offscreen fixtures, supported OpenGL fixtures and actual GPU bloom/perturbation validation are still unimplemented and unproven.

## BR-20260729-10 — D3D11 WARP production readback fixture

**Status:** Passed within bounded WARP fixture scope; PH-02 remains incomplete  
**Date:** 2026-07-29  
**Scope:** Add and verify the first reproducible Windows GPU fixture without changing production hardware rendering.

### Changed

- Added a test-only WARP initialization option to `Direct3D11Renderer`; all existing app callers retain hardware D3D11.
- Added `MandelbrotD3D11WarpFixtures` and its CTest entry. It uses a hidden production window, fixed 256x144 scene/time, real production render, output-texture readback, dimension/non-uniform checks and exact repeated-frame equality.

### Verification

- Fresh Visual Studio 18 x64 Release build compiled and linked the fixture.
- Direct fixture execution passed.
- `ctest --test-dir build-codex-runtime -C Release -R D3D11WarpFixtureSelfCheck --output-on-failure` passed 1/1 in 9.19 seconds.

### Remaining / unproven

- Reviewed CPU/GPU baselines and threshold calibration.
- Expanded WARP scenes, hardware D3D11 comparison, supported OpenGL offscreen fixture, and actual bloom/perturbation coverage.

## BR-20260729-11 — D3D11 WARP bloom and perturbation expansion

**Status:** Passed within bounded WARP fixture scope; PH-02 remains incomplete  
**Date:** 2026-07-29

- Expanded the production WARP fixture from its standard scene to fixed bloom-enabled and explicit perturbation scenes.
- Direct execution produced exact non-uniform repeated readbacks for all three scenes at 256x144.
- The isolated CTest entry passed 1/1 in 9.32 seconds.
- Remaining PH-02 gates: reviewed baselines/threshold calibration, hardware D3D11 evidence and supported OpenGL offscreen evidence.

## BR-20260729-12 — Hardware D3D11 production readback fixture

**Status:** Passed on the recorded adapter; PH-02 remains incomplete  
**Date:** 2026-07-29

- Added explicit hardware mode to the WARP fixture without changing its WARP CTest default.
- Direct native execution passed standard, bloom and perturbation production readback exact-repeatability at 256x144 on AMD Radeon RX 7900 XT / Direct3D 11.1.
- Remaining: reviewed baselines/threshold calibration and supported OpenGL offscreen evidence.

## BR-20260729-13 — OpenGL production readback fixture

**Status:** Passed on the recorded OpenGL environment; PH-02 baseline approval remains open  
**Date:** 2026-07-29

- Added hidden-context production OpenGL fixture through `GpuRenderer` with explicit OpenGL preference.
- Standard, bloom and perturbation readbacks passed direct and isolated CTest (1/1, 0.37 seconds) on the recorded AMD OpenGL 4.6 context.
- Remaining PH-02 gate: reviewed visual baselines and calibrated thresholds.

## BR-20260729-14 — PH-02 approved CPU baselines

**Status:** PH-02 complete  
**Date:** 2026-07-29

- User approved the PH-02 visual baselines.
- Promoted the four canonical CPU candidates as reviewed `baseline.ppm` assets and recorded exact thresholds in `tests/baselines/visual/APPROVAL.md`.
- Strict baseline comparison passed for all four fixtures; seam equality and deliberate mutation detection also passed.
- PH-02 is complete. PH-03 is the next canonical phase for remaining runtime-specific verification.

## BR-20260729-15 — PH-03 native core verification

**Status:** Native compile/core evidence passed; PH-03 interaction gate remains open  
**Date:** 2026-07-29

- Current native Release `MandelbrotCoreTests.exe` passed.
- Native MSVC Release compilation/linking covers the current PH-03 application integration.
- Remaining: Windows interaction evidence for gesture coalescing, transactional replacements and preview behaviour.

## BR-20260729-16 — PH-03 preview-navigation history correction

**Status:** Native core regression passed; runtime retest pending  
**Date:** 2026-07-29

- Fixed history coalescing that rejected navigation events when they changed different subsets of compensated camera fields.
- Same-token preview navigation now merges camera-field unions while preserving exact undo/redo endpoints.
- Added and passed a native core regression test. User interaction retest remains required.

## BR-20260729-17 — PH-03 live dialog preview and candidate rollback

**Status:** Native compile/test evidence passed; Windows interaction retest pending  
**Date:** 2026-07-29

- Palette, Equation, Settings and Journey use temporary candidate state for live preview and rollback on Cancel.
- Palette and Equation saved libraries no longer mutate persisted settings until their dialogs are accepted.
- Settings preview candidate render options are isolated from the persisted settings until acceptance; Journey Apply no longer counts as dialog acceptance.
- Current MSVC Release application build linked successfully. Release CTest passed all six targets: core, path, CPU visual, D3D11 WARP, OpenGL and version consistency.
- Remaining: manual Windows evidence that each dialog updates the preview while open and that Cancel restores both preview and persisted state.

## BR-20260729-18 — PH-03 runtime interaction confirmation

**Status:** PH-03 complete at documented scope  
**Date:** 2026-07-29

- User-confirmed Windows runtime evidence: preview navigation undo works; Palette, Equation, Settings and Journey preview candidate changes live and Cancel rolls them back.
- BR-20260729-16/17 provide the matching native Release build/link and complete CTest evidence.
- PH-03 is complete at its parameter-adapter/render-fingerprint scope. PH-04 camera/palette undo/redo interaction is the next canonical task.

## BR-20260729-19 — PH-04 runtime interaction confirmation

**Status:** PH-04 complete at documented scope  
**Date:** 2026-07-29

- User-confirmed Windows runtime evidence: camera and palette/post-control Undo/Redo work, and a divergent edit after Undo correctly removes the Redo branch.
- Native Release compile/link and portable/core history regression evidence are recorded in preceding deliveries.
- PH-04 is complete at its bounded camera/palette history scope. PH-05 structural replacement interaction is the next canonical task.

## BR-20260729-20 — PH-05 palette replacement interaction

**Status:** Partial PH-05 Windows interaction evidence  
**Date:** 2026-07-29

- User clarification: the reported Palette Editor undo/redo failure was observed in a different Android-source build. The Windows native build under this programme correctly Undoes and Redoes accepted Palette Editor changes.
- Corrected the Palette Editor acceptance boundary so only OK commits its candidate preset/library; Cancel cannot produce a phantom accepted replacement.
- Native Release application rebuilt/linked and `MandelbrotCoreTests` passed after the correction. Equation, Settings, Journey, preset/import and Scout Apply runtime checks remain.

## BR-20260730-01 — Deep-zoom upgrade pack integration

**Status:** Proposed PH-12–PH-15 programme integrated; structural/native build/test baseline passed; no deep implementation or schema change  
**Date:** 2026-07-30

### Inspected

- Read the complete combined deep-zoom pack and the live canonical owners.
- Audited the proposed phase, requirement, acceptance, validation, decision, risk and route ranges; all were unused before this integration.
- Rechecked the live camera/precision/render/export limitations against current source evidence described by the pack.
- Confirmed this checkout has no `.git` and no Git bundle, so no branch, commit or clean-tree result is claimed.

### Changed

- Integrated REQ-021–REQ-040, AC-021–AC-040, proposed PH-12–PH-15, VAL-037–VAL-060, DEC-028–DEC-037, RISK-013–RISK-030 and ROUTE-015–ROUTE-018 into their canonical owners.
- Added exact-camera, planner/orbit, validity/correction, exact mapping, animation/Scout, export, diagnostics, resource, migration and support-claim contracts to the affected architecture/feature owners.
- Kept the combined file as a supporting design alias; canonical owners now define accepted statuses and mappings.
- Preserved the active PH-05 interaction pointer. No phase was marked implemented or passed from the proposal.
- Reconciled the stale source audit to the accepted shared 500 ms `PreviewNavigation` implementation and live dialog candidate preview source. DEC-038 supersedes the prior 250 ms split gesture rule.
- Changed no C++ application code, schema, durable file format, fingerprint version, renderer policy, external dependency or FFmpeg boundary in this delivery.

### Ran and passed

`C:\Python314\python.exe scripts\verify-source.py`

The initial run failed on stale `PreviewPan`; two further runs identified stale PH-05 evidence wording and a pre-dialog-preview PH-07 marker. After narrow reconciliation, the final complete source audit passed through PH-09 plus the proposed PH-12–PH-15 governance markers.

`cmake --build build --config Release --parallel`

The first sandboxed run failed because Visual Studio `FileTracker` could not access dependency paths (`E_ACCESSDENIED`). The permitted native retry succeeded and linked the wallpaper plus all Release test/fixture targets.

`ctest --test-dir build -C Release --output-on-failure`

Native Release CTest passed 6/6 in 14.66 seconds: core, path isolation, CPU visual, D3D11 WARP, OpenGL and version consistency.

### Failed, skipped and unproven

- Initial structural runs failed as recorded above; their defects were corrected before the final pass.
- PH-12–PH-15 implementation, exact parser/migration, native/GPU numerical evidence, deep resource/lifecycle evidence and release support remain unproven.
- At BR-20260730-01, the listed authority was open. BR-20260730-02 subsequently records accepted user direction for DEC-015, forward-only schema 10/preset schema 3 migration, DEC-036 and PH-10/PH-11 disposition; the exact high-precision dependency's licence/package review and measured thresholds remain unproven.

## BR-20260730-02 — Deep-zoom governance decisions

**Status:** User decisions recorded; no implementation, schema or package mutation
**Date:** 2026-07-30

### Changed

- Accepted split ownership for precision intent: project/preset render intent, user defaults/local presentation preferences, and transient capability/execution/adaptive state.
- Selected a reviewed additional high-precision implementation behind the platform-neutral boundary, subject to a specific dependency licence/package review before code or installer changes.
- Assigned settings schema 10 and preset schema 3 for a forward-only, unreleased single-instance migration; no old-format downgrade writer will be implemented, while the pre-migration local file remains preserved.
- Accepted user-selected CPU-reference or validated-GPU export renderer, recorded in immutable job/manifest identity with no silent GPU-to-CPU fallback.
- Allowed PH-12 to proceed ahead of PH-10/PH-11 using only direct scoped dependencies and without a predecessor-completion claim.

### Unproven

- At BR-20260730-02, exact package/library selection, licence compatibility, native MSVC/x64 packaging, numerical correctness, performance, cancellation and all PH-12+ implementation evidence remained unproven. BR-20260730-03 subsequently resolves the concrete package and source/notice packaging decision.

### Rollback

Revert the BR-20260730-02 governance-only documentation and verifier markers. Current schemas, renderers and user data were not mutated. A Git rollback identifier is unavailable in this source checkout.

## BR-20260730-03 — High-precision package and licence review

**Status:** Source-integrated; native Release build and CTest 6/6 passed; production-route acceptance pending  
**Date:** 2026-07-30

### Objective

Resolve DEC-029's concrete high-precision package, licence and packaging stop condition with an inspectable, reproducible and bounded implementation without beginning PH-12 persistence or production-renderer migration.

### Inspected

- Every path in `dec029-boost-multiprecision-package-review-source`, with task-scoped content comparison against the live root.
- The supplied package manifest, 178-file hash inventory, BSL-1.0 text, notice policy, CMake/installer integration, project-owned backend, tests and governance.
- Existing custom `FixedReal`, `BuildReferenceOrbitArbitrary`, registered perturbation profiles, compensated camera input and current 128/256/512-bit intent.

### Implemented

- Vendored `third_party/boost-multiprecision-1.83.0/` as a hash-locked standalone source subset with BSL-1.0 and no binaries.
- Added `scripts/verify-third-party.py`; source verification now checks package integrity, CMake/backend markers and notice packaging.
- Added private CMake interface target `MandelbrotBoostMultiprecision`, `BOOST_MP_STANDALONE`, an exact Boost version check and no package-manager/network lookup.
- Added project-owned `src/Core/Precision/HighPrecisionBackend.h/.cpp` using fixed 512-bit `cpp_bin_float` with expression templates disabled.
- Added bounded reference-orbit comparisons against the existing arbitrary path, compensated-camera and Tricorn coverage, plus fail-closed tests for unsupported profiles and non-finite input.
- Added `THIRD_PARTY_NOTICES.md` to CMake and Inno Setup package contents.
- Accepted DEC-029 for this pin and updated the canonical dependency, persistence, validation, security, index, changelog and operating-rule records.

### Contract impact

- No settings or preset schema implementation changed.
- No exact-camera migration was performed.
- No production precision planner, renderer fallback, fingerprint, output format or user-visible workflow changed.
- No Boost type appears in a public project interface.
- No runtime DLL, static library, service, executable or installer registration was added.

### Verification

Passed on the final integrated root:

- `C:\Python314\python.exe scripts\verify-third-party.py`: package identity, exact reviewed file set, content-index digest and all 178 source/licence file digests passed.
- `C:\Python314\python.exe scripts\verify-source.py`: complete source/governance audit passed through PH-09 plus the proposed PH-12–PH-15 markers and accepted DEC-029 package authority.
- `cmake --build build --config Release --parallel`: Visual Studio 18 2026 x64 Release compiled and linked `HighPrecisionBackend.cpp`, `MandelbrotCore`, the application, core/path tests and CPU/D3D11/OpenGL fixture targets with warnings-as-errors.
- `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed in 15.04 seconds—core, path isolation, CPU visual, D3D11 WARP, OpenGL and version consistency.
- `cmake --install build --config Release --prefix build-codex-runtime\package-review-install`: installed the application, project licence, README, presets and `THIRD_PARTY_NOTICES.md`; inspection found no `.dll` or `.lib` in the staged package.

The first sandboxed build attempt failed before compilation because Visual Studio `FileTracker` could not access dependency paths (`E_ACCESSDENIED`); the permitted native retry passed. A separate fresh candidate-tree configure was blocked by duplicate `Path`/`PATH` entries in the host MSBuild process environment and is not counted as product evidence.

### Remaining unproven

- Production-route cancellation, performance and memory measurements because this delivery adds an independent test/reference backend rather than connecting it to long-running production work.
- PH-12 exact parsing, migration, planner integration, GPU correction and measured deep release thresholds.
- Byte-for-byte equivalence of the Debian-provenance imported files to the named upstream archive; the reviewed local package hashes are the source-package authority.

### Rollback

Remove the vendored package, verifier, reference backend, tests, notices and CMake entries as one bounded change. User data and durable schemas are unaffected. A Git rollback identifier is unavailable in this source checkout.

## BR-20260730-04 — PH-10 Scout candidate identity

**Status:** Partial PH-10 implementation; native Release build/test passed; dialog interaction and visual fixtures pending  
**Date:** 2026-07-30

### Implemented

- Added a SHA-256 `FRACTAL-SCOUT-CANDIDATE/V1` identity to each temporary Scout candidate.
- The identity includes canonical source/candidate render fingerprints, explicit search camera, goal and resolved bounded limits; it deliberately excludes transient ranked-list position.
- Kept candidate presets suitable for Apply/Save while deriving their identity from a separate canonical thumbnail-render projection.
- Extended the core regression test to require valid SHA-256 identities and equality across repeated identical searches.

### Evidence passed

- `cmake --build build --config Release --parallel`: Visual Studio 18 2026 x64 Release compiled and linked the application, Scout core/UI, tests and fixture targets.
- `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed in 14.80 seconds, including the new Scout identity regression.

### Remaining unproven

- Native interaction proof that the Scout dialog's temporary preview remains isolated and its selected Apply action remains one undoable transaction.
- Stable visual-thumbnail fixture baselines and common job-reporting resource integration.
- Exact-camera, generation/revalidation and all PH-12/PH-15 deep integration.

### Rollback

Remove the Scout identity member/helper and associated core test/documentation as one bounded change. No persisted schema or user data changed; this checkout has no Git rollback identifier.

## BR-20260730-05 — PH-12 exact-decimal and legacy-adapter foundation

**Status:** Partial PH-12 implementation; native Release build/test passed; durable migration and production integration pending  
**Date:** 2026-07-30

### Decision and implementation

- Under delegated user authority, accepted DEC-028's ASCII-only exact decimal grammar: optional sign, decimal point and `e`/`E` exponent; no whitespace, separators or non-finite values.
- Canonical non-zero values use normalized scientific form and lowercase `e`; zero is always `0`. One value is bounded to 16 KiB input, 8,192 significant digits and normalized exponent magnitude 1,000,000.
- Added platform-neutral `ExactDecimal`, `ExactCamera` and one-way `AdaptExactCameraToLegacy` core contracts. Canonical text is retained before any floating-point conversion; unrepresentable/non-finite legacy values fail closed and representable lossy conversions report loss.
- Added parser, canonicalisation, bounds, exact-adapter, loss and fail-closed core tests. The subsequent PH-12 continuation added schema-3 exact preset text, schema-10 settings, schema-2 legacy reconstruction and original-preserving/re-read-verified local promotion; fingerprint, renderer, Journey, Scout and export migration remain pending.
- Added `mw-render-state-v2-exact-camera`: a separate SHA-256 canonical stream using exact centre/half-height strings. It rejects absent exact state and coexists with, rather than reinterpreting, `mw-render-state-v1`.
- Frame-sequence export now creates only schema-2 manifests, records `mw-render-state-v2-exact-camera` explicitly and rejects a prior schema-1/v1 manifest rather than resuming it against rounded camera state.
- Main-window coordinate-triplet and X/Y/scale entry now parse `ExactDecimal`, retain canonical exact text, and adapt a legacy camera only for the current preview path; non-adaptable values are rejected without replacing exact state.
- Jump-to-Coordinates now uses the same exact triplet parser and adapter, while preset loading reuses the exact-text control synchronisation; the active animation and high-resolution rendering paths remain legacy-adapter consumers.
- High-resolution export now retains the parsed exact camera in its immutable request and only derives the legacy adapter immediately before invoking current CPU/GPU renderers. Native dialog interaction and numerical deep-render equivalence remain unproven.
- Added a platform-neutral symbolic global-pixel mapping contract: exact camera text plus rational horizontal/vertical half-height factors. Rotation is deliberately surfaced as an explicit precision-adapter requirement; current render loops are not represented as exact execution.
- The accepted independent Boost 512-bit reference backend now accepts `ExactCamera` directly. It preserves only its measured 154-significant-decimal-digit envelope and fails closed outside it; this is not a claim that the full 8,192-digit persistence grammar is executable by that backend.
- Scout candidates now rebuild exact camera authority after their generated legacy camera changes, before identity/thumbnail work; applying one carries that exact state through the existing single `ScoutApply` history transaction.
- Animated preview snapshots now rebuild exact camera authority when their runtime frame differs from the persisted working camera, preventing stale exact text from reaching Scout, static capture or high-resolution export snapshots.
- General Animation now rebuilds exact camera authority whenever X, Y or scale tracks change a frame-local preset; regression coverage checks it against the canonical legacy reconstruction.
- Scout candidate identity now uses the exact-camera v2 fingerprint for source/candidate state and canonical exact search-camera text, creating a distinct V2 identity rather than reinterpreting prior candidates.
- Added the platform-neutral deterministic `PrecisionPlanner`: it derives a required precision from exact camera text, selects only Float64 or the accepted 512-bit CPU reference when sufficient, classifies formula capability, and otherwise refuses rather than silently lowering precision. The current Float64 preview, still-render and frame-export paths now ask it before rendering; they refuse unsupported exact cameras rather than claiming an exact deep render. The 512-bit route is not yet a production pixel renderer.
- Added `ReferenceOrbitService`: immutable exact-camera/plan requests backed only by the accepted Boost-512 reference tier, registered-profile/iteration validation, cooperative cancellation, byte- and entry-bounded LRU reuse, and caller-generation stamping on cache reuse. It remains synchronous and detached from pixel rendering; this is PH-13 service groundwork, not a deep-render completion claim.
- Added stable deep-formula capability identities: `analytic-quadratic-mandelbrot/v1` and `tricorn-power2/v1`. The reference service returns the selected identity/version and refuses an equation whose resolved profile no longer matches its immutable precision plan.
- Added a bounded Boost-512 direct exact-pixel escape primitive. It evaluates canonical camera text plus rational global-pixel offsets without a legacy `CameraState` conversion, agrees with ordinary CPU escape classification for a representable fixture, and fails closed for rotation or cancellation. It remains detached from colouring, tiling, exports and GPU execution.
- Added `RenderExactDirectStillImage`: a streamed-row, unrotated AA-1 Boost-512 direct still route using the same CPU colour mapping. A small representable fixture matches ordinary CPU pixels exactly; request validation refuses absent writer, rotation, AA above one and out-of-bound iterations. It is not yet wired to UI/export selection or claimed as broad deep-render support.
- High-resolution CPU export now selects `RenderExactDirectStillImage` only when its central plan chooses Boost-512 and the request is CPU, unrotated and AA-1; otherwise it retains the legacy renderer or gives an explicit refusal. The changed Win32 dialog source compiled, but final application link/runtime proof is deferred while the active `MandelbrotWallpaper.exe` locks its output file.
- Core regression now renders a complete 8×6 streamed direct still at exact half-height `1e-40` with Boost-512 and no legacy camera adaptation. This proves one bounded deep-coordinate execution tier only; dynamic precision, broad numerical fixtures, AA/rotation, frame/video and GPU parity remain unproven.
- Frame-sequence core now recognizes immutable renderer ID `cpu-exact-boost512-v1`. Its distinct exact fingerprint/manifest identity and base/per-frame planner checks prevent it from being resumed or represented as legacy Float64 CPU output. The Win32 dialog now explicitly offers CPU Float64 compatibility or CPU exact Boost-512; selecting the latter dispatches the streamed direct row renderer and refuses unsupported rotation, anti-aliasing, formula, or exact-camera input without fallback. Compilation/runtime acceptance remains pending because the active executable locks the final link target.
- Near-unlimited zoom direction is now explicit: exact camera text stays authoritative, and CPU execution will use a planner-owned adaptive Boost ladder (512, 2,048, then 8,192 bits) rather than asserting a fixed 512-bit ceiling is unlimited. Each request must select an equal-or-higher bounded tier or refuse; stored coordinates above the enabled execution cap remain valid data but are not silently rounded into an export.
- Implemented the first adaptive execution extension: `CpuBoost2048Direct` is a separate planner backend with a fixed 2,048-bit Boost evaluator. High-resolution export selects it automatically when 512 bits are insufficient, while frame export exposes the separate immutable renderer ID `cpu-exact-boost2048-v1`. A `1e-200` exact direct-still regression completes through that tier without a legacy camera adapter; the existing 512-bit reference-orbit cache remains intentionally limited to its validated tier.
- Completed the currently accepted direct CPU ladder with `CpuBoost8192Direct`, the fixed 8,192-bit Boost evaluator and immutable renderer ID `cpu-exact-boost8192-v1`. The planner selects it only when lower tiers cannot satisfy the exact request; a `1e-1000` direct-still regression and immutable frame-job regression pass without legacy camera adaptation. This is bounded high precision, not a claim of unlimited compute, arbitrary formula support, correction, GPU parity or video acceptance.
- Upgraded frame-export identity to forward-only schema 3 / `mw-render-state-v3-exact-precision-plan`. It binds exact camera text plus planner version, capability, reason, required bits, selected bits and backend; schema-1/v1 and schema-2/v2 manifests are refused. Core coverage proves one exact camera cannot resume across the 2,048-bit and 8,192-bit renderer routes.
- Added `ReferenceOrbitWorker`: a single deterministic, bounded background worker over the existing validated 512-bit reference service. Equal numerical requests coalesce, each subscriber keeps its own generation, cancellation suppresses delivery/publication, and shutdown cancels owned work. Core coverage verifies two generation-distinct subscribers receive the same bounded orbit result. There is no renderer upload, correction, GPU or 2,048/8,192-bit reference-orbit claim.
- Named the existing renderer transport as `mw-orbit-float4-expansion/v1`: four 32-bit float expansion components per coordinate. The descriptor deliberately reports no validated bit ceiling; measurement and backend adoption remain future evidence-gated work.
- Fixed bounded significant-digit scans in both planner and Boost conversion paths for canonical exact decimals without an exponent marker; core regressions cover ordinary `0,0,1` exact input as well as deep refusal.

## BR-20260731-01 — D3D11 WARP reference-orbit texture transport

**Status:** Passed within bounded WARP transport scope; no precision-ceiling claim  
**Scope:** Production immutable reference-orbit texture upload/staging readback

### Changed

- Added a diagnostic-only `Direct3D11Renderer::CaptureReferenceOrbitTexture` seam. It stages the existing real/imaginary immutable `R32G32B32A32_FLOAT` textures and returns their four-float payload; it does not change rendering policy.
- Expanded `MandelbrotD3D11WarpFixtures` after the fixed perturbation render. It independently builds the canonical CPU double reference orbit and requires every real/imaginary float component read back from the production texture to have identical IEEE-754 bits.
- The orbit descriptor remains `mw-orbit-float4-expansion/v1` with `validatedCeilingAvailable == false`; no bit-depth limit was introduced or inferred.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotD3D11WarpFixtures --parallel`: passed.
- `ctest --test-dir build -C Release -R D3D11WarpFixtureSelfCheck --output-on-failure`: passed 1/1 in 9.51 seconds.

### Remaining unproven

- Shader reconstruction and perturbation numerical error, an orbit-encoding precision ceiling, correction validity, immutable planner metadata at GPU consumption, hardware D3D11 equivalence and VAL-051 end-to-end direct-reference comparison.

## BR-20260731-02 — Independent float4 reference-orbit payload comparison

**Status:** Passed within bounded selected-sample numerical scope; no ceiling claim  
**Scope:** Independent Boost-512 versus current fixed-point reference-orbit payload comparison

### Changed

- Strengthened `TestIndependentHighPrecisionBackend` to compare every IEEE-754 component in the real and imaginary four-float expansion at selected iterations, instead of comparing only a `double` reconstruction.
- Covered both analytic quadratic Mandelbrot and the registered exact power-2 Tricorn profile. Escape metadata also agrees for each bounded fixture.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R MandelbrotCoreTests --output-on-failure`: passed 1/1 in 2.30 seconds.

### Remaining unproven

- A measured maximum error or supported precision ceiling for `mw-orbit-float4-expansion/v1`, GPU upload/shader reconstruction equivalence, hardware-driver coverage, arbitrary exact-camera coverage, perturbation correction and end-to-end output comparison.

## BR-20260731-03 — Reference-orbit cache boundary enforcement

**Status:** Passed within bounded cache-enforcement scope; workload calibration remains open  
**Scope:** Reference-orbit service byte/entry limits, eviction and refusal

### Changed

- Added core coverage for a one-entry cache sized to one known 64-iteration orbit. A distinct exact camera evicts the prior entry; requesting the evicted camera rebuilds it instead of receiving a stale cache hit.
- Added an over-budget fixture that sets the byte cap one byte below the same orbit’s measured byte size and requires refusal with zero cached entries/bytes.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R MandelbrotCoreTests --output-on-failure`: passed 1/1 in 2.28 seconds.

### Remaining unproven

- Calibration of the default cache/queue limits under real deep workloads, worker queue-full/cancellation pressure, renderer upload lifetime, long-session allocation behaviour and resource thresholds for correction or multi-reference work.

## BR-20260803-01 — Fixture-specific float4 orbit reconstruction measurement

**Status:** Passed within bounded producer/reconstruction fixture scope; no validated ceiling  
**Scope:** Existing float4 orbit encoding reconstructed as ordered shader-coordinate additions

### Changed

- Added `OrbitEncodingMeasurement` at the platform-neutral independent Boost-512 boundary. It compares each source coordinate with the ordered `float4` sum used when both current perturbation shaders form `Z` for magnitude/classification.
- Added `MandelbrotOrbitEncodingMeasurement` CTest fixture. It emits independently inspectable numerical observations for registered Mandelbrot and Tricorn profiles without altering rendering policy or marking the descriptor validated.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotOrbitEncodingMeasurement --parallel`: passed.
- Direct fixture execution: Mandelbrot (256 coordinates) maximum absolute/relative error `3.4413387754277605e-08` / `5.7453155647240968e-08`; Tricorn (28 coordinates; escaped at 13) `1.3645269757469331e-06` / `5.3683366176295286e-08`.
- `ctest --test-dir build -C Release -R "MandelbrotCoreTests|OrbitEncodingMeasurementSelfCheck" --output-on-failure`: passed 2/2 in 2.15 seconds.

### Remaining unproven

- Device/compiler shader arithmetic, float4 use in the reference-times-delta path, bounded error over a meaningful camera/profile/iteration matrix, correction validity, driver variance and any accepted GPU precision envelope. `CurrentOrbitEncoding().validatedCeilingAvailable` remains false.

## BR-20260803-02 — Unvalidated GPU orbit transport planner refusal

**Status:** Passed central-policy scope; GPU execution remains unavailable  
**Scope:** Planner handling of present but unvalidated float4 orbit transport

### Changed

- Made the planner’s correction reason explicit: a declared float4 GPU transport with zero validated bits has no validated precision envelope and requires direct correction.
- Added regression coverage that confirms this condition still selects the bounded CPU reference plan; it cannot be represented as a safe GPU deep route.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R MandelbrotCoreTests --output-on-failure`: passed 1/1 in 2.37 seconds.

### Remaining unproven

- Any validated GPU execution backend, direct correction implementation, exact-camera GPU request contract, transport precision envelope, shader/driver equivalence and end-to-end GPU perturbation output.

## BR-20260803-03 — Exact direct CPU anti-aliasing

**Status:** Implemented and core/native-build verified within bounded direct CPU scope  
**Scope:** Exact rational AA 1–4 for unrotated Boost direct still/frame rendering

### Changed

- Added `BuildExactStillRenderSubpixelSample`, which derives AA grid offsets as rational half-height factors. The former centre-pixel builder delegates to its 1×1 form.
- Extended `RenderExactDirectStillImage` from AA 1 only to bounded AA 1–4, averaging independently evaluated exact samples. Unsupported rotations and AA levels outside 1–4 still fail closed.
- Updated high-resolution and frame-sequence UI text/validation to expose the supported AA range without changing immutable renderer IDs or permitting GPU fallback.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R MandelbrotCoreTests --output-on-failure`: passed 1/1 in 2.72 seconds, including rational AA-2 determinism and AA-4 upper-bound checks.
- `cmake --build build --config Release --target MandelbrotWallpaper --parallel`: compiled and linked the changed Win32 UI paths successfully.

### Remaining unproven

- Deep tiled/rotated exact rendering, broad AA numerical/error fixtures, GPU parity/correction, frame/video runtime flows, Windows dialog interaction and long-running resource measurements.

## BR-20260803-04 — Exact frame-render preflight boundary

**Status:** Implemented and core verified; runtime encoder flow remains unproven  
**Scope:** Fail-closed exact CPU frame export validation before job/frame output

### Changed

- Added a shared exact-frame renderer contract to the core frame-sequence path. Exact CPU job creation refuses rotation and AA outside 1–4 before emitting an immutable job.
- Applied the same contract after every evaluated timeline frame and before the caller's renderer callback, preventing an animated unsupported frame from starting a temporary output.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R MandelbrotCoreTests --output-on-failure`: passed 1/1 in 2.81 seconds, including unsupported base rotation refusal and a later timeline rotation rejected before the renderer callback.

### Remaining unproven

- Rotation support, all timeline/frame runtime behaviours, actual WIC output/encoder files, deep video validity/correction and GPU export.

## BR-20260803-05 — Central GPU compatibility precision policy

**Status:** Implemented and core/GPU-fixture verified; exact GPU execution remains unavailable  
**Scope:** Remove D3D11/OpenGL-local automatic precision thresholds and fallback order

### Changed

- Added `ResolveLegacyGpuPrecision` to the platform-neutral `PrecisionPlanner`. It centrally owns the existing legacy compatibility transition thresholds and deterministic fallback order.
- Replaced local D3D11/OpenGL automatic-policy tables with capability mapping calls. The backends no longer encode zoom thresholds or an independent fallback order.
- Added core coverage for reported capability combinations and explicit incompatible-mode refusal.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R MandelbrotCoreTests --output-on-failure`: passed 1/1 in 2.68 seconds.
- `cmake --build build --config Release --target MandelbrotD3D11WarpFixtures MandelbrotOpenGLFixtures --parallel`: passed.
- `ctest --test-dir build -C Release -R "D3D11WarpFixtureSelfCheck|OpenGLFixtureSelfCheck" --output-on-failure`: passed 2/2 in 9.97 seconds.
- Source audit: legacy automatic thresholds remain only in `src/Core/Precision/PrecisionPlanner.cpp`.

### Remaining unproven

- Immutable exact-camera GPU request/plan consumption, validated GPU deep backend, float4 envelope, correction, hardware driver parity and end-to-end perturbation numerical output.

## BR-20260803-06 — Exact direct coefficient-animation refusal

**Status:** Implemented and core/native-build verified; time-varying exact equations remain unsupported  
**Scope:** Prevent exact CPU deep paths from silently ignoring animated equation coefficients

### Changed

- Added fail-closed coefficient-animation rejection to the public direct high-precision sample evaluator and the exact direct still renderer.
- Added matching high-resolution UI preflight and shared exact frame job/per-frame validation, so unsupported state is refused before exact rendering begins.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R MandelbrotCoreTests --output-on-failure`: passed 1/1 in 3.03 seconds, covering evaluator, still and frame-job refusal.
- `cmake --build build --config Release --target MandelbrotWallpaper --parallel`: compiled and linked the changed Win32 preflight path.

### Remaining unproven

- Exact high-precision coefficient-time semantics, animated exact still/frame/video output, GPU equivalence/correction, tiled/rotated rendering and runtime dialog validation.

## BR-20260803-07 — Reference-orbit worker shutdown and queue boundary

**Status:** Implemented and core verified; runtime/render integration remains unproven  
**Scope:** Bounded worker shutdown ownership and independent queue refusal

### Changed

- Made `ReferenceOrbitWorker::Shutdown` a completion barrier: it refuses future work, cancels subscribers, joins the owned worker thread and releases active/pending request state before return.
- Made concurrent shutdown callers wait for the same stopped state. A worker completion callback is explicitly forbidden from synchronously shutting down that same worker thread.
- Added core regressions for stopped-worker refusal, zero-capacity queue refusal without retention, and post-shutdown request-state release.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R "^MandelbrotCoreTests$" --output-on-failure`: passed 1/1 in 2.70 seconds.

### Remaining unproven

- Default-depth queue-pressure behaviour, cancellation timing during long Boost computations, allocation-failure handling, renderer upload/resource ownership, GPU teardown, UI integration and long-session resource use.

## BR-20260803-08 — Reference-orbit immutable provenance binding

**Status:** Implemented and core verified; renderer binding remains unproven  
**Scope:** Prevent plan/encoding substitution and cache reuse across immutable orbit identities

### Changed

- Added the central `mw-precision-plan-v1` identity to every `PrecisionPlan`; immutable reference requests carry the named orbit-encoding identifier/version.
- The reference service validates those identities, incorporates them into cache and worker-coalescing keys, and returns them alongside the existing formula capability provenance.
- Replaced the frame export’s duplicated plan-version literal with the central identifier.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R "^MandelbrotCoreTests$" --output-on-failure`: passed 1/1 in 2.80 seconds, including substituted plan/encoding refusal without cache mutation.

### Remaining unproven

- Renderer upload/job linkage, GPU transport or perturbation correctness, future plan/encoding migration, default queue pressure, cancellation under a long computation and runtime resource evidence.

## BR-20260803-09 — Reference-orbit per-subscriber plan delivery

**Status:** Implemented and core verified; correction execution remains unproven  
**Scope:** Preserve each coalesced subscriber's immutable precision/correction policy

### Changed

- Added the full immutable `PrecisionPlan` to reference-orbit results.
- Worker delivery now replaces the numerical builder's plan with the subscriber's own plan before generation stamping. This lets equal numerical requests coalesce without sharing a correction-policy decision.
- Added two-generation coverage with distinct direct-correction requirements.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R "^MandelbrotCoreTests$" --output-on-failure`: passed 1/1 in 2.71 seconds.

### Remaining unproven

- Direct correction implementation/thresholds, renderer upload generation fencing, GPU transport/execution, cancellation pressure, runtime resource behavior and end-to-end deep output.

## BR-20260803-10 — Reference-orbit subscriber resource bound

**Status:** Implemented and core verified; default-limit calibration remains unproven  
**Scope:** Bound retained callbacks for equal-key orbit-work coalescing

### Changed

- Added `maximumSubscribersPerKey` to the platform-neutral worker limits, with a default of 64.
- Enqueue now refuses a zero or exhausted subscriber limit before retaining an independent key or callback. The existing bounded-key queue remains unchanged.
- Added zero-capacity regression coverage and post-shutdown state checks.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R "^MandelbrotCoreTests$" --output-on-failure`: passed 1/1 in 2.71 seconds.

### Remaining unproven

- Measured default queue/subscriber thresholds, non-zero-cap pressure under active work, cancellation timing, allocation failures, renderer/GPU lifetime and long-session resource behavior.

## BR-20260803-11 — Ultra-deep schema-3 exact-camera reload

**Status:** Implemented and core verified; compatibility rendering remains intentionally lossy  
**Scope:** Preserve exact cameras whose positive half-height is below double range during preset load

### Changed

- Updated the one-way exact-to-legacy adapter to use `denorm_min` with explicit loss for positive half-height underflow, while retaining failure for invalid or overflowing values.
- Updated preset normalization so the derived legacy placeholder cannot reject or overwrite a valid exact-authoritative schema-3 camera.
- Added direct adapter and schema-3 `1e-1000` serialization/reload regressions.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests --parallel`: passed.
- `ctest --test-dir build -C Release -R "^MandelbrotCoreTests$" --output-on-failure`: passed 1/1 in 2.69 seconds.

### Remaining unproven

- Exact camera mutation authority across UI/history/animation/Scout, legacy renderer refusal/diagnostics for lossy deep state, GPU execution/correction, and runtime still/frame/video output.

## BR-20260803-12 — Exact-camera transaction boundary

**Status:** Implemented and core/native-build verified; manual interaction remains unproven  
**Scope:** Commit parsed exact UI coordinates without a legacy binary reconstruction

### Changed

- Added the platform-neutral `ApplyProjectExactCameraMutation` transaction. It validates/adapts the exact camera once, commits exact text as authoritative, derives `CameraState` only for compatibility, and reports camera invalidation/history metadata.
- Routed AppWindow calls that supply an `ExactCamera` through this boundary, including the main coordinate route; legacy preview gestures continue to use their explicit scalar compatibility path.
- Added ultra-deep exact commit and invalid-negative-scale rollback tests.

### Evidence passed

- `cmake --build build --config Release --target MandelbrotCoreTests MandelbrotWallpaper --parallel`: passed.
- `ctest --test-dir build -C Release -R "^MandelbrotCoreTests$" --output-on-failure`: passed 1/1 in 2.80 seconds.

### Remaining unproven

- Manual Win32 coordinate interaction, exact camera history replay, exact animation/Journey/Scout authority, deep live preview, GPU/correction, and runtime deep output.

### Evidence passed

- `cmake --build build --config Release --parallel`: Visual Studio 18 2026 x64 Release compiled and linked the new precision core units, application, tests and fixture targets.
- `ctest --test-dir build -C Release --output-on-failure`: 6/6 passed in 13.99 seconds after the schema migration continuation.
- `C:\Python314\python.exe scripts\verify-source.py`: passed after its DEC-028 decision expectation was updated.
- Latest PH-12 planner continuation: Release application/core build completed; all 6 CTest targets passed. Core coverage includes a `1e-200` exact frame-export camera being rejected before a Float64 job is created, while an ordinary exact camera still produces a deterministic immutable job.
- Latest PH-12 planner continuation: source and offline-policy verification passed, including Boost.Multiprecision 1.83.0 package-integrity validation. Native interactive dialog flow and deep numerical equivalence remain unproven.
- Latest PH-13 service continuation: Release build and all 6 CTest targets passed after compiling `ReferenceOrbitService`; the source verifier and Boost 178-file package-integrity check passed. This is unit/core evidence, not a worker, UI, GPU or pixel-equivalence result.

### Remaining unproven

- Promotion of `ExactCamera` to the sole mutable project authority, exact v2 fingerprint/manifests and all affected UI/Journey/timeline/Scout/export paths.
- Numerical adapter limits beyond the bounded parser/legacy failure tests, production high-precision cancellation/resource behavior, and all native interaction/lifecycle evidence.

### Required-path inventory

- Mutable/persisted authority: `Preset.camera`, model validation, `SettingsStore` JSON schema-2 preset import/export and schema-9 settings custom presets.
- Shared edits/history: `ProjectState` camera descriptors, parameter mutation coordinator, snapshots, undo/redo and `AppWindow` camera mutations.
- Input/presentation: main coordinate controls, coordinate triplets, Jump to Coordinates and high-resolution-render coordinate text.
- Runtime consumers: animation, General Animation/Journey conversion, Scout request/refinement, still tile/sample mapping, deep/reference orbit generation, high-precision reference backend and frame/export snapshots.
- Identity: `mw-render-state-v1` serializes the five legacy camera doubles and must remain readable rather than being reinterpreted as an exact identity.

### Rollback

Remove `ExactDecimal.*`, `ExactCamera.*`, their CMake/test/verifier/docs entries and DEC-028 implementation record together. Existing camera persistence and user data remain unchanged; this checkout has no Git rollback identifier.

## BR-20260803-13 — Exact-camera history replay

**Status:** Implemented and core/native-build verified; manual interaction remains unproven  
**Scope:** Preserve authoritative exact camera text through project undo/redo

### Changed

- Added a core regression test that records a `1e-1000` exact-camera transaction through `ProjectHistory` and verifies both endpoints.
- Confirmed the existing replay-equivalence guard selects an atomic full-preset history entry when scalar camera mutations would reconstruct a different exact authority.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed, including the application, core tests and fixture targets.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 14.12 seconds.

### Remaining unproven

- Manual Win32 undo/redo interaction, exact animation/Journey authority, deep live preview, and CPU/GPU deep numerical correctness.

### Rollback

- Remove the focused core regression test and its documentation record. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-14 — Immutable exact frame-plan hand-off

**Status:** Implemented and core/native-build verified; runtime export remains unproven  
**Scope:** Bind the exact frame renderer to the planner-selected execution tier

### Changed

- Added `PrecisionPlan` to the per-frame immutable request after exact-camera and selected-renderer validation.
- Routed the Windows direct CPU frame encoder through that plan's selected bit tier rather than duplicating renderer-ID tier logic.
- Added a `1e-1000` exact frame-export regression that verifies the callback receives the 8,192-bit direct CPU plan.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed, including the application, core tests and fixtures.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 15.81 seconds.

### Remaining unproven

- Real WIC frame export, exact animation semantics, deep tiled output, correction, GPU parity and FFmpeg/video output.

### Rollback

- Remove the per-frame planner field, its Windows consumption and focused test as one change. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-15 — Exact direct row-memory admission

**Status:** Implemented and core/native-build verified; measured support remains unproven  
**Scope:** Bound one-row working allocation before direct exact CPU rendering

### Changed

- Added a 64 MiB maximum direct-exact output-row allocation before `RenderExactDirectStillImage` allocates its row buffer.
- Added a no-allocation regression fixture for a width one pixel above the bound.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed, including the application, core tests and fixtures.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 15.93 seconds.

### Remaining unproven

- Total output storage, elapsed time, WIC encoder limits, exact animation semantics, deep tiled output, correction, GPU parity and FFmpeg/video output.

### Rollback

- Remove the direct-exact row bound and its focused test as one change. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-16 — Explicit perturbation sample validity

**Status:** Implemented and core/native-build verified; production correction remains unproven  
**Scope:** Make unstable perturbation samples fail closed instead of appearing equivalent to normal output

### Changed

- Added `Stable`, `Rebased` and `Unresolved` validity states to the platform-neutral perturbation sample result.
- Classified a successful reference refresh as `Rebased` and a no-refresh instability as `Unresolved`.
- Added stable, rebase and unresolved regression assertions.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 16.16 seconds.

### Remaining unproven

- Production GPU validity transport/final-colour rejection, direct correction, calibrated rebase thresholds, deep tiled output and hardware parity.

### Rollback

- Remove the explicit validity member and focused assertions as one core change. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-17 — Exact direct global tile mapping

**Status:** Implemented and core/native-build verified; tile scheduling remains unproven  
**Scope:** Preserve exact full-frame coordinates when rendering a bounded direct CPU crop

### Changed

- Added full-frame dimensions and tile origin to the direct exact CPU request.
- Validated crop bounds before rendering and used global coordinates for every exact subpixel sample.
- Added full-frame/crop equality and out-of-frame refusal coverage.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 15.57 seconds.

### Remaining unproven

- UI tile scheduling/streaming, large-image WIC behavior, rotation, correction, GPU parity and FFmpeg/video output.

### Rollback

- Remove the exact direct full-frame/tile-origin fields and their focused tests as one core change. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-18 — Deep exact cancellation before row publication

**Status:** Implemented and core/native-build verified; encoder/runtime cancellation remains unproven  
**Scope:** Stop an 8,192-bit exact CPU render before it can publish a partial row

### Changed

- Added a regression that cancels the `1e-1000` / 8,192-bit direct renderer during its first sample and verifies its writer receives no row.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 14.28 seconds.

### Remaining unproven

- WIC temporary-file cleanup, cancellation after committed rows, long-running resource behavior, deep tile scheduling, GPU parity and video cancellation.

### Rollback

- Remove the focused deepest-tier cancellation regression and documentation record. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-19 — Frame renderer route refusal

**Status:** Implemented and core/native-build verified; no GPU route is implemented  
**Scope:** Prevent unregistered export renderer selections from silently executing the production CPU renderer

### Changed

- Registered only `cpu-production-still` and the three executable exact CPU Boost tiers as deterministic frame-export routes.
- Refused every other renderer ID during immutable job construction and manifest save/load, before output creation.
- Added a Windows callback defence that refuses an unexpected non-exact renderer instead of running `RenderStillImageTiled`.
- Added a regression for `gpu-d3d11-validated-v1` proving it cannot be represented as a CPU export job or written to a manifest.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 15.56 seconds.
- `C:\Python314\python.exe .\scripts\verify-source.py`: passed, including Boost package integrity (178 files).

### Remaining unproven

- A registered GPU frame route, its measured precision/equivalence classification, runtime dialog behavior, real frame/video output and hardware-GPU deep correctness.

### Rollback

- Remove the registered-route admission check, defensive Windows callback branch and focused regression as one change. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-20 — High-resolution exact renderer status

**Status:** Implemented and native-build/test verified; manual dialog behavior remains unproven  
**Scope:** Identify the actual direct exact CPU route during high-resolution export

### Changed

- Updated the high-resolution render status to report the exact CPU direct evaluator whenever the central planner selects that route.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 13.75 seconds.

### Remaining unproven

- Manual dialog interaction, exact output/encoder behavior, performance, GPU parity and video output.

### Rollback

- Restore the generic CPU-tiled status text. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-21 — Deep camera animation refusal

**Status:** Implemented and core/native-build verified; exact keyframes remain a separate deferred capability  
**Scope:** Preserve deep exact camera authority from double-valued animation tracks

### Changed

- Added a loss check before any camera-track evaluation clears and rebuilds exact camera state.
- Refused camera animation when its immutable base exact camera cannot adapt to the legacy double representation without loss.
- Added a `1e-1000` exact-camera regression proving the deterministic refusal.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 15.63 seconds.

### Remaining unproven

- Durable exact keyframes, exact interpolation/Journey conversion, deep Scout promotion, runtime editor behavior, GPU parity and deep export.

### Rollback

- Remove the animation loss check and focused regression together. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-22 — Deep Scout source refusal

**Status:** Implemented and core/native-build verified; exact Scout coordinates remain a separate deferred capability  
**Scope:** Prevent double-coordinate Scout from presenting approximate candidates for a lossy deep exact source

### Changed

- Added exact-to-legacy loss inspection before bounded Scout search starts.
- Refused searches from a source exact camera whose centre or half-height cannot be represented losslessly by the existing search coordinates.
- Added a `1e-1000` source-camera regression proving refusal before candidate generation.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 15.81 seconds.

### Remaining unproven

- Exact Scout coordinate/candidate semantics, deep candidate promotion, runtime UI behavior, GPU parity and measured deep support.

### Rollback

- Remove the pre-search loss check and focused regression together. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260803-23 — Boost 16,384-bit direct exact tier

**Status:** Implemented and native-build/full-suite/source verification passed; runtime evidence remains pending  
**Scope:** Extend the bounded CPU exact ladder for deeper still and frame rendering

### Changed

- Added a fixed 16,384-bit Boost.Multiprecision direct evaluator and central planner backend.
- Added CPU high-resolution and immutable frame-export renderer routes with a distinct `cpu-exact-boost16384-v1` identity.
- Added `1e-3000` direct exact still, planner-selection and immutable frame-plan regressions.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 18.69 seconds.
- `C:\Python314\python.exe .\scripts\verify-source.py`: passed, including Boost package integrity.

### Remaining unproven

- Runtime UI/output behavior, performance/resource thresholds, GPU parity and deep video output.

## BR-20260804-01 — Planner-matched high-precision reference orbits

**Status:** Implemented and native-build/full-suite verified; renderer consumption remains pending  
**Scope:** Preserve the planner-selected exact CPU tier when bounded reference-orbit work is requested

### Changed

- Extended exact reference-orbit generation to the supported 512/2,048/8,192/16,384-bit Boost tiers.
- Made `ReferenceOrbitService` reject any backend/bit mismatch and pass the immutable selected tier into the numerical builder.
- Added a `1e-3000` service regression that verifies a 16,384-bit plan produces a matching 16,384-bit orbit result.

### Evidence passed

- `cmake --build build --config Release --parallel`: passed.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 18.85 seconds.

### Remaining unproven

- Float4 orbit transport has no validated deep GPU envelope. No CPU/GPU perturbation, correction scheduler, renderer upload, runtime UI behavior or deep video route consumes this service yet.

### Rollback

- Remove the expanded exact reference builder, service-tier admission and focused regression together. No persisted schema, user data or dependency changed; this checkout has no Git rollback identifier.

## BR-20260805-01 — Windows UX flow and action regression correction

**Status:** Implemented; native Release build/full suite, source verification and bounded Win32 runtime checks passed  
**Scope:** Correct audited Windows button/action, dialog-lifetime, candidate-state, default-layout, keyboard and DPI regressions without changing persistence schemas or renderer policy

### Changed

- Associated modeless Settings, Palette and Equation tools with the main owner so shutdown destroys their nested loops cleanly; kept Quick Controller modeless and made Timeline, Journey and frame/video export ownership match their modal route contracts.
- Restored Journey candidate state on Cancel, including Apply-then-Cancel inside Settings; colour, precision and adaptive Settings actions now publish their accepted live-preview candidate consistently.
- Copied authoritative exact-camera canonical text when available, labelled the Quick action as `Add to Slideshow`, clarified monitor assignment guidance, and added confirmation/feedback for destructive log and custom-preset actions.
- Disabled built-in preset Update/Delete and empty animation keyframe actions, added save-failure rollback, and shortened clipped action labels.
- Treated requested dialog dimensions as client dimensions, removed the stale main-window Equation label, added main-window keyboard dialog traversal, and applied per-monitor DPI font/geometry/suggested-rectangle handling.

### Evidence passed

- `cmake --build build --config Release`: passed with MSBuild 18.8.2 after the expected sandboxed Visual Studio FileTracker `E_ACCESSDENIED` and permitted retry.
- `ctest --test-dir build -C Release --output-on-failure`: passed 7/7 in 16.93 seconds after the final product-source change.
- `C:\Python314\python.exe .\scripts\verify-source.py`: passed all governed source/offline checks after the final product-source change.
- Isolated Win32 capture/inventory covered the main Preview/Desktop/Status pages and Settings, Palette, Quick Controller, Equation, Preset Library, Journey, Timeline, Scout, high-resolution, frame and video dialogs. The current 96-DPI default captures show the audited controls without unwanted default scrollbars or clipped action labels; modal/modeless owner enablement matched the route map.
- Exit with Settings open completed within five seconds and left no Settings window. Quick Copy Coordinates produced the exact canonical `-5e-1,0,1.5e0`. A nested Journey Apply → Cancel → Settings OK round trip preserved the original empty Journey text.

### Skipped and remaining unproven

- Destructive log clearing, custom-preset overwrite/delete, real still/frame/video output and FFmpeg execution were not exercised; their confirmation, refusal and rollback paths have source/native-build evidence only.
- Colour-picker acceptance, precision/adaptive live preview, main-window keyboard traversal, non-96-DPI/mixed-monitor movement, high contrast, reduced screen size, screen-reader output and long-running cancellation still need manual Windows interaction evidence.
- This does not complete the remaining PH-05 Equation/Settings/Journey/import/Scout atomic undo/redo matrix or any open PH-06–PH-09 output/runtime gate.

### Rollback

- Revert the listed Windows UI/dialog/support changes, verifier assertions and current-workflow labels together. No schema, dependency, preset data or renderer algorithm changed. This checkout has no Git rollback identifier.

## BR-20260907-01 — Modeless camera retention and preview precision recovery

**Status:** Implemented; native Release build/full suite and production renderer fixtures passed; interactive journey pending  
**Date:** 2026-09-07  
**Scope:** Preserve current camera/zoom while Palette or Equation is open, restore automatic preview precision switching, and recover the preview after precision/backend failure

### Inspected

- Palette and Equation candidate/live-preview/accept/cancel routes, main-window camera synchronisation and exact-coordinate admission.
- Preview renderer lifetime, precision settings application, D3D11/OpenGL facade fallback and central legacy GPU precision resolution.
- The current application log and precision settings on this host. The log showed D3D11 correctly handing explicit Float64 to OpenGL, followed by AMD GLSL failures at a double `exp` overload and the reserved identifier `smooth`; the stopped preview renderer then had no Settings/Precision restart path.

### Changed

- Added domain-scoped Palette and Equation candidate merges. Each live preview and accepted edit now overlays only the editor-owned fields on the latest authoritative preset, preserving camera, exact camera, zoom, rotation and unrelated changes made while the modeless window is open.
- Corrected the non-exact camera-control synchronisation branch, which recursively called itself instead of updating the controls.
- Made exact-coordinate admission consult the active preview GPU capabilities and central legacy policy instead of requiring an unrelated CPU Float64 capability.
- Added a bounded preview-renderer restart when precision settings change or when a prior render failure left the preview stopped.
- Made the OpenGL Float64 shader compile on the installed AMD driver by avoiding unavailable double-transcendental overloads and the reserved GLSL identifier `smooth`. Double coordinate/recurrence arithmetic remains; transcendental helpers use float built-ins as an explicit compatibility boundary.
- Added core domain-merge regressions, D3D11 automatic-strategy production checks at scale 1.5, `1e-7` and `1e-14`, OpenGL native-Float64 production coverage and governed source markers.

### Ran and passed

- Native `cmake --build build --config Release --parallel 4` passed with MSBuild 18.8.2 and linked the application and all fixture/test targets.
- `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 17.54 seconds.
- Direct D3D11 WARP production fixture passed standard, bloom, perturbation, float4 transport and automatic preview precision switching at 256x144 on Microsoft Basic Render Driver / Direct3D 11.1.
- Direct D3D11 hardware production fixture passed the same checks, including automatic preview precision switching, at 256x144 on AMD Radeon RX 7900 XT / Direct3D 11.1.
- Direct OpenGL production fixture passed standard, bloom, perturbation and native Float64 at 256x144 on AMD Radeon RX 7900 XT / OpenGL 4.6.0 Compatibility Profile Context 26.6.4.260624.

### Failed and corrected

- The first sandboxed MSVC build attempt failed in Visual Studio FileTracker with `E_ACCESSDENIED`; the permitted native retry then reached link and found the previously launched application locking the executable. After the exact process was closed, the Release build passed.
- The first expanded D3D11 fixture run used a stale expected display string; correcting it to the renderer's actual strategy label made the fixture pass.
- The first OpenGL Float64 compatibility attempt exposed the second AMD GLSL error at reserved identifier `smooth`; renaming the local completed the native Float64 fixture.
- Native screen capture for an interactive journey was unavailable because the Windows capture API returned `0x80004002` (`No such interface supported`).

### Skipped and remaining unproven

- The full pointer-driven journey—open Palette/Equation, navigate/zoom the main preview, edit the dialog, then Save/Cancel—was not captured end to end. The domain behaviour has native build and core regression evidence, but final interaction confirmation remains pending.
- No desktop wallpaper mutation, export, package, installer, mixed-DPI, accessibility, device-loss, long-session or performance benchmark was run. The fixtures prove strategy selection and bounded production rendering only, not a general cross-backend pixel-equivalence or deep-GPU precision envelope.

### Rollback

- Revert the editor-domain merge helpers/routes, preview capability/restart changes, OpenGL compatibility shader changes and their tests/docs together. No persistence schema, dependency or user data changed; this checkout has no Git rollback identifier.

## BR-20260908-01 — PH-10 production Scout thumbnail fixtures

**Status:** Partial PH-10 implementation; native production-thumbnail fixture passed; dialog interaction pending  
**Date:** 2026-09-08  
**Scope:** Add deterministic production-rendered evidence for retained Scout candidates without changing search policy, candidate ordering or baseline approval

### Inspected

- Canonical PH-10 requirements, validation IDs, Scout status, visual-regression contract and production `FractalScout`/still-render paths.
- Existing exact-camera V2 candidate identity, candidate-copy isolation, bounded request limits and one-transaction Scout Apply history coverage.
- A fresh isolated native application startup and the available Win32 accessibility surface for modeless-dialog interaction.

### Changed

- Extended `MandelbrotVisualFixtures` with `fractal-scout-thumbnails`. It runs one fixed bounded production Scout request twice, retains three candidates and requires identical per-rank identity, score and pixels, non-uniform output and distinct candidate image hashes.
- The fixture writes per-rank current/repeat/diff PPM images and metrics plus a shared state record containing exact-camera V2 identities, camera values, scores, image hashes, resolved limits, source path, compiler and system metadata.
- Added `--no-scout-thumbnail-check` for focused fixture work. The default self-check includes Scout thumbnails.
- Updated the canonical Scout, visual-regression, implementation, traceability, validation and roadmap owners without promoting a baseline or changing persistence.

### Ran and passed

- `cmake --build build --config Release --target MandelbrotVisualFixtures --parallel 4` passed with MSBuild 18.8.2.
- Focused `VisualFixtureSelfCheck` passed 1/1 in 2.63 seconds and reported three deterministic, distinct, non-uniform, exactly repeatable thumbnails and identities.
- Final `cmake --build build --config Release --parallel 4` passed and linked the application and all test/fixture targets.
- Final `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 17.35 seconds.
- `C:\Python314\python.exe .\scripts\verify-source.py` passed all governed source/offline checks.
- Recorded image hashes were `cb70154358f0b7f3`, `2ebbb4fb9d317c5d` and `bf7f3dc9c0ba27f1`; each per-rank comparison was exact with SSIM 1. The generated contact sheet visually confirmed three distinct, non-uniform candidate images.

### Failed and corrected

- The sandboxed full MSVC retry failed in Visual Studio FileTracker with `E_ACCESSDENIED`; the permitted native retry passed. This was an execution-host restriction, not a product compile failure.
- Invoking the WinGet FFmpeg alias directly failed before process start because Windows had no associated application for the alias. Resolving the exact installed executable produced the contact sheet successfully; FFmpeg is not a fixture dependency.
- Native application startup and Equation-dialog construction succeeded, but the automation host could not capture pixels or expose reliable dialog-input geometry (`SetIsBorderRequired` returned `0x80004002`). The isolated application was then closed cleanly.

### Skipped and remaining unproven

- No Scout baseline was approved or promoted. Cross-run/cross-machine thumbnail stability is not claimed beyond the recorded deterministic native environment.
- Native Scout preview isolation, Apply, Save as New, Cancel, Undo and Redo interaction remain pending. The fixture does not prove Win32 button wiring, accessibility, DPI, cancellation timing or long-running resource behaviour.
- No search/scoring/order policy, persistence schema, dependency, desktop runtime, export or user data changed.

### Artifacts

- `build/test_artifacts/visual/fractal-scout-thumbnails/cpu/candidate/state.json`
- `build/test_artifacts/visual/fractal-scout-thumbnails/cpu/candidate/rank-1..3/`
- `build/test_artifacts/visual/fractal-scout-thumbnails/cpu/candidate/contact-sheet.png`

### Rollback

- Remove the Scout check and CLI flag from `VisualFixtureTool.cpp` and revert the linked PH-10/visual evidence records. Existing Scout behaviour and data remain unchanged; this checkout has no Git rollback identifier.

## BR-20260908-02 — PH-11 process-scoped Windows release validation

**Status:** Automated Windows workflow passed; manual PH-01/PH-11 matrix pending  
**Date:** 2026-09-08  
**Scope:** Correct canonical startup discovery and rerun the clean native build/test/metadata/package/runtime evidence workflow

### Inspected

- Current `AGENTS.md`, phase/index owners, Windows validation specification and release scripts.
- The failed validator reports, isolated application logs, exact generated executable owner and historical process-scoped startup evidence.
- The dedicated `run-current-build-runtime-smoke.ps1` implementation that had already replaced unreliable global window discovery with process-owned enumeration.

### Changed

- Replaced `validate-windows-release.ps1` startup discovery with `EnumWindows` filtered by launched process ID and exact class name. Preflight enumeration also detects an existing application by class without relying on global `FindWindowW`.
- Added governed source assertions for enumeration, process ownership and rejection of the obsolete global lookup.
- Reconciled PH-01/PH-10/PH-11 and deep-programme status across the implementation plan, integrated roadmap, validation owner, Windows validation specification, delivery ledger and Scout evidence.

### Failed and corrected

- First report `artifacts/windows-validation/20260908-ph11-autonomous/` failed before build because stale isolated PID 10012 held `build/Release/MandelbrotWallpaper.exe`. It was the exact prior validation instance, not an ambiguous user process; after termination, no Mandelbrot process remained.
- Second report `artifacts/windows-validation/20260908-ph11-autonomous-rerun/` passed source verification, native build/CTest, metadata and package inspection, then logged successful D3D11 startup but falsely reported no main window. The validator still used the known-unreliable global `FindWindowW` route.
- The process-scoped correction is protected by `scripts/verify-source.py`; the final workflow passed.

### Ran and passed

- `C:\Python314\python.exe .\scripts\verify-source.py` passed after the script correction.
- Final report `artifacts/windows-validation/20260908-ph11-process-scoped/report.json` records `Automated checks passed` on Windows 10 build 19045 with PowerShell 7.6.5.
- Visual Studio 18 2026/MSVC 19.51.36252.0 performed a clean x64 Release configure/build and linked the application plus all test/fixture targets.
- Release CTest passed 7/7 in 18.92 seconds.
- Embedded file/product version is 1.13.1. The portable ZIP contains 87 inspected entries, includes the required executable/README/licence and contains none of the governed forbidden local/build artifacts.
- The isolated process-owned main window appeared; the normal loop ran for two seconds; D3D11 initialized on AMD Radeon RX 7900 XT / Direct3D 11.1; the application accepted its normal Exit command and logged clean shutdown.

### Skipped and remaining unproven

- Installer generation was explicitly skipped. Installer install, upgrade, uninstall and preceding-release data preservation are unproven.
- Windows 11, other GPU/driver configurations, manual dialogs and desktop modes, mixed-DPI/multi-monitor, Explorer/session/power/device-loss lifecycle, settings migration, real FFmpeg, cancellation and resource soak remain open.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Artifacts

- `artifacts/windows-validation/20260908-ph11-process-scoped/report.json`
- `artifacts/windows-validation/20260908-ph11-process-scoped/report.md`
- `artifacts/windows-validation/20260908-ph11-process-scoped/validation.log`
- `dist/Mandelbrot-Live-Wallpaper-1.13.1-win-x64.zip`

### Rollback

- Revert the process-scoped release-validator lookup and its verifier/documentation updates together. Runtime application code, persistence and user data did not change; this checkout has no Git rollback identifier.

## BR-20260909-01 — PH-09 real FFmpeg production-path fixture

**Status:** Real FFmpeg 8.1.1 success, owned-process cancellation, integrated export cancellation and encoder-failure containment passed; Win32 dialog and broader encoder coverage pending  
**Date:** 2026-09-09  
**Scope:** Exercise the production PH-08-to-PH-09 boundary with an explicitly supplied external encoder without changing the application workflow, persistence or distribution boundary

### Inspected

- Current `AGENTS.md`, project index/settings, PH-09 implementation/traceability/validation owners, offline-export and rendering contracts, and external-tool security boundary.
- Production `FrameSequenceExport`, `ExternalVideoExport`, `ExternalProcess`, WIC image codec and CMake paths plus existing core success/failure/cancellation/cleanup tests.
- The installed FFmpeg executable resolved outside the WinGet alias, its capability output, generated source manifest/receipts, bounded encoder logs and final MP4.

### Changed

- Added `MandelbrotExternalVideoFixture` as a Windows-only opt-in test target. It requires an explicit FFmpeg executable and a non-existing artifact directory; it is not registered in default CTest and adds no download, bundle, path discovery or persistence.
- The fixture generates three 96x64 PNGs through production tiled CPU rendering and WIC, revalidates the production manifest/receipts, then executes the immutable production H.264/MP4 encode and decode-probe path through the owned-process runner. A separate fixture-owned real-time replay exercises bounded child-process cancellation; a 120-frame 640x360 verified sequence exercises full `RunExternalVideoExport` cancellation; and a second sequence is temporarily moved only after production preflight to exercise the real exact-argument encoder failure boundary.
- Added immutable video-job preflight for the fixed yuv420p requirement: odd source width or height is now refused before FFmpeg starts. PH-08 PNG sequence dimensions are unchanged.
- Its machine-readable report records the executable filename and version/capabilities but deliberately excludes the selected installation directory. It also records the immutable job fingerprint, dimensions/count, progress, final size, source preservation, temporary-file absence, cancellation timing/exit result and failure-containment results.
- Extended the governed verifier and PH-09 canonical status/evidence owners. No production renderer/export algorithm, argument vector, schema, dependency, package, application UI or user data changed.

### Ran and passed

- `cmake --build build --config Release --target MandelbrotExternalVideoFixture --parallel 4` passed with MSBuild 18.8.2 after the expected sandbox FileTracker denial and permitted native retry.
- The privacy-corrected native Release fixture passed with FFmpeg 8.1.1 full build: version, `libx264` and MP4 muxer capability probes; production PNG sequence generation/verification; fixed-vector H.264/yuv420p encoding; progress through frame 3; decode probe; atomic promotion; and source preservation.
- The bounded lower-level cancellation extension requested cancellation after 200 ms and returned in 257 ms with Windows exit code 1223. Follow-up inspection found no FFmpeg process and no additional output.
- The integrated cancellation extension built 120 verified 640x360 same-volume linked frames from a production-rendered seed, selected the production `veryslow` job and armed cancellation only after its process runner began. The child and export boundary both reported cancellation with exit code 1223 in 116 ms; no final or temporary MP4 remained, both bounded logs existed, and every source frame, receipt and manifest entry revalidated.
- The post-preflight failure extension launched FFmpeg with the exact immutable production arguments while its fixture-owned source directory was temporarily unavailable. FFmpeg returned its real missing-sequence error; bounded logs remained, partial/final MP4s were absent, the directory was restored, and all three PNGs, receipts and the manifest revalidated.
- The focused native core test passed after adding the odd-dimension preflight regression.
- The promoted `verified-fixture.mp4` is 3,426 bytes. Three 7,409-byte PNGs, three receipts, the schema-3 manifest and bounded logs remain; no `.part.mp4` remains.
- Inspection of `report.json` confirmed `ffmpegPathPersisted=false` and no full WinGet/user path.
- Read-only `ffprobe.exe` inspection independently reported H.264, yuv420p, 96x64, 3/1 fps, three frames, 1.000000-second duration and 3,426-byte size.
- The final complete MSVC Release build passed and linked the application plus every test/fixture target. `ctest --test-dir build -C Release --output-on-failure` passed 7/7 in 17.90 seconds, and `C:\Python314\python.exe .\scripts\verify-source.py` passed the governed source/offline audit after integration.

### Failed and bounded

- A first run used an invalid timeline ID and was corrected before encoding.
- A later run under the longer `artifacts/external-video/...` hierarchy passed capability probing and encoding but could not open its 265-character log filename. The standalone fixture lacks the production application's embedded long-path-aware manifest. A fresh shorter artifact root passed without changing production behavior; the failed artifacts remain available for diagnosis.
- A sandboxed invocation could not see the user-installed encoder. The permitted explicit invocation then passed; this was an environment access boundary, not an encoder capability failure.

### Artifacts

- `artifacts/p9-v7/report.json`
- `artifacts/p9-v7/media-inspection.json`
- `artifacts/p9-v7/verified-fixture.mp4`
- `artifacts/p9-v7/frames/.mw-frame-sequence/manifest.json`
- `artifacts/p9-v7/frames/.mw-frame-sequence/receipt-0000.json` through `receipt-0002.json`
- `artifacts/p9-v7/frames/.mw-frame-sequence/video-export/*.stdout.log` and `*.stderr.log`
- `artifacts/p9-v7/cancellation-frames/` contains the 120-frame integrated cancellation source/receipt set and cancellation logs.
- `artifacts/p9-v7/failure-frames/` contains the separately revalidated source/receipt set and real failure logs.

### Skipped and remaining unproven

- The modal Video Export dialog, progress controls, keyboard/DPI behavior and wallpaper pause/GPU-release/resume route were not exercised.
- Full real `RunExternalVideoExport` cancellation cleanup passed in the bounded fixture. Interactive cancellation from the modal Video Export dialog remains unproven.
- Compatibility and licensing were not reviewed for any FFmpeg build other than the exact externally installed 8.1.1 full build used here. No FFmpeg distribution claim is made.
- No interactive or destructive runtime route was inferred from the automated build, tests or media inspection.

### Rollback

- Remove `ExternalVideoFixtureTool.cpp`, its opt-in CMake target and BR-20260909-01 verifier/documentation entries. Production PH-09 behavior, schemas, packages and user data are unaffected; this checkout has no Git rollback identifier.

## BR-20260909-02 — PH-01 native installer artifact validation

**Status:** Automated clean native build/test/package/startup and installer production passed; install/upgrade/uninstall, signing and the broader manual matrix remain open  
**Date:** 2026-09-09  
**Scope:** Make the canonical Windows release workflow discover supported per-user Inno Setup installations and strengthen installer artifact evidence without installing software or changing user settings

### Inspected

- Current release plan, version owner, Windows validation contract, risk register, `build-release.ps1`, `validate-windows-release.ps1`, Inno Setup definition and the installed compiler location.
- The generated portable ZIP, installer file metadata, SHA-256, Authenticode status, isolated application-data logs and the machine-readable validation report.

### Changed

- Extended the existing scalar-safe Inno Setup lookup with a `Get-Command ISCC.exe` fallback so machine-wide, per-user and `PATH` installations are supported without hard-coding a user directory.
- Strengthened installer validation from existence-only to require a non-empty file, embedded file version 1.13.1, SHA-256 capture and explicit Authenticode status.
- Extended the source-policy gate and canonical release evidence owners. No application runtime, installer payload definition, persistence schema, dependency, user data or system configuration changed.

### Ran and passed

- `scripts/validate-windows-release.ps1 -ReportDirectory artifacts/windows-validation/20260909-ph11-installer-final` passed its source/offline gate, clean Visual Studio 18 2026 x64 Release configure/build, complete CTest 7/7, executable metadata, 87-entry portable ZIP inspection, isolated process-scoped D3D11 startup/render/clean shutdown and installer artifact validation.
- Inno Setup 6.7.3 produced `dist/Mandelbrot-Live-Wallpaper-1.13.1-Setup-x64.exe`. The final report records its non-zero size, version 1.13.1, SHA-256 and `NotSigned` Authenticode state.
- Normal `%LOCALAPPDATA%` settings were not loaded or changed; the runtime smoke used the report-owned `MW_APPDATA_DIR`.

### Skipped and remaining unproven

- The installer was not launched, installed, upgraded or uninstalled because those operations change per-user installation state, shortcuts and uninstall registration. Compatibility with a preceding installed release remains unproven.
- Code signing was not attempted because no accepted certificate/signing authority or credentials are present. The artifact is explicitly unsigned.
- Windows 11, manual dialog/desktop interaction, mixed-DPI/multi-monitor, Explorer/session/power/device-loss lifecycle and soak remain open. Repository cleanliness remains unproven because this supplied source copy has no `.git` directory.

### Artifacts

- `artifacts/windows-validation/20260909-ph11-installer-final/report.json`
- `artifacts/windows-validation/20260909-ph11-installer-final/report.md`
- `artifacts/windows-validation/20260909-ph11-installer-final/validation.log`
- `dist/Mandelbrot-Live-Wallpaper-1.13.1-win-x64.zip`
- `dist/Mandelbrot-Live-Wallpaper-1.13.1-Setup-x64.exe`

### Rollback

- Revert the Inno Setup discovery fallback, strengthened validator assertions and BR-20260909-02 documentation/verifier markers together. Generated build/package/report artifacts may be discarded independently; no persisted application or user data migration is involved.

## BR-20260909-03 — Native Palette/Equation camera retention, precision and history interaction

**Status:** Native process-scoped interaction fixture passed; remaining PH-05 and broader Windows matrix stay open  
**Date:** 2026-09-09  
**Scope:** Convert the reported Palette/Equation camera-retention and Automatic preview-precision regressions from source/core-only coverage into a repeatable native Windows interaction gate without touching normal settings or desktop runtime

### Inspected

- Current governance, active PH-05 plan, UI routes, history owner and Windows validation contract.
- `AppWindow` controls and command handlers, Palette/Equation window classes and controls, exact-camera canonicalisation, precision status reporting and structural history replay.
- Existing BR-20260907-01 production correction and core merge/planner tests.

### Changed

- Added the Windows-only `MandelbrotWindowsInteractionFixture` target and registered it as a serial 90-second CTest gate with an explicit dependency on the real `MandelbrotWallpaper` executable.
- The fixture launches only its owned application process with a unique isolated `MW_APPDATA_DIR`, opens Palette/Equation through the real main-window commands, drives real control notifications, reads the production status/history controls, applies no desktop mode and requires the real Exit command plus clean shutdown log. Its failure fallback can terminate only its owned process.
- Added structural verifier and canonical route/history/release evidence markers. No production application behavior, schema, dependency, settings, installer definition or desktop integration changed in this delivery.

### Ran and passed

- The first warnings-as-errors MSVC build attempt was denied by sandboxed Visual Studio FileTracker access; the identical permitted native build passed and linked the application and fixture.
- The first focused runtime run failed only because the fixture expected the input spelling `0.00000001` after the production exact parser correctly canonicalised it to `1e-8`. Cleanup still used the real Exit route, the isolated log recorded shutdown and no process remained. The assertion was corrected to the accepted normalized-scientific grammar.
- The strengthened focused CTest gate passed in 14.74 seconds. Automatic preview precision changed from `GPU float32` to `Split high/low float` for canonical camera `1.25e-1, -2.5e-1, 1e-8`, then returned to `GPU float32` at `-5e-1, 0, 2.5e-1`.
- Accepting Palette frequency 1.75 and Equation power 3 retained the cameras changed while each editor was open. Native controls exposed `Edit Palette` and `Edit Equation` structural history labels. Equation Undo/Redo restored powers 2/3 and preserved the newer camera.
- The application exited cleanly and no `MandelbrotWallpaper` or fixture process remained. The cumulative clean release workflow subsequently passed CTest 8/8 and is recorded under `artifacts/windows-validation/20260909-ph11-interaction-final/`.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`
- `artifacts/windows-validation/20260909-ph11-interaction-final/report.json`
- `artifacts/windows-validation/20260909-ph11-interaction-final/report.md`
- `artifacts/windows-validation/20260909-ph11-interaction-final/validation.log`

### Skipped and remaining unproven

- Settings, Journey, preset/import and Scout Apply structural interaction remain open for PH-05/PH-10.
- This message-driven native fixture does not prove keyboard, DPI, accessibility, visual layout, desktop modes, Windows 11, multi-monitor, lifecycle/device-loss, installer install/upgrade/uninstall or long-session soak behavior.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Rollback

- Remove `WindowsInteractionFixtureTool.cpp`, its CMake/CTest wiring and BR-20260909-03 verifier/documentation entries. The fixture creates no schema migration or normal user-state change; generated report directories can be discarded independently.

## BR-20260909-04 — Native preset load history interaction

**Status:** Native preset Load/Undo/Redo interaction passed; Settings, Journey, import and Scout Apply remain open  
**Date:** 2026-09-09  
**Scope:** Extend the isolated PH-05 Windows interaction gate through one complete built-in preset replacement and remove timing races from the dialog fixture

### Inspected

- Active PH-05 plan, REQ-015, VAL-016/VAL-021, UI/history owners and the current native interaction evidence.
- Main-window preset combo selection, classified `LoadSelectedPreset` replacement, structural history labels/replay and exact camera refresh.
- Palette/Equation dialog construction, nested message loops and the existing process-scoped fixture's close/commit observation boundary.

### Changed

- The native fixture now waits for the required Palette/Equation controls before editing, preventing final dialog population from overwriting a premature test message.
- Accepted-dialog assertions now wait for the main-window history label that proves the nested dialog returned and its candidate committed, rather than treating top-level window destruction as commit completion.
- Added built-in preset selection through the real combo notification. The fixture requires `Load Preset`, an actual camera change, exact Undo restoration, and Redo restoration of both camera and visible `Seahorse Valley` identity.
- Updated only the current validation/route/history/plan evidence owners and structural verifier. No production behavior, persistence schema, dependency, desktop integration or normal user data changed.

### Ran and passed

- Sandboxed MSVC initially reproduced the known Visual Studio FileTracker `E_ACCESSDENIED` boundary; the identical permitted warnings-as-errors build passed and linked the application and fixture.
- During diagnosis, the unstabilised fixture reproduced both timing races. Its evidence showed the preview precision and Palette camera-retention path remained correct while Equation commit inspection could occur too early. The final readiness/commit synchronization removed that nondeterminism.
- `ctest --test-dir build -C Release -R "^WindowsInteractionFixture$" --output-on-failure`: passed in 14.48 seconds with native preset Load/Undo/Redo evidence.
- `ctest --test-dir build -C Release --output-on-failure`: passed 8/8 after the fixture synchronization change.
- `C:\Python314\python.exe scripts\verify-source.py`: passed after the implementation and documentation update.
- The isolated report records clean application shutdown and no fixture-owned process remained.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`

### Skipped and remaining unproven

- Settings, Journey, imported-preset and Scout Apply native atomic interaction remain open.
- The fixture does not apply desktop modes or prove keyboard, DPI, accessibility, visual layout, Windows 11, multi-monitor, lifecycle/device-loss, installer install/upgrade/uninstall or soak behavior.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Rollback

- Remove the preset-load block and dialog readiness/commit waits from `WindowsInteractionFixtureTool.cpp`, then remove the BR-20260909-04 verifier/documentation entries. Generated isolated report/app-data directories can be discarded independently; there is no product migration or user-state rollback.

## BR-20260909-05 — Native Settings history and main-window synchronisation

**Status:** Implemented; native Release build, full suite and source verifier passed; broader PH-05 routes remain open  
**Date:** 2026-09-09  
**Scope:** Extend the isolated native Settings transaction check and correct stale main-window camera controls after Settings acceptance

### Inspected

- Settings candidate-copy, accept, history and renderer-restart paths; main-window camera fields and their synchronisation helper.
- The existing process-scoped Windows interaction fixture and PH-05 VAL-016/VAL-021 evidence boundary.

### Changed

- Added native Settings rotation accept/Undo/Redo coverage through the real window controls and `Edit Project Settings` transaction.
- Replaced the accepted Settings path's coordinate-triplet-only refresh with `SyncMainWindowCameraControls`, so X, Y, scale and rotation all reflect the authoritative working preset.
- Updated the current plan, route, history, validation and delivery owners plus the governed structural verifier. No persistence schema, dependency, precision policy, desktop integration or normal user data changed.

### Ran and passed

- The first focused native fixture failed with `the accepted Settings rotation was not reflected in the main-window control`, directly reproducing the stale-control regression.
- The warnings-as-errors native application/fixture rebuild passed after the synchronization fix.
- `ctest --test-dir build -C Release -R WindowsInteractionFixture --output-on-failure`: passed in 14.70 seconds. The report records Settings rotation `0`/`17.5` accept/Undo/Redo, Palette/Equation camera retention and Automatic preview precision transitions.
- `cmake --build build --config Release --parallel`: passed for the application and every test/fixture target.
- `ctest --test-dir build -C Release --output-on-failure`: passed 8/8 in 31.81 seconds; the repeated Windows interaction fixture passed in 14.32 seconds.
- `C:\Python314\python.exe scripts\verify-source.py`: passed after the final implementation, fixture and documentation updates.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`

### Skipped and remaining unproven

- Journey, imported-preset and Scout Apply native atomic interaction remain open.
- Keyboard, DPI, accessibility, visual layout, desktop modes, Windows 11, multi-monitor, lifecycle/device-loss, installer install/upgrade/uninstall and soak behavior remain outside this focused check.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Rollback

- Restore the accepted `OpenSettings` path to its prior coordinate-triplet refresh, remove the Settings block from `WindowsInteractionFixtureTool.cpp`, and remove the BR-20260909-05 verifier/documentation entries. No schema or user-data rollback is required.

## BR-20260909-06 — Native Journey history interaction

**Status:** Native Release build, complete suite and source verifier passed; import and Scout Apply remain open  
**Date:** 2026-09-09  
**Scope:** Extend the isolated PH-05 interaction gate through exact structured Journey acceptance and atomic replay

### Inspected

- Current PH-05 plan, REQ-015, VAL-016/VAL-021, UI/history owners and the current native fixture.
- Journey dialog control construction, UTF-8 conversion, validation, candidate preview/rollback, accepted replacement and history labelling.

### Changed

- Added the production Journey Settings class and control identities to the process-scoped fixture.
- Added a valid two-row waypoint edit through the real dialog, followed by exact accepted-state, Undo and Redo inspections through repeated dialog opens.
- Updated only the current validation, route, history, plan, roadmap and delivery owners plus the structural verifier. No production behavior, persistence schema, dependency, desktop integration or normal user data changed.

### Ran and passed

- `cmake --build build --config Release --target MandelbrotWindowsInteractionFixture --parallel`: passed with warnings as errors.
- `ctest --test-dir build -C Release -R "^WindowsInteractionFixture$" --output-on-failure`: passed in 14.96 seconds.
- The isolated report records exact Journey before/after replay and clean application shutdown.
- `cmake --build build --config Release --parallel`: passed for every application/test/fixture target.
- `ctest --test-dir build -C Release --output-on-failure`: passed 8/8 in 32.37 seconds; the repeated Windows interaction fixture passed in 14.66 seconds.
- `C:\Python314\python.exe scripts\verify-source.py`: passed after the implementation and evidence updates.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`

### Skipped and remaining unproven

- Import and Scout Apply native atomic interaction remain open.
- Journey desktop playback, keyboard, DPI, accessibility, visual layout, Windows 11, multi-monitor, lifecycle/device-loss, installer install/upgrade/uninstall and soak behavior remain unproven.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Rollback

- Remove the Journey block/constants from `WindowsInteractionFixtureTool.cpp` and remove the BR-20260909-06 verifier/documentation entries. Generated isolated report/app-data directories can be discarded independently; no product migration or user-state rollback is required.

## BR-20260910-01 — Native preset Import history interaction

**Status:** Native Release build, complete suite and source verifier passed; Scout Apply remains open  
**Date:** 2026-09-10  
**Scope:** Extend the isolated PH-05 interaction gate through the native preset Open dialog and atomic Import replay

### Inspected

- Current PH-05 plan, REQ-015, VAL-016/VAL-021, UI/history owners, production preset import/serialisation paths and the current native fixture.
- Win32 common-dialog control hierarchy and the fixture's completion synchronization after submitting an absolute preset path.

### Changed

- Linked the fixture to `MandelbrotCore`, created its candidate with the production preset serialiser and required a production deserialisation round trip before interaction.
- Added native Import selection through the real common Open dialog followed by exact selected-identity and camera checks after accept, Undo and Redo.
- Corrected the fixture's dialog-completion wait to treat destruction of the submitted Open dialog as successful completion instead of misreporting it as failed directory navigation.
- Updated only the current validation, route, history, plan, roadmap and delivery owners plus the structural verifier. No production behavior, persistence schema, dependency, desktop integration or normal user data changed.

### Ran and passed

- `cmake --build build --config Release --target MandelbrotWindowsInteractionFixture --parallel`: passed with warnings as errors.
- Two consecutive `ctest --test-dir build -C Release -R WindowsInteractionFixture --output-on-failure` runs passed in 16.10 and 15.52 seconds.
- The isolated report records exact preset Import before/after camera and selected-identity replay plus clean application shutdown.
- `cmake --build build --config Release --parallel`: passed for every application/test/fixture target.
- `ctest --test-dir build -C Release --output-on-failure`: passed 8/8 in 31.84 seconds; the Windows interaction fixture passed in 15.61 seconds.
- `C:\Python314\python.exe scripts\verify-source.py`: passed before and after the final evidence update.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`

### Skipped and remaining unproven

- Scout Apply native atomic interaction remains open.
- Invalid/malicious import-file UI handling, keyboard, DPI, accessibility, visual layout, Windows 11, multi-monitor, lifecycle/device-loss, installer install/upgrade/uninstall and soak behavior remain unproven.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Rollback

- Remove the Import block/helpers and `MandelbrotCore` fixture link from `WindowsInteractionFixtureTool.cpp`/`CMakeLists.txt`, then remove the BR-20260910-01 verifier/documentation entries. Generated isolated report/app-data directories can be discarded independently; no product migration or user-state rollback is required.

## BR-20260910-02 — Native Fractal Scout history interaction

**Status:** Native Release build, complete suite and source verifier passed; automated PH-05 interaction matrix complete  
**Date:** 2026-09-10  
**Scope:** Complete the automated PH-05 native replacement matrix through production Scout search and camera-only Apply replay

### Inspected

- Current PH-05/PH-10 plan, AC-018, VAL-036, UI/history owners, production `FractalScoutDialog` controls and AppWindow's Scout Apply mutation boundary.
- Automatic search completion, candidate selection, `Use in Preview`, exact camera replay, history label and selected-preset identity behavior.

### Changed

- Added the production Scout window/control identities to the process-scoped fixture.
- Waited for the automatically started bounded production search to expose an enabled candidate, invoked `Use in Preview`, and verified one `Apply Scout Camera` entry.
- Added exact before/after camera and unchanged selected-preset identity checks across native Undo and Redo.
- Updated only the current validation, route, history, plan, roadmap and delivery owners plus the structural verifier. No production behavior, persistence schema, dependency, desktop integration or normal user data changed.

### Ran and passed

- `cmake --build build --config Release --target MandelbrotWindowsInteractionFixture --parallel`: passed with warnings as errors.
- Two consecutive `ctest --test-dir build -C Release -R WindowsInteractionFixture --output-on-failure` runs passed in 16.92 and 16.45 seconds.
- The isolated report records exact Scout Apply camera replay, stable preset identity and clean application shutdown.
- `cmake --build build --config Release --parallel`: passed for every application/test/fixture target.
- `ctest --test-dir build -C Release --output-on-failure`: passed 8/8 in 32.62 seconds; the Windows interaction fixture passed in 16.51 seconds.
- `C:\Python314\python.exe scripts\verify-source.py`: passed before and after the final evidence update.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`

### Skipped and remaining unproven

- Scout cancellation, refinement and save-as-new interaction; keyboard, DPI, accessibility, visual layout, Windows 11, multi-monitor, desktop modes, lifecycle/device-loss, installer install/upgrade/uninstall and soak behavior remain unproven.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Rollback

- Remove the Scout block/constants from `WindowsInteractionFixtureTool.cpp` and remove the BR-20260910-02 verifier/documentation entries. Generated isolated report/app-data directories can be discarded independently; no product migration or user-state rollback is required.

## BR-20260910-03 — Native Fractal Scout isolation and PH-10 completion

**Status:** Native Release build, complete suite and source verifier passed; PH-10 complete at documented scope  
**Date:** 2026-09-10  
**Scope:** Close the PH-10 native preview-isolation gap without changing the deterministic Scout engine

### Inspected

- PH-10 plan, AC-018, VAL-034–VAL-036, the canonical Scout owner, production dialog candidate/result ownership and the completed PH-05 Apply route.

### Changed

- Extended the process-scoped fixture to wait for a completed selected Scout candidate, invoke Close and require exact camera, preset identity and Undo-label equality.
- Reopened the production dialog and retained exact camera-only Apply/Undo/Redo replay, so isolation and commit are checked in one serial native process.
- Updated only the current Scout, validation, route, plan, roadmap and delivery owners plus the structural verifier. No production behavior, search policy, schema, dependency, desktop integration or normal user data changed.

### Ran and passed

- `cmake --build build --config Release --target MandelbrotWindowsInteractionFixture --parallel`: passed with warnings as errors.
- Two consecutive `ctest --test-dir build -C Release -R WindowsInteractionFixture --output-on-failure` runs passed in 17.64 and 17.47 seconds.
- The isolated report records candidate-selection/Close isolation, exact Scout Apply replay, stable preset identity and clean application shutdown.
- `cmake --build build --config Release --parallel`: passed for every application/test/fixture target.
- `ctest --test-dir build -C Release --output-on-failure`: passed 8/8 in 33.40 seconds; the Windows interaction fixture passed in 17.43 seconds.
- `C:\Python314\python.exe scripts\verify-source.py`: passed before and after the complete run.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`

### Skipped and remaining unproven

- Scout cancellation, refinement, save-as-new, optional style/session extensions and deep exact Scout execution remain outside this delivery.
- Keyboard, DPI, accessibility, visual layout, Windows 11, multi-monitor, desktop modes, lifecycle/device-loss, installer install/upgrade/uninstall and soak behavior remain unproven.
- Repository cleanliness and a Git rollback identifier remain unproven because this supplied checkout has no `.git` directory.

### Rollback

- Remove the Scout Close-isolation preflight from `WindowsInteractionFixtureTool.cpp` and remove the BR-20260910-03 verifier/documentation entries. Generated isolated report/app-data directories can be discarded independently; no product migration or user-state rollback is required.

## BR-20260910-04 — Native Animation Timeline clock and candidate interaction

**Status:** Native Release build, complete suite and source verifier passed; bounded Timeline interaction advanced  
**Date:** 2026-09-10  
**Scope:** Add production Timeline clock, candidate rollback and runtime-only acceptance evidence

### Changed

- Added production Timeline controls and exercised Add Track, Add Current Value, midpoint scrub, Play and Stop.
- Verified Cancel discards the candidate without project/history mutation and OK retains one runtime-only track on reopen.

### Ran and passed

- Focused warnings-as-errors fixture build passed.
- Two consecutive focused interaction runs passed in 18.26 and 17.99 seconds.
- The isolated report records scrub/play/stop, Cancel rollback, runtime-only OK/reopen state and clean shutdown.
- Complete warnings-as-errors MSVC Release build passed for every target.
- Release CTest passed 8/8 in 34.06 seconds; the Windows interaction fixture passed in 17.97 seconds.
- The governed source/offline verifier passed before and after the complete run.

### Remaining unproven

- Rendered visual playback, native Journey conversion, wallpaper/export clocks, DPI, keyboard, accessibility, lifecycle and soak remain open.

### Rollback

- Remove the Timeline block/constants and common-control include from `WindowsInteractionFixtureTool.cpp`, then remove the BR-20260910-04 verifier/documentation entries. No data migration is required.

## BR-20260910-05 — Native Journey and Timeline conversion interaction

**Status:** Native Release build, complete suite and source verifier passed; Journey/Timeline interaction advanced  
**Date:** 2026-09-10  
**Scope:** Add bounded production Journey-to-Timeline and reverse-preparation interaction evidence

### Changed

- Added a valid Journey to the fixture-owned production-serialised import preset.
- Invoked Journey → Tracks and required exactly three camera tracks, then invoked Tracks → Journey asynchronously and required its successful preparation confirmation.
- Closed the Windows confirmation through standard `IDOK` command semantics and proved Timeline Cancel leaves project/history state unchanged.

### Ran and passed

- Focused warnings-as-errors fixture build passed.
- Two consecutive focused interaction runs passed in 18.39 and 18.11 seconds.
- The isolated report records both conversion directions, successful preparation, Cancel isolation and clean shutdown.
- Complete warnings-as-errors MSVC Release build passed for every target.
- Release CTest passed 8/8 in 34.17 seconds; the Windows interaction fixture passed in 18.10 seconds.
- The governed source/offline verifier passed before and after the complete run.

### Remaining unproven

- Accepted reverse conversion that changes Journey data, rendered visual playback, DPI, keyboard, accessibility, lifecycle and soak remain open.

### Rollback

- Remove the conversion interaction block/constants and fixture-owned Journey value, then remove the BR-20260910-05 verifier/documentation entries. No data migration is required.

## BR-20260910-06 — Native frame-sequence export and cancellation interaction

**Status:** Native WIC success, matching resume, refusal, visible cancellation and close-while-active cancellation passed; broader UI/lifecycle gates remain open  
**Date:** 2026-09-10  
**Scope:** Extend the isolated Windows fixture through the production PH-08 Frame Sequence Export dialog without applying a desktop mode

### Inspected

- PH-08 canonical plan, ROUTE-014, VAL-027–VAL-030, production dialog controls, immutable job construction, WIC row encoding, promotion/resume/cancellation boundaries and AppWindow wallpaper-pause wrapper.
- Existing runtime-only Timeline interaction and process-scoped fixture cleanup/state-isolation boundaries.

### Changed

- Added production Frame Sequence Export class/control identities and bounded combo/PNG-inspection helpers to `WindowsInteractionFixtureTool.cpp`.
- The fixture accepts a 0.04-second palette-offset timeline, sets an ordinary exact camera and runs six isolated dialog jobs: one-frame success, exact matching-manifest resume, mismatched-manifest refusal, untracked-final refusal, visible early cancellation and close-while-active cancellation.
- Success requires one 32x24 PNG, manifest, independently read PNG signature/IHDR dimensions and no `.part` file. Resume requires `0 rendered, 1 resumed`. The refusal cases require their production error text and exact preservation of the verified PNG or fixture sentinel. Both cancellation routes require a joined worker/control re-enable, retained resumable metadata and no partial file; close while active must retain the cancelled dialog until a second close.
- Closing each completed dialog must restore the enabled main owner without changing exact camera text, selected preset identity or Undo label. No production behavior, persistence schema, dependency, desktop integration or user-owned path changed.

### Ran and passed

- Focused warnings-as-errors MSVC fixture build passed.
- Two consecutive focused Windows interaction runs with the active-close path passed in 21.06 and 21.04 seconds.
- Complete warnings-as-errors MSVC Release build passed for every target.
- Release CTest passed 8/8 in 37.84 seconds; the cumulative Windows interaction fixture passed in 21.20 seconds.
- The isolated report records WIC success, matching resume, manifest/untracked refusal, visible Cancel and close-while-active cleanup, owner/project isolation and clean application shutdown.
- `C:\Python314\python.exe scripts\verify-source.py` passed after the implementation and evidence updates.

### Artifacts

- `build/test_artifacts/windows-interaction/report.md`
- Latest fixture-owned application-data directory beneath `build/test_artifacts/windows-interaction/`, containing the completed PNG/manifest and cancelled-job manifest.

### Skipped and remaining unproven

- Open Folder launch, mid-sequence cancellation after verified frames, approved cross-platform selected-frame visuals, wallpaper pause/GPU-release/resume, DPI, keyboard, accessibility, lifecycle and soak remain open.

### Rollback

- Remove the Frame Sequence Export block/constants/helpers and bounded Timeline target/duration setup from `WindowsInteractionFixtureTool.cpp`, then remove the BR-20260910-06 verifier/documentation entries. Fixture-owned output is disposable; no data migration is required.

## BR-20260918-01 — Native Video Export validation interaction

**Status:** Native typed-sequence summary, input validation, missing-executable containment, real verified MP4 success and visible cancellation passed; broader lifecycle gates remain open  
**Date:** 2026-09-18  
**Scope:** Extend the isolated Windows fixture through production PH-09 Video Export validation and an opt-in explicitly supplied real-encoder success route without applying a desktop mode

### Inspected

- PH-09 canonical plan, production dialog controls, manifest summary, CRF/input validation, executable preflight, temporary/final MP4 boundary and AppWindow wallpaper-pause wrapper.
- Existing BR-20260909-01 real FFmpeg production-path fixture and BR-20260910-06 verified PH-08 output.

### Changed

- `VideoExportDialog.cpp` now refreshes its manifest summary when the sequence path is typed or pasted, matching the existing Browse action.
- The native fixture opens the production dialog against its verified one-frame sequence, checks the refreshed 32x24 summary, dismisses missing-input and CRF-range errors, and exercises nonexistent-executable refusal.
- `MW_TEST_FFMPEG_PATH` optionally enables a production-dialog success run without making default CTest depend on FFmpeg. It selects the bounded ultrafast preset, completes capability checks/encode/decode verification/promotion, requires Open Output to enable, and inspects the fixture-owned final/source/temp-file boundary.
- `MW_TEST_VIDEO_CANCEL_SEQUENCE` optionally enables a visible Cancel run against the existing fixture-owned verified 120-frame sequence. It waits for owned encoding, cancels the process, revalidates the source manifest/frames and requires no final or temporary cancellation MP4.
- The failure route must re-enable Start, leave no final or `.part.mp4` output, restore the main owner and preserve exact camera, preset identity and history. No FFmpeg process, desktop mode, persistence schema, dependency or user-owned path is involved.

### Ran and passed

- Focused warnings-as-errors MSVC fixture build passed.
- Two consecutive focused Windows interaction runs passed in 21.41 and 21.38 seconds.
- Two opt-in production-dialog runs with the explicitly resolved FFmpeg 8.1.1 executable passed in 22.82 and 22.09 seconds. The latest verified/promoted MP4 is 1,650 bytes; its source PNG remains and no `.part.mp4` survives.
- Two combined production-dialog success/cancellation runs passed in 22.85 and 23.08 seconds. Visible Cancel terminated the owned encode, the verified 120-frame source sequence remained intact, and neither final nor temporary cancellation output survived.
- Complete warnings-as-errors MSVC Release build passed for every target.
- Release CTest passed 8/8 in 37.82 seconds; the cumulative Windows interaction fixture passed in 21.52 seconds.
- The isolated opt-in report records typed-sequence summary refresh, validation/refusal containment, verified MP4 completion, owner/project isolation and clean application shutdown.
- `C:\Python314\python.exe scripts\verify-source.py` passed after the implementation and canonical evidence updates.

### Skipped and remaining unproven

- Open Output launch through the dialog, wallpaper pause/GPU-release/resume, DPI, keyboard, accessibility, other FFmpeg builds, licensing suitability, lifecycle and soak remain open.

### Rollback

- Remove the `SequenceEdit` summary-refresh handler and Video Export fixture block/constants/helpers including the opt-in environment route, then remove the BR-20260918-01 verifier/documentation entries. Fixture-owned output is disposable; no data migration is required.

## BR-20260919-01 — File-backed desktop modes

**Status:** Implemented; native Release build, source policy, CTest 8/8 and opt-in desktop MP4 start/stop passed  
**Date:** 2026-09-19  
**Scope:** Remove continuous fractal desktop rendering while retaining plain-image Static/Slideshow and adding an already-exported MP4 desktop mode

### Inspected

- Desktop-mode controls, Quick Controller commands, startup/default application, static capture/gallery/slideshow transitions and failure fallbacks.
- `WallpaperController` render loop, GPU retry/CPU-render fallback, WIC image decode/paint, Explorer reattachment and display-change handling.
- Settings schema/default-mode migration and existing external H.264/MP4 export boundary.

### Changed

- Desktop modes are now None, Static image, Slideshow and Video file. Live image, Journey desktop, desktop zoom and desktop colour-cycle actions were removed; Journey/timeline animation remains available for preview and export authoring.
- `WallpaperController` no longer exposes a live `Start` API and contains no per-tick fractal render, recovery render or CPU-render fallback. Static/slideshow decode WIC image files and paint pixels; slideshow ticks only change files.
- Exported local `.mp4` files can be selected, validated for a video stream, muted, looped and scaled through Windows Media Foundation. Playback does not invoke FFmpeg or the fractal renderer.
- Settings schema 11 persists `videoWallpaper.filePath`. Schema-10 `live-image`/`journey` defaults and legacy `startWallpaperOnLaunch=true` migrate to None through the existing original-preserving promotion path.
- Static-gallery and video failures stop visibly and cannot fall back to continuous rendering. Static capture remains a one-time bounded render that writes a normal image and shuts its capture renderer down.

### Ran and passed

- Native Visual Studio 18 2026 x64 Release configure/compile/link passed after one expected sandbox-only MSVC FileTracker access failure; the rerun outside the sandbox succeeded.
- `C:\Python314\python.exe scripts\verify-source.py` passed, including absence checks for removed live desktop routes and renderer/fallback code.
- The final Release CTest passed 8/8 in 39.38 seconds. Core migration/persistence tests, CPU visuals, D3D11 WARP/hardware, OpenGL and cumulative native interaction passed.
- The first CTest run failed only because the interaction fixture still parsed the old `| wallpaper` precision-label suffix; after updating that assertion to the new `| desktop file-backed` status, the rerun passed.
- The final opt-in production desktop route passed with `artifacts\p9-v7\verified-fixture.mp4`: the application opened the verified MP4 through Windows Media Foundation, reported playback start, accepted the production Stop action, detached the desktop host and shut down cleanly. The report is `build\test_artifacts\windows-interaction-video-wallpaper-final\report.md`.

### Skipped and remaining unproven

- Static/slideshow application, full end-of-file loop observation, independent mute verification, Explorer restart, display changes, pause/resume, multiple monitors, DPI/accessibility and soak remain runtime/manual gates. The bounded MP4 start/stop fixture proves the production route and clean detach, not those broader lifecycle properties.
- No installer/package/signing, Windows 11 or alternate-device matrix was run in this batch.

### Rollback

- Restore the schema-10 DesktopMode values/UI/controller live start and render loop, remove Media Foundation linkage and `videoWallpaper`, then restore the schema-10 verifier/docs. A settings downgrade writer is intentionally absent; rollback would require an explicit schema-11 read/migration policy.

## BR-20260923-01 — Current scope governance alignment

**Status:** Canonical governance aligned; source policy verification passed  
**Date:** 2026-09-23  
**Scope:** Make DEC-039's file-backed desktop boundary authoritative across current product, roadmap, phase, validation and maintenance documentation

### Inspected

- Current `AGENTS.md` and the canonical owner map in `PROJECT_INDEX.md`.
- Product foundation, traceability, implementation plan, roadmap, animation/export specifications, UI routes, data ownership, validation, maintenance, settings and decision history.
- Stale current-scope wording that still implied a desktop animation clock, wallpaper GPU renderer, deep desktop rendering or preview/wallpaper/export renderer arbitration.

### Changed

- Defined the application as a native offline fractal-authoring and file-backed desktop-presentation product while retaining its existing product name.
- Made preview, Journey, timeline, Scout and deep rendering authoring/output concerns only. Desktop presentation is limited to saved static images, saved-image slideshows and already-exported local MP4 video.
- Marked `AnimationClockDomain::Wallpaper` as inert compatibility/test state rather than a supported product domain; no timeline or precision plan may drive the desktop.
- Reframed export pause/resume policy around file-backed desktop presentation, which owns no fractal GPU renderer.
- Removed future PH-15 authority for deep/live desktop rendering and changed it to preview/export integration plus lifecycle validation of resulting files.
- Added explicit maintenance, release-matrix and non-goal rules preventing a failure path or future phase from restoring continuous desktop rendering.
- Updated the current schema observation to settings schema 11/preset schema 3 and linked RISK-031.
- Updated the structural verifier's PH-15 governance-title marker to match the narrowed authoring/output scope.

### Ran and passed

- `C:\Python314\python.exe scripts\verify-source.py` passed after the governance alignment.

### Skipped and remaining unproven

- No production application source, build files, persisted schemas or runtime behaviour changed. Only the governance-title marker in the structural verifier changed, so native compilation, CTest and runtime interaction were not rerun for this governance-only batch.
- Existing BR-20260919-01 runtime boundaries remain: static/slideshow application, full video-loop observation, independent mute verification, Explorer/display/multi-monitor lifecycle, DPI/accessibility and soak are still open.

### Rollback

- Revert only the BR-20260923-01 governance wording and ledger/history entries. Do not revert DEC-039, schema 11 or the BR-20260919-01 implementation.

## BR-20260923-02 — Desktop failure containment and monitor-scope removal

**Status:** Implemented; native Release build, CTest 8/8 and source policy passed  
**Date:** 2026-09-23  
**Scope:** Remove inert Independent monitor assignment, make asynchronous video/slideshow failures fail closed, remove obsolete desktop-renderer messaging and record MFPlay technical debt

### Inspected

- `AppSettings`, settings validation/serialization/promotion, both monitor-mode UI surfaces and their native control-ID consumers.
- MFPlay callback events, immediate playback operations, video-window update operations, slideshow cycling, main-window runtime state/default persistence and visible error handling.
- Status, Quick Controller, preview failure text, copied diagnostics and adaptive-performance sampling for removed desktop-renderer assumptions.
- Canonical scope, UI, persistence, traceability, risk, maintenance and decision owners.

### Changed

- Removed Independent monitor mode, per-monitor preset assignment state and both assignment UIs. Mirror and Span remain. Schema 12 migrates schema-11 `independent` to Mirror, ignores the obsolete assignment object, omits it on rewrite and preserves the original before promotion.
- Preserved retired numeric control-ID slots without recreating the controls, so the established native interaction contract remains stable.
- MFPlay callback errors and immediate `Play`, `Pause`, `SetPosition` and `UpdateVideo` failures now post into one controller failure path. That path logs, stops, detaches and exposes a one-shot runtime error to the main window, which clears running/default state and shows a visible error.
- Slideshow cycling now stops and reports visibly when every configured image fails to load instead of logging and retrying forever.
- Removed wallpaper FPS, GPU-release snapshot, static fallback and generic desktop GPU renderer claims from status, Quick Controller, preview failure messaging, adaptive sampling and copied diagnostics.
- Accepted DEC-040 for monitor-scope removal and DEC-041/RISK-032 for the retained legacy MFPlay dependency.
- Added bounded readiness and observed-control diagnostics to the native interaction fixture after the first cumulative run exposed an existing main-window-construction race.

### Ran and passed

- The first sandboxed MSVC invocation failed in Visual Studio `FileTracker` with access denied; the required rerun outside the sandbox completed the full warnings-as-errors x64 Release build.
- A focused native interaction run initially exposed shifted numeric IDs after control deletion; restoring retired ID slots fixed the compatibility regression. The focused fixture then passed in 18.98 seconds.
- `C:\Python314\python.exe scripts\verify-source.py` passed, including schema-12, removed-assignment and fail-closed media markers.
- Final Release CTest passed 8/8 in 35.20 seconds, including core migration coverage and the native Windows interaction fixture.

### Skipped and remaining unproven

- No deterministic injection fixture currently forces an MFPlay asynchronous callback failure or corrupts all slideshow files during an active cycle; those paths are source-verified and native-compiled but not runtime-induced in this batch.
- Full video-loop observation, independent mute verification, codec variation, Explorer restart, display changes, Mirror/Span multi-monitor visuals, pause/resume failure injection, DPI/accessibility, soak, Windows 11, installer/package and signing remain open.
- MFPlay replacement was intentionally not attempted; it remains the technical debt recorded by DEC-041.

### Rollback

- Reverting schema 12 requires an explicit policy for settings already promoted from schema 11; no downgrade writer exists. Source/UI failure handling and governance entries can otherwise be reverted as one batch, but restoring Independent assignment would reintroduce an inert contract and requires a new accepted product decision.

## BR-20260923-03 — PH-11 desktop and installer hardening

**Status:** Bounded native hardening passed; PH-11 remains in progress  
**Date:** 2026-09-23  
**Scope:** REQ-006/011/014–020/041, AC-003–006/012–020/041, VAL-006/007/008/017/061, affected risk and phase status reconciliation

### Inspected

Current AGENTS/index and canonical owners; child-process/app-data isolation; desktop timer/media/error paths; installer registration, mutex and deletion scope; live Windows/GPU/display state; earlier scoped PH-03–PH-13 evidence. The archive has no usable Git metadata.

### Changed

- Continued the handed-off static/slideshow/invalid-video/mute/loop/failure fixture. Disabled foreground-dependent/adaptive auto-pause only in fixture settings and explicitly asserted detachment after failure.
- Corrected valid-video startup handling: repaint and pause/resume wait for successful MFPlay playback readiness; display refresh uses the paint route, and first-loop evidence follows completed restart rather than submitted seek. MFPlay remains DEC-041/RISK-032 debt.
- Removed filename-global forced process termination and blanket recursive install-directory deletion. Setup/uninstall check the existing application mutex, automatic application closure is disabled, and untracked outputs survive uninstall. Added the isolated installer validator.
- Reconciled the active PH-11 pointer, requirement/phase/risk statuses, canonical document roles and release matrix. PH-14/15 remain proposed; implemented PH-12/13 slices do not imply phase completion. Updated the source verifier's stale PH-05 header marker to match the scoped native evidence.

### Run and passed

- Native Visual Studio 18 2026 x64 Release build and Release CTest **8/8**, **64.93 seconds**; interaction test **48.93 seconds** with the existing verified MP4.
- Static image, slideshow advancement and all-files failure, invalid MP4, API mute readback, completed video loop, and visible stop/detach after a posted asynchronous-failure message.
- Current-topology display handler, simulated owned-window host loss/reattachment, and bounded native tab-stop/button-name checks.
- **60-second requested static resource interval**; full fixture **109.091635 seconds**. Working set **-7,614,464 bytes**, handles **-111**, GDI **0**, USER **-2**. This is an endpoint-growth check, not long-duration leak proof.
- Isolated AppId install, production launch/Exit, running-app uninstall refusal, same-version reinstall and uninstall. Settings and untracked output survived. Normal data retained **132/132 identical hashes** and recorded wallpaper configuration stayed unchanged from the pre-installer snapshot through final tests.
- Final source verification and canonical link/status audit. [Full ledger and artifacts](../artifacts/ph11-20260923-03/report.md).

### Failed attempts

Restricted MSVC FileTracker and GPU access were denied; restricted desktop testing could not see Progman. Two native attempts reproduced valid-video startup HRESULT `0xC00D36B2`; corrected runs pass. Source verification initially rejected the reconciled PH-05 header because its text marker was stale; the marker was aligned and checks rerun. Logs are retained, rather than replaced by final-pass claims.

### Skipped and unproven

Windows 10 Pro 22H2 build 19045.6466 and AMD Radeon RX 7900 XT were observed, with two 100%-scaled monitors including negative X coordinates. Windows 11, mixed DPI, Mirror/Span visual correctness, physical reconnect, actual Explorer restart, lock/sleep/RDP/device-loss lifecycle, full accessibility, decoder-originated failure, dedicated startup pause/resume races and long-duration soak remain unproven. Real FFmpeg encoding/cancellation opt-ins were not rerun. Isolated same-version reinstall is not preceding-release upgrade or normal-AppId install evidence. Test installer is unsigned; no final package/release was produced.

### Rollback

Before-edit archive copies and selected-file hashes are retained under `artifacts/ph11-20260923-03/before/` and `source-before.json`. They already contain the handed-off fixture/media changes. Reverting this continuation does not revert schema 12, modify user data or establish a Git rollback point. PH-11 remains active.

## BR-20260924-01 — File-backed desktop export pause hardening

**Status:** Bounded native hardening passed; PH-08/PH-11 remain open  
**Date:** 2026-09-24  
**Scope:** REQ-006/009/017/018/041, AC-006/015–017/041, VAL-006/017/027–033/061

Inspected current governance, AppWindow modal/timer/user/automatic pause paths, file-backed WallpaperController and the isolated native fixture. Fixed automatic lifecycle recovery releasing an export pause before its dialog closed; both export routes now share a separate hold and re-evaluate current conditions on close. Also defer a queued reopen during owner enablement until the preceding modal loop returns.

Native MSVC Release build and CTest **8/8** passed in **154.86 seconds**; interaction **138.97 seconds**. All **24** static/slideshow/local-MP4 modal pause cases passed, preserving host, camera and Undo label. Existing WIC success/resume/refusal/cancellation checks passed separately with desktop presentation stopped. Initial regression and first-fix reopen failure are retained in the [evidence ledger](../artifacts/ph11-20260924-01/report.md), alongside final output, build/CTest/source logs, diff and before-edit rollback files.

Physical sleep/resume, visual/video-position pause proof, active export jobs with running desktop playback, Open Folder/Open Output, mixed DPI/full keyboard/accessibility and wider PH-11 release gates remain unproven. External FFmpeg encode/cancel opt-ins, soak and installer/package work were not rerun. DEC-039 still prohibits renderer-backed desktop modes; no phase completion or release is claimed.

## BR-20260924-02 — Exclusive editor-window state hardening

**Status:** Bounded native editor-only hardening passed; PH-05/PH-11 remain open  
**Date:** 2026-09-24  
**Scope:** REQ-014/015/020, AC-010–013/019–020, VAL-014–016/019–022

### Inspected

Current AppWindow command routing, Palette/Equation/Settings ownership, Quick Controller access, candidate-state commit/rollback paths, the native interaction fixture and current PH-11 governance. The existing modeless routes could admit a second state-editing window, leaving each window with a stale candidate snapshot and allowing reverse close order to restore old settings.

### Changed

- Added one process-wide `AppWindow::AuxiliaryWindowSession` RAII gate. State-editing auxiliary routes disable the main window and any open Quick Controller for the full candidate dialog lifetime; `WM_ENABLE` and command guards prevent legacy owner re-enablement and competing posted/tray routes from opening another editor.
- Kept Stop and Exit available as recovery actions while the gate is held, and restored the owner/controller enabled state only after the active route returns.
- Reconciled the active UI workflow, state/parameter, traceability, decision and validation owners with DEC-042. The earlier concurrent modeless-camera assumption remains historical evidence and is no longer an active contract.
- Added a process-scoped editor-only fixture mode covering Palette→Equation, Equation→Settings and Settings→Palette competition checks.

### Run and passed

- Direct MSVC `/c /std:c++20 /EHsc /MD` compilation passed for the changed AppWindow, Settings dialog and interaction-fixture sources; a second `/W4 /WX` pass also passed.
- Manually linked native editor-only binaries using the existing Release objects and the newly compiled AppWindow and fixture objects; the Settings dialog was compiled separately.
- `MW_TEST_EDITOR_EXCLUSIVITY_ONLY=1` native fixture passed all three cases: active editor disabled the main owner, a competing command produced no second editor during the 750 ms observation, and closing the active editor restored the owner. The fixture recorded clean application startup and shutdown. Evidence: `../artifacts/ph11-20260924-02/editor-only-runtime/report.md`.
- `scripts/verify-source.py` passed after the implementation and governance updates. Evidence: `../artifacts/ph11-20260924-02/source-check-final.log`.

### Failed or blocked

- The normal CMake/MSBuild build was retried and stopped in Visual Studio FileTracker with `E_ACCESSDENIED` before compilation. The required escalated retry was rejected by the host approval quota, so no cumulative CTest or Release build claim is made. Evidence: `../artifacts/ph11-20260924-02/build-attempt-sandbox.log` and `../artifacts/ph11-20260924-02/build-trackoff.log`.
- The broad desktop interaction fixture was not promoted because the restricted host could not expose the `Progman` desktop window. Evidence: `../artifacts/ph11-20260924-02/runtime-exclusive/report.md`.

### Skipped and unproven

Journey, Timeline, Scout, preset, render, export, file-picker and Quick Controller combinations beyond the three editor routes; physical window lifecycle, accessibility/keyboard coverage, mixed DPI, package/installer, ARM64 and full PH-11 release gates remain unproven. This batch proves the exclusive gate and the three bounded native routes only.

### Rollback

Before-edit copies and source hashes are retained under `../artifacts/ph11-20260924-02/before/`. Reverting the batch removes DEC-042 and the exclusive gate together; no persisted schema or user-data migration is involved.
