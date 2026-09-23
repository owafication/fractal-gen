# Traceability

**Status:** Current canonical ID and evidence-status register; proposed work remains explicitly marked  
**Purpose:** Own immutable IDs, mappings and implementation status  
**Owner:** Project governance  
**Reading trigger:** Any code, documentation, test, release or migration task  
**Update trigger:** Requirement, phase, acceptance, validation, route, risk or status change

## Identifier rules

- IDs are immutable and never reused.
- Definitions live in their canonical owner; this file owns cross-links and status.
- Status values: `Proposed`, `Observed`, `In progress`, `Passed`, `Failed`, `Deferred`, `Blocked`, `Unproven`, `Superseded`.
- A requirement is not complete until its linked acceptance criteria and validation have evidence within their stated scope.

## Requirement mapping

| Requirement | Scope | Architecture / owner | Phase | Expected files | Acceptance | Validation | Status |
|---|---|---|---|---|---|---|---|
| REQ-001 | Current | Win32/CMake baseline | PH-01 | existing Windows target | AC-003, AC-004 | VAL-003–VAL-008 | Native build/test and isolated startup plus install/reinstall/uninstall evidenced; Windows 11, preceding-release upgrade and broader manual matrix remain |
| REQ-002 | Current | Security/privacy | PH-00, PH-11 | security docs, existing local paths | AC-019, AC-020 | VAL-001, VAL-018 | Observed source |
| REQ-003 | Current | Rendering contracts | PH-01, PH-02 | renderer and fixture files | AC-007–AC-009 | VAL-009–VAL-013 | PH-02 complete at documented CPU/D3D11/OpenGL fixture scope; evidence remains environment-specific |
| REQ-004 | Current | State/rendering | PH-02, PH-03 | fingerprint and adapter files | AC-010 | VAL-012, VAL-014 | Documented PH-03 adapter/fingerprint scope complete with portable and native evidence |
| REQ-005 | Current | Data/security | PH-00, PH-11 | settings, JSON, tests | AC-005 | VAL-015, VAL-018 | Observed source |
| REQ-006 | Current | Windows integration | PH-01, PH-11 | DesktopHost/WallpaperController | AC-006 | VAL-006, VAL-007 | Bounded static/slideshow/video, visible failure/detach, display-message handling and simulated host recovery pass; physical lifecycle and visual layout matrix remain |
| REQ-007 | Superseded by REQ-041 | UI routes | PH-01 | historical AppWindow/dialogs | AC-006 | VAL-006 | Continuously rendered Live/Journey desktop contract removed by DEC-039 |
| REQ-008 | Current | Persistence | PH-01, PH-03 | settings and migration fixtures | AC-005, AC-010 | VAL-015, VAL-016 | Observed source |
| REQ-009 | Current | Jobs/resources | PH-01, PH-08, PH-11 | existing and future job code | AC-016 | VAL-017 | Partly observed |
| REQ-010 | Current | Diagnostics | PH-01, PH-11 | Logger, diagnostics UI | AC-020 | VAL-006, VAL-018 | Observed source |
| REQ-011 | Foundation | Release/versioning | PH-00, PH-01 | version checks, presets, validation reports | AC-002–AC-004 | VAL-002–VAL-008 | Version/build/test and historical package evidence pass; isolated installer lifecycle passes in BR-20260923-03; preceding-release upgrade and release gates remain |
| REQ-012 | Foundation | Visual regression | PH-02 | fixture harness/baselines | AC-007–AC-009 | VAL-009–VAL-013 | Complete at documented scope: approved strict CPU baselines and native CPU/D3D11/OpenGL fixtures pass |
| REQ-013 | Foundation | Render fingerprint | PH-03 | canonical serializer/fingerprint | AC-009, AC-010 | VAL-012, VAL-014 | Implementation complete; canonical SHA-256 fingerprint integrated into CPU fixtures |
| REQ-014 | Foundation | State adapters | PH-03, PH-11 | snapshots, parameter coordinator, gesture/replacement boundaries, exclusive editor gate | AC-010, AC-011 | VAL-014–VAL-016 | Complete at documented PH-03 scope; BR-20260924-02 adds native Palette/Equation/Settings competing-route exclusion; broader dialog matrix remains |
| REQ-015 | Active | Undo/redo | PH-04, PH-05 | history/coordinator/UI/tests | AC-012, AC-013 | VAL-019–VAL-022 | Complete at automated native PH-05 scope; all bounded structural replacement routes and exact replay evidenced |
| REQ-016 | Active | Animation tracks | PH-06, PH-07 | evaluator/timeline/editor/tests | AC-014 | VAL-023–VAL-026 | Implemented; native clock/editor and Journey conversion pass; rendered visual playback and DPI/keyboard remain |
| REQ-017 | Current | Frame export | PH-08 | job, manifest, frames/tests | AC-015, AC-016 | VAL-027–VAL-030 | Implemented; native WIC success/resume/refusal/cancellation pass; modal desktop pause/restoration passes; Open Folder, DPI/keyboard and desktop pause during active output jobs remain |
| REQ-018 | Current | External video export | PH-09 | `ExternalVideoExport`, `ExternalProcess`, `VideoExportDialog`, tests | AC-017 | VAL-031–VAL-033 | Implemented; FFmpeg 8.1.1 and bounded native dialog success/cancellation evidence pass; other encoder builds remain |
| REQ-019 | Active | Scout integration | PH-10 | adapters/undo/fixtures | AC-018 | VAL-034–VAL-036 | Complete at documented PH-10 bounded/native scope; identity/thumbnails, selection isolation and Apply replay pass |
| REQ-020 | Cross-cutting | Validation/evidence | All | reports and governance updates | AC-019, AC-020 | VAL-001–VAL-036 | Active; phase-specific evidence and remaining boundaries are recorded |
| REQ-021 | Deep camera | Exact camera/persistence | PH-12 | exact number/camera/parser/adapters | AC-021–AC-024 | VAL-038–VAL-040 | Bounded exact decimal/camera and schema-3/12 migration implemented; acceptance remains scoped to PH-12 evidence |
| REQ-022 | Deep state | Single camera authority | PH-12 | exact authority + one-way adapters | AC-025 | VAL-041 | Exact entry/transaction authority and one-way compatibility adapters implemented; remaining authority audit is separately gated |
| REQ-023 | Deep identity | State/export fingerprints | PH-12, PH-15 | versioned canonical state/manifests | AC-027 | VAL-042, VAL-060 | Versioned exact render identity and immutable precision-plan frame manifests implemented; PH-15 integration remains proposed |
| REQ-024 | Deep policy | Central precision planner | PH-13 | planner/plan/backend adapters | AC-028, AC-029 | VAL-045, VAL-046 | Central exact CPU tiers and legacy GPU compatibility policy implemented; validated deep perturbation rendering remains open |
| REQ-025 | Deep policy | Intent/capability separation | PH-12, PH-13 | project/preset intent, user default and transient plan | AC-028, AC-032 | VAL-041, VAL-045 | Core intent/capability separation implemented in planner; broader authoring/diagnostic integration remains gated |
| REQ-026 | Deep formulas | Versioned capability profiles | PH-13, PH-14 | quadratic/Tricorn fingerprints | AC-030, AC-033 | VAL-047, VAL-051, VAL-052 | Quadratic/Tricorn core capability fingerprints implemented; PH-14 production recurrence validation remains proposed |
| REQ-027 | Deep safety | Fail-closed formula selection | PH-13 | capability planner | AC-030 | VAL-047 | Bounded exact/planner routes reject unsupported profiles; no general deep-GPU completion claim |
| REQ-028 | Deep orbit | Reference-orbit service | PH-13 | immutable service/cache/encoding | AC-031 | VAL-048–VAL-050 | Bounded planner-matched reference service/cache/worker implemented; production perturbation consumption remains open |
| REQ-029 | Deep concurrency | Generation safety | PH-13, PH-15 | generation/fingerprint fences | AC-031, AC-039 | VAL-049, VAL-059 | Generation-stamped service and bounded worker evidence pass; renderer upload/commit integration remains open |
| REQ-030 | Deep diagnostics | Precision-stage reporting | PH-13 | plan/orbit diagnostics | AC-032 | VAL-045, VAL-048 | Plan/encoding metadata and CPU evaluator status implemented; full support diagnostics remain proposed |
| REQ-031 | Deep correctness | Perturbation validity | PH-14 | masks/classification | AC-034, AC-037 | VAL-053, VAL-056 | Stable/Rebased/Unresolved core classification implemented; PH-14 renderer validity/correction acceptance remains proposed |
| REQ-032 | Deep correctness | Rebase/direct correction | PH-14 | rebase/correction scheduler | AC-035 | VAL-054 | Proposed |
| REQ-033 | Deep correctness | Bounded multi-reference | PH-14 | atlas/tile planner | AC-036 | VAL-055, VAL-056 | Proposed |
| REQ-034 | Deep mapping | Exact global samples/tiles | PH-12, PH-15 | mapping/halo contracts | AC-038 | VAL-057 | Exact rational global samples, AA 1–4 and CPU tile origins implemented; PH-15 scheduler/integration remains proposed |
| REQ-035 | Deep resources | Enforced resource bounds | PH-13–PH-15 | budgets/accounting/cancellation | AC-031, AC-036, AC-039 | VAL-050, VAL-056, VAL-059 | Bounded direct rows, orbit/cache/worker cancellation implemented; PH-14/15 correction and full resource matrix remain proposed |
| REQ-036 | Regression | Ordinary/current-limit safety | PH-12–PH-15 | compatibility fixtures | AC-028, AC-033, AC-038 | VAL-009–VAL-013, VAL-051, VAL-052, VAL-057 | Existing CPU/D3D11/OpenGL regression evidence retained; future deep profiles require their own acceptance |
| REQ-037 | Deep motion | Deterministic exact navigation | PH-12, PH-15 | Journey/timeline/Scout adapters | AC-022, AC-038 | VAL-044, VAL-058 | Lossy exact animation/Scout sources fail closed; exact keyframes and deep navigation execution remain proposed |
| REQ-038 | Deep output | Failure-safe output | PH-15 | still/frame/video validity | AC-039 | VAL-059, VAL-060 | Proposed |
| REQ-039 | Deep diagnostics | Reproducible bounded reports | PH-13–PH-15 | diagnostics/support metadata | AC-032, AC-037 | All deep validations | Bounded plan/encoding/evaluator diagnostics implemented; full PH-15 support reporting remains proposed |
| REQ-040 | Product claims | Measured support only | PH-15 | support matrix/release docs | AC-040 | VAL-060 | Proposed |
| REQ-041 | Current desktop presentation | UI/Windows integration/persistence | PH-01, PH-11 | AppWindow, WallpaperController, SettingsStore | AC-041 | VAL-061 | Mirror/Span contract retained; native static/slideshow, invalid MP4, mute/loop and injected failure stop/detach pass; broader lifecycle/codec/visual matrix remains |

