# Project Index

**Status:** Current canonical governance index; future phases retain their individual gates  
**Baseline:** Supplied source archive labelled 1.13.1, inspected 2026-07-27  
**Owner:** Project governance  
**Update trigger:** Any canonical file, ID status, supersession, phase or delivery change

## Current pointers

- Active programme: [Integrated Creative Roadmap](../docs/roadmaps/INTEGRATED-CREATIVE-ROADMAP.md)
- Active phase: PH-11 integration hardening. PH-03–PH-05 and PH-10 are complete at their documented scopes; PH-06–PH-09 are implementation-complete with phase-specific gates open. PH-12 and PH-13 have bounded implemented slices; PH-14–PH-15 remain proposed.
- Current delivery: [BR-20260924-02](DELIVERY_REPORT.md#br-20260924-02--exclusive-editor-window-state-hardening)
- Traceability owner: [TRACEABILITY.md](TRACEABILITY.md)
- Project settings: [PROJECT_SETTINGS.md](PROJECT_SETTINGS.md)

## Baseline observations

- `CMakeLists.txt` declares project version 1.13.1 and C++20.
- The source contains a platform-neutral `MandelbrotCore` static library, a Win32 executable target, core and path-isolation test executables, release scripts, an Inno Setup definition, historical release notes and historical verification reports.
- This distributed source checkout has no `.git` directory. Historical governance records name a prior `main` baseline commit `8f82119`, but no Git bundle is present in this checkout; commit, branch and clean-tree claims are therefore unproven here.
- Release-facing source version fields now agree at 1.13.1. CMake configuration and CTest enforce this across CMake, `app.manifest`, `resources.rc`, the release script and installer. VAL-002 passed in the recorded portable environment; built-executable metadata remains part of the native Windows gate.
- Historical 1.13.1 verification reports portable compiler and sanitizer passes, but states native MSVC was not rerun. BR-20260727-02 separately passed a GNU 14.2 warnings-as-errors build and CTest; sanitizers and native MSVC were not rerun.
- BR-20260919-01 removes continuously rendered desktop Live/Journey, retains decoded image/slideshow presentation, adds muted looping exported-MP4 playback and schema-11 migration, with native Release build, CTest 8/8 and an opt-in production MP4 start/stop route passed.
- BR-20260923-01 aligns the canonical mission, product boundary, roadmap, phase plan, animation/export ownership and maintenance rules with DEC-039: fractal rendering is preview/export-only and desktop presentation is file-backed.
- BR-20260923-02 removes inert Independent monitor assignments through schema-12 migration, makes asynchronous video and slideshow exhaustion stop visibly, removes obsolete renderer diagnostics and records MFPlay as legacy technical debt.
- The current project `build` records Visual Studio 18 2026 x64 Release configuration with tests and warnings-as-errors enabled. BR-20260909-03 adds a serial native process-scoped interaction fixture over the real Win32 application: Palette/Equation camera retention, automatic preview precision switching, structural history labels and Equation Undo/Redo pass with isolated application data and clean shutdown. BR-20260909-04 stabilises that fixture at the completed-dialog/committed-main-window boundaries and adds native built-in preset Load/Undo/Redo coverage. BR-20260909-05 adds Settings accept/Undo/Redo and fixes the reproduced stale main-window camera-control state after accepting project settings. BR-20260909-06 adds exact Journey accept/Undo/Redo through the real dialog. BR-20260910-01 adds a production-serialised preset submitted through the native Open dialog and exact Import/Undo/Redo camera and identity replay. BR-20260910-02 completes the automated PH-05 replacement matrix through the production Fractal Scout search and exact Apply/Undo/Redo camera replay with stable preset identity. BR-20260910-03 proves a completed Scout candidate selection can be closed without camera, identity or history mutation, completing PH-10 at its documented scope. BR-20260910-04/05 add native Timeline clock/candidate and Journey conversion interaction. BR-20260910-06 adds production WIC frame success, matching-manifest resume, refusal, visible cancellation and close-while-active cancellation with project/history isolation. BR-20260918-01 adds native Video Export typed-summary refresh, bounded validation/failure containment, opt-in verified MP4 success and visible owned-process cancellation through FFmpeg 8.1.1. The latest cumulative 8/8 Release CTest run is recorded in the current delivery. The preceding BR-20260909-02 workflow passed portable-package inspection, isolated startup/shutdown and installer production. This does not prove the remaining manual UI/desktop matrix, installer install/upgrade/uninstall, signing, hardware-GPU deep numerical correctness or PH-12+ open gates.

BR-20260923-03 adds bounded native static/slideshow/MP4 mute-loop/failure-detach, current-topology display handling, simulated desktop-host recovery, native tab/name checks, a 60-second static resource run and isolated installer install/reinstall/uninstall evidence. Windows 11, physical lifecycle changes, mixed DPI, full accessibility, preceding-release upgrade and long-duration soak remain open.

BR-20260924-02 adds a process-wide exclusive AppWindow edit-session gate. Direct MSVC compilation and a manually linked editor-only native fixture pass for Palette, Equation and Settings competing-route blocking and owner restoration. The full CMake/MSBuild build was retried but FileTracker access was denied before compile; no cumulative CTest or release claim is made for this batch.

## Canonical manifest

| Domain | Canonical owner | Read when |
|---|---|---|
| Identity, users, scope, requirements | [PROJECT_FOUNDATION.md](PROJECT_FOUNDATION.md) | Product or scope decisions |
| ID mapping and status | [TRACEABILITY.md](TRACEABILITY.md) | Any implementation or verification task |
| Phases and delivery sequence | [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) | Planning or executing work |
| Repository, versions and releases | [REPOSITORY_AND_VERSIONING.md](REPOSITORY_AND_VERSIONING.md) | Build, package, release or Git work |
| Current architecture and future state boundary | [PROJECT-STATE-AND-PARAMETERS.md](../docs/architecture/PROJECT-STATE-AND-PARAMETERS.md) | Model, state, persistence or mutation work |
| Rendering and output contracts | [RENDERING-AND-EXPORT-CONTRACTS.md](../docs/architecture/RENDERING-AND-EXPORT-CONTRACTS.md) | Renderer, precision, still or video output work |
| UI workflows and route IDs | [UI_WORKFLOWS_AND_ROUTES.md](UI_WORKFLOWS_AND_ROUTES.md) | Win32 UI or workflow changes |
| Data and migrations | [DATA_AND_PERSISTENCE.md](DATA_AND_PERSISTENCE.md) | Settings, presets, files or schema changes |
| Validation and evidence | [VALIDATION_AND_EVIDENCE.md](VALIDATION_AND_EVIDENCE.md) | Tests, claims, baselines or reports |
| Security, privacy and risk | [SECURITY_PRIVACY_AND_RISK.md](SECURITY_PRIVACY_AND_RISK.md) | Inputs, files, logging, desktop integration, commands or external tools |
| High-precision dependency package/licence | [HIGH_PRECISION_DEPENDENCY_REVIEW.md](HIGH_PRECISION_DEPENDENCY_REVIEW.md) | Precision dependency selection, source provenance, licence, packaging or upgrades |
| Decisions and change history | [DECISIONS_AND_CHANGE_HISTORY.md](DECISIONS_AND_CHANGE_HISTORY.md) | Cross-cutting or irreversible choices |
| Debugging and maintenance | [DEBUGGING_AND_MAINTENANCE.md](DEBUGGING_AND_MAINTENANCE.md) | Faults, support, upgrades or cleanup |
| Coding-assistant operating rules | [PROJECT_SETTINGS.md](PROJECT_SETTINGS.md) | Every coding-assistant session |
| Delivery evidence | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) | Handoff or audit |

