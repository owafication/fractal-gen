# Mandelbrot Live Wallpaper — Deep-Zoom Upgrade Pack (Combined)

**Status:** Proposed  
**Mode:** File generation  
**Source baseline:** 1.13.1 PH-09 roadmap-progress archive  

This combined file mirrors the separately packaged Markdown files. The separate files remain the preferred integration form.


---

## FILE: `README.md`

# Mandelbrot Live Wallpaper — Deep-Zoom Upgrade Pack

**Mode:** File generation  
**Pack status:** Proposed  
**Pack version:** 2.0  
**Source baseline:** Mandelbrot Live Wallpaper 1.13.1, archive labelled PH-09 roadmap progress  
**Application source changed:** No  
**Canonical phase status changed:** No

## Objective

Provide a source-reconciled foundation for upgrading the existing deep-zoom system from its current bounded `double`/compensated-coordinate and single-reference perturbation design to a deterministic, exact-coordinate, resource-bounded workflow.

The target is continuous deep zoom without:

- coordinate collapse or loss during copy, persistence, animation, Scout, still or video workflows;
- frozen, repeated, block-distorted or plausibly incorrect pixels;
- unexplained D3D11/OpenGL/CPU precision-selection differences;
- stale or synchronous reference-orbit work blocking or overwriting newer state;
- visible tile seams or tile-order-dependent output;
- uncontrolled worker, RAM, VRAM, cache, correction, retry or temporary-storage growth;
- unsupported formula families entering an incompatible perturbation path.

## Evidence basis

- Source archive SHA-256: `630cf8089eadfce33c4edecb99e9596793d1be8e32052823a8cffabd2f86757d`
- Supplied proposal SHA-256: `fc99cd2c0804e9080e172e61ca1458cf2b4cb30647d9815205f971338bbd1de9`
- The archive was extracted and inspected as a 256-entry source tree.
- The portable GNU 14.2 Release core build completed with warnings treated as errors.
- CTest passed 4/4: core tests, path tests, CPU visual fixture self-check and version consistency.
- `scripts/verify-source.py` failed at its PH-03 AppWindow gesture-marker check. The verifier expects separate `PreviewPan` and `PreviewWheelZoom` markers, while the current AppWindow routes both through `ParameterGestureKind::PreviewNavigation`. This baseline inconsistency is not treated as an application runtime failure, but it must be reconciled before new deep-zoom work.

## Source-specific correction to the supplied proposal

The application does **not** lack perturbation rendering. It already contains:

- `Float32`, `Float64`, split-float, double-reference perturbation and arbitrary-reference perturbation modes;
- compensated camera-centre high/low values;
- analytic quadratic and exact power-2 Tricorn perturbation profiles;
- a custom fixed-point reference-orbit generator selectable at 128, 256 or 512 bits;
- D3D11 and OpenGL reference-orbit upload and perturbation shaders;
- GPU-tiled high-resolution still output;
- deterministic CPU frame-sequence output and external FFmpeg H.264/MP4 encoding.

This pack therefore hardens and extends existing contracts. It does not propose a parallel renderer or pretend these capabilities are absent.

## Claim boundary

“Infinite zoom” is not an accepted technical or product claim.

Approved direction:

> Continuous deep zoom with dynamically planned precision, bounded resources and reported measured limits for the selected formula, backend, device and output workflow.

Every release claim must identify tested depth, formula profile, output size, backend, device class, precision plan and unresolved limitations.

## Frozen integration ranges

Before repository integration, confirm these ranges remain unused:

- phases: `PH-12`–`PH-15`;
- requirements: `REQ-021`–`REQ-040`;
- acceptance criteria: `AC-021`–`AC-040`;
- validation: `VAL-037`–`VAL-060`;
- decisions: `DEC-028`–`DEC-037`;
- risks: `RISK-013`–`RISK-030`;
- routes: `ROUTE-015`–`ROUTE-018`.

Never renumber accepted IDs. If any range is occupied, allocate a new range and update every mapping before implementation.

## Pack files

1. `README.md`
2. `PACK_MANIFEST.md`
3. `SOURCE_ASSESSMENT_AND_DIRECTION.md`
4. `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md`
5. `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md`
6. `DEEP_ZOOM_BUILD_PLAN.md`
7. `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md`
8. `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md`
9. `CANONICAL_INTEGRATION_AND_HANDOFF.md`
10. `AUDIT_REPORT.md`

## Reading sequence

1. Repository `/AGENTS.md`.
2. `project_docs/PROJECT_INDEX.md`.
3. Current active phase and canonical owners.
4. `SOURCE_ASSESSMENT_AND_DIRECTION.md`.
5. Requirements and architecture in this pack.
6. Only the proposed phase being implemented.
7. Directly relevant source, tests, shaders, persistence and UI routes.
8. Validation, risk and migration sections affected by the change.

## Completion boundary

The deep-zoom upgrade is complete only when:

- exact camera centre **and scale** survive all durable and deterministic workflows;
- one precision planner owns algorithm selection for preview, wallpaper, still and frame export;
- formula capability is fingerprinted and fail-closed;
- asynchronous orbit results cannot reach a stale generation;
- supported perturbation output agrees with a direct high-precision reference within approved criteria;
- unstable pixels are corrected, safely rebased or explicitly rejected rather than silently rendered;
- tiled and full-frame mappings are equivalent at global sample coordinates;
- cancellation, device loss, application exit and encoder failure cannot promote incomplete output;
- long-session RAM, VRAM, cache, worker and temporary-storage growth remains within accepted budgets;
- ordinary-zoom visual fixtures remain within approved tolerances;
- native Windows D3D11, OpenGL, UI, device-loss and real-FFmpeg evidence is recorded;
- the support matrix states measured limits and contains no literal infinite/unlimited claim.


---

## FILE: `PACK_MANIFEST.md`

# Pack Manifest

**Status:** Complete response-generated pack  
**Authority:** Supporting proposal only; canonical repository owners prevail  
**Owner:** Deep-zoom planning and integration  
**Update trigger:** Source-baseline change, accepted decision, ID conflict or phase-scope change

## Purpose

Register the pack, its ownership boundaries and the canonical repository files that must be updated if the proposal is accepted.

## File register

| File | Purpose | Read trigger | Canonical ownership affected |
|---|---|---|---|
| `README.md` | Baseline, claim boundary, frozen ranges and reading sequence | First read | `PROJECT_INDEX.md`, `AGENTS.md` |
| `SOURCE_ASSESSMENT_AND_DIRECTION.md` | Inspected source capability, defects, conflicts and product direction | Planning or audit | Foundation, architecture, validation, maintenance |
| `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md` | Scope, requirements, formula support and user outcomes | Product or acceptance change | `PROJECT_FOUNDATION.md`, `TRACEABILITY.md` |
| `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md` | Exact camera, planner, orbit, perturbation, export and resource contracts | Code or schema work | Architecture and rendering owners |
| `DEEP_ZOOM_BUILD_PLAN.md` | PH-12–PH-15 implementation sequence | Build execution | `IMPLEMENTATION_PLAN.md` |
| `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md` | AC/VAL catalogue, fixtures and mappings | Test or completion claim | `TRACEABILITY.md`, `VALIDATION_AND_EVIDENCE.md` |
| `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md` | Decisions, risks, compatibility and rollback | Cross-cutting change | Decision, risk and persistence owners |
| `CANONICAL_INTEGRATION_AND_HANDOFF.md` | Exact canonical update map and agent handoff | Repository integration | All affected owners |
| `AUDIT_REPORT.md` | Pack audit, evidence boundary and unproven items | Delivery or compatibility review | `DELIVERY_REPORT.md` |

## Canonical integration targets

Update only after acceptance and direct repository inspection:

```text
project_docs/PROJECT_INDEX.md
project_docs/PROJECT_FOUNDATION.md
project_docs/IMPLEMENTATION_PLAN.md
project_docs/TRACEABILITY.md
project_docs/VALIDATION_AND_EVIDENCE.md
project_docs/SECURITY_PRIVACY_AND_RISK.md
project_docs/DECISIONS_AND_CHANGE_HISTORY.md
project_docs/DATA_AND_PERSISTENCE.md
project_docs/DEBUGGING_AND_MAINTENANCE.md
project_docs/UI_WORKFLOWS_AND_ROUTES.md
project_docs/PROJECT_SETTINGS.md
docs/architecture/PROJECT-STATE-AND-PARAMETERS.md
docs/architecture/RENDERING-AND-EXPORT-CONTRACTS.md
docs/testing/VISUAL-REGRESSION.md
docs/features/ANIMATION-TRACKS-PLAN.md
docs/features/OFFLINE-EXPORT-PLAN.md
docs/features/FRACTAL-SCOUT-STATUS.md
docs/roadmaps/INTEGRATED-CREATIVE-ROADMAP.md
scripts/verify-source.py
```

## Ownership rule

This pack may define proposed IDs and contracts, but it must not become a competing canonical owner. After integration:

- requirement definitions belong in `PROJECT_FOUNDATION.md`;
- phase definitions and status belong in `IMPLEMENTATION_PLAN.md` and `TRACEABILITY.md`;
- rendering/data/UI contracts belong in their current canonical owners;
- decision and risk status belongs in the existing registers;
- this pack remains a supporting design record linked from `PROJECT_INDEX.md`.

## Status by file

| File | Status |
|---|---|
| `README.md` | Complete |
| `PACK_MANIFEST.md` | Complete |
| `SOURCE_ASSESSMENT_AND_DIRECTION.md` | Complete |
| `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md` | Complete |
| `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md` | Complete |
| `DEEP_ZOOM_BUILD_PLAN.md` | Complete |
| `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md` | Complete |
| `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md` | Complete |
| `CANONICAL_INTEGRATION_AND_HANDOFF.md` | Complete |
| `AUDIT_REPORT.md` | Complete |


---

## FILE: `SOURCE_ASSESSMENT_AND_DIRECTION.md`

# Source Assessment and Product Direction

**Status:** Proposed assessment grounded in the supplied archive  
**Purpose:** Record observed capability, evidence, defects, contradictions and the least-complex justified upgrade direction  
**Owner:** Architecture and delivery planning  
**Reading trigger:** Deep-zoom, precision, camera, rendering, export, Scout, animation or persistence work

## Assessment method

### Inspected

- governance and routing under `/AGENTS.md`, `project_docs/` and the active architecture/feature documents;
- `CameraState`, `Preset`, `PrecisionSettings`, schema validation and JSON persistence;
- `DeepZoom`, CPU still rendering, D3D11, OpenGL and GPU tiled high-resolution rendering;
- general animation, Journey conversion, Scout, frame-sequence export and external video export;
- core tests, visual fixtures, CMake configuration and the source-structure verifier.

### Ran

- `python3 scripts/verify-source.py`;
- a clean portable GNU 14.2 Release configure/build with warnings as errors;
- CTest for the resulting build.

### Results

- **Passed:** portable configure, compile and link.
- **Passed:** 4/4 CTest targets.
- **Failed:** `scripts/verify-source.py` at the PH-03 AppWindow gesture-marker check.
- **Unproven:** native MSVC application build for this extracted copy, Windows UI, D3D11 hardware, OpenGL runtime, desktop attachment, device loss and real FFmpeg execution.

A passing portable build proves the platform-neutral core and test scope only. It does not prove Windows application or GPU behaviour.

## Current application overview

### Observed product baseline

The source is a native offline Win32 C++20 fractal application with:

