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