## Detailed feature specifications

| Alias | Canonical owner |
|---|---|
| Creative programme / long-term roadmap | [INTEGRATED-CREATIVE-ROADMAP.md](../docs/roadmaps/INTEGRATED-CREATIVE-ROADMAP.md) |
| Undo history | [UNDO-REDO-PLAN.md](../docs/features/UNDO-REDO-PLAN.md) |
| General animation | [ANIMATION-TRACKS-PLAN.md](../docs/features/ANIMATION-TRACKS-PLAN.md) |
| Offline frames and video | [OFFLINE-EXPORT-PLAN.md](../docs/features/OFFLINE-EXPORT-PLAN.md) |
| Existing discovery integration | [FRACTAL-SCOUT-STATUS.md](../docs/features/FRACTAL-SCOUT-STATUS.md) |
| Visual baselines | [VISUAL-REGRESSION.md](../docs/testing/VISUAL-REGRESSION.md) |
| Native Windows release validation | [WINDOWS-RELEASE-VALIDATION.md](../docs/testing/WINDOWS-RELEASE-VALIDATION.md) |
| Deep-zoom upgrade supporting design (proposed) | [Mandelbrot-Deep-Zoom-Upgrade-Pack-1.13.1-v2-COMBINED.md](../Mandelbrot-Deep-Zoom-Upgrade-Pack-1.13.1-v2-COMBINED.md) |