- editable preview and desktop output;
- D3D11 primary rendering, OpenGL fallback and CPU still/static fallback;
- data-only equation and palette editors;
- compensated camera-centre coordinates;
- deep-zoom precision controls;
- static, live, slideshow and Journey desktop modes;
- multi-monitor support;
- tiled PNG/JPEG/TIFF/BMP high-resolution output;
- deterministic CPU PNG frame sequences;
- external FFmpeg H.264/MP4 encoding;
- local settings, presets, diagnostics and bounded background work;
- deterministic Fractal Scout;
- runtime undo/redo, animation timeline and canonical render fingerprints.

### Observed deep-zoom pipeline

```text
CameraState(double centre high/low + double scale)
       |
PrecisionSettings / renderer-local threshold logic
       |
EquationSupportsPerturbation
       |
BuildReferenceOrbitDouble or BuildReferenceOrbitArbitrary
       |
Four-float orbit expansion upload
       |
D3D11/OpenGL single-reference perturbation shader
       |
Per-pixel instability fallback to direct split-float
```

### Current formula support

The current perturbation gate supports:

- analytic quadratic parameter maps that satisfy the bounded profile conditions;
- exact standard power-2 Tricorn when its coefficients match the expected recurrence.

It rejects Julia, Newton, absolute-value, swapped-component, unary-transform, reciprocal, iteration-term, animated-coefficient and unsupported conjugate configurations. This is safer and broader than the supplied proposal’s claim that only standard Mandelbrot exists, but it is not yet a versioned capability contract.

## Source evidence summary

| Observation | Source evidence |
|---|---|
| Camera centre uses two doubles; scale is one double | `src/Core/Models.h:119-126` |
| Durable coordinates are parsed through `std::stod` | `src/App/AppWindow.cpp:235-246`, `1233-1246`; `HighResRenderDialog.cpp:133-160` |
| Copy/export coordinate text is limited to normal double formatting | `src/App/AppWindow.cpp:2698-2704`; `HighResRenderDialog.cpp:128-160` |
| Scale is clamped to `1e-32`–`4.0` | `src/Core/Models.cpp:770-772` |
| Maximum zoom is clamped to `1e30` | `src/Core/Models.cpp:775-777` |
| Arbitrary reference precision is limited to 128–512 bits | `src/Core/Models.cpp:920-923`; `PrecisionDialog.cpp:103-107` |
| Arbitrary orbit input is reconstructed from centre high/low doubles | `src/Core/DeepZoom.cpp:372-388` |
| Orbit samples are uploaded as four floats per component | `src/Core/DeepZoom.h:9-15`; `DeepZoom.cpp:94-104`, `400-401` |
| GPU perturbation scale is a float shader value | `src/Rendering/Direct3D11Renderer.cpp:110`; `OpenGLRenderer.cpp:190` |
| Precision selection is implemented separately in D3D11 and OpenGL | `Direct3D11Renderer.cpp:526-566`; `OpenGLRenderer.cpp:344-379` |
| D3D11 and OpenGL automatic fallback order differs | same ranges above |
| Reference orbit generation occurs synchronously in renderer upload | `Direct3D11Renderer.cpp:568-580`; `OpenGLRenderer.cpp:381-386` |
| Instability returns to direct split-float per pixel | D3D11 shader near line 110; OpenGL shader near line 190 |
| GPU high-resolution still output is already tiled | `src/App/HighResRenderDialog.cpp:347-537` |
| Deterministic frame export uses CPU tiled rendering | `FrameSequenceExportDialog.cpp:514`; `DEC-026` |
| Frame export fingerprint context pins precision to Float64 but uses CPU rendering | `src/Core/FrameSequenceExport.cpp:675-681`; dialog worker path |
| Source verifier disagrees with current consolidated preview navigation route | verifier expects separate markers; AppWindow uses `PreviewNavigation` |

## Critical findings

### FIND-001 — Exact input is lost before arbitrary precision begins

**Observed:** coordinate entry and persistence are binary doubles. Centre precision is extended only by a second low double; scale remains one double.

**Consequence:** a long decimal coordinate cannot survive entry, copy, settings save, preset save, Journey conversion, timeline editing or export identity. Increasing reference-orbit precision to 512 bits cannot recover digits discarded before orbit generation.

**Required direction:** exact decimal or exact binary scientific camera values must become the durable authority. Current doubles become derived adapters.

### FIND-002 — Current depth is deliberately capped near `1e30`

**Observed:** `camera.scale` cannot be smaller than `1e-32`, `maximumZoom` cannot exceed `1e30`, and animation clamps to the same floor.

**Consequence:** the current application can provide useful deep zoom, but not the proposed open-ended precision escalation.

**Required direction:** replace scale-as-double with a representation that carries mantissa/exponent or exact decimal value, while retaining an explicit resource and tested-depth policy.

### FIND-003 — “Arbitrary precision” is bottlenecked by input and GPU encoding

**Observed:** the fixed-point orbit starts from doubles and uploads each component as four floats, documented as roughly 96 bits. Shader scale remains float.

**Consequence:** selecting 256 or 512 reference bits does not make the complete render path 256 or 512-bit accurate. The current label is directionally meaningful but can overstate end-to-end precision.

**Required direction:** report reference precision, uploaded orbit precision, delta precision and camera precision separately. Either upgrade the GPU encoding/scale contract or state the measured ceiling.

### FIND-004 — Precision policy has two backend authorities

**Observed:** D3D11 and OpenGL each own threshold tables and fallback order. D3D11 does not follow the same float64 preference used by OpenGL for incompatible perturbation formulas.

**Consequence:** identical project state can select different precision semantics for reasons not captured by one planner or fingerprint.

**Required direction:** centralise planning in `src/Core`, returning a backend-specific execution plan with a recorded reason and fallback chain.

### FIND-005 — Reference generation is synchronous and not generation-safe

**Observed:** renderer upload calls orbit generation inline. No request ID, cancellation token, worker service or stale-result gate exists at that boundary.

**Consequence:** high-precision setup can stall rendering and cannot be cancelled independently. Future asynchronous work would risk applying an orbit generated for stale state unless identity is added first.

**Required direction:** immutable requests, generation IDs, cancellable workers and renderer-owned upload of completed matching results.

### FIND-006 — Instability fallback can still produce plausible incorrect pixels

**Observed:** the perturbation shader returns `directSplit(p)` when relative delta or q-state is unstable.

**Consequence:** direct split-float may be insufficient at the same deep scale. The fallback is not a correctness proof, no invalidity mask is retained, and no bounded correction/rebase strategy exists.

**Required direction:** emit validity/error metadata; rebase, assign another reference or perform direct high-precision correction. Unresolved pixels must not be silently accepted.

## High findings

### FIND-007 — No bounded multi-reference strategy

A single reference orbit is cached per renderer. Difficult regions can force broad fallback rather than deterministic tile subdivision and bounded additional references.

### FIND-008 — CPU still/frame rendering is not a high-precision correctness oracle

The CPU tiled renderer maps and evaluates through ordinary floating-point values. It is deterministic and bounded, but it cannot validate far deeper exact-camera output without a separate direct high-precision reference path.

### FIND-009 — Exact-camera identity is absent from current persistence and fingerprints

The canonical render fingerprint accurately records current IEEE-754 fields, but cannot identify decimal digits that never entered the model. Schema 9 and preset schema 2 store numeric camera fields.

### FIND-010 — Animation and Journey preserve compensated centres but not arbitrary depth

General animation correctly treats centre high/low as a compensated value and uses logarithmic scale interpolation. The scale and waypoint parser still use doubles and bounded `std::stod` conversion.

### FIND-011 — GPU still and CPU frame export have different rendering paths

High-resolution stills can use GPU tiles; deterministic frame sequences intentionally use CPU tiles. This is a valid current decision, but a future deep video claim requires an explicit choice between canonical CPU reference frames and validated GPU deep frames.

### FIND-012 — Verification infrastructure is internally inconsistent

The portable core tests pass while the source-marker verifier fails. New phases must not build on a baseline where the project’s required structural gate is red or stale.

## Medium findings

- The custom fixed-point engine is embedded in `DeepZoom.cpp`, limiting independent testing, replacement and reuse.
- Reference-orbit keys are renderer-local text strings instead of a versioned typed fingerprint.
- Precision UI describes “deepest supported zooms” without displaying measured camera/orbit/delta limits.
- Precision settings mix user intent, algorithm forcing and capability fallback in one persisted structure.
- Current iteration maximum is 4096; deeper regions may require a separately planned iteration policy.
- Current cache is small rather than unbounded, but lacks explicit byte accounting, eviction telemetry and cross-renderer reuse.
- Current formula gating uses near-equality and implicit profile logic rather than a versioned capability fingerprint.

## Reconciliation with the supplied proposal

| Supplied proposal point | Source-reconciled result |
|---|---|
| Add perturbation rendering | Already implemented; harden it instead |
| Support standard Mandelbrot first | Preserve existing analytic quadratic and exact Tricorn support; version both profiles |
| Add arbitrary-precision orbit generation | Already present in a custom bounded form; replace or isolate it behind a tested service |
| Add tiled GPU export | Already present for stills; extend deterministic deep-frame export only after validation |
| Add reference rebasing | CPU sample helper refresh exists, but production GPU path lacks governed rebase/correction |
| Add multiple references | Still required |
| Add exact camera | Still required and is the highest-priority architectural change |
| Add central precision planner | Still required |
| Add bounded resources/cancellation | Existing jobs are bounded; orbit/correction/cache-specific policy is still required |
| Append phases after PH-11 | Retained, with PH-12–PH-15 and a baseline-repair entry gate |

## Recommended product direction

### MVP deep-zoom upgrade

1. Repair the verification baseline and freeze current output fixtures.
2. Add exact centre and exact scale authority without immediately replacing every renderer.
3. Centralise precision planning and version formula capabilities.
4. Move reference-orbit generation behind a cancellable typed service.
5. Harden existing single-reference D3D11/OpenGL perturbation with validity output and safe rebase/correction.
6. Add bounded multi-reference only after single-reference correctness is evidenced.
7. Extend still/frame/video export after preview and wallpaper semantics are stable.

### Later

- series approximation;
- reference compression beyond the first validated encoding;
- additional perturbation formula families;
- distributed/network rendering;
- literal project-level deep-zoom sharing across devices.

### Non-goals

- rewriting the application around a new engine;
- replacing D3D11/OpenGL/CPU simultaneously;
- making every custom equation perturbation-compatible;
- claiming infinite zoom;
- adding cloud, telemetry, agents, services or a new database;
- bundling or downloading FFmpeg.


---

## FILE: `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md`

# Deep-Zoom Requirements and Scope

**Status:** Proposed  
**Purpose:** Define the upgrade boundary, immutable requirements and user-visible outcomes  
**Owner:** Product and architecture  
**Reading trigger:** Scope, priority, compatibility, acceptance or formula-support change  
**Linked IDs:** `REQ-021`–`REQ-040`, `AC-021`–`AC-040`, `PH-12`–`PH-15`

## Objective

Extend the existing 1.13.1 deep-zoom implementation so that camera intent is not limited by ordinary double parsing and persistence, precision selection is deterministic and centralised, and unsupported or unstable render paths fail safely.

## MVP boundary

### In scope

- exact durable camera centre and viewport scale;
- compatibility adapters to current `CameraState`;
- managed settings, preset, Journey, timeline, Scout and export migration;
- central precision planner;
- versioned formula capability fingerprint;
- typed and cancellable reference-orbit service;
- existing analytic quadratic and exact Tricorn perturbation profiles;
- validity/error output, rebase and bounded correction;
- bounded multi-reference rendering for difficult tiles;
- deterministic full-frame/tile/global-sample mapping;
- preview, live wallpaper, still, frame sequence and video-source-frame integration;
- explicit resource budgets, diagnostics, cancellation and device-loss behaviour;
- native D3D11/OpenGL/CPU validation and measured support reporting.

