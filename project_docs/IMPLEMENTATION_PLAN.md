# Implementation Plan

**Status:** Current delivery plan; PH-11 active, bounded PH-12/PH-13 slices implemented, PH-14/PH-15 proposed  
**Purpose:** Own phase order, prerequisites, tasks, outputs, rollback and stop conditions
**Owner:** Delivery lead
**Reading trigger:** Before any durable implementation
**Update trigger:** Phase status, dependency, acceptance or sequencing change

## Programme rule

Every durable change begins with PH-00. A phase completes only when its stated evidence passes; source presence or token checks alone are insufficient.

DEC-039 is a cross-phase product boundary: preview and explicit still/frame/video jobs may render or evaluate animation; desktop integration may only present saved images, image slideshows or already-exported local video. No phase task or future deep-rendering item authorises a renderer-backed desktop mode.

## PH-00 — Repository and Governance Baseline

**Status:** Historical repository-execution baseline passed. This archive has no usable `.git` or supplied Git bundle; current branch, commit and clean-tree status are unproven.
**Objective:** Establish the project root, local repository state and canonical governance before code changes.
**Prerequisites:** User accepts this pack or an amended equivalent.
**Required reading:** `AGENTS.md`, `PROJECT_INDEX.md`, `REPOSITORY_AND_VERSIONING.md`.
**Expected state:** A local repository root containing `CMakeLists.txt`, clean baseline commit, canonical docs and no unrequested remote.

**Tasks**
1. Confirm the extracted project root.
2. Inspect Git status, branch, remotes, ignored files and tracked archive artifacts.
3. Initialise Git only when explicitly operating in repository-execution mode and appropriate.
4. Add or reconcile this governance pack through managed migration.
5. Record baseline hashes or a file inventory for the imported source.
6. **Implemented 2026-07-27:** align `resources.rc` to 1.13.1, enforce cross-file consistency during configure/CTest and verify executable metadata in the Windows release script.
7. **Implemented 2026-07-27:** initialise the confirmed root on branch `main`, include the imported historical records, add a bounded `.gitignore`, create baseline commit `8f82119`, and add no remote.
8. Review staged files before any commit; do not add a remote unless requested.

**Proposed files:** `/AGENTS.md`, `/project_docs/**`, canonical feature specifications, optional `.gitignore`.
**Acceptance:** AC-002, AC-019, AC-020.
**Validation:** VAL-001, VAL-002.
**Governance update:** Delivery ledger and DEC-011/DEC-012.
**Report:** Baseline report with inspected Git state and version scan.
**Rollback point:** Imported source before governance migration.
**Stop conditions:** Unknown root, unreviewed destructive cleanup, conflicting version authority or user rejects governance paths.

## PH-01 — Windows Release Stabilisation

**Status:** In progress; the 2026-09-09 process-scoped automated workflow passed a clean Visual Studio 18 2026 x64 Release build, CTest 8/8 including the serial Palette/Equation camera-retention/automatic-preview-precision/history fixture, executable metadata, portable-ZIP inspection, isolated startup/window/D3D11/clean-shutdown smoke and Inno Setup 6.7.3 installer production on Windows 10 build 19045. BR-20260923-03 adds isolated install/launch/same-version reinstall/uninstall; preceding-release upgrade, normal-AppId installer behavior and the manual cross-version matrix remain required.
**Objective:** Prove the current baseline on the intended native Windows release toolchain before architecture expansion.
**Prerequisites:** PH-00; Windows 10/11 x64 environment; Visual Studio 2022 C++ workload; accepted version.
**Required reading:** repository/versioning, Windows integration, validation, risk register.
**Expected state:** Release executable builds, starts, renders and packages under controlled evidence.

