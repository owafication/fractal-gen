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