### Later

- perturbation for additional formula families;
- series approximation;
- persisted general animation timelines if DEC-014 is separately resolved;
- more than the first accepted orbit-upload encoding;
- optional GPU deterministic frame export after canonical equivalence is proven;
- H.265/VP9 or other encoders after H.264/MP4 remains stable.

### Optional

- user-selectable deep-zoom resource profiles;
- developer diagnostics showing tile/reference/error overlays;
- exported support report beside a render job;
- prefetch of future Journey references under strict memory and cancellation bounds.

### Excluded

- literal infinite or mathematically unlimited rendering claims;
- arbitrary executable equations, shader source or scripts;
- a second independently mutable project model;
- network rendering, accounts, cloud sync or telemetry;
- microservices, queues, databases, agents or vector storage;
- bundling, downloading or persisting the path of FFmpeg;
- silently approximating exact values without recording that approximation.

## Requirements

| ID | Requirement |
|---|---|
| `REQ-021` | Preserve exact camera centre X/Y and viewport scale from text entry through copy, save, load, preset, Journey, timeline, Scout, preview, wallpaper and export. |
| `REQ-022` | Retain one authoritative project camera; current double/high-low values are derived renderer adapters, not a competing mutable authority. |
| `REQ-023` | Provide deterministic canonical serialisation and fingerprinting for exact camera, formula capability, precision plan, output geometry, time and seed. |
| `REQ-024` | Use one platform-neutral precision planner for CPU, D3D11 and OpenGL preview, wallpaper, still and frame workflows. |
| `REQ-025` | Separate persisted user precision intent from transient device capability, selected execution path and adaptive presentation state. |
| `REQ-026` | Preserve and version the existing analytic quadratic and exact power-2 Tricorn perturbation capabilities. |
| `REQ-027` | Fail closed when a formula fingerprint is not compatible with the selected perturbation profile. |
| `REQ-028` | Generate reference orbits from exact camera values at planner-selected precision through a cancellable typed service. |
| `REQ-029` | Prevent stale orbit, correction, tile or frame results from reaching a newer render generation. |
| `REQ-030` | Record camera precision, reference precision, uploaded orbit precision and delta precision separately. |
| `REQ-031` | Detect non-finite, unstable, inaccurate or otherwise invalid perturbation output and retain a machine-readable validity classification. |
| `REQ-032` | Rebase or directly correct invalid regions without changing canonical camera intent or creating a visible jump outside accepted tolerance. |
| `REQ-033` | Support a deterministic, bounded multiple-reference strategy for difficult frames and tiles. |
| `REQ-034` | Use identical global viewport and sample mapping for full-frame, tiled, preview, wallpaper, still and video-source-frame rendering. |
| `REQ-035` | Keep orbit, reference, correction, tile, worker, RAM, VRAM, retry and temporary-storage use within explicit enforceable budgets. |
| `REQ-036` | Preserve ordinary-zoom output and performance within approved fixture and interaction tolerances. |
| `REQ-037` | Make continuous zoom, Journey and timeline evaluation derive from immutable start state and deterministic time rather than cumulative floating-point stepping. |
| `REQ-038` | Ensure cancellation, device loss, application exit, write failure and encoder failure cannot promote an incomplete final result. |
| `REQ-039` | Expose active profile, plan, backend, precision fields, reference count, correction state, fallback reason and measured limits in diagnostics. |
| `REQ-040` | Publish tested support limits and prohibit unsupported infinite/unlimited claims in UI, documentation and release material. |

## Formula support contract

### Existing profiles to preserve

#### Analytic quadratic parameter profile

A versioned profile derived from the currently supported bounded recurrence subset:

```text
z(n+1) = A·z(n)^2 + B·z(n) + C·c + D
z(0) = 0
```

Subject to the accepted gate for render mode, parameter power, transforms, Julia mode, iteration terms, reciprocal terms and coefficient animation.

#### Exact power-2 Tricorn profile

```text
z(n+1) = conjugate(z(n))^2 + c
z(0) = 0
```

With the current exact coefficient constraints.

### Direct-render fallback initially

- Julia variants;
- Newton basins;
- higher powers and Multicorn powers above two;
- absolute-value and swapped-component formulas;
- rational/reciprocal terms;
- sine, cosine, exponential and logarithmic transforms;
- iteration-dependent and animated coefficients;
- unsupported custom equation combinations.

“Fallback” does not mean float32 is always adequate. The planner must choose a validated direct path or report that the requested depth is unsupported.

### Capability rule

A formula enters a perturbation path only when a stable capability fingerprint explicitly matches the implementation version. Human-readable equation summaries are not sufficient authority.

## User outcomes

### Exact coordinate workflow

A user can paste a long exact camera value, save it, restart, copy it and receive the same canonical value. The UI may display a shortened preview, but Copy and Edit Exact must retain all authoritative digits.

### Continuous zoom

A user can zoom beyond the current `1e30` product cap without camera collapse, subject to the measured limits of the selected profile and resource policy. Mode changes do not move the camera.

### Pause/resume and seek

Pausing freezes an exact camera. Resuming starts from that exact state. Seeking to the same deterministic timestamp produces the same camera and render fingerprint.

### Preset Save As

Built-in locations remain immutable. Save As clones the complete effective state, including exact camera and precision intent, into a new user-owned identity.

### Deep still and frame export

A deep render reports its plan before work starts, remains cancellable, uses temporary output, validates completed output and promotes only after success. Tile order or worker count does not change deterministic output.

### Unsupported formula

The user receives a specific limitation and safe alternative. The application does not silently enter an incompatible perturbation recurrence or claim a valid deep result.

### Low-resource system

The application reduces speculative work and presentation quality before correctness. It stops with a clear resource error if the required precision cannot be maintained.

## Acceptance boundaries

- Exact round-trip means canonical value equality, not visually similar double values.
- “GPU-first” means the first validated GPU path in the central plan, not unconditional GPU use.
- “Arbitrary precision” must state which stage has that precision.
- Per-pixel direct split-float fallback alone is not accepted as deep-correctness evidence.
- A source-marker or unit test does not prove native GPU or UI behaviour.
- Portable CPU fixtures do not prove D3D11/OpenGL equivalence.
- A rendered image is not accepted when unresolved invalid pixels are hidden.
- A phase is not passed until its mapped AC and VAL evidence is recorded by the canonical owners.


---

## FILE: `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md`

# Deep-Zoom Architecture and Contracts

**Status:** Proposed  
**Purpose:** Define the least-complex managed evolution from the current source to exact, deterministic deep zoom  
**Owner:** Core architecture and rendering  
**Reading trigger:** Camera, persistence, precision, formula, renderer, animation, Scout or export work

## Architecture principle

Adapt the current model and renderer stack. Do not create a parallel fractal engine or independently mutable project state.

```text
UI / Preset / Journey / Timeline / Scout
                  |
                  v
            ExactCamera
                  |
                  v
       ExactRenderSnapshot v2
                  |
                  v
          PrecisionPlanner
         /        |         \
 direct path   perturbation  unsupported
                  |
          ReferenceOrbitService
          OrbitCache / generation gate
                  |
       PerturbationCoordinator
       Validity + Rebase + Correction
                  |
        D3D11 / OpenGL / CPU reference
                  |
      preview / wallpaper / tiled output
                  |
   still encoder / frame manifest / FFmpeg
```

## State ownership

### Canonical project state

The accepted project authority gains exact camera values. Existing `CameraState` remains during migration as a derived compatibility/render view.

```cpp
struct ExactDecimal {
    std::string canonical;
};

struct ExactComplex {
    ExactDecimal real;
    ExactDecimal imaginary;
};

struct ExactCamera {
    ExactComplex centre;
    ExactDecimal viewportHalfHeight;
};
```

Required properties:

- ASCII, locale-independent canonical form;
- finite numeric value only;
- positive non-zero half-height;
- canonical sign, exponent and zero representation;
- bounded input length and exponent range under resource policy;
- parsing and serialisation that never route through `double`;
- exact equality defined by canonical numeric value.

### Compatibility adapter

```cpp
struct CameraAdapterResult {
    CameraState camera;
    bool exactAtRequestedScale;
    PrecisionLossReason lossReason;
};
```

The adapter converts exact state to current high/low/scale values only for render paths that can safely use them. It must not write the approximation back into the exact authority.

### Exact render snapshot

```cpp
struct ExactRenderSnapshot {
    ExactCamera camera;
    EquationSettings equation;
    FormulaCapabilityFingerprint formulaCapability;
    PaletteAndPostSnapshot paletteAndPost;
    OutputGeometry output;
    SamplingPlan sampling;
    AnimationTime time;
    std::uint64_t seed;
    PrecisionIntent precisionIntent;
    ResourcePolicyId resourcePolicy;
    RendererContractVersion rendererContract;
};
```

The snapshot is immutable after a render/export job starts.

## Precision ownership

### Persisted intent

Proposed user/project intent:

```text
Automatic
PreferSpeed
Balanced
PreferAccuracy
ForceDirectReference
ManualMinimumPrecision
```

The existing exact ownership decision remains open under DEC-015. The upgrade must resolve whether intent is project state, user settings or split between the two before schema work.

### Transient capability

Never persisted as project truth:

- active GPU adapter and driver;
- float64/shader/resource support;
- shader self-test result;
- available VRAM/RAM/disk;
- active fallback reason;
- current frame time or adaptive render scale.

### Captured job plan

A deterministic export stores the effective plan and planner version. It does not replan silently midway unless the contract explicitly permits a fail-safe backend transition and records it.

## Central precision planner

Pure decision boundary:

```text
ExactRenderSnapshot
+ FormulaCapability
+ DeviceCapabilitySnapshot
+ ResourceBudget
= PrecisionPlan
```

```cpp
struct PrecisionPlan {
    PrecisionAlgorithm algorithm;
    RendererBackend preferredBackend;
    std::vector<RendererBackend> fallbackOrder;
    std::uint32_t cameraPrecisionBits;
    std::uint32_t referencePrecisionBits;
    OrbitEncoding orbitEncoding;
    DeltaEncoding deltaEncoding;
    std::uint32_t maximumIterations;
    TilePlan tilePlan;
    CorrectionBudget correctionBudget;
    RebasePolicyId rebasePolicy;
    ReproducibilityClass reproducibility;
    PrecisionReason reason;
    PrecisionPlannerVersion version;
};
```

Initial algorithms:

```text
GpuFloat32
GpuFloat64
GpuSplitCoordinate
GpuSingleReferencePerturbation
GpuMultiReferencePerturbation
CpuDirectHighPrecision
Unsupported
```

Rules:

- one threshold and capability authority in `src/Core`;
- backend adapters may report capability, not invent policy;
- plan identity is included in render/export fingerprints;
- unsupported direct depth returns `Unsupported`, not unconditional float32;
- automatic mode uses hysteresis for interactive transitions;
- deterministic export captures one plan before rendering.

## Formula capability fingerprint

The fingerprint includes every field that can change recurrence semantics:

- render mode and recurrence family;
- power and parameter power;
- quadratic, linear, parameter and constant coefficients;
- Julia mode and parameter;
- initial-state mode and value;
- conjugate, absolute, swap and unary-transform flags;
- reciprocal and iteration terms;
- animated-coefficient state and evaluated values;
- bailout and derivative/orbit-data requirements;
- capability-contract version.

Palette-only changes do not invalidate a reference orbit unless the selected colouring path requires orbit data not present in the cached result.

## Arbitrary-precision backend boundary

Move the current private `FixedReal` implementation behind a narrow core interface before replacing or extending it.