**Tasks:** configure x64 Release with warnings as errors; compile resources; link D3D11/DXGI/OpenGL/WIC/Win32; run CTest; start/exit; exercise preview, file-backed desktop modes, required dialogs, Scout, still capture and tiled export; prove no desktop route enters fractal rendering; package portable ZIP; build installer when Inno Setup is available; test upgrade from preceding release. **Implemented preparation:** canonical native MSVC presets, path-isolated test state through `MW_APPDATA_DIR`, automated package and installer artifact inspection, machine-wide/per-user/PATH Inno Setup discovery, isolated startup/shutdown smoke, serial Palette/Equation camera-retention/automatic-preview-precision/history interaction fixture, JSON/Markdown/log reporting, and a manual Windows runtime matrix.
**Proposed files:** only narrow fixes and proportional tests/reports.
**Acceptance:** AC-002–AC-006.
**Validation:** VAL-002–VAL-008.
**Governance update:** release evidence, known limitations and version ledger.
**Rollback point:** PH-00 baseline commit.
**Stop conditions:** native build failure, inconsistent versions, data-loss symptom, unexplained GPU failure or missing runtime environment.

## PH-02 — Visual Regression Foundation

**Status:** Complete. CPU baselines are user-approved with strict recorded thresholds; native CPU, D3D11 WARP, hardware D3D11 and OpenGL production readback fixtures are implemented and evidenced.
**Objective:** Add a small fixture harness that invokes production renderers without changing visible application behaviour.
**Prerequisites:** PH-01 passed under the original gate. DEC-016 authorised the bounded portable implementation while formal PH-01 evidence remained open. DEC-017 now records that pending PH-02 verification is non-blocking for later implementation, without marking PH-02 complete.
**Required reading:** visual regression and rendering contracts.
**Expected state:** Deterministic fixture command, baseline approval workflow and diagnostic artifacts.

**Implemented tasks:** `MandelbrotVisualFixtures`; standard Mandelbrot, Tricorn Cyan Fire Ring, rotated bloom-state and deep perturbation-state fixtures; same-process exact repeatability; full/tiled and seam-strip comparison; environment/state/metric artifacts; explicit baseline comparison inputs; deliberate palette/depth mutation; shell, PowerShell and CMD runners; CTest integration.
**Remaining boundary:** documented approved CPU and environment-specific native fixtures complete the PH-02 scope; new hardware/drivers and deep numerical claims require separate validation.
**Files:** `src/Core/VisualRegression.*`, `src/Tools/VisualFixtureTool.cpp`, fixture scripts, CMake integration and visual-regression documentation.
**Acceptance:** AC-007–AC-009 are evidenced at the documented PH-02 scope, including approved CPU baselines and native backend fixtures; no general cross-backend pixel-equivalence claim.
**Validation:** VAL-009–VAL-013 have the bounded CPU, WARP/hardware and OpenGL evidence recorded in the visual-regression owner and BR-20260729-08/10/11/12/13/14.
**Rollback point:** PH-01 checkpoint.
**Stop conditions:** fixture uses a duplicate renderer, nondeterminism is unexplained, thresholds are guessed globally or baselines lack review.


## PH-03 — Parameter Adapter and Render Fingerprint

**Status:** Complete at the documented PH-03 scope; native Release compile/link, CTest and Windows interaction evidence are recorded.
**Objective:** Add immutable captures and stable parameter identity over existing authoritative models.
**Prerequisites:** PH-02 stable under the original gate; DEC-017 authorises implementation to continue while PH-02 verification remains explicitly pending.
**Required reading:** state/parameters, data persistence and rendering contracts.
**Expected state:** Camera/palette adapters, canonical serializer and fingerprint; no second authority.

**Implemented tasks:** bounded 17-key descriptor registry; explicit unknown-key failure; camera and palette/post snapshots; exact compensated-coordinate preservation; `mw-render-state-v1` canonical serialisation and SHA-256 fingerprint; transactional model-normalised scalar/camera/palette mutations; explicit mutation origin and history eligibility; shared preview-navigation coalescing with a 500 ms inactivity boundary across pan and wheel input; classified transactional preset load/import and Palette/Equation/Settings/Journey replacement boundaries; candidate-copy dialog editing; direct inspected whole-preset/camera writes removed; classified broad replacements prepared for the PH-05 atomic history layer.
**Acceptance:** AC-010 and AC-011 pass for the inspected PH-03 implementation scope. AC-009 remains bounded by PH-02 evidence scope.
**Validation:** VAL-014 and VAL-016 pass in the recorded portable/unit/structural scope, including gesture token policy, replacement rollback/no-op behaviour and origin classification. Native MSVC compilation/core-test execution and user-confirmed Windows interaction now cover navigation coalescing plus Palette, Equation, Settings and Journey live preview/Cancel rollback.
**Rollback point:** BR-20260728-03 palette/origin checkpoint.
**Stop conditions:** second mutable project authority, persistence-format change, broad operations represented as incomplete scalar history, or unclassified direct project replacements.

