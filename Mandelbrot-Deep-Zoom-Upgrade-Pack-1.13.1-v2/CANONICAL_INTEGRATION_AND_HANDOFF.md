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