```cpp
class IHighPrecisionNumberFactory;
class IReferenceOrbitCalculator;
class IDirectHighPrecisionRenderer;
```

No selected library type may escape `src/Core/Precision`.

### Decision options

1. **Extend and extract the current fixed-point engine.** Lowest packaging impact; highest arithmetic, parsing and maintenance burden.
2. **Adopt a reviewed arbitrary-precision library.** Lower algorithmic burden; requires licence, MSVC, x64 packaging, deterministic parsing, cancellation and maintenance evidence.

Default: do not commit to either until the PH-12 decision spike measures both against exact parsing, reference fixtures and Windows packaging.

## Reference-orbit service

```cpp
struct ReferenceOrbitRequest {
    RenderGeneration generation;
    ExactComplex referencePoint;
    FormulaCapabilityFingerprint formula;
    std::uint32_t maximumIterations;
    std::uint32_t precisionBits;
    OrbitEncoding encoding;
    OrbitContractVersion contractVersion;
    CancellationToken cancellation;
};

struct ReferenceOrbitResult {
    ReferenceOrbitKey key;
    EncodedReferenceOrbit orbit;
    OrbitValidity validity;
    OrbitMetrics metrics;
};
```

Requirements:

- asynchronous and cancellable;
- bounded worker pool;
- duplicate-request coalescing;
- immutable result;
- stale-generation rejection before upload;
- no UI or persisted-state mutation;
- byte-accounted cache with bounded eviction;
- device-specific GPU upload remains on the render thread.

Cache key fields:

- exact reference point;
- formula capability fingerprint;
- maximum iterations;
- reference precision;
- orbit encoding and contract versions.

Camera scale is not part of the mathematical reference orbit key but is part of the consuming render plan and validity region.

## Orbit encoding

The current four-float expansion is retained as `OrbitEncoding::FloatExpansion4V1` only after its measured precision ceiling is documented.

A future encoding may use more components, block exponents or another GPU format. Any incompatible change receives a new encoding version and fixtures.

Diagnostics must distinguish:

- source camera precision;
- reference calculation precision;
- encoded orbit precision;
- shader delta precision.

## Perturbation contract

### Analytic quadratic profile

For reference `Z` and local delta `δz`, implement the accepted coefficient-aware recurrence corresponding to the current analytic quadratic profile. The recurrence, indexing, bailout, smoothing, derivative and colour-orbit data contract must be written once and shared by tests.

### Tricorn profile

Retain the current conjugate power-2 recurrence as a separate versioned profile. Do not generalise it by near-name or power alone.

### Validity output

Each sample or block produces a classification:

```text
Valid
EscapedValid
InteriorCandidate
NeedsRebase
NeedsDirectCorrection
UnsupportedOrbitData
NonFinite
BudgetExceeded
Cancelled
```

The production GPU path must be able to retain a validity mask or equivalent compact block summary. Returning a colour from direct split-float is not sufficient evidence that an unstable deep pixel is valid.

## Rebase and correction

Rebase triggers may include:

- predicted perturbation error above pixel tolerance;
- delta/reference ratio above policy;
- non-finite values;
- classification disagreement with a lower-resolution/direct check;
- excessive invalid pixels in a tile;
- material iteration-plan change.

Use hysteresis and minimum dwell for interactive rendering.

Correction order:

1. reuse a matching cached reference;
2. generate a new tile or region reference;
3. subdivide the invalid tile within bounded depth;
4. direct-render a bounded correction set at high precision;
5. fail the frame/job if unresolved pixels exceed budget.

## Multiple-reference strategy

Deterministic first implementation:

1. use the camera-centre reference;
2. render validity/error summaries per tile;
3. accept tiles within tolerance;
4. select the highest-error deterministic candidate from failed tiles;
5. create no more than the plan’s reference limit;
6. assign tiles by predicted error, not screen distance alone;
7. directly correct the remaining bounded set;
8. fail rather than silently accept unresolved output.

Reference count, subdivision depth, corrected pixels, retries and elapsed work are all bounded.

## Exact camera mapping

For width `W`, height `H`, centre `(cx, cy)`, vertical half-height `s` and sample offsets `(ox, oy)`:

```text
aspect = W / H
u = (globalX + ox) / W
v = (globalY + oy) / H
localX = (2u - 1) × aspect × s
localY = (2v - 1) × s
point = centre + rotate(localX, localY)
```

Every tile uses global coordinates and full-output dimensions. Tile cameras may be derived for current GPU APIs, but the derivation must be proven equivalent to global exact mapping.

Neighbour-sampling post-processing renders an explicit halo and crops it before assembly.

## Deterministic animation

Canonical camera evaluation derives from immutable start/keyframe values and deterministic time.

Prohibited as the durable authority:

```cpp
scale *= zoomFactor;
```

Preferred conceptual form:

```text
logScale(t) = interpolate(exactLogScaleStart, exactLogScaleEnd, t)
scale(t) = exactExp(logScale(t))
```

The chosen exact logarithm/interpolation representation must be deterministic and versioned. Pause freezes an exact camera; resume starts from it.

## Scout contract

Scout may continue searching approximately under bounded CPU rules, but:

- candidate camera values must be promoted to exact canonical form before save/apply;
- exact candidate identity must be included in fingerprints;
- stale candidate results are rejected by generation;
- Apply remains one undoable action;
- preview remains isolated from persisted state.

## Still, frame and video contracts

### Still

- preserve current GPU tiled and CPU fallback paths;
- add exact plan and validity reporting;
- retain temporary output and verified promotion;
- no unbounded full-frame allocation requirement.

### Frame sequence

Current CPU PNG sequence remains the canonical deterministic baseline until a GPU deep-frame path passes equivalence fixtures.

A future GPU frame path must capture:

- exact snapshot and time;
- precision plan and capability fingerprint;
- tile/reference/correction policy versions;
- backend/device evidence class;
- frame validity summary.

### FFmpeg

FFmpeg remains an external encoder only. It does not:

- select precision;
- calculate camera values;
- render fractals;
- alter frame timing or count;
- select references;
- hide invalid source frames.

The existing fixed-vector, bounded-log, decode-probe and atomic-promotion contract remains authoritative.

## Threading and lifetime

- UI thread: user state, commands and presentation only.
- Render thread: GPU context, resource creation, dispatch, readback and presentation.
- Precision workers: orbit generation and direct correction.
- Export worker: immutable frame/tile orchestration.
- Encoder process: external FFmpeg after source-frame validity.

Shutdown order:

1. stop accepting new jobs;
2. advance generation and request cancellation;
3. stop speculative workers;
4. drain submitted GPU work within policy;
5. stop owned encoder process;
6. clean owned temporary output;
7. destroy renderer resources;
8. persist only valid accepted user state.

## Resource policy

```cpp
struct DeepZoomResourceBudget {
    std::uint32_t maximumPrecisionWorkers;
    std::uint64_t maximumOrbitCacheBytes;
    std::uint64_t maximumGpuDeepZoomBytes;
    std::uint32_t maximumReferencesPerFrame;
    std::uint64_t maximumCorrectionPixels;
    std::uint32_t maximumSubdivisionDepth;
    std::uint32_t maximumInFlightTiles;
    std::uint32_t maximumTileRetries;
    std::uint32_t maximumReferencePrecisionBits;
    std::uint32_t maximumOrbitIterations;
    std::uint64_t minimumFreeDiskReserveBytes;
    std::chrono::milliseconds cancellationDrainTimeout;
};
```

Resource-pressure order:

1. cancel stale speculative work;
2. evict unused cache entries;
3. reduce in-flight tiles;
4. disable prefetch;
5. reduce interactive presentation resolution;
6. reduce optional interactive post-processing;
7. lower interactive frame rate;
8. choose a safe planned backend;
9. stop with a resource error.

Never reduce required numerical precision silently.

## Diagnostics

Expose at minimum:

- exact camera summary and digit count;
- formula capability ID/version;
- planner version and reason;
- selected/fallback backend;
- camera/reference/orbit-upload/delta precision;
- iteration count;
- reference count and cache hit/miss;
- invalid, corrected and unresolved pixel counts;
- tile count/retries;
- RAM/VRAM/cache/temporary-storage use;
- cancellation or fallback reason;
- render fingerprint.

Logs remain bounded and should avoid full user paths or full coordinate strings unless explicitly exported by the user for diagnosis.


---

## FILE: `DEEP_ZOOM_BUILD_PLAN.md`

# Deep-Zoom Build Plan

**Status:** Proposed  
**Purpose:** Define the managed implementation sequence after the current PH-00–PH-11 programme  
**Owner:** Delivery planning  
**Reading trigger:** Phase planning, coding, validation or handoff

## Programme rule

No phase status changes merely because this pack exists. The archive label does not prove PH-09 acceptance, and new work does not waive current native/runtime gates.

Every implementation session must:

1. read `/AGENTS.md` and `PROJECT_INDEX.md`;
2. identify the current phase and affected risk domains;
3. inspect current source before editing;
4. map changes to REQ, AC and VAL IDs;
5. make a narrow reversible slice;
6. run proportionate checks;
7. update canonical owners and delivery evidence;
8. report inspected, changed, run, passed, failed, skipped and unproven.

## PH-00 prerequisite for this programme

Before PH-12 begins:

- confirm the repository root and working-tree state;
- confirm ID ranges remain unused;
- reconcile the failing `scripts/verify-source.py` gesture-marker expectation with the current `PreviewNavigation` implementation and tests;
- run the accepted baseline checks again;
- record the exact source commit/archive and validation result;
- decide whether PH-10 Scout integration and PH-11 hardening are complete prerequisites or whether specific deep-zoom changes are formally added to those phases.

**Stop:** do not begin schema or renderer mutation while the required structural verifier is failed or its authority is unresolved.

## Existing-phase integration

### PH-02 — Visual regression

Add before behaviour changes:

- ordinary and current-limit deep fixtures for analytic quadratic and Tricorn;
- exact tile/global-coordinate fixtures;
- current D3D11/OpenGL/CPU output captures;
- an injected perturbation-instability fixture;
- a deliberate precision-plan mutation fixture.

### PH-03 — State and fingerprint

Extend only after exact-camera design acceptance:

- exact camera adapter;
- `mw-render-state-v2` or another explicitly versioned canonical form;
- formula capability and precision-plan identity;
- no direct write-back from approximate adapters.

### PH-06/PH-07 — Animation

- exact camera track values;
- deterministic exact scale interpolation;
- managed Journey conversion;
- no cumulative double stepping.

### PH-08/PH-09 — Export

- exact snapshot/plan in frame manifest;
- source-frame validity summary;
- preserve current FFmpeg boundary;
- no GPU deep-frame claim until equivalence validation passes.

### PH-10 — Scout

- exact candidate promotion and fingerprint;
- generation-safe cancellation;
- preserve one-action Apply history.

### PH-11 — Integration hardening

- deep device loss, monitor lifecycle, long-session resource soak and migration/downgrade evidence.

# PH-12 — Baseline Repair and Exact Camera Foundation

**Status:** Proposed  
**Objective:** Establish a green baseline and exact durable camera authority without replacing the production renderer.

## Prerequisites

- PH-00 prerequisite above passed;
- current visual fixtures captured;
- ID ranges accepted;
- DEC-015 precision-intent ownership resolved or explicitly bounded for this phase;
- exact-number backend spike authorised.

## Tasks