## PH-04 — Camera and Palette Undo/Redo

**Status:** Complete at the documented PH-04 scope; native Release compile/link, CTest and Windows camera/palette undo interaction evidence are recorded.
**Objective:** Deliver the first bounded history slice.
**Prerequisites:** PH-03 adapters.
**Required reading:** undo plan and UI routes.
**Expected state:** Transaction API, bounded history, UI commands and coalescing for camera/palette edits.

**Implemented tasks:** typed camera/palette deltas; atomic replay; camera metadata capture; pan/wheel/palette-thumb coalescing; dedicated rotation edit; invalidation aggregation; labelled Undo/Redo buttons and Ctrl+Z/Ctrl+Y; redo-branch truncation; configurable entry/estimated-memory bounds; runtime exclusion. PH-05 supersedes the temporary broad-replacement clearing boundary with atomic structural entries.
**Acceptance:** AC-012 and AC-013 pass for the bounded PH-04 implementation scope and remain covered by PH-05.
**Validation:** VAL-019, VAL-020 and VAL-022 pass in portable/unit/structural scope. User-confirmed Windows interaction covers camera and palette undo/redo plus redo-branch truncation.
**Rollback point:** PH-03 checkpoint.
**Stop conditions:** runtime events enter history, undo mutates user settings, or coalescing loses user-visible steps.

## PH-05 — Complete Project Undo/Redo

**Status:** Implementation complete; current native Release compile/link and CTest evidence passed. The process-scoped Win32 fixture now passes Palette/Equation accepted-replacement history, Equation Undo/Redo, built-in preset Load/Undo/Redo, Settings accept/Undo/Redo, Journey accept/Undo/Redo, preset Import/Undo/Redo and Fractal Scout Apply/Undo/Redo.
**Objective:** Cover structural project edits without forcing them into scalar deltas.
**Prerequisites:** PH-04 accepted.
**Implemented tasks:** equation and post-processing changes; palette-stop insertion/removal/reorder/replacement; preset and imported-candidate application; journey row/text replacement; accepted Palette/Equation/Settings/Journey dialog transactions; atomic full-preset before/after entries; automatic scalar-completeness proof with structural fallback; full-render invalidation; one-entry Scout Apply coverage; startup-history suppression; bounded dynamic snapshot accounting.
**Acceptance:** AC-012 and AC-013 pass in the portable complete-history scope. AC-018 passes through core exact replay and native production Scout search/Apply/Undo/Redo interaction.
**Validation:** VAL-019–VAL-022 and VAL-036 pass in portable/unit/structural scope. BR-20260909-03 adds native Palette/Equation structural-history and Equation Undo/Redo interaction evidence; BR-20260909-04 adds built-in preset Load/Undo/Redo with exact before/after camera and selected-identity checks; BR-20260909-05 adds Settings accept/Undo/Redo and main-window camera-control synchronisation; BR-20260909-06 adds exact Journey text acceptance and replay; BR-20260910-01 adds production-serialised preset Import/Undo/Redo through the native Open dialog; BR-20260910-02 completes the automated native matrix with production Scout search and exact camera-only Apply/Undo/Redo replay. BR-20260924-02 adds an exclusive AppWindow editor-session gate for Palette, Equation and Settings; the editor-only fixture proves competing routes are blocked until close.
**Rollback point:** PH-04 checkpoint.
**Stop conditions:** unstable object identity, partial broad-operation history or unmigrated direct mutation in a claimed domain.

## PH-06 — Deterministic General Animation Evaluator