## Historical and compatibility documents

- `docs/FEATURES-*.md`, `docs/VERIFICATION-*.md` and `docs/BUILD-FIXES-*.md` are historical evidence records. They do not own current contracts.
- `docs/PHASE-COMPLETION.md` and `docs/FRACTAL-STYLE-BUILD-PLAN.md` remain historical completion records.
- `docs/ARCHITECTURE.md`, `docs/TESTING.md`, `docs/PRIVACY-SECURITY.md` and `docs/WINDOWS-INTEGRATION.md` are concise compatibility entry points. The canonical owners above prevail on conflict.

## Supersession records

| Record | Superseded claim | Current authority |
|---|---|---|
| SUP-001 | Architecture text naming earlier settings schemas | `src/Core/Models.h` shows schema 12; see [DATA_AND_PERSISTENCE.md](DATA_AND_PERSISTENCE.md) |
| SUP-002 | Privacy text describing only a quadratic four-coefficient equation | Current `EquationSettings` supports a broader bounded data model; see [SECURITY_PRIVACY_AND_RISK.md](SECURITY_PRIVACY_AND_RISK.md) |
| SUP-003 | Roadmap treating Fractal Scout as unimplemented | Scout exists in 1.13.1; only integration gaps remain |
| SUP-004 | Any release-ready inference from portable checks alone | Native Windows/MSVC and runtime evidence are required |

## Delivery ledger