1. Reconcile `verify-source.py`, AppWindow gesture routes and tests.
2. Inventory every camera read/write/parse/format/persist/fingerprint path.
3. Define bounded exact numeric syntax and canonicalisation.
4. Implement exact centre and exact half-height types behind `src/Core/Precision`.
5. Add exact-to-current `CameraState` adapters with explicit loss reporting.
6. Add exact camera to project/preset candidate state without adding a second mutable authority.
7. Update coordinate entry, Copy Coordinates and high-resolution render entry.
8. Update built-in/user preset Save As cloning.
9. Add managed settings/preset migration with original-file preservation.
10. Extend render fingerprint version; retain v1 reader/identity where required.
11. Extend Journey, timeline and Scout camera adapters.
12. Add unit, parser, migration, round-trip and downgrade tests.
13. Keep existing renderers operating through the compatibility adapter.

## Proposed files

Adapt naming to current structure after inspection:

```text
src/Core/Precision/ExactDecimal.*
src/Core/Precision/ExactCamera.*
src/Core/Precision/CameraAdapter.*
src/Core/Models.*
src/Core/ProjectState.*
src/Core/SettingsStore.*
src/Core/GeneralAnimation.*
src/Core/FractalScout.*
src/App/AppWindow.*
src/App/HighResRenderDialog.*
tests/ExactCameraTests.* or existing CoreTests.cpp sections
```

## Acceptance

`AC-021`–`AC-027`

## Validation

`VAL-037`–`VAL-044`

## Rollback point

- preserve original settings/preset files;
- no automatic rewrite merely from opening;
- exact fields can be feature-disabled while legacy values remain readable;
- do not drop exact fields on save through an older compatibility path without a visible warning.

## Stop conditions

- exact text passes through `double` before canonicalisation;
- exact and approximate cameras can both be independently edited;
- migration can overwrite the only valid original;
- fingerprint omits exact camera;
- an unsupported exponent/input can allocate unbounded memory;
- backend/library decision lacks licence or Windows packaging evidence.

# PH-13 — Central Precision Planner and Orbit Service

**Status:** Proposed  
**Objective:** Remove renderer-local policy and synchronous orbit generation while preserving current visual behaviour.

## Prerequisites

- PH-12 passed;
- exact camera fixtures stable;
- current D3D11/OpenGL capability reporting inspected;
- arbitrary-precision backend decision accepted.

## Tasks

1. Define formula capability fingerprints for current analytic quadratic and Tricorn profiles.
2. Implement central deterministic precision planner.
3. Replace D3D11/OpenGL local thresholds with plan consumption.
4. Capture planner version/reason in fingerprints and diagnostics.
5. Extract current fixed-point/reference logic behind interfaces.
6. Implement immutable orbit requests/results and typed keys.
7. Add bounded cancellable precision worker pool.
8. Add request coalescing, byte-accounted cache and eviction.
9. Add generation IDs and stale-result rejection.
10. Version the current four-float orbit encoding and measure its ceiling.
11. Upload completed matching orbits on the render thread.
12. Add backend capability/self-test inputs.
13. Preserve ordinary-zoom output within fixture tolerance.

## Proposed files

```text
src/Core/Precision/PrecisionIntent.*
src/Core/Precision/PrecisionPlan.*
src/Core/Precision/PrecisionPlanner.*
src/Core/DeepZoom/FormulaCapability.*
src/Core/DeepZoom/ReferenceOrbitService.*
src/Core/DeepZoom/OrbitCache.*
src/Core/DeepZoom/OrbitEncoding.*
src/Rendering/Direct3D11Renderer.*
src/Rendering/OpenGLRenderer.*
```

## Acceptance

`AC-028`–`AC-032`

## Validation

`VAL-045`–`VAL-050`

## Rollback point

- retain a compatibility planner profile reproducing accepted 1.13.1 thresholds for ordinary/current-limit fixtures;
- allow the new orbit service to be disabled in favour of current synchronous generation during development only;
- do not retain two production planner authorities.

## Stop conditions

- either renderer still invents an unregistered threshold/fallback order;
- stale results can upload;
- cancellation cannot prevent new orbit work;
- cache has no enforceable byte bound;
- formula profile uses a human-readable summary as the only key;
- reported precision conflates reference and complete render precision.

# PH-14 — Perturbation Validity, Rebase and Multi-Reference

**Status:** Proposed  
**Objective:** Turn current single-reference perturbation from a best-effort colour path into a validated, correct-or-fail deep-render path.

## Prerequisites

- PH-13 passed;
- direct high-precision reference samples/tiles exist;
- natural and injected difficult fixtures exist;
- GPU readback/validity evidence path exists.

## Tasks

1. Define shared recurrence/index/bailout/smoothing contracts.
2. Add validity/error output to D3D11 and OpenGL perturbation paths.
3. Replace silent `directSplit` instability acceptance with classified handling.
4. Implement deterministic rebase policy with hysteresis.
5. Implement bounded direct high-precision pixel/tile correction.
6. Add deterministic tile subdivision.
7. Add bounded multi-reference atlas and assignment by predicted error.
8. Record reference/correction/unresolved metrics.
9. Fail frames with unresolved pixels beyond policy.
10. Add device-loss-safe reference resource rebuild.
11. Add interactive last-valid-frame behaviour during orbit replacement.
12. Calibrate limits from fixtures rather than hard-coded unsupported claims.

## Proposed files

```text
src/Core/DeepZoom/PerturbationValidity.*
src/Core/DeepZoom/RebasePolicy.*
src/Core/DeepZoom/CorrectionScheduler.*
src/Core/DeepZoom/ReferenceAtlas.*
src/Core/DeepZoom/TileReferencePlanner.*
src/Rendering/DeepZoomGpuContracts.*
D3D11/OpenGL shader contract changes
```

## Acceptance

`AC-033`–`AC-037`

## Validation

`VAL-051`–`VAL-056`

## Rollback point

Independently disable:

- speculative prefetch;
- multi-reference;
- direct correction;
- new orbit encoding;

while retaining exact camera, central planning and the last accepted single-reference path. A disabled correction path must reduce claimed support rather than silently accept invalid pixels.

## Stop conditions

- validity is inferred only from final colour;
- correction/reference count can grow without bound;
- unresolved pixels are hidden;
- rebase changes exact camera;
- tile/reference assignment depends on worker completion order;
- D3D11/OpenGL recurrence semantics diverge without a documented capability split.

# PH-15 — Deep Export and Integration Hardening

**Status:** Proposed  
**Objective:** Extend validated deep rendering through still, frame, video, Scout, desktop and lifecycle workflows and publish measured limits.

## Prerequisites

- PH-14 passed for intended formula profiles;
- full/tile equivalence fixtures passed;
- source-frame validity contract accepted;
- current PH-08/PH-09 safety contracts retained.

## Tasks

1. Add exact snapshot/plan identity to still and frame jobs.
2. Add preflight resource and temporary-storage checks.
3. Add validated GPU deep-tile still path and CPU reference fallback.
4. Decide whether deterministic deep frames remain CPU canonical or allow a validated GPU class.
5. Add deep-frame manifest validity/reference/correction metadata.
6. Preserve FFmpeg fixed-vector, probe and atomic-promotion behaviour.
7. Integrate exact Journey/timeline/Scout workflows.
8. Add concurrent preview/wallpaper/export arbitration.
9. Add sleep, lock, RDP, monitor, Explorer and device-loss recovery.
10. Run long-session zoom and resource soak.
11. Run migration/upgrade/downgrade and corrupt-input fixtures.
12. Complete accessibility and diagnostics review.
13. Publish `DEEP-ZOOM-SUPPORT-MATRIX.md` with measured limits.
14. Update release, troubleshooting and user documentation.

## Proposed files

```text
src/Core/StillImageRenderer.*
src/Core/FrameSequenceExport.*
src/Core/ExternalVideoExport.*
src/App/HighResRenderDialog.*
src/App/FrameSequenceExportDialog.*
src/App/VideoExportDialog.*
src/WindowsIntegration/* affected lifecycle owners
docs/testing/DEEP-ZOOM-FIXTURES.md
docs/reports/DEEP-ZOOM-SUPPORT-MATRIX.md
```

## Acceptance

`AC-038`–`AC-040` plus all prior deep criteria

## Validation

`VAL-057`–`VAL-060` plus cumulative regression

## Rollback point

- preserve accepted frame-sequence and FFmpeg formats/readers;
- retain source frames on any encode failure;
- keep exact camera files readable even when deep GPU export is disabled;
- disable only the failing backend/profile through capability policy.

## Stop conditions

- incomplete or invalid frames can reach FFmpeg;
- output promotion occurs before validation;
- migration or downgrade can silently discard exact values;
- resource soak grows with session duration;
- native D3D11/OpenGL/Windows evidence is absent;
- support claims exceed measured evidence.


---

## FILE: `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md`

# Deep-Zoom Validation and Traceability

**Status:** Proposed  
**Purpose:** Own acceptance criteria, validation IDs, fixtures and REQ-to-phase mapping for the upgrade  
**Owner:** Validation and governance  
**Reading trigger:** Implementation, test, phase completion, release or support claim

## Evidence vocabulary

- **Inspected:** directly read in source or generated artifact.
- **Ran:** command or check executed.
- **Passed/Failed:** result of that exact check only.
- **Verified:** a claim supported by the required checks across its defined scope.
- **Unproven:** required evidence not yet produced.

No unit, token or source-marker check proves native GPU, UI, desktop integration or real encoder behaviour.

## Acceptance criteria

| ID | Criterion |
|---|---|
| `AC-021` | Exact centre and scale parse and canonicalise without binary floating-point conversion. |
| `AC-022` | Copy → save → load → copy returns identical canonical exact camera values. |
| `AC-023` | Existing schema-9 settings and preset-schema-2 files load without destructive camera change. |
| `AC-024` | Legacy numeric values are identified as migrated exact representations of their stored binary values, not falsely reconstructed original decimal text. |
| `AC-025` | Exact and approximate camera representations cannot be independently mutated. |
| `AC-026` | Built-in Save As creates a new editable user preset retaining complete exact state. |
| `AC-027` | Equivalent exact values fingerprint identically; any render-affecting exact digit change alters the v2 fingerprint. |
| `AC-028` | Identical planner inputs produce the same plan, reason and fallback order. |
| `AC-029` | D3D11 and OpenGL consume the central plan and do not retain independent policy thresholds. |
| `AC-030` | Formula profiles accept only their defined recurrence fingerprints. |
| `AC-031` | Reference requests are cancellable, bounded and stale results cannot upload. |
| `AC-032` | Diagnostics distinguish camera, reference, orbit-upload and delta precision. |
| `AC-033` | Supported perturbation samples/tiles agree with direct high-precision references within accepted criteria. |
| `AC-034` | Non-finite, unstable and injected-invalid perturbation output is classified before final acceptance. |
| `AC-035` | Rebase and correction preserve exact camera and do not create an unexplained visible discontinuity. |
| `AC-036` | Multiple-reference planning terminates within configured reference, subdivision, correction, retry and time budgets. |
| `AC-037` | Unresolved pixels beyond policy fail the frame/job rather than being silently coloured. |
| `AC-038` | Full-frame, tiled and reordered-tile output use equivalent global sample mapping and approved seam tolerance. |
| `AC-039` | Cancellation, device loss, exit, write failure and FFmpeg failure cannot promote incomplete final output. |
| `AC-040` | Release documentation reports measured profile/backend/depth limits and contains no literal infinite/unlimited claim. |

## Validation catalogue