**Status:** Implementation complete; current native MSVC Release compile/link and CTest evidence passed. BR-20260910-04 adds native preview-clock scrub/play/stop and reset interaction; rendered visual playback remains pending.
**Objective:** Implement timeline data and evaluation before a complex editor.
**Prerequisites:** PH-03; portable visual-fixture foundation, with pending PH-02 gates non-blocking under DEC-017.
**Implemented tasks:** bounded caller-supplied stable timeline/track/keyframe IDs; stable-name target registry; typed and range-validated targets; immutable `Preset` base snapshot; compensated camera-centre interpolation; logarithmic camera-scale interpolation; shortest-path rotation; Step/Linear/Smoothstep modes; deterministic track-ID ordering; duplicate enabled target and duplicate time rejection; disabled unknown-target retention; Clamp/Loop/PingPong time resolution; independent clock storage; invalidation aggregation; no persistence or per-frame undo. Preview and export are the supported active domains; the legacy wallpaper clock is retained only as inert compatibility/test state and does not drive desktop presentation.
**Acceptance:** AC-014 passes in the recorded portable core scope.
**Validation:** VAL-023–VAL-025 pass in portable unit tests. BR-20260910-04 adds bounded native preview-clock interaction and project/history-isolation evidence. VAL-026 passes in portable core tests and BR-20260910-05 native Journey/Timeline conversion interaction.
**Rollback point:** PH-05 compatible checkpoint; remove `src/Core/GeneralAnimation.*`, its CMake/tests and documentation entries. No persisted migration is required.
**Stop conditions:** frame evaluation mutates base project, reduces deep coordinates to ordinary doubles, enters undo per frame, accepts duplicate enabled Replace targets, or changes persistence before DEC-014 is resolved.

## PH-07 — Basic Animation Editor and Journey Adapter

**Status:** Implementation complete; current native MSVC Release compile/link and CTest evidence passed. Native editor state/clock and Journey/Timeline conversion interaction pass; DPI/keyboard and rendered visual playback remain pending.
**Objective:** Add a bounded editor while preserving the simpler Journey workflow.
**Implemented tasks:** modal candidate editor; track list and enabled state; registry target picker; add/remove tracks; keyframe table; Add Current Value from the authoritative `Preset`; selected keyframe time/interpolation update; duration and Clamp/Loop/PingPong mode; scrubber and Play/Pause/Stop; validation/conflict display; strict Journey-to-camera-track conversion; loss-aware camera-only reverse conversion; AppWindow preview evaluation through the PH-06 preview clock; accepted reverse conversion through the existing atomic Journey replacement/history path.
**Acceptance:** AC-014 passes in the recorded portable/core/structural scope; existing Journey text remains the only persisted route format and no schema changed.
**Validation:** VAL-023–VAL-026 portable/core tests and source-audit structure pass. BR-20260910-04 adds native dialog construction, candidate rollback, preview-clock controls and runtime-only OK/reopen evidence; BR-20260910-05 adds native lossless Journey-to-Tracks and Tracks-to-Journey preparation with Cancel isolation. DPI/keyboard smoke and rendered visual playback remain pending.
**Rollback point:** PH-06 evaluator archive.
**Stop conditions:** conversion silently drops unsupported data, timeline persistence changes before DEC-014, editor introduces a second playback clock authority, or candidate preview mutates `workingPreset_`.

## PH-08 — Deterministic Frame-Sequence Export

**Status:** Implementation complete; current native MSVC Release compile/link and CTest evidence passed. Bounded native WIC success, matching-manifest resume, mismatch/untracked refusal, visible cancellation and close-while-active cancellation interaction pass; Bounded modal file-backed desktop pause/restoration passes in BR-20260924-01; Open Folder, DPI/keyboard and desktop pause during active output jobs remain pending.
**Objective:** Reuse production still/tiled rendering for exact offline frames.
**Prerequisites:** PH-06 evaluator.
**Implemented tasks:** immutable normalised project/timeline/settings job; SHA-256 job/project/timeline identity; rational direct-index frame timing; bounded PNG naming/count; production CPU tiled rendering with pinned quality; reusable scanline WIC encoder; temporary frame, WIC dimension validation, digest receipt and promotion; atomic manifest update/recovery; exact typed/fingerprint resume validation; cancellation/progress; deterministic selected-frame tests; modal export UI; file-backed desktop presentation pause/resume policy. Desktop presentation owns no fractal GPU renderer to release.
**Acceptance:** AC-015 and AC-016 pass in the recorded portable/core/structural scope.
**Validation:** VAL-027–VAL-030 pass in portable tests and source audit. BR-20260910-06 adds native production-dialog evidence for one WIC PNG plus manifest, exact matching-manifest resume, mismatched-manifest and untracked-final refusal, early Cancel and close-while-active worker joins, no partial file, owner restoration and project/history isolation. Bounded modal file-backed desktop pause/restoration passes in BR-20260924-01; Open Folder, DPI/keyboard and desktop pause during active output jobs remain pending.
**Rollback point:** PH-07 archive; remove `FrameSequenceExport.*`, the export dialog/WIC row-encoder extraction, AppWindow route and PH-08 documentation entries. No persisted settings migration is required.
**Stop conditions:** final files can be partial, resume ignores any job/typed manifest mismatch, cancellation deletes verified frames, output overwrites untracked finals, adaptive quality remains enabled, or a detached worker can access destroyed UI/state.