## Phase status

| Phase | Status | Entry dependency | Exit evidence owner |
|---|---|---|---|
| PH-00 | Historical working-repository baseline passed; this archive has no usable Git metadata or supplied bundle | Accepted project root and governance use | DELIVERY_REPORT.md |
| PH-01 | In progress; native build/tests, historical package checks and isolated install/launch/reinstall/uninstall pass; preceding-release upgrade and broader matrix remain | PH-00; Windows toolchain | VALIDATION_AND_EVIDENCE.md |
| PH-02 | Complete: approved CPU baselines and native CPU/D3D11 WARP/hardware/OpenGL evidence recorded | Original prerequisites retained | VISUAL-REGRESSION.md |
| PH-03 | Complete at documented scope; native compile/core evidence and recorded navigation/dialog interaction pass | Pending PH-02 verification is non-blocking under DEC-017 | PROJECT-STATE-AND-PARAMETERS.md |
| PH-04 | Complete at documented scope; camera/palette Undo/Redo and redo-branch interaction confirmed | PH-03 camera/palette adapters | UNDO-REDO-PLAN.md |
| PH-05 | Complete at automated native scope; Palette/Equation, preset Load/Import, Settings, Journey and Scout Apply history replay pass | PH-04 accepted | UNDO-REDO-PLAN.md |
| PH-06 | Implementation/native tests and preview-clock interaction pass; rendered visual playback remains | PH-03; portable visual fixtures under DEC-017 | ANIMATION-TRACKS-PLAN.md |
| PH-07 | Implementation/native tests, editor state/clock and Journey conversion interaction pass; visual playback and DPI/keyboard remain | PH-06 evaluator | ANIMATION-TRACKS-PLAN.md |
| PH-08 | Implementation/native Release evidence and bounded WIC success, resume, refusal, visible cancellation and close-while-active cancellation interaction complete; modal desktop pause/restoration passes; Open Folder, DPI/keyboard, active-output pause and physical lifecycle remain | PH-06 deterministic evaluator | OFFLINE-EXPORT-PLAN.md |
| PH-09 | Implemented / partly verified; native Release compile/test, real FFmpeg 8.1.1 success/owned-process cancellation/integrated export cancellation/failure fixture and bounded dialog summary/validation/failure/verified-MP4/cancellation interaction passed; broader encoder coverage pending | PH-08 frame sequence; DEC-009/DEC-027 accepted | OFFLINE-EXPORT-PLAN.md |
| PH-10 | Complete at documented bounded/native scope; deterministic identity/thumbnails, selection/Close isolation and Apply/Undo/Redo pass | PH-03 and PH-05 | FRACTAL-SCOUT-STATUS.md |
| PH-11 | Active; bounded desktop media/failure, display-message/host recovery, native tab/name, 60-second static resource and isolated installer checks pass; broader/manual matrix remains | PH-01–PH-10 as applicable | VALIDATION_AND_EVIDENCE.md |
| PH-12 | Exact decimal/camera foundation, schema-3/schema-12 forward migration, main-window exact entry, v2 camera identity and schema-3 v3 precision-plan export manifests implemented; animation and renderer authority remain pending | Green baseline; schemas 12/3; original-preserving forward-only migration; accepted DEC-028 grammar/bounds; reviewed precision dependency | PROJECT-STATE-AND-PARAMETERS.md |
| PH-13 | Planner-owned exact CPU tiers, central legacy GPU selection and bounded planner-matched reference service are implemented; measured encoding limits and validated production perturbation rendering remain proposed | PH-12; DEC-029 accepted | RENDERING-AND-EXPORT-CONTRACTS.md |
| PH-14 | Proposed | PH-13; direct high-precision reference evidence | RENDERING-AND-EXPORT-CONTRACTS.md |
| PH-15 | Proposed | PH-14; scoped PH-10/PH-11 dependencies without predecessor completion claim | VALIDATION_AND_EVIDENCE.md |