| ID | Validation | Required evidence |
|---|---|---|
| `VAL-037` | Baseline structural-gate reconciliation | `verify-source.py` and accepted baseline checks green on the same source state |
| `VAL-038` | Exact numeric syntax/canonicalisation | malformed, equivalent, extreme exponent, negative zero and length-bound tests |
| `VAL-039` | Exact camera round trip | UI text, clipboard, settings, preset and render-job canonical equality |
| `VAL-040` | Legacy settings/preset migration | schema 1–9/current fixtures, preset schema fixtures, original preservation and deterministic defaults |
| `VAL-041` | Single-authority audit | all camera writes enumerated; exact authority only; adapters one-way |
| `VAL-042` | Exact render fingerprint v2 | stable bytes/hash, positive/negative zero equivalence, exact-digit mutation detection |
| `VAL-043` | Built-in/user preset ownership | immutable built-in, Save As identity, collision and persistence tests |
| `VAL-044` | Journey/timeline/Scout exact adapters | supported round trips, explicit lossy rejection and stale result rejection |
| `VAL-045` | Precision planner table | formula, depth, output, device and resource matrix with deterministic expected plans |
| `VAL-046` | Backend policy removal audit | no unregistered D3D11/OpenGL thresholds or fallback order |
| `VAL-047` | Formula capability fingerprint | analytic quadratic, Tricorn and unsupported mutation matrix |
| `VAL-048` | Reference-orbit numerical comparison | selected orbit samples against accepted high-precision reference |
| `VAL-049` | Orbit cancellation/generation races | cancellation, duplicate requests, stale completion, shutdown and allocation failure |
| `VAL-050` | Orbit cache/resource accounting | byte bound, eviction, reuse, upload lifetime and no growth by session duration |
| `VAL-051` | D3D11 perturbation comparison | WARP and hardware readback against direct reference with plan metadata |
| `VAL-052` | OpenGL perturbation comparison | supported driver readback against same semantic reference |
| `VAL-053` | Validity/glitch injection | NaN, overflow, relative-delta, classification disagreement and forced error masks |
| `VAL-054` | Rebase/correction continuity | camera identity, boundary continuity and corrected classification |
| `VAL-055` | Multi-reference determinism | reference choice/order independent of worker/tile completion order |
| `VAL-056` | Correction/resource termination | reference/subdivision/pixel/retry/time budgets and explicit unresolved failure |
| `VAL-057` | Global mapping/tile seams | full, tiled, reversed order, odd dimensions, portrait, ultrawide, rotation, AA and halo |
| `VAL-058` | Deterministic animation and frame timing | exact timestamp seek, pause/resume, long Journey and frame-index timing |
| `VAL-059` | Failure/lifecycle matrix | cancellation, device loss, sleep, lock, RDP, monitor, Explorer, disk full and shutdown |
| `VAL-060` | Deep still/frame/FFmpeg release matrix | source-frame validity, WIC output, manifests, real FFmpeg H.264/MP4, decode probe and support report |

## Numerical comparison rules

### Exact equality

Use canonical exact-value equality for:

- camera centre and scale;
- deterministic time inputs where represented exactly;
- formula capability fingerprints;
- precision-plan identity;
- render fingerprints and frame manifests.

### Orbit comparison

For selected iterations record:

- reference real/imaginary absolute and relative error;
- escaped/interior classification;
- escape iteration;
- reference precision and encoded precision;
- backend/device/contract version.

Thresholds are fixture-specific and must be accepted from measured evidence. Do not use one vague “looks correct” threshold.

### Pixel classification

For release fixtures:

- escaped/interior disagreement is zero except for explicitly documented boundary-tolerance fixtures;
- every corrected pixel records its path;
- unresolved pixels are a failure unless the test specifically validates failure reporting;
- colour comparison cannot override a classification mismatch.

### Colour comparison

Define and record:

- colour space and gamma assumptions;
- maximum per-channel error;
- percentile errors;
- RMS error;
- structural similarity or edge metric where useful;
- backend/device metadata.

### Seam comparison

Compare boundary strips for:

- classification;
- smooth iteration;
- orbit-derived data used by colouring;
- final colour after halo crop.

Tile order and worker count must not change deterministic export output.

## Fixture catalogue

| Fixture | Purpose |
|---|---|
| `DZ-FIX-001` | Default Mandelbrot ordinary view |
| `DZ-FIX-002` | Current Seahorse Valley fixture with compensated centre |
| `DZ-FIX-003` | Analytic quadratic near current `1e-30` limit |
| `DZ-FIX-004` | Exact camera deeper than current double-scale product cap |
| `DZ-FIX-005` | Exact power-2 Tricorn current deep fixture |
| `DZ-FIX-006` | Tricorn deeper than current product cap |
| `DZ-FIX-007` | Long filament/high-iteration region |
| `DZ-FIX-008` | Periodic or near-critical region |
| `DZ-FIX-009` | Natural single-reference instability region |
| `DZ-FIX-010` | Injected non-finite/error-mask fixture |
| `DZ-FIX-011` | Multi-reference stress frame |
| `DZ-FIX-012` | Direct-correction budget boundary |
| `DZ-FIX-013` | Unsupported Julia fallback/refusal |
| `DZ-FIX-014` | Unsupported Burning Ship/absolute formula |
| `DZ-FIX-015` | Odd-size portrait tiled output |
| `DZ-FIX-016` | Ultrawide rotated tiled output with bloom halo |
| `DZ-FIX-017` | Long deterministic continuous-zoom timeline |
| `DZ-FIX-018` | Pause/resume and arbitrary timestamp seek |
| `DZ-FIX-019` | Scout candidate promote/apply exact identity |
| `DZ-FIX-020` | Device-loss/cancelled export recovery |

Every fixture records:

- exact camera;
- formula capability fingerprint;
- dimensions, rotation, AA and post-processing;
- iteration plan;
- deterministic time and seed;
- precision plan and versions;
- reference/orbit encoding details;
- backend, adapter, driver and device class;
- comparison mode and thresholds;
- baseline provenance and digest.

## Requirement traceability

| Requirement | Scope/owner | Phase | Acceptance | Validation |
|---|---|---|---|---|
| `REQ-021` | Exact camera/persistence | `PH-12` | `AC-021`–`AC-024` | `VAL-038`–`VAL-040` |
| `REQ-022` | State authority | `PH-12` | `AC-025` | `VAL-041` |
| `REQ-023` | Fingerprint/export identity | `PH-12`, `PH-15` | `AC-027` | `VAL-042`, `VAL-060` |
| `REQ-024` | Precision planner | `PH-13` | `AC-028`, `AC-029` | `VAL-045`, `VAL-046` |
| `REQ-025` | Intent/capability separation | `PH-12`, `PH-13` | `AC-028`, `AC-032` | `VAL-041`, `VAL-045` |
| `REQ-026` | Existing profile preservation | `PH-13`, `PH-14` | `AC-030`, `AC-033` | `VAL-047`, `VAL-051`, `VAL-052` |
| `REQ-027` | Fail-closed formulas | `PH-13` | `AC-030` | `VAL-047` |
| `REQ-028` | Orbit service | `PH-13` | `AC-031` | `VAL-048`–`VAL-050` |
| `REQ-029` | Generation safety | `PH-13`, `PH-15` | `AC-031`, `AC-039` | `VAL-049`, `VAL-059` |
| `REQ-030` | Precision reporting | `PH-13` | `AC-032` | `VAL-045`, `VAL-048` |
| `REQ-031` | Validity classification | `PH-14` | `AC-034`, `AC-037` | `VAL-053`, `VAL-056` |
| `REQ-032` | Rebase/correction | `PH-14` | `AC-035` | `VAL-054` |
| `REQ-033` | Multi-reference | `PH-14` | `AC-036` | `VAL-055`, `VAL-056` |
| `REQ-034` | Global mapping | `PH-12`, `PH-15` | `AC-038` | `VAL-057` |
| `REQ-035` | Resource bounds | `PH-13`–`PH-15` | `AC-031`, `AC-036`, `AC-039` | `VAL-050`, `VAL-056`, `VAL-059` |
| `REQ-036` | Ordinary regression safety | all | `AC-028`, `AC-033`, `AC-038` | existing VAL-009–VAL-013 plus `VAL-051`, `VAL-052`, `VAL-057` |
| `REQ-037` | Deterministic zoom/animation | `PH-12`, `PH-15` | `AC-022`, `AC-038` | `VAL-044`, `VAL-058` |
| `REQ-038` | Failure-safe output | `PH-15` | `AC-039` | `VAL-059`, `VAL-060` |
| `REQ-039` | Diagnostics | `PH-13`–`PH-15` | `AC-032`, `AC-037` | all deep validations include metadata audit |
| `REQ-040` | Measured claims | `PH-15` | `AC-040` | `VAL-060` and support-matrix audit |

## UI route traceability

| Route | Flow | Exact state | Fallback/failure | Validation |
|---|---|---|---|---|
| `ROUTE-015` | Edit/Copy exact coordinates | exact project camera | parse refusal; no partial commit | `VAL-038`, `VAL-039`, `VAL-041` |
| `ROUTE-016` | Configure precision and inspect plan | persisted intent + transient plan | safe plan or explicit unsupported reason | `VAL-045`–`VAL-047` |
| `ROUTE-017` | Continuous/Journey/Scout deep navigation | exact camera + generation | last valid frame, cancellation, stale discard | `VAL-044`, `VAL-049`, `VAL-058` |
| `ROUTE-018` | Deep still/frame/video export | immutable exact job | temporary output, preserve source frames, no promotion | `VAL-057`, `VAL-059`, `VAL-060` |

## Phase evidence rule

A phase report must include:

- exact source state and environment;
- files inspected and changed;
- REQ/AC/VAL scope;
- commands run and complete outputs or artifact locations;
- passed, failed, skipped and unproven items;
- numerical thresholds and fixture provenance;
- resource metrics;
- migration/rollback result;
- canonical governance updates.


---

## FILE: `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md`

# Deep-Zoom Risks, Decisions and Migration

**Status:** Proposed  
**Purpose:** Record cross-cutting choices, unresolved architecture points, risks, schema migration and rollback  
**Owner:** Architecture, security and persistence  
**Reading trigger:** Dependency, schema, precision, renderer, compatibility, resource or release change

## Proposed decision register

| ID | State | Decision | Consequence |
|---|---|---|---|
| `DEC-028` | Proposed | Exact camera centre and half-height become canonical project state; `CameraState` is a derived compatibility/render adapter. | Prevents irrecoverable precision loss and competing camera authorities. |
| `DEC-029` | Open | Select the high-precision backend after a bounded PH-12 spike: extract/extend current fixed-point code or adopt a reviewed library. | Affects licence, MSVC packaging, parsing, performance, testing and maintenance. |
| `DEC-030` | Proposed | One platform-neutral precision planner owns algorithm and fallback selection. | D3D11/OpenGL report capabilities but do not own thresholds. |
| `DEC-031` | Proposed | Preserve both current perturbation capabilities as versioned profiles: analytic quadratic and exact power-2 Tricorn. | Avoids regression and avoids falsely narrowing the existing source. |
| `DEC-032` | Proposed | Treat the current four-float orbit upload as a named versioned encoding with a measured limit, not as end-to-end 512-bit rendering. | Makes precision claims truthful and allows later encoding migration. |
| `DEC-033` | Proposed | Reference generation becomes an immutable cancellable service with generation-safe results and bounded caching. | Removes render-thread setup ownership and stale-result risk. |
| `DEC-034` | Proposed | Invalid perturbation output must be rebased, directly corrected or rejected; direct split-float colour fallback alone is not correctness evidence. | Prevents plausible silent corruption. |
| `DEC-035` | Proposed | Add multiple references only after single-reference validity/correction passes; keep reference/correction growth bounded and deterministic. | Controls complexity and resource risk. |
| `DEC-036` | Open | Decide deterministic deep-frame authority: remain CPU-reference-only or allow a separately classified validated GPU path. | Affects frame manifests, reproducibility and export performance. Default: retain CPU canonical output until GPU equivalence passes. |
| `DEC-037` | Proposed | Release claims use measured profile/backend/depth limits and prohibit literal infinite/unlimited language. | Aligns product wording with evidence. |

