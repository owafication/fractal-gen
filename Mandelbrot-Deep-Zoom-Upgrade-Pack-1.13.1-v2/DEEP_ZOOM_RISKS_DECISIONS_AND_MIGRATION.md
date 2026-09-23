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
