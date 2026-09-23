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