## Decision required — high-precision backend

### Option A — Extract and extend current fixed-point engine

**Advantages**

- no new runtime/package dependency;
- existing recurrence code and tests can be reused;
- deterministic integer-limb behaviour under project control.

**Costs/risks**

- exact decimal parser and exponent handling must be built and audited;
- multiplication, rounding, overflow, cancellation and performance remain project-maintained;
- current implementation is capped, embedded and converts orbit state for bailout/upload;
- higher maintenance and numerical-correctness burden.

### Option B — Adopt a reviewed arbitrary-precision library

**Advantages**

- mature parsing and arithmetic;
- less bespoke numerical code;
- easier expansion beyond 512 bits if the selected type supports it.

**Costs/risks**

- licence and distribution review;
- MSVC/x64 build and installer impact;
- binary or header size;
- deterministic configuration and cancellation integration;
- ongoing security/maintenance responsibility.

### Default criterion

Select the least-complex option that passes:

- exact decimal round trip;
- analytic quadratic and Tricorn reference fixtures;
- cancellation checkpoints;
- native MSVC Release build;
- acceptable startup/package effect;
- measured reference-generation performance;
- licence/distribution review.

Do not select based only on benchmark speed.

## Risk register

| ID | Risk | Severity | Containment |
|---|---|---:|---|
| `RISK-013` | Long decimal coordinates are rounded before entering exact state. | Critical | Exact parser; prohibit initial `double` conversion; round-trip tests. |
| `RISK-014` | Exact and current camera fields become independently mutable. | Critical | One exact authority; one-way adapters; single-authority audit. |
| `RISK-015` | Scale remains a double/float bottleneck after exact centre migration. | Critical | Exact half-height; versioned shader delta/scale representation; depth fixtures. |
| `RISK-016` | “512-bit” UI wording overstates four-float GPU orbit/delta accuracy. | High | Separate precision fields; measured encoding limits; revised UI wording. |
| `RISK-017` | D3D11 and OpenGL choose different semantics for identical state. | High | Central plan; backend policy audit; captured plan identity. |
| `RISK-018` | Unsupported equation enters the wrong perturbation recurrence. | Critical | Versioned formula capability fingerprint and fail-closed planner. |
| `RISK-019` | Synchronous high-precision orbit generation stalls rendering/UI. | High | Cancellable bounded worker service and last-valid-frame presentation. |
| `RISK-020` | A stale orbit/correction result overwrites current generation. | Critical | Generation/fingerprint checks before upload/commit. |
| `RISK-021` | Single-reference instability returns plausible incorrect split-float colour. | Critical | Validity mask, direct reference fixtures, rebase/correction or rejection. |
| `RISK-022` | Multi-reference/correction expands without bound. | High | Explicit reference, pixel, subdivision, retry, memory and time budgets. |
| `RISK-023` | Tile camera derivation diverges from global exact mapping. | High | Exact global sample fixtures, reversed tile order and seam strips. |
| `RISK-024` | Migration or downgrade discards exact values. | Critical | Original preservation, versioned fields, downgrade warning/read-only policy. |
| `RISK-025` | Existing ordinary views regress while deep paths improve. | High | Freeze current fixtures; compatibility planner profile; cumulative visual checks. |
| `RISK-026` | CPU “reference” path remains ordinary double and validates the wrong result. | Critical | Independent direct high-precision renderer/samples for deep fixtures. |
| `RISK-027` | Orbit/cache/temporary storage grows with session duration. | High | Byte accounting, eviction, cancellation, disk reserve and soak tests. |
| `RISK-028` | Device loss or cancellation promotes partial still/frame/video output. | Critical | Existing temp/promotion rules extended to validity-aware frames; fault injection. |
| `RISK-029` | Static verification and current source remain contradictory. | High | Repair baseline gate before PH-12; record exact result. |
| `RISK-030` | Product copy claims infinite support beyond measured evidence. | High | Support matrix, release review and prohibited wording. |

## Data migration

### Current observed formats

- `AppSettings` schema: 9.
- Preset serialisation schema: 2.
- Camera fields: numeric `centreX`, `centreY`, `scale`, `centreXLow`, `centreYLow`.
- Journey rows and several UI flows parse decimal text into doubles.
- `mw-render-state-v1` fingerprints exact current IEEE-754 bit patterns, not arbitrary decimal camera state.

### Proposed exact fields

Names are provisional until the canonical data owner accepts them:

```json
{
  "camera": {
    "centreXExact": "-0.743643887037151007923...",
    "centreYExact": "0.131825904205311970493...",
    "halfHeightExact": "1e-120",
    "legacy": {
      "centreX": -0.743643887037151,
      "centreXLow": 2.6e-17,
      "centreY": 0.13182590420533,
      "centreYLow": -1.1e-17,
      "scale": 1e-32
    }
  }
}
```

Do not assign the next schema numbers until accepted. The migration requires new schema versions because exact string fields change durable semantics.

### Legacy-to-exact conversion

For legacy values:

1. read and validate the stored high/low/scale numbers under the existing schema;
2. reconstruct the exact mathematical value of the stored IEEE-754 fields, not the unknown original user text;
3. canonicalise that exact binary value into the new exact representation;
4. mark migration provenance as legacy binary conversion where diagnostics require it;
5. retain the original file until the migrated data validates and a failure-safe save succeeds.

Do not claim restoration of digits that were never stored.

### Exact-to-legacy compatibility

When an old path or version cannot represent the exact camera:

- do not silently overwrite exact fields with a rounded value;
- use read-only compatibility, explicit export-as-approximation or visible confirmation;
- record the approximation and precision loss;
- preserve a backup before any destructive downgrade.

### Preset migration

- built-in presets remain immutable catalogue data;
- user Save As generates a new stable ID and copies exact camera, equation, palette, animation and precision intent;
- imported legacy presets gain no built-in privilege;
- name collisions do not merge identities;
- invalid exact fields fail without partially importing a preset.

### Journey/timeline migration

- extend syntax or structured storage only under an accepted versioned contract;
- reject lossy conversion when exact values cannot be preserved;
- keep runtime-only timeline ownership under DEC-014 until separately changed;
- deterministic evaluation version is included in export identity.

### Render fingerprint migration

- retain `mw-render-state-v1` for historical/current artifact recognition;
- introduce a new canonical version for exact camera and precision-plan identity;
- never reinterpret v1 bytes as v2;
- export manifests declare the fingerprint version explicitly;
- resume refuses mixed versions unless a specific tested compatibility rule exists.

## Rollback procedure

For each phase:

1. record the pre-change commit/archive and data schema.
2. preserve representative settings, presets, Journey rows, manifests and outputs.
3. run migration in memory before writing.
4. write to temporary files and validate.
5. promote only after validation.
6. on failure, retain the original and report the rejected migration.
7. keep backend/profile feature switches narrow enough to disable a failing execution path without discarding exact data.
8. never roll back by silently deleting new exact fields.

## Security and privacy additions

- exact numeric text remains bounded data, never shader source or command text;
- cap digit count, exponent magnitude, parse time and resulting precision allocation;
- do not log full exact coordinates by default;
- do not include exact coordinates in crash/report output without explicit user action;
- keep FFmpeg arguments application-owned; exact metadata must not become raw command arguments;
- cache and temporary files remain local and application-owned;
- clean only files proven to belong to the active job.

## Stop conditions

Stop schema or release work when:

- next schema ownership/version is unresolved;
- high-precision dependency licence or packaging is unresolved;
- exact values can be rounded by a hidden compatibility save;
- formula capability lacks a stable version;
- resource bounds are advisory rather than enforced;
- source verifier and required baseline checks disagree without a recorded resolution;
- native Windows evidence required by the affected phase is missing;
- release wording exceeds the support matrix.


---

## FILE: `CANONICAL_INTEGRATION_AND_HANDOFF.md`

# Canonical Integration and Handoff

**Status:** Proposed  
**Purpose:** Define how to integrate this supporting pack into the current governance without duplication  
**Owner:** Project governance and implementation lead  
**Reading trigger:** Repository execution, canonical document update or coding-assistant handoff

## Integration rule

Do not copy every section into every canonical file. Each definition is added once to its current owner; other files receive compact links/status mappings.

## Required canonical updates

### `/AGENTS.md`

Add only concise rules if not already covered:

- exact camera is authoritative once PH-12 accepts it;
- precision algorithms and thresholds come only from the central planner;
- unsupported or unresolved deep output fails closed;
- reference/correction work is generation-safe, cancellable and bounded;
- do not flatten exact camera into `double` in a path claiming exact/deep safety.

Keep the file below its existing size policy.

### `project_docs/PROJECT_INDEX.md`

Add:

- this pack as a supporting design alias;
- active deep phase pointer when accepted;
- links to the exact camera/precision/rendering owners;
- delivery ledger entry for pack integration;
- no duplicate requirement or decision definitions.

### `project_docs/PROJECT_FOUNDATION.md`

Add:

- scope text for exact camera and deep correctness;
- `REQ-021`–`REQ-040` definitions;
- `AC-021`–`AC-040` definitions or accepted compact ownership link if acceptance remains in validation owner;
- non-goal prohibiting literal infinite claims;
- formula support boundary.

### `project_docs/IMPLEMENTATION_PLAN.md`

Append `PH-12`–`PH-15` after confirming IDs are unused.

Each phase must contain:

- objective;
- prerequisites;
- required reading;
- expected state;
- tasks;
- proposed files;
- acceptance and validation IDs;
- governance updates;
- report;
- rollback point;
- stop conditions.

### `project_docs/TRACEABILITY.md`

Add:

- requirement mappings from this pack;
- phase statuses initially `Proposed`;
- `VAL-037`–`VAL-060` catalogue;
- `ROUTE-015`–`ROUTE-018` links;
- no phase marked passed without evidence.

### `project_docs/VALIDATION_AND_EVIDENCE.md`

Record:

- the baseline source-verifier failure and its resolution;
- portable build/test evidence separately from native evidence;
- exact-camera, planner, orbit, GPU validity, resource and export evidence sections;
- numerical thresholds and fixture provenance;
- support-matrix evidence boundary.

### `project_docs/SECURITY_PRIVACY_AND_RISK.md`

Add `RISK-013`–`RISK-030` and controls for:

- bounded exact numeric input;
- precision allocation;
- local cache/temp data;
- exact-coordinate logging privacy;
- stale work and invalid output;
- migration/downgrade;
- resource and output safety.

### `project_docs/DECISIONS_AND_CHANGE_HISTORY.md`

Add `DEC-028`–`DEC-037` with accepted/open status and full decision records for:

- exact camera authority;
- high-precision backend;
- central planner;
- profile preservation;
- orbit encoding;
- orbit service;
- invalidity handling;
- multi-reference sequencing;
- deterministic frame authority;
- measured claims.

Resolve DEC-015 before durable precision-intent migration. Preserve DEC-026 and DEC-027 unless explicitly superseded.

### `project_docs/DATA_AND_PERSISTENCE.md`

Add:

- current schema observations;
- exact field ownership;
- legacy binary-to-exact migration semantics;
- failure-safe save and original preservation;
- downgrade/read-only policy;
- fingerprint version migration;
- no invented next schema number before acceptance.

### `project_docs/UI_WORKFLOWS_AND_ROUTES.md`

Add:

- `ROUTE-015` exact coordinate edit/copy;
- `ROUTE-016` precision intent/plan diagnostics;
- `ROUTE-017` deep navigation/Journey/Scout;
- `ROUTE-018` deep still/frame/video export.