## PH-09 — External FFmpeg Encoding

**Status:** Implementation complete; current native MSVC Release compile/link and CTest evidence passed; real FFmpeg 8.1.1 capability/encode/decode, owned-process cancellation, integrated export cancellation and encoder-failure containment fixture passed. Bounded Win32 summary/validation/missing-executable, verified MP4 success and visible cancellation interaction also pass.
**Objective:** Encode verified frame sequences without shell injection or source-frame loss.
**Prerequisites:** PH-08 frame sequence and accepted DEC-009/DEC-027 boundary.
**Implemented tasks:** explicit executable selection plus `PATH` discovery; version, `libx264` and MP4 muxer capability probes; complete digest-bound PH-08 sequence validation; even-dimension preflight for fixed yuv420p output; fixed H.264/MP4 argument vectors; exact `CreateProcessW` application launch without a shell; bounded stdout/stderr capture; progress parsing; owned cancellation; adjacent temporary MP4; post-encode decode probe; atomic promotion; failure/cancellation frame preservation; optional manifest-scoped frame cleanup only after verified success; modal UI route with file-backed desktop pause/resume policy.
**Acceptance:** AC-017 passes in the recorded portable/core/structural scope.
**Validation:** VAL-031–VAL-033 pass in core tests and source audit. The opt-in production-path fixture passed capability probing, PNG generation, H.264/MP4 encoding, decode probing, atomic promotion, source preservation, bounded owned-process cancellation, full `RunExternalVideoExport` cancellation/cleanup and exact-argument encoder-failure containment with FFmpeg 8.1.1. Core regression rejects odd source dimensions before launch. BR-20260918-01 adds native typed-sequence summary refresh, missing-input/CRF validation, missing-executable containment without MP4 output, two opt-in verified MP4 successes and two visible Cancel runs that revalidate the complete source sequence and leave no output. Other FFmpeg builds remain pending.
**Rollback point:** PH-08 verified frame-sequence workflow; remove `ExternalVideoExport.*`, `ExternalProcess.*`, `VideoExportDialog.*`, the AppWindow route and PH-09 documentation entries. No settings migration is required.
**Stop conditions:** arbitrary command templates, shell interpolation, bundled/downloaded encoder, persisted executable path, unbounded logs, promotion before decode verification, source-frame deletion on failure/cancellation, or cleanup outside manifest-tracked files.

## PH-10 — Existing Scout Integration

**Status:** Complete at the documented bounded/native scope. Deterministic candidate identity, production thumbnails, dialog close-without-mutation isolation and one-transaction Apply/Undo/Redo pass; optional extensions and deep exact execution remain separately gated.
**Objective:** Connect existing Scout to shared contracts without replacing its deterministic engine.
**Prerequisites:** PH-03 and PH-05.
**Tasks:** candidate fingerprints (implemented); snapshot adapter (implemented); preview isolation check (native interaction passed); one-transaction Apply (native interaction passed); visual thumbnail fixtures (implemented and native-tested); optional style/session extensions separately gated.
**Acceptance:** AC-018 passes at the documented bounded/native scope.
**Validation:** VAL-034–VAL-036 pass at their documented scopes; BR-20260910-03 supplies the native VAL-035/VAL-036 interaction boundary.
**Rollback point:** Existing Scout behaviour.
**Stop conditions:** candidate preview mutates active state, ordering changes without accepted reason or resource ceilings regress.