## Validation IDs

| ID | Check | Evidence required |
|---|---|---|
| VAL-001 | Governance batch audit | Paths, links, ownership and ID uniqueness |
| VAL-002 | Version-surface scan | CMake, manifest, RC, script, installer and package names agree |
| VAL-003 | CMake configure | Native x64 Release configuration log |
| VAL-004 | MSVC compile/link | Complete build log including resources and Windows libraries |
| VAL-005 | Core tests | CTest output for the exact build |
| VAL-006 | Windows smoke matrix | Recorded startup, dialogs, preview and shutdown results |
| VAL-007 | Desktop-mode matrix | None/static/slideshow/video start-stop evidence |
| VAL-008 | Packaging/upgrade | ZIP, installer, integrity and upgrade evidence |
| VAL-009 | CPU fixture repeatability | Repeated canonical output and environment metadata |
| VAL-010 | D3D11 fixture comparison | WARP/hardware-specific metrics and artifacts |
| VAL-011 | OpenGL fixture comparison | Supported-environment metrics and artifacts |
| VAL-012 | Tiled/full comparison | Full image, tiled image, seam strips and metrics |
| VAL-013 | Deliberate visual mutation | Expected failed diff with diagnostic artifacts |
| VAL-014 | Snapshot round trip | Camera low components and selected palette fields preserved |
| VAL-015 | Settings migration fixtures | Old/current schema load, defaults and non-destructive save |
| VAL-016 | Single-authority audit | Direct mutation paths enumerated and migrated domain has one path |
| VAL-017 | Cancellation/resource tests | Bounded cancellation, memory and output cleanup evidence |
| VAL-018 | Input/privacy review | Bounds, executable-data rejection and log-content review |
| VAL-019 | Scalar undo/redo | Forward/reverse state equality; PH-04/PH-05 portable tests passed |
| VAL-020 | Coalescing | Pan/zoom/palette-control gestures become intended entries; PH-04/PH-05 portable tests passed |
| VAL-021 | Structural undo/redo | Palette-stop/equation/journey/preset subtree replacements reverse exactly in portable tests |
| VAL-022 | History eligibility | Runtime and background scalar/structural events absent from history; PH-05 portable tests passed |
| VAL-023 | Animation determinism | Same snapshot/timeline/time/seed yields same state |
| VAL-024 | Deep interpolation | Precision-safe camera interpolation fixtures |
| VAL-025 | Clock separation | Preview/export clocks remain independent; the retained wallpaper clock is inert compatibility state and cannot drive desktop presentation |
| VAL-026 | Journey conversion | Supported round trips and explicit unsupported cases |
| VAL-027 | Frame timing | Exact frame index/time mapping |
| VAL-028 | Frame atomicity | No partial promoted frames after interruption |
| VAL-029 | Resume manifest | Fingerprint match resumes; mismatch refuses |
| VAL-030 | Selected-frame visuals | Approved frames across timeline |
| VAL-031 | FFmpeg discovery | Executable identity/version/capability log |
| VAL-032 | Safe invocation | Fixed argument vector; no shell interpolation |
| VAL-033 | Encode failure/cancel | Frames preserved, output rejected, logs captured |
| VAL-034 | Scout determinism | Existing candidate order/fingerprint retained |
| VAL-035 | Scout preview isolation | Preview does not mutate active/persisted project |
| VAL-036 | Scout apply history | Apply is one undoable transaction |
| VAL-037 | Baseline structural-gate reconciliation | `verify-source.py` and accepted baseline checks green on the same source state |
| VAL-038 | Exact numeric syntax/canonicalisation | Malformed/equivalent/extreme exponent/negative-zero/length-bound tests |
| VAL-039 | Exact camera round trip | UI, clipboard, settings, preset and render-job canonical equality |
| VAL-040 | Legacy settings/preset migration | Supported schema fixtures, original preservation and deterministic defaults |
| VAL-041 | Single-authority audit | Every camera write inventoried; exact authority only; adapters one-way |
| VAL-042 | Exact render fingerprint v2 | Stable bytes/hash, zero equivalence and exact-digit mutation detection |
| VAL-043 | Built-in/user preset ownership | Immutable built-ins, Save As identity, collision and persistence tests |
| VAL-044 | Journey/timeline/Scout exact adapters | Exact round trips, explicit lossy rejection and stale-result rejection |
| VAL-045 | Precision planner table | Deterministic formula/depth/output/device/resource plan matrix |
| VAL-046 | Backend policy removal audit | No unregistered D3D11/OpenGL thresholds or fallback order |
| VAL-047 | Formula capability fingerprint | Quadratic, exact power-2 Tricorn and unsupported mutation matrix |
| VAL-048 | Reference-orbit numerical comparison | Selected samples against an accepted independent high-precision reference |
| VAL-049 | Orbit cancellation/generation races | Cancellation, coalescing, stale completion, shutdown and allocation failure |
| VAL-050 | Orbit cache/resource accounting | Byte bound, eviction, reuse, upload lifetime and no session-duration growth |
| VAL-051 | D3D11 perturbation comparison | WARP/hardware readback against direct reference with plan metadata |
| VAL-052 | OpenGL perturbation comparison | Supported driver readback against the same semantic reference |
| VAL-053 | Validity/glitch injection | NaN, overflow, relative-delta, disagreement and forced-error masks |
| VAL-054 | Rebase/correction continuity | Exact camera identity, boundary continuity and corrected classification |
| VAL-055 | Multi-reference determinism | Reference choice/order independent of worker/tile completion order |
| VAL-056 | Correction/resource termination | Reference/subdivision/pixel/retry/time budgets and explicit unresolved failure |
| VAL-057 | Global mapping/tile seams | Full/tiled/reordered/odd/portrait/ultrawide/rotation/AA/halo matrix |
| VAL-058 | Deterministic animation/frame timing | Exact timestamp seek, pause/resume, long Journey and frame-index timing |
| VAL-059 | Failure/lifecycle matrix | Cancel, device loss, sleep, lock, RDP, monitor, Explorer, disk-full and shutdown |
| VAL-060 | Deep still/frame/FFmpeg release matrix | Source validity, WIC, manifests, real H.264/MP4, decode probe and support report |
| VAL-061 | File-backed desktop contract | Schema-10 Live/Journey migration to None; schema-11 Independent/assignments migration to Mirror/no map; no public continuous-render start/per-tick/fallback route; WIC static/slideshow; Media Foundation local MP4 validation/mute/loop; async video and slideshow exhaustion stop visibly; native build/tests and bounded runtime playback evidence |