Map screen → state → permission → fallback → validation.

### `project_docs/DEBUGGING_AND_MAINTENANCE.md`

Add a deep-debug order:

1. exact camera canonical text/fingerprint;
2. formula capability and plan;
3. generation and orbit key;
4. orbit calculation/encoding;
5. shader validity output;
6. rebase/reference assignment/correction;
7. tile/global mapping;
8. output promotion/encoder;
9. device/lifecycle and resource metrics.

### `project_docs/PROJECT_SETTINGS.md`

Require the coding assistant to:

- inspect current camera/persistence/shader contracts before editing;
- use the central planner only;
- preserve exact values and current contracts unless a managed migration is requested;
- make narrow, reversible, testable changes;
- run the smallest relevant checks first, then cumulative checks;
- report numerical precision and evidence boundaries;
- update canonical governance and delivery evidence.

### `docs/architecture/PROJECT-STATE-AND-PARAMETERS.md`

Own:

- exact camera authority and compatibility adapter;
- parameter identity/value type for exact camera;
- mutation coordinator and history semantics;
- canonical fingerprint v2;
- animation/Scout adapter boundaries.

### `docs/architecture/RENDERING-AND-EXPORT-CONTRACTS.md`

Own:

- central precision plan;
- formula capability fingerprint;
- orbit request/result/cache/encoding;
- perturbation validity and correction;
- tile/global sample mapping;
- still/frame/FFmpeg boundaries;
- resource and cancellation contracts.

### Feature and testing documents

Update:

- `ANIMATION-TRACKS-PLAN.md`: exact camera interpolation and deterministic time;
- `OFFLINE-EXPORT-PLAN.md`: exact job/plan/validity manifest and deep-frame authority decision;
- `FRACTAL-SCOUT-STATUS.md`: exact candidate promotion and generation safety;
- `VISUAL-REGRESSION.md`: deep fixtures and numerical/classification comparison;
- `INTEGRATED-CREATIVE-ROADMAP.md`: PH-12–PH-15 summary and dependencies.

### `scripts/verify-source.py`

Before deep work:

- reconcile expected gesture markers with the accepted `PreviewNavigation` implementation;
- retain a source audit that catches direct camera writes after exact authority migration;
- add markers for central planner consumption and absence of backend-local thresholds;
- treat source checks as structural evidence only.

## Proposed source structure

Use current directories and adapt names after direct inspection:

```text
src/Core/Precision/
  ExactDecimal.*
  ExactCamera.*
  CameraAdapter.*
  PrecisionIntent.*
  PrecisionPlan.*
  PrecisionPlanner.*

src/Core/DeepZoom/
  FormulaCapability.*
  ReferenceOrbitService.*
  OrbitCache.*
  OrbitEncoding.*
  PerturbationValidity.*
  RebasePolicy.*
  CorrectionScheduler.*
  ReferenceAtlas.*
  TileReferencePlanner.*
```

A directory split is proposed for independent ownership. Do not move current files merely for style. Move only when the phase needs the ownership boundary and update CMake/tests atomically.

## Implementation sequencing constraints

- Exact camera precedes deeper shader arithmetic.
- Planner precedes renderer policy removal.
- Typed orbit service precedes asynchronous generation.
- Validity output precedes multi-reference.
- Direct high-precision reference precedes deep GPU acceptance.
- Preview/wallpaper correctness precedes export expansion.
- Frame validity precedes FFmpeg encoding.
- Native evidence precedes release claims.

## Agent task categories

### Camera/persistence

Read models, state, settings, UI parsers, Journey, timeline, Scout and fingerprint owners. Stop if schema/downgrade authority is missing.

### Planner/orbit

Read renderer capabilities, current thresholds, DeepZoom arithmetic and diagnostics. Stop if formula or precision contracts are ambiguous.

### GPU perturbation

Read both shaders, CPU reference, visual fixtures, validity contract and device-loss lifecycle. A change to one backend requires an explicit compatibility review of the other.

### Export

Read still, frame, WIC, manifest, process and FFmpeg owners. Preserve DEC-026/027 safety behaviour.

## Required handoff format

```markdown
## Mode
Repository execution

## Scope
Phase plus REQ/AC/VAL/RISK/DEC IDs.

## Inspected
Exact governance, source, shader, test, schema and evidence files.

## Changed
Files and contract changes; state whether schema/fingerprint changed.

## Ran
Exact commands and environment.

## Passed
Only checks that passed.

## Failed
Every failure and its effect.

## Skipped or unproven
Native/GPU/UI/manual/FFmpeg/device/resource boundaries.

## Numerical evidence
Camera/reference/upload/delta precision, fixtures and thresholds.

## Resource evidence
CPU, RAM, VRAM, cache, workers, disk and cancellation.

## Governance
Canonical owners and delivery report updated.

## Rollback
Commit/archive, feature gate, schema and data compatibility.
```

## Stop conditions for coding assistants

Stop before mutation when:

- repository/source state differs materially from the baseline and has not been re-audited;
- ID ranges conflict;
- exact camera authority or schema migration is unresolved;
- high-precision dependency authority is missing;
- a renderer-local threshold would be introduced;
- formula capability is inferred rather than proven;
- destructive migration/downgrade lacks approval;
- numerical reference evidence is unavailable;
- resource bounds or cancellation cannot be enforced;
- required native evidence is unavailable for a release claim.


---

## FILE: `AUDIT_REPORT.md`

# Pack Audit Report

**Audit level:** Batch  
**Status:** Passed for generated pack structure and internal mapping, subject to the unproven items below  
**Mode:** File generation

## Objective

Audit the generated Markdown pack against the supplied 1.13.1 source archive and proposal, checking source grounding, scope, phase sequence, IDs, ownership, contradictions, traceability and evidence boundaries.

## Inspected source areas

- root governance and project index;
- project foundation, implementation plan, traceability, decisions, risk, persistence and validation;
- camera, precision settings, schema validation and settings persistence;
- current DeepZoom fixed-point and perturbation code;
- D3D11/OpenGL precision selection, orbit upload and shader fallback;
- CPU still and GPU tiled high-resolution paths;
- animation/Journey, Scout, frame-sequence and external video paths;
- core tests, visual fixtures, CMake and source verifier.

## Commands run

1. Archive listing and path-safety check.
2. Safe extraction.
3. `python3 scripts/verify-source.py`.
4. Clean portable CMake Release configure with GNU 14.2 and warnings as errors.
5. Portable build.
6. CTest with failure output.

## Results

### Passed

- archive contained no unsafe absolute or parent-traversal paths;
- portable configuration and version consistency;
- portable core and fixture compilation/linking;
- CTest 4/4;
- pack file register complete;
- all proposed REQ, AC, VAL, DEC, RISK, ROUTE and PH IDs are unique within this pack;
- requirement mappings cover `REQ-021`–`REQ-040`;
- all proposed phases contain objective, prerequisites, tasks, acceptance, validation, rollback and stop conditions;
- source-present perturbation, Tricorn, tiled still, deterministic frame and FFmpeg capabilities are preserved rather than re-proposed as absent;
- exact camera, scale, planner, orbit service, validity, correction and multi-reference dependencies are sequenced in causal order;
- current FFmpeg safety decisions remain intact;
- no application code or canonical source files were modified.

### Failed baseline check

`scripts/verify-source.py` stopped at:

```text
PH-03 main-window mutation coordinator integration missing:
ParameterGestureKind::PreviewPan
```

The current AppWindow uses `ParameterGestureKind::PreviewNavigation` for preview pan and wheel routes. The enum/tests still contain older/specialised values. This may be an intentional consolidation with a stale verifier or an incomplete cleanup; the pack does not guess. PH-12 requires reconciliation and a green accepted baseline.

## Major contradictions resolved from the supplied proposal

1. **Perturbation already exists.** The pack changes from “add perturbation” to “make existing perturbation exact, generation-safe and correct-or-fail.”
2. **Tricorn already exists.** The pack preserves the exact power-2 Tricorn profile rather than excluding it.
3. **Arbitrary reference already exists.** The pack identifies its double-input, four-float-upload and float-scale ceilings.
4. **GPU tiled still output already exists.** The plan extends its exactness and validity rather than creating it from scratch.
5. **Frame/video export already exists.** The plan retains CPU deterministic frames and external FFmpeg safety until deep GPU equivalence is proven.
6. **Existing governance IDs are sequential.** The pack uses `REQ-021`, `AC-021`, `VAL-037`, `DEC-028`, `RISK-013` and `ROUTE-015` ranges instead of unrelated 200-series ranges.
7. **Current programme has PH-00–PH-11.** The pack appends PH-12–PH-15 and preserves existing status/evidence boundaries.
8. **Precision-policy ownership is open under DEC-015.** The pack requires resolution before durable migration rather than silently choosing an owner.

## Internal compatibility checks

- `PH-12` exact camera precedes planner/orbit changes.
- `PH-13` planner/orbit service precedes asynchronous/multi-reference work.
- `PH-14` validity and correction precede export claims.
- `PH-15` retains PH-08/PH-09 output safety.
- `REQ → phase → AC → VAL` mappings are present.
- UI routes map state, fallback and validation.
- migration does not invent lost original decimal digits.
- rollback preserves exact data and prior valid output.
- “infinite zoom” is consistently prohibited as an evidence claim.

## Source-grounding audit

The pack distinguishes:

- **Observed:** current source structures and bounds;
- **Ran/Passed/Failed:** exact checks executed for this task;
- **Proposed:** future architecture, IDs, phases and file names;
- **Open/Decision required:** precision ownership, high-precision backend and deep-frame authority;
- **Unproven:** native Windows/GPU/UI/FFmpeg and performance limits.

No native build, GPU output, exact migration, deep-depth performance or release support is claimed as verified.

## Remaining decisions

1. Resolve DEC-015: project versus user ownership of precision intent.
2. Select the high-precision backend under DEC-029.
3. Assign accepted next settings/preset schema versions.
4. Decide deep deterministic frame authority under DEC-036.
5. Establish measured thresholds for plan transitions, validity, rebase, correction and resources.
6. Decide whether PH-10/PH-11 must pass fully before PH-12 or whether specific dependencies are formally incorporated.

## Unproven items

- exact parser/library behaviour;
- migration from every real user settings/preset variant;
- practical depth beyond current `1e30` cap;
- orbit encoding accuracy ceiling;
- D3D11/OpenGL deep perturbation parity;
- direct high-precision renderer performance;
- multi-reference thresholds and termination under real workloads;
- long-session RAM/VRAM/cache stability;
- Windows UI responsiveness during orbit generation;
- GPU device-loss recovery;
- high-resolution WIC behaviour at extreme dimensions;
- real FFmpeg colour, frame-rate and dimension parity for deep output;
- installer/package impact of a precision dependency;
- release suitability.

## Pack completeness

| File | Result |
|---|---|
| `README.md` | Complete |
| `PACK_MANIFEST.md` | Complete |
| `SOURCE_ASSESSMENT_AND_DIRECTION.md` | Complete |
| `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md` | Complete |
| `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md` | Complete |
| `DEEP_ZOOM_BUILD_PLAN.md` | Complete |
| `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md` | Complete |
| `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md` | Complete |
| `CANONICAL_INTEGRATION_AND_HANDOFF.md` | Complete |
| `AUDIT_REPORT.md` | Complete |

## Final batch conclusion

The pack is internally consistent as a proposed source-reconciled upgrade foundation. It is not implementation evidence. Repository integration must begin by reconciling the failed structural verifier, confirming IDs and updating canonical owners through a managed governance change.