## PH-11 — Integration Hardening

**Status:** In progress. BR-20260923-03 passes bounded native static/slideshow/MP4 mute-loop and failure-stop/detach checks, current-topology display-message handling, simulated host reattachment, native tab/name checks, a 60-second static resource run, and isolated install/launch/same-version reinstall/uninstall. Windows 11, physical lifecycle, mixed DPI, full accessibility, preceding-release upgrade and long-duration soak remain open.
**Objective:** Complete release evidence across Windows, GPU, display, lifecycle, migration and long-running jobs.
**Tasks:** Windows 10/11; integrated/dedicated GPU for preview/export; D3D11/OpenGL; file-backed static/slideshow/video desktop matrix; Mirror/Span multi-monitor and mixed DPI; Explorer restart; sleep/resume; device loss; soak; installer upgrade; settings migration; resource profiling; documentation; verify asynchronous media and slideshow failures stop visibly and no desktop failure or lifecycle route enters fractal rendering.
**Acceptance:** AC-003–AC-020 as affected.
**Validation:** Full applicable VAL set.
**Rollback point:** Last accepted feature checkpoint.
**Stop conditions:** unexplained visual change, leak, data loss, version conflict or unsupported release claim.

## Deep-zoom programme entry rule

PH-12/PH-13 have the bounded implemented slices listed below; PH-14/PH-15 remain proposed. These do not change the active phase, PH-11. Before PH-12 implementation, retain the green `scripts/verify-source.py` baseline established by BR-20260730-01, capture ordinary/current-limit fixtures, and record repository/source provenance. PH-12 may proceed before PH-10/PH-11; it incorporates only the camera/fingerprint and mutation-boundary dependencies it directly affects, without claiming either predecessor phase complete. This checkout has no `.git`, so a commit or clean-tree rollback point cannot be claimed.

DEC-029 now accepts the pinned Boost.Multiprecision 1.83.0 standalone source package and BSL-1.0 packaging policy. Do not begin exact-camera schema or renderer mutation until the remaining PH-12 migration, parser, numerical, cancellation and resource gates are satisfied. DEC-015 split ownership, settings schema 12/preset schema 3 and forward-only migration policy are accepted.

## PH-12 — Baseline Repair and Exact Camera Foundation

**Status:** Exact decimal/camera types, schema-3 preset persistence including reload below legacy-double scale range, original-preserving settings migration through schema 12, main-window/high-resolution exact entry and transaction boundary with verified exact structural undo/redo fallback, v2 render/export/Scout identity, exact-aware Scout and animation snapshots, symbolic exact global-pixel mapping, and immutable per-frame exact planner hand-off are implemented. Current CPU/GPU execution remains an explicit legacy-adapter route for preview/export; deep numerical execution and bounded planner policy remain later work.
**Objective:** Establish exact durable camera authority while current renderers continue through an explicit compatibility adapter.
**Prerequisites:** Green structural/native baseline on one source state; accepted exact syntax and resource bounds; DEC-029 package review accepted with production-route numerical/cancellation evidence still required; settings schema 12/preset schema 3 and forward-only migration authority.
**Required reading:** foundation, state/parameters, persistence, decisions, risk, visual fixtures and the supporting deep-zoom pack.
**Expected state:** Exact centre and half-height are one authoritative project representation; legacy values are derived with explicit loss reporting; `mw-render-state-v1` remains recognisable and a separately versioned exact identity exists.

**Tasks**
1. Inventory every camera parse, format, mutation, persistence, fingerprint, Journey, timeline, Scout and export path.
2. Define bounded exact decimal syntax and canonicalisation.
3. Implement platform-neutral exact number/camera types and one-way legacy adapters.
4. Update coordinate edit/copy, preset Save As and immutable render-job capture without adding a second authority.
5. Maintain original-preserving one-way settings/preset migration to schema 12/preset schema 3; do not implement an old-format downgrade writer.
6. Version render fingerprints/manifests; never reinterpret v1 bytes as the new form.
7. Add parser, round-trip, migration, fingerprint, preset and adapter tests.
8. Retain current production renderers behind the compatibility adapter.