| ID | Date | Status | Scope | Report |
|---|---|---|---|---|
| BR-20260923-02 | 2026-09-23 | Implemented / native build, CTest 8/8 and source policy passed | Remove Independent monitor assignments; contain asynchronous video/slideshow failures; remove stale desktop-renderer diagnostics; record MFPlay debt | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260923-02--desktop-failure-containment-and-monitor-scope-removal) |
| BR-20260923-01 | 2026-09-23 | Governance aligned / source policy verified | Make the accepted file-backed desktop boundary authoritative across current scope, roadmap, phase, validation and maintenance owners | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260923-01--current-scope-governance-alignment) |
| BR-20260919-01 | 2026-09-19 | Implemented / partly verified | Remove continuously rendered desktop modes; retain file-backed images/slideshow; add exported-MP4 wallpaper and schema-11 migration | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260919-01--file-backed-desktop-modes) |
| BR-20260727-01 | 2026-07-27 | Proposed | Canonical Markdown foundation and governed-source package | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260727-02 | 2026-07-27 | Implemented / partly verified | Version correction, automated version gate and portable build/test evidence | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260727-03 | 2026-07-27 | Implemented / partly verified | Local Git baseline, canonical MSVC presets, isolated startup state and report-producing Windows validation workflow | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260727-04 | 2026-07-27 | Implemented / partly verified | Portable CPU production-renderer visual fixture foundation | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260727-05 | 2026-07-27 | Implemented / partly verified | PH-03 parameter descriptors, adapters and canonical render fingerprint foundation | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260728-01 | 2026-07-28 | Implemented / partly verified | PH-03 transactional coordinator and main-window palette/post scalar migration | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260728-02 | 2026-07-28 | Implemented / partly verified | PH-03 deliberate main-window camera mutation route | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260728-03 | 2026-07-28 | Implemented / partly verified | PH-03 built-in palette selection and mutation-origin metadata | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260728-04 | 2026-07-28 | Implemented / partly verified | PH-03 gesture coalescing and classified broad replacements | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260728-04--ph-03-gesture-and-broad-replacement-completion) |
| BR-20260729-17 | 2026-07-29 | Native compile/test evidence; runtime retest pending | PH-03 live dialog preview and candidate rollback | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-17--ph-03-live-dialog-preview-and-candidate-rollback) |
| BR-20260728-05 | 2026-07-28 | Implemented / partly verified | PH-04 bounded camera/palette undo and redo | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260728-06 | 2026-07-28 | Implemented / partly verified | PH-05 complete project undo and redo | [DELIVERY_REPORT.md](DELIVERY_REPORT.md) |
| BR-20260728-07 | 2026-07-28 | Implemented / partly verified | PH-06 deterministic general animation evaluator | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260728-07--ph-06-deterministic-general-animation-evaluator) |
| BR-20260729-01 | 2026-07-29 | Implemented / user-reported native build/run | PH-07 basic animation editor and Journey adapter | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-01--ph-07-basic-animation-editor-and-journey-adapter) |
| BR-20260729-02 | 2026-07-29 | Implemented / partly verified | PH-08 deterministic PNG frame-sequence export | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-02--ph-08-deterministic-png-frame-sequence-export) |
| BR-20260729-03 | 2026-07-29 | Implemented / source verified | PH-08 Visual Studio COM declaration build fix | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-03--ph-08-visual-studio-com-declaration-build-fix) |
| BR-20260729-04 | 2026-07-29 | Implemented / partly verified | PH-09 external FFmpeg H.264/MP4 export with fixed process invocation and verified cleanup boundary | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-04--ph-09-external-ffmpeg-h264mp4-export) |
| BR-20260729-05 | 2026-07-29 | Native Release build/test evidence recorded | Current project build provenance and CTest 4/4 evidence for PH-01–PH-09 compile integrity | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-05--current-native-release-build-evidence) |
| BR-20260729-06 | 2026-07-29 | PH-01 runtime smoke failed | Existing Release executable started and initialized D3D11, but canonical main-window discovery and clean Exit/shutdown evidence failed | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-06--ph-01-runtime-smoke-attempt) |
| BR-20260729-07 | 2026-07-29 | PH-01 isolated runtime smoke passed | Process-scoped window discovery corrected the verifier; existing Release app loaded with D3D11 and exited cleanly | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-07--ph-01-corrected-isolated-runtime-smoke) |
| BR-20260729-08 | 2026-07-29 | PH-01 package smoke and PH-02 native CPU fixtures passed | Fresh portable ZIP startup smoke plus native CPU production-renderer fixture suite | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-08--ph-01-portable-package-smoke-and-ph-02-native-cpu-fixtures) |
| BR-20260729-09 | 2026-07-29 | Fresh MSVC Release rebuild and CTest passed | Visual Studio 18 x64 compiled/linked the current wallpaper and CTest passed 4/4 | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-09--fresh-msvc-release-rebuild-and-ctest) |
| BR-20260729-10 | 2026-07-29 | D3D11 WARP fixture passed | Production WARP hidden-window render/readback exact-repeatability check | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-10--d3d11-warp-production-readback-fixture) |
| BR-20260729-11 | 2026-07-29 | WARP bloom and perturbation passed | Exact production WARP readback for standard, bloom and perturbation scenes | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-11--d3d11-warp-bloom-and-perturbation-expansion) |
| BR-20260729-12 | 2026-07-29 | Hardware D3D11 fixture passed | Adapter-specific production readback on AMD Radeon RX 7900 XT | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260729-12--hardware-d3d11-production-readback-fixture) |
| BR-20260730-01 | 2026-07-30 | Proposed programme integrated; structural/native build/test gates passed | Reconciled the live source verifier and integrated the supporting deep-zoom design as proposed PH-12–PH-15 governance without changing schemas, renderer code or active phase | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260730-01--deep-zoom-upgrade-pack-integration) |
| BR-20260730-02 | 2026-07-30 | Governance decisions accepted; no implementation | Recorded deep-zoom ownership, migration, export and dependency direction | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260730-02--deep-zoom-governance-decisions) |
| BR-20260730-03 | 2026-07-30 | Implemented; native Release build and CTest 6/6 passed | Pinned Boost.Multiprecision 1.83.0 package, BSL-1.0 review, independent reference backend and supply-chain integrity gates | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260730-03--high-precision-package-and-licence-review) |
| BR-20260730-04 | 2026-07-30 | Partial PH-10 implementation; native Release build/test passed | Deterministic Scout candidate identity and core regression | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260730-04--ph-10-scout-candidate-identity) |
| BR-20260731-01 | 2026-07-31 | Passed within bounded WARP transport scope | Bit-exact production float4 reference-orbit texture upload/staging-readback check; no precision-ceiling claim | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260731-01--d3d11-warp-reference-orbit-texture-transport) |
| BR-20260731-02 | 2026-07-31 | Passed within bounded selected-sample scope | Independent Boost-512 and current fixed-point float4 reference-orbit payload agreement for Mandelbrot and Tricorn; no precision-ceiling claim | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260731-02--independent-float4-reference-orbit-payload-comparison) |
| BR-20260731-03 | 2026-07-31 | Passed within bounded cache-enforcement scope | Reference-orbit cache eviction and over-budget refusal with no stale reuse/publication | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260731-03--reference-orbit-cache-boundary-enforcement) |
| BR-20260803-01 | 2026-08-03 | Passed within bounded fixture scope | Ordered float4 orbit-coordinate reconstruction measurements against Boost-512 producer; no validated GPU ceiling | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-01--fixture-specific-float4-orbit-reconstruction-measurement) |
| BR-20260803-02 | 2026-08-03 | Passed central-policy scope | Unvalidated GPU float4 transport is direct-correction-required, never a safe selectable deep backend | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-02--unvalidated-gpu-orbit-transport-planner-refusal) |
| BR-20260803-03 | 2026-08-03 | Implemented; core/native-build verified | Unrotated exact rational AA 1–4 for bounded Boost direct still/frame CPU route | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-03--exact-direct-cpu-anti-aliasing) |
| BR-20260803-04 | 2026-08-03 | Implemented; core verified | Exact frame export refuses unsupported rotation/AA before job or per-frame renderer callback | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-04--exact-frame-render-preflight-boundary) |
| BR-20260803-05 | 2026-08-03 | Implemented; core/GPU-fixture verified | Centralized D3D11/OpenGL legacy automatic precision policy; backends report capabilities only | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-05--central-gpu-compatibility-precision-policy) |
| BR-20260803-06 | 2026-08-03 | Implemented; core/native-build verified | Exact CPU routes fail closed for unsupported animated equation coefficients | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-06--exact-direct-coefficient-animation-refusal) |
| BR-20260803-07 | 2026-08-03 | Implemented; core verified | Bounded reference-orbit worker shutdown/join and queue-refusal boundary | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-07--reference-orbit-worker-shutdown-and-queue-boundary) |
| BR-20260803-08 | 2026-08-03 | Implemented; core verified | Reference-orbit immutable plan/encoding provenance and substitution refusal | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-08--reference-orbit-immutable-provenance-binding) |
| BR-20260803-09 | 2026-08-03 | Implemented; core verified | Coalesced reference-orbit work preserves each subscriber correction plan | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-09--reference-orbit-per-subscriber-plan-delivery) |
| BR-20260803-10 | 2026-08-03 | Implemented; core verified | Bounded retained subscriber callbacks for coalesced reference-orbit work | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-10--reference-orbit-subscriber-resource-bound) |
| BR-20260803-11 | 2026-08-03 | Implemented; core verified | Schema-3 exact-camera reload below legacy double scale range | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-11--ultra-deep-schema-3-exact-camera-reload) |
| BR-20260803-12 | 2026-08-03 | Implemented; core/native-build verified | Exact camera transaction, no binary reconstruction on exact UI commit | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-12--exact-camera-transaction-boundary) |
| BR-20260803-13 | 2026-08-03 | Implemented; core/native-build verified | Exact deep-camera undo/redo structural replay | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-13--exact-camera-history-replay) |
| BR-20260803-14 | 2026-08-03 | Implemented; core/native-build verified | Immutable exact planner tier delivered to the frame renderer | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-14--immutable-exact-frame-plan-hand-off) |
| BR-20260803-15 | 2026-08-03 | Implemented; core/native-build verified | Exact direct still/frame row-memory admission bound | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-15--exact-direct-row-memory-admission) |
| BR-20260803-16 | 2026-08-03 | Implemented; core/native-build verified | Explicit stable/rebased/unresolved perturbation sample classification | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-16--explicit-perturbation-sample-validity) |
| BR-20260803-17 | 2026-08-03 | Implemented; core/native-build verified | Exact direct CPU global tile-origin mapping | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-17--exact-direct-global-tile-mapping) |
| BR-20260803-18 | 2026-08-03 | Implemented; core/native-build verified | Deep exact cancellation before partial row publication | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-18--deep-exact-cancellation-before-row-publication) |
| BR-20260803-19 | 2026-08-03 | Implemented; core/native-build verified | Immutable frame export rejects unimplemented renderer IDs with no CPU fallback | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-19--frame-renderer-route-refusal) |
| BR-20260803-20 | 2026-08-03 | Implemented; native-build/test verified | High-resolution exact CPU route reports its actual evaluator | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-20--high-resolution-exact-renderer-status) |
| BR-20260803-21 | 2026-08-03 | Implemented; core/native-build verified | Double-keyframe camera animation refuses lossy exact authority | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-21--deep-camera-animation-refusal) |
| BR-20260803-22 | 2026-08-03 | Implemented; core/native-build verified | Double-coordinate Scout refuses lossy exact sources | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-22--deep-scout-source-refusal) |
| BR-20260803-23 | 2026-08-03 | Implemented; native full-suite verified | Boost 16,384-bit direct exact still/frame tier | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260803-23--boost-16384-bit-direct-exact-tier) |
| BR-20260804-01 | 2026-08-04 | Implemented; native full-suite verified | Planner-matched 512–16,384-bit exact reference-orbit service | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260804-01--planner-matched-high-precision-reference-orbits) |
| BR-20260805-01 | 2026-08-05 | Implemented; native full-suite and bounded Win32 runtime verified | Windows UX flow, action-safety, exact-copy, dialog layout and shutdown regressions | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260805-01--windows-ux-flow-and-action-regression-correction) |
| BR-20260907-01 | 2026-09-07 | Implemented; native full-suite and production renderer fixtures verified; interactive journey pending | Modeless Palette/Equation camera retention and automatic preview precision recovery | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260907-01--modeless-camera-retention-and-preview-precision-recovery) |
| BR-20260908-01 | 2026-09-08 | Partial PH-10 implementation; native production-thumbnail fixture passed; dialog interaction pending | Deterministic production Scout thumbnail fixtures and artifacts | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260908-01--ph-10-production-scout-thumbnail-fixtures) |
| BR-20260908-02 | 2026-09-08 | PH-11 automated Windows workflow passed; manual matrix pending | Process-scoped release validation, clean native build/package and isolated startup/shutdown evidence | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260908-02--ph-11-process-scoped-windows-release-validation) |
| BR-20260909-01 | 2026-09-09 | PH-09 real success, owned-process cancellation, integrated export cancellation and encoder-failure containment passed for FFmpeg 8.1.1; dialog/broader builds pending | Opt-in production PNG-to-H.264/MP4 fixture; evidence at `artifacts/p9-v7/` | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260909-01--ph-09-real-ffmpeg-production-path-fixture) |
| BR-20260909-02 | 2026-09-09 | Automated native release and installer-production checks passed; manual install/upgrade/uninstall and broader matrix pending | Per-user/PATH Inno Setup discovery plus version/hash/signature-aware installer artifact validation | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260909-02--ph-01-native-installer-artifact-validation) |
| BR-20260909-03 | 2026-09-09 | Native Win32 interaction fixture passed; broader manual matrix pending | Native Palette/Equation camera-retention, automatic preview precision and structural history/Equation Undo/Redo evidence | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260909-03--native-paletteequation-camera-retention-precision-and-history-interaction) |
| BR-20260909-04 | 2026-09-09 | Native preset Load/Undo/Redo interaction passed; remaining PH-05 routes pending | Dialog-ready/commit-complete fixture synchronisation plus exact preset replacement replay | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260909-04--native-preset-load-history-interaction) |
| BR-20260909-05 | 2026-09-09 | Native Settings accept/Undo/Redo interaction passed; remaining PH-05 routes pending | Main-window camera-control synchronisation after Settings commit plus structural replay | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260909-05--native-settings-history-and-main-window-synchronisation) |
| BR-20260909-06 | 2026-09-09 | Native Journey accept/Undo/Redo interaction passed; import and Scout Apply remain | Exact structured-route acceptance and structural replay through the production dialog | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260909-06--native-journey-history-interaction) |
| BR-20260910-01 | 2026-09-10 | Native preset Import/Undo/Redo interaction passed; Scout Apply remains | Production-serialised preset selection through the native Open dialog and exact replacement replay | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260910-01--native-preset-import-history-interaction) |
| BR-20260910-02 | 2026-09-10 | Automated native PH-05 replacement interaction matrix passed | Production Scout search, Use in Preview and exact camera-only Apply/Undo/Redo replay | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260910-02--native-fractal-scout-history-interaction) |
| BR-20260910-03 | 2026-09-10 | PH-10 complete at documented bounded/native scope | Native Scout candidate selection and Close isolation plus retained Apply replay evidence | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260910-03--native-fractal-scout-isolation-and-ph-10-completion) |
| BR-20260910-04 | 2026-09-10 | Native Timeline clock and candidate-state interaction passed; visual playback and Journey conversion interaction remain | Production add/current-value, scrub/play/stop, Cancel rollback and OK/reopen runtime-state evidence | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260910-04--native-animation-timeline-clock-and-candidate-interaction) |
| BR-20260910-05 | 2026-09-10 | Native Journey/Timeline conversion preparation passed; visual playback and DPI/keyboard remain | Production Journey-to-Tracks, Tracks-to-Journey confirmation and Cancel isolation | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260910-05--native-journey-and-timeline-conversion-interaction) |
| BR-20260910-06 | 2026-09-10 | Native WIC frame success, matching resume, refusal, visible cancellation and close-while-active cancellation passed; broader UI/lifecycle gates remain | Production Frame Sequence Export Start/Resume/refusal/Cancel/close-active, manifest/output inspection and project isolation | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260910-06--native-frame-sequence-export-and-cancellation-interaction) |
| BR-20260918-01 | 2026-09-18 | Native Video Export typed-summary, validation, missing-executable containment, verified MP4 success and visible cancellation passed | Production Video Export summary/validation/refusal, opt-in FFmpeg 8.1.1 encode/decode/promotion/cancellation and project isolation | [DELIVERY_REPORT.md](DELIVERY_REPORT.md#br-20260918-01--native-video-export-validation-interaction) |
| BR-20260923-01 | 2026-09-23 | Current scope aligned | Preview/export authoring and file-backed desktop boundary | [Delivery](DELIVERY_REPORT.md#br-20260923-01--current-scope-governance-alignment) |
| BR-20260923-02 | 2026-09-23 | Implemented; native checks passed | Failure containment and monitor-scope removal | [Delivery](DELIVERY_REPORT.md#br-20260923-02--desktop-failure-containment-and-monitor-scope-removal) |
| BR-20260923-03 | 2026-09-23 | Bounded native hardening passed; PH-11 remains open | Desktop media, readiness, resource and isolated installer evidence | [Delivery](DELIVERY_REPORT.md#br-20260923-03--ph-11-desktop-and-installer-hardening) |
| BR-20260924-01 | 2026-09-24 | Bounded modal pause matrix and native CTest 8/8 passed | Preserve file-backed desktop pause through export dialogs and synthetic lifecycle events | [Delivery](DELIVERY_REPORT.md#br-20260924-01--file-backed-desktop-export-pause-hardening) |
| BR-20260924-02 | 2026-09-24 | Exclusive editor-window gate passed in direct MSVC compile and editor-only native fixture; full CMake build blocked by FileTracker access | One AppWindow edit session disables main/Quick Controller and blocks competing Palette/Equation/Settings routes | [Delivery](DELIVERY_REPORT.md#br-20260924-02--exclusive-editor-window-state-hardening) |