## Current PH-02 evidence

- **VAL-009:** four canonical CPU fixtures rendered twice through `RenderStillImageTiled` and matched exactly in the recorded GNU same-process run; approved cross-run baselines remain open.
- **VAL-010:** D3D11 WARP/hardware fixture evidence is unproven.
- **VAL-011:** OpenGL fixture evidence is unproven.
- **VAL-012:** rotated full-width and tiled output, including dedicated boundary strips, matched exactly in the recorded GNU run.
- **VAL-013:** a deliberate palette-offset/depth mutation changed 90.49% of pixels, maximum channel error 168, RMS channel error 73.76 and structural similarity 0.7895 in the recorded run.

## Current PH-01/PH-11 release evidence

- **VAL-002–VAL-006:** BR-20260909-03 passes source policy, a clean Visual Studio 18 2026 x64 Release configure/build, CTest 8/8 including the native interaction fixture, executable file/product version 1.13.1, portable-package inspection and process-scoped isolated D3D11 startup/render/shutdown on Windows 10 build 19045.
- **VAL-008 partial:** the release script now discovers machine-wide, per-user and `PATH`-provided Inno Setup. The validator requires a non-empty installer, matching version 1.13.1, a recorded SHA-256 and explicit Authenticode status. `artifacts/windows-validation/20260909-ph11-installer-final/report.json` is the current evidence owner.
- Installer install, upgrade and uninstall execution remain unproven. The produced artifact is unsigned, and repository cleanliness is unproven because this supplied checkout has no `.git` directory.

