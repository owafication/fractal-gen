# Fractal Scout Status and Integration Plan

**Status:** PH-10 complete at the documented bounded/native scope; optional extensions and deep exact execution remain separately gated  
**Purpose:** Own current Scout capability boundary and avoid duplicate implementation  
**Owner:** Core discovery/UI  
**Reading trigger:** Scout search, candidate state, thumbnails, Apply/Save or future discovery work  
**Update trigger:** Search contract, scoring, resource limit, candidate identity or integration change  
**Linked IDs:** REQ-019, PH-10, AC-018, VAL-034–VAL-036

## Observed current capabilities

The 1.13.1 source includes `Core/FractalScout.*` and a Win32 Scout dialog. Public contracts show:

- deterministic bounded search requests and resolved limits;
- Balanced, Boundary, Filament and Symmetry goals;
- multi-scale search settings;
- candidate metrics and public deterministic score calculation;
- thumbnail rendering;
- progress and cancellation callbacks;
- near-duplicate suppression reporting;
- candidate presets returned without mutating the request preset.

Existing README and historical 1.12.4–1.12.6 records describe refinement, temporary preview and explicit Save as New behaviour. Runtime behaviour was not rerun for this pack.

## Integration status

- Candidate state is copied into the bounded request, so the worker operates on
  an immutable source preset/search-camera snapshot. The dialog retains results
  locally until a user selects one.
- Every returned candidate has a `FRACTAL-SCOUT-CANDIDATE/V1` SHA-256 identity
  derived from the canonical source and candidate render fingerprints, explicit
  search camera, goal and resolved limits. It excludes ranked-list position.
- Core regression coverage proves candidate identity, scores and thumbnails are
  deterministic for an identical request.
- Existing Scout Apply uses the shared mutation coordinator and records one
  labelled undoable camera transaction.
- The native `MandelbrotVisualFixtures` target now runs the same bounded Scout request twice, writes the three retained production thumbnails and their exact-camera V2 identities/metrics, and requires identical per-rank identity, score and pixels plus distinct non-uniform candidate images.
- BR-20260910-03 native interaction opens the real dialog, waits for a completed candidate selection, closes it and proves camera, preset identity and history remain unchanged. A second production search uses `Use in Preview`; exact before/after camera replay and unchanged preset identity pass through one `Apply Scout Camera` history entry.
- Map resource ceilings and cancellation into the common job-reporting policy.
- Preserve current deterministic ordering and seed behaviour.

## Non-goal

Do not replace the existing candidate engine, target scoring, multi-scale search or suppression logic unless direct inspection and measured evidence show the narrower adapter approach is insufficient.

## Optional later work

- style-only variations that keep camera/equation families constrained;
- curated equation-family exploration;
- lightweight search-session persistence;
- richer candidate comparison.

Each option requires a separate requirement and must preserve bounds, deterministic seed behaviour and preview isolation.

## Candidate identity

Proposed fingerprint inputs:

- source project/render fingerprint;
- search goal and resolved limits;
- deterministic seed/order version;
- candidate camera and relevant style state;
- scoring version.

Do not use list index as persistent candidate identity.

## Apply semantics

- Preview: temporary runtime state, no persisted mutation, no history entry.
- Apply: one undoable project transaction; saved preset identity remains explicit.
- Save as New: existing validated preset lifecycle; never overwrite a built-in entry.

## Validation

- identical request produces identical cameras, scores, ordering, suppression count and thumbnails in a pinned deterministic environment;
- cancellation discards partial candidate state safely;
- preview leaves canonical active/persisted state unchanged;
- Apply creates one reversible history entry;
- resource bounds remain within resolved limits.

## Proposed exact deep candidate boundary

Scout may retain bounded approximate search and scoring, but a candidate exposed for durable Apply/Save must be promoted to canonical exact camera text, formula capability and versioned render fingerprint. Candidate work carries a generation; cancellation or a newer request makes prior results ineligible to preview or Apply.

Preview remains temporary and history-free. Apply revalidates the candidate generation/fingerprint and commits one exact camera transaction; Save as New follows immutable built-in/new stable user identity rules. Deep thumbnail/reference work shares the central planner and resource budget rather than inventing Scout-specific precision thresholds.

PH-10 is complete at this documented scope. BR-20260730-04 adds the bounded
candidate identity contract and native build/core-test evidence. BR-20260908-01
adds the production thumbnail fixture and native visual-test evidence.
BR-20260910-03 adds native dialog isolation and Apply replay. None changes the
existing Scout engine or claims PH-12/PH-15, VAL-044, VAL-049 or VAL-058 evidence.

The PH-12 continuation rebuilds canonical exact camera text immediately after
the candidate camera changes, before candidate identity or thumbnail work. The
AppWindow Apply route carries that rebuilt exact value through the existing
single ScoutApply history transaction. Native ordinary-coordinate dialog
isolation and Apply interaction now pass; deep-planner execution remains unproven.

Candidate identity now uses `mw-render-state-v2-exact-camera` for source and
thumbnail state, and canonical exact search-camera text. This is a distinct
`FRACTAL-SCOUT-CANDIDATE/V2-EXACT-CAMERA` byte contract; it never reinterprets
prior candidate identities.
