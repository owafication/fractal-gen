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