## Current PH-12/PH-13 deep evidence

- **VAL-057:** BR-20260803-03 proves exact rational AA subpixel factors for a 2× grid and deterministic bounded direct CPU AA-2/AA-4 execution. Tiled/rotated seam matrices remain open.
- **VAL-060:** BR-20260803-04 rejects unsupported exact frame rotation at immutable job creation and rechecks each evaluated frame before its render callback. Runtime encoder/video evidence remains open.
- **VAL-060:** BR-20260803-06 rejects animated equation coefficients at exact sample/still/frame boundaries because the exact evaluator has no time-phase contract.
- **VAL-048:** BR-20260731-02 compares all real/imaginary float4 components at selected Mandelbrot and Tricorn orbit iterations between the current fixed-point arbitrary producer and the independent Boost-512 producer; payload bits and escape metadata match. It is bounded selected-sample evidence, not a precision ceiling or GPU claim.
- **VAL-048:** BR-20260803-01 adds fixture-specific ordered-float reconstruction measurements against the Boost-512 producer: Mandelbrot maximum absolute/relative errors `3.4413387754277605e-08` / `5.7453155647240968e-08`; Tricorn `1.3645269757469331e-06` / `5.3683366176295286e-08` before escape. These values are not an accepted encoding ceiling or GPU validation.
- **VAL-045:** BR-20260803-02 proves the central planner treats zero validated GPU float4 bits as correction-required, not as a selectable/adequate deep backend.
- **VAL-046:** BR-20260803-05 moves D3D11/OpenGL legacy automatic precision thresholds and fallback order to the platform-neutral planner; backend source retains capability reports only. Core policy and both production GPU fixtures pass.
- **VAL-050:** BR-20260731-03 proves bounded service cache eviction and over-budget refusal without cache publication. Default limit calibration, worker pressure and upload lifetime remain open.
- **VAL-051:** BR-20260731-01 passes a deterministic WARP-only, bit-exact staging readback of the production float4 reference-orbit texture payload against the canonical CPU upload source. This is partial transport evidence only; the required WARP/hardware perturbation output comparison against a direct reference with immutable plan metadata remains open.