**Proposed files:** `src/Core/Precision/ExactDecimal.*`, `ExactCamera.*`, `CameraAdapter.*`, affected model/state/settings/Journey/timeline/Scout/UI files and core tests.
**Acceptance:** AC-021–AC-027.
**Validation:** VAL-037–VAL-044.
**Governance update:** Exact-camera decision, accepted schema/fingerprint versions, migration and evidence owners.
**Report:** Exact syntax/bounds, source-state provenance, migration/rollback results, precision-loss cases and native evidence.
**Rollback point:** Original schema-9/schema-2 files plus the pre-PH-12 source archive; feature-disable exact execution without deleting exact data.
**Stop conditions:** Input passes through `double` before canonicalisation; exact/approximate values are both mutable; migration can overwrite the only valid original; fingerprint omits exact camera; allocation is unbounded; dependency licence/package authority is absent.

## PH-13 — Central Precision Planner and Orbit Service

**Status:** Planner-owned exact CPU tiers, planner-owned legacy GPU compatibility selection, bounded planner-matched 512/2,048/8,192/16,384-bit reference service/worker coalescing/shutdown with capped subscribers and immutable per-subscriber plan/encoding identity, and bounded direct CPU execution at those tiers are implemented; measured encoding limits and validated production perturbation rendering remain proposed.
**Objective:** Remove backend-local precision policy and synchronous orbit ownership while preserving accepted ordinary/current-limit output.
**Prerequisites:** PH-12 passed; exact fixtures stable; DEC-029 accepted; current D3D11/OpenGL capabilities and thresholds inventoried.
**Required reading:** rendering/export contracts, exact state owner, decisions, risk, validation and both production backends.
**Expected state:** One deterministic planner emits a versioned plan; immutable orbit requests execute in bounded cancellable workers; cache/upload results are generation-fenced.

**Tasks**
1. Define versioned analytic quadratic and exact power-2 Tricorn capability fingerprints.
2. Implement precision intent, plan and central deterministic planner.
3. Replace D3D11/OpenGL local thresholds/fallback selection with plan consumption.
4. Extract reference generation behind immutable request/result contracts.
5. Add bounded workers, coalescing, byte-accounted cache, cancellation and stale-result rejection.
6. Name/version the current four-float orbit encoding and measure its supported ceiling.
7. Capture planner reason, precision stages, capability and generation in fingerprints/diagnostics.
8. Preserve accepted ordinary/current-limit output and a development-only compatibility profile.

**Proposed files:** `src/Core/Precision/PrecisionIntent.*`, `PrecisionPlan.*`, `PrecisionPlanner.*`, `src/Core/DeepZoom/FormulaCapability.*`, `ReferenceOrbitService.*`, `OrbitCache.*`, `OrbitEncoding.*`, both renderer owners and tests.
**Acceptance:** AC-028–AC-032.
**Validation:** VAL-045–VAL-050.
**Governance update:** Planner/profile/orbit decisions, resource budgets and measured encoding limits.
**Report:** Plan matrix, backend policy audit, orbit numerical/error evidence, worker/cache/cancellation metrics and native results.
**Rollback point:** Accepted exact-camera state plus one compatibility planner profile; no second production planner authority.
**Stop conditions:** A backend retains unregistered policy; stale results can upload; cancellation cannot stop new work; cache lacks a byte bound; formulas use only display text as identity; reported precision conflates stages.

## PH-14 — Perturbation Validity, Rebase and Multi-Reference

**Status:** Proposed.
**Objective:** Make existing perturbation a validated correct-or-fail path through classification, bounded correction and deterministic reference planning.
**Prerequisites:** PH-13 passed; independent direct high-precision reference samples/tiles exist; natural and injected instability fixtures exist; GPU validity/readback evidence is available.
**Required reading:** rendering contracts, both shaders, CPU/direct reference, visual fixtures, device-loss lifecycle, decisions and resource policy.
**Expected state:** Invalid perturbation output is classified before colour acceptance, corrected/rebased within budgets or explicitly rejected; multiple references are optional, deterministic and bounded.

