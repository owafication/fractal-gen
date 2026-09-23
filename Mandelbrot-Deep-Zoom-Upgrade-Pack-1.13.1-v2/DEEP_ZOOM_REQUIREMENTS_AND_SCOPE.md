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