## Current PH-03 evidence

- **VAL-014:** camera and palette/post snapshots round-trip exactly through explicit apply adapters; compensated X/Y low components remain distinct and unchanged.
- Canonical SHA-256 implementation matches the standard empty-string and `abc` vectors.
- `mw-render-state-v1` produces identical bytes/digests for excluded metadata changes and positive/negative zero, and different digests for compensated-low, palette and output-dimension changes.
- Visual fixture environment artifacts now include the canonical digest and exact canonical input bytes.
- **VAL-016:** passed for the inspected PH-03 implementation scope: palette/post scalars, deliberate camera routes, built-in palette selection, preview pan/wheel gesture metadata and classified preset load/import plus Palette/Equation/Settings/Journey replacement. Unit tests cover transactional rejection, model normalisation, compensated-low preservation, no-op behaviour, invalidation aggregation, discrete palette validation/custom-stop clearing, gesture token lifetime, the shared 500 ms preview-navigation inactivity policy, replacement normalisation/rollback and origin-kind classification. The source audit rejects direct writes for the migrated scalar, palette, working-camera and whole-working-preset routes. PH-05 now records classified broad replacements atomically rather than flattening them into incomplete scalar deltas.

## Current PH-05 evidence

- **VAL-019:** scalar camera, palette and post-processing mutations restore exact before/after state, including camera-associated metadata.
- **VAL-020:** drag, wheel and palette-control gestures coalesce only for matching origin, token and target set; structural entries do not coalesce.
- **VAL-021:** custom palette stops, multi-field equation changes, journey rows and preset application round-trip exactly as atomic entries.
- **VAL-022:** animation/export/replay/migration/system origins remain excluded for scalar and structural history.
- **VAL-036:** one Scout Apply camera transaction creates one labelled entry and restores exact pre/post project state.
- **VAL-016/VAL-021:** BR-20260909-03 opens the real Palette/Equation editors, changes exact camera state while each remains open, accepts an editor-owned setting and retains the newer camera. It verifies Palette/Equation structural history labels plus Equation Undo/Redo powers 2/3 through native commands.
- **VAL-016/VAL-021:** BR-20260909-04 waits for complete dialog construction and the post-dialog main-window commit, then selects the second built-in preset through the real combo notification. `Load Preset` Undo/Redo restores the exact before/after camera text and selected preset identity.
- **VAL-016/VAL-021:** BR-20260909-05 accepts a real Settings project rotation change, requires the main-window field to reflect it, and verifies one `Edit Project Settings` Undo/Redo entry restores the before/after values. The initial failing fixture directly reproduced the stale-control regression before the AppWindow synchronisation fix.
- **VAL-016/VAL-021:** BR-20260909-06 edits a valid two-row structured Journey through the real dialog, requires one `Edit Journey` entry, and reopens the dialog after acceptance, Undo and Redo to compare exact before/after waypoint text.
- **VAL-016/VAL-021:** BR-20260910-01 serialises a bounded preset with the production settings codec, submits it through the native Open dialog, requires one `Import Preset` entry, and verifies exact camera and selected-identity restoration through Undo and Redo.
- **VAL-036:** BR-20260910-02 runs the production Fractal Scout search, invokes `Use in Preview`, requires one `Apply Scout Camera` entry, and verifies exact camera restoration plus unchanged preset identity through Undo and Redo.
- **VAL-016/VAL-021:** BR-20260924-02's editor-only native fixture opens Palette, Equation and Settings through the production routes. Each disables the main window, blocks a competing editor command for 750 ms, and restores the owner after close. This supersedes the concurrency assumption in the earlier modeless camera-retention fixture; no concurrent candidate snapshots are accepted.