**Tasks**
1. Freeze shared recurrence, indexing, bailout and smoothing semantics.
2. Add D3D11/OpenGL validity/error output and injection fixtures.
3. Replace silent split-float instability acceptance with classified handling.
4. Add deterministic rebase hysteresis, direct high-precision correction and bounded subdivision.
5. Add a bounded reference atlas and deterministic tile/reference assignment only after single-reference correction passes.
6. Record invalid/corrected/unresolved metrics and reject unresolved frames beyond policy.
7. Rebuild reference resources safely after device loss and present the last valid interactive frame while replacement work completes.

**Proposed files:** `src/Core/DeepZoom/PerturbationValidity.*`, `RebasePolicy.*`, `CorrectionScheduler.*`, `ReferenceAtlas.*`, `TileReferencePlanner.*`, shared GPU contracts, both shaders and tests.
**Acceptance:** AC-033–AC-037.
**Validation:** VAL-051–VAL-056.
**Governance update:** Invalidity/rebase/multi-reference decisions, calibrated thresholds and backend support records.
**Report:** Direct-reference comparisons, classification/correction counts, termination budgets, device metadata and unresolved failures.
**Rollback point:** Independently disable speculative prefetch, multi-reference, correction or new encoding while reducing claimed support; retain exact camera/planner and last accepted path.
**Stop conditions:** Validity is inferred from final colour; work can grow without bound; unresolved pixels are hidden; rebase alters exact camera; assignment depends on completion order; backend semantics diverge without a capability split.

## PH-15 — Deep Export and Authoring Integration Hardening

**Status:** Proposed.
**Objective:** Extend validated deep rendering through preview, still, frame, video-export, animation and Scout authoring workflows, then harden playback of the resulting files through the separate file-backed desktop lifecycle and publish measured support.
**Prerequisites:** PH-14 passed for intended profiles; full/tile equivalence passed; source-frame validity accepted; PH-08/PH-09 safety contracts retained; PH-10/PH-11 dependency disposition recorded.
**Required reading:** offline export, animation, Scout, rendering contracts, Windows lifecycle, persistence, risk and validation.
**Expected state:** Deep jobs carry exact snapshot/plan/validity identity; incomplete frames cannot reach FFmpeg; lifecycle/resource evidence supports a measured support matrix.

**Tasks**
1. Add exact snapshot/plan and validity metadata to still/frame jobs and manifests.
2. Add preflight memory, cache, temporary-storage and cancellation checks.
3. Validate GPU deep tiled still output against the direct reference and retain a safe fallback.
4. Implement the DEC-036 user renderer selection and manifest classification; enable a GPU selection only for validated profiles and fail rather than silently falling back.
5. Preserve FFmpeg fixed-vector invocation, source-frame validation, decode probe and atomic promotion.
6. Integrate exact Journey/timeline/Scout promotion and concurrent preview/export arbitration; desktop playback remains file-backed and outside precision planning and animation evaluation.
7. Exercise renderer device loss plus file-backed desktop sleep, lock, RDP, monitor and Explorer lifecycles, cancellation, disk-full, migration and downgrade paths without coupling desktop presentation to a fractal renderer.
8. Run long-session resource soak and publish a measured `DEEP-ZOOM-SUPPORT-MATRIX.md`.

**Proposed files:** affected still/frame/video/UI/Windows lifecycle owners plus deep fixture/support reports.
**Acceptance:** AC-038–AC-041 and all preceding deep criteria.
**Validation:** VAL-057–VAL-060 plus cumulative native, visual, migration and resource regression.
**Governance update:** Deep-frame authority, support matrix, release/troubleshooting wording and final phase evidence.
**Report:** Exact job identity, numerical/visual comparisons, resource soak, lifecycle/fault matrix, real FFmpeg evidence and measured support limits.
**Rollback point:** Preserve accepted frame-sequence/FFmpeg formats and exact camera readability; disable only the failing backend/profile.
**Stop conditions:** Invalid frames can reach FFmpeg; output promotes before validation; downgrade discards exact values; resources grow with session duration; required native evidence is absent; claims exceed measured support.