## Current PH-10 evidence

- **VAL-034:** candidate identity, scores, ordering and production thumbnails are deterministic for the bounded pinned request. BR-20260908-01 runs the production Scout twice under native MSVC, retains three V2 exact-camera candidates, and requires exact identity/score/pixel equality, non-uniform output and distinct candidate image hashes.
- **VAL-035:** candidate-copy/core evidence, production thumbnails and BR-20260910-03 native candidate-selection/Close interaction prove the dialog preserves project camera, preset identity and history until Apply.
- **VAL-036:** core history and BR-20260910-02/03 native dialog interaction prove Scout Apply is one reversible labelled camera transaction that preserves preset identity.

## Current PH-06 evidence

- **VAL-023:** repeated evaluation with the same valid `Preset`, timeline, time and seed returns exactly equal frame-local state, resolved time, applied track ordering and invalidation metadata; the base snapshot remains unchanged.
- **VAL-024:** compensated camera-centre keyframes retain non-zero low components during interpolation, while scale uses logarithmic interpolation and rotation uses shortest-path wrapping.
- **VAL-025:** preview, legacy wallpaper and export clock storage can be set, advanced and reset independently; invalid updates fail without changing the selected clock. DEC-039 narrows the product claim: only preview/export are active animation domains and the wallpaper clock is not connected to desktop presentation.
- **VAL-023/VAL-025:** BR-20260910-04 uses the production Timeline dialog to add a track and current-value keyframes, scrub and play the preview clock, stop/reset it, and Cancel without changing project/history state. OK retains one runtime track on reopen while project state and history remain unchanged.
- **VAL-026:** strict Journey rows convert to deterministic camera tracks and supported camera-only timelines round-trip; malformed rows, unsupported enabled targets, mismatched timing/interpolation and lossy compensated values are explicitly refused without partial output.
- **VAL-026:** BR-20260910-05 drives Journey-to-Tracks and Tracks-to-Journey through the production editor, requires three camera tracks and the successful preparation confirmation, then proves Cancel leaves project/history state unchanged.
- Validation also covers Clamp/Loop/PingPong edges, duplicate enabled targets, duplicate times, integer Step-only targets, bounded IDs/resources, disabled unknown targets and per-frame history exclusion.
- Native MSVC compilation passes in the current Release build. BR-20260910-04 supplies bounded Win32 editor/preview-clock interaction and BR-20260910-05 supplies native Journey conversion interaction; rendered visual playback remains pending.

## Current PH-08 evidence

- **VAL-027:** rational frame rates are bounded; frame time is derived directly from index; aligned inclusive/exclusive counts and non-aligned grid behaviour pass portable tests.
- **VAL-028:** each frame is written under `.mw-frame-sequence` as `.part`, WIC-dimension validation is required before receipt/final promotion, untracked final files are refused, and cancellation preserves only verified promoted frames. Portable callback tests prove the core promotion/cancellation contract. BR-20260910-06 additionally produces one 32x24 PNG through the production WIC/dialog route and cancels a separate 1024x1024 job with its worker joined and no `.part` file left.
- **VAL-029:** identical immutable inputs produce the same SHA-256 job fingerprint; matching manifests verify and resume completed frames, while project/output changes, typed-manifest tampering and malformed entries are refused. BR-20260910-06 reopens the production dialog against its completed one-frame manifest and observes `0 rendered, 1 resumed` with no partial file. The same native route rejects a changed width as a manifest mismatch and preserves the existing verified PNG; a separate untracked final PNG is refused and preserved byte-for-byte.
- **VAL-030:** start, middle and end timeline frames rendered twice through `RenderStillImageTiled` match exactly per frame and differ across the animated palette track. These are portable generated checks, not approved cross-platform image baselines.
- Source audit verifies the Win32 route, joined worker, reusable WIC row encoder, file-backed desktop pause/resume policy, no desktop fractal-render fallback, settings schema-12 compatibility and the PH-08 documentation boundary. Native MSVC compilation plus bounded PNG success/reopen, matching resume, visible Cancel and close-while-active cancellation now pass. Native mismatched-manifest and untracked-final refusal dialogs also pass. BR-20260924-01 adds the bounded modal desktop pause matrix. Open Folder, DPI/keyboard/accessibility, desktop pause during active output jobs and physical lifecycle remain pending.

## UI traceability

Detailed screen/route/state/permission/fallback mappings are owned by [UI_WORKFLOWS_AND_ROUTES.md](UI_WORKFLOWS_AND_ROUTES.md).

Deep routes `ROUTE-015`–`ROUTE-018` cover exact coordinate edit/copy, precision intent/plan diagnostics, exact Journey/Scout navigation and validity-gated still/frame/video export. ROUTE-015 exact-entry/copy and ROUTE-016 legacy-preview resolution have bounded implemented slices; broader deep navigation and validity-gated output remain separately gated. The route owner maps their remaining scope to VAL-038–VAL-060.

## Runtime AI traceability

Not applicable. The observed application has no runtime AI, model API, prompt, retrieval or tool boundary.

## Current PH-09 evidence

- **VAL-031:** capability parser tests accept a version probe only when `libx264` and MP4 muxer evidence are present and reject an encoder missing the required capability. The Win32 route supports explicit selection and `PATH` lookup without persisting the path.
- **VAL-032:** source audit requires exact `CreateProcessW` launch and fixed application-owned argument vectors and rejects shell-token integration in the production encoder/process sources. Core tests inspect the generated H.264/MP4 and decode-probe vectors and reject odd source dimensions before fixed yuv420p execution.
- **VAL-033:** core tests prove complete manifest/digest validation, adjacent temporary output, verification before promotion, frame preservation on process failure/cancellation, bounded-log paths and manifest-only optional cleanup after success. The opt-in real FFmpeg 8.1.1 fixture additionally passed capability checks, production PNG generation, fixed-vector encoding, decode verification, atomic promotion, source preservation, bounded owned-process termination, full `RunExternalVideoExport` cancellation cleanup and exact-argument post-preflight encoder-failure containment in `artifacts/p9-v7/report.json`. BR-20260918-01 adds production-dialog missing-input/CRF validation, nonexistent-executable refusal with no output, two verified MP4 successes, and two visible Cancel runs that preserve and revalidate all 120 source frames while leaving no final or temporary MP4. Other third-party builds remain pending.

## Current PH-11 evidence

BR-20260924-02 adds the process-wide exclusive AppWindow edit-session gate. Direct MSVC compilation of the changed AppWindow, Settings dialog and fixture sources passed, and the manually linked editor-only native fixture passed Palette, Equation and Settings owner disabling, competing-route blocking and close restoration. The full CMake/MSBuild build was attempted but blocked by the host's FileTracker `E_ACCESSDENIED`; no full CTest or release claim is made for this batch. The desktop-host/media portion of the broad fixture could not be used in the restricted host because Progman was unavailable. Broader auxiliary routes, physical lifecycle, accessibility and release gates remain open.

BR-20260923-03 covers VAL-006/007/008/017/061 at bounded native scope. See [Validation and Evidence](VALIDATION_AND_EVIDENCE.md#ph-11-desktop-and-installer-hardening--br-20260923-03) for exact checks and exclusions. A posted display message is not a physical reconnect; reparenting the owned host is not an Explorer restart; a posted failure message is not a decoder-originated fault.

## File-backed desktop export pause — BR-20260924-01

REQ-006/009/017/018/041, AC-006/015–017/041 and VAL-006/017/027–033/061 gain bounded native modal pause/restoration coverage. The 24 cases span three desktop modes, two export dialogs and four pause/lifecycle states; power messages are synthetic. Native Release build and CTest 8/8 passed. Active-job pause and physical lifecycle remain unproven; PH-08/PH-11 are not complete. [Evidence](../artifacts/ph11-20260924-01/report.md).
