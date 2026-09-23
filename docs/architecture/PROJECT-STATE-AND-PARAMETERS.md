# Project State and Parameters

**Status:** Current state/parameter contract; bounded native history and Timeline interaction evidenced, future roots and deep integration separately gated  
**Purpose:** Own current state observations, future boundaries, parameter identity and mutation migration
**Owner:** Core architecture
**Reading trigger:** Models, UI mutation, persistence, undo, animation, Scout or renderer invalidation work
**Update trigger:** State authority, parameter type, object identity or mutation path change

## Current observed authorities

- `AppSettings` owns persisted user/application settings and custom assets.
- `Preset` contains most render-affecting scene state.
- `AppWindow::workingPreset_` is the editable preview copy.
- `WallpaperController` owns only file-backed desktop presentation state: decoded static/slideshow images or a selected local MP4 playback session. It does not own fractal scene or animation state.
- Renderer inputs are copied into `RenderRegion` and `RenderOptions`.
- Job dialogs create request/candidate state for high-resolution rendering and Scout.

This is workable for the current app but must be mapped before introducing general history/timelines.

## Implemented PH-03 foundation

`src/Core/ProjectState.h` and `.cpp` now provide:

- a bounded stable descriptor registry for 17 camera, palette and post-processing keys, including discrete `palette.selection`;
- explicit unknown-key failure through `ParameterKeyFromStableName`;
- value snapshots for compensated camera/rotation and selected palette/colour/post fields;
- explicit apply adapters that update only those fields on the caller-owned authoritative `Preset`;
- canonical render-state serialisation and SHA-256 fingerprinting, with unsupported enum rejection and the existing 4,096-stop custom-palette safety boundary preserved;
- production visual-fixture integration through `render-state.canonical` and `environment.json`;
- transactional `ApplyProjectParameterMutations` batches with descriptor type checks, finite-value rejection, model-owned normalisation, duplicate-key rejection and selective invalidation aggregation;
- one migrated main-window palette/post scalar route for brightness, contrast, saturation and colour offset;
- one deliberate main-window camera route for coordinate-triplet edits, separate centre/scale controls, Jump to Coordinates, Reset View and Scout Apply;
- one discrete main-window built-in palette-selection route that atomically clears custom stops only when the selection changes; PH-05 detects the implicit subtree change and records it atomically;
- explicit mutation-origin and future-history eligibility metadata for user controls, user gestures, Scout Apply, replay, loading/import, evaluation, migration and system/runtime origins.
- preview pan/wheel navigation coalescing with one shared token until a 500 ms inactivity boundary;
- classified whole-preset replacements for preset load, import, Palette Editor, Equation Editor, Settings and Journey Settings, with mismatch rejection and PH-05 atomic history integration.
- owner-associated Palette and Equation candidates that retain only their owned fields and run under one exclusive AppWindow edit session. The main window, Quick Controller and competing auxiliary routes are disabled until commit or cancel, so a second in-memory candidate cannot close later and restore a stale snapshot.

The adapters are value copies, not observable live models. The coordinator commits only requested fields back to the caller-owned `Preset`; it does not create a second mutable authority or change the persistence schema.

## Implemented PH-04/PH-05 history boundary

`ProjectHistory` records validated parameter deltas after the PH-03 coordinator commits them. It keeps no second project authority: scalar entries contain immutable before/after stable-key values and replay through `ApplyProjectParameterMutations` with `UndoRedo` origin.

PH-05 adds `PresetReplacementOperation` for structural and broad transactions. Each entry stores exact authoritative before/after `Preset` snapshots and replays them atomically. Before accepting a scalar entry, history reconstructs the committed result from its deltas and camera metadata; any mismatch falls back to one structural entry. This prevents implicit palette-stop deletion, mixed wider-domain changes or dialog/preset replacement from becoming partial history.

The bounded history includes compensated camera state, palette/post-processing scalars, custom palette stops, equation state, journey rows/text, preset/import application and accepted project dialogs. Drag, wheel and palette-thumb changes coalesce only when origin, gesture kind, token and target set match; structural entries never coalesce. Runtime/evaluation origins are rejected by eligibility policy.

The application exposes labelled Undo/Redo commands and Ctrl+Z/Ctrl+Y, resynchronises all affected main-window controls after replay, requests full render invalidation for structural entries and uses configurable hard containment bounds. History remains runtime-only and does not alter persistence schemas.

## Proposed state roots

```text
ProjectSnapshot
  artistic/render state
  compensated camera and rotation
  equation and project-owned precision
  palette/colouring/post-processing
  structured journey
  general animation timeline
  project schema

UserSettings
  output defaults
  desktop/startup defaults
  performance and UI preferences
  recent locations

RuntimeSession
  playback clocks
  active desktop mode
  GPU/Win32 resources
  workers/jobs/progress
  temporary preview/candidate state
  adaptive-performance state
```

## Adapter-first migration

1. Enumerate all reads/writes for the selected domain.
2. Keep current models authoritative.
3. Add an immutable snapshot/view adapter.
4. Add `ParameterRegistry` accessors for a narrow key set.
5. Route one domain's mutations through a coordinator.
6. Add validation, invalidation and round-trip evidence.
7. Remove old direct writes only after every caller is inspected.
8. Expand to the next domain.

Do not create a new independently mutable `ProjectSnapshot` beside `workingPreset_` and `AppSettings`.

## Parameter identity

Proposed shape:

```cpp
struct ParameterPath {
    ParameterDomain domain;
    ParameterKey key;
    std::optional<ObjectId> objectId;
    std::optional<ParameterKey> childKey;
};
```

Implemented initial keys:

- camera centre X high/low;
- camera centre Y high/low;
- camera scale;
- rotation;
- palette selection;
- palette offset;
- palette frequency;
- palette gamma;
- brightness;
- contrast;
- saturation;
- stripe strength;
- bloom strength/radius;
- edge-light strength.

Each implemented descriptor defines stable identity, value type, interpolation support, history eligibility and the smallest current invalidation class. The implemented coordinator applies proposed values to a temporary copy, uses `ValidateAndNormalise` as the range authority, then commits only requested normalised fields.

## Value types

A generic variant may contain bool, integer, double, colour, enum, string and stable ID, but deep camera values require an explicit compensated/high-precision representation. Never flatten high/low coordinates into one ordinary double in a path that claims deep-coordinate safety.

## Mutation coordinator

Responsibilities:

- read current value;
- validate/normalise proposed value;
- apply through the authoritative model;
- record origin and history eligibility;
- emit selective invalidation and UI notifications;
- create a transaction boundary for broad actions;
- refuse unknown/unsupported parameters safely.

The current bounded coordinator implements value/type validation, transactional commit, invalidation and explicit `ParameterMutationOrigin` reporting. It marks only user-control, user-gesture and Scout Apply changes as eligible for future history; preset load/import, undo/redo replay, animation/export evaluation, migration and system/runtime origins are explicitly ineligible. It does not persist origin or create history yet.

## Canonical render fingerprint

The fingerprint serialises only render-affecting state with stable field order, locale-independent numbers, explicit enums, stable IDs/order, fixed time/seed and renderer/precision policy where necessary. It excludes handles, clocks, paths, progress and GPU resources.

Uses:

- visual fixture identity;
- export resume validation;
- Scout candidate identity;
- render-cache/invalidation diagnostics;
- reproducibility reports.

The implemented `mw-render-state-v1` contract uses stable field order, length-prefixed strings, explicit enum names, exact IEEE-754 bit-pattern hexadecimal for finite floats/doubles, canonical positive zero, and SHA-256. DEC-018 records this choice. The fingerprint currently identifies render jobs and visual fixtures; no project-file or settings schema persists it yet.

## Invalidation classes

- camera/viewport;
- equation/precision/reference orbit;
- palette/colouring;
- post-processing;
- output dimensions/AA/tile overlap;
- runtime-only presentation.

A parameter descriptor maps to the smallest safe invalidation class. When uncertain, invalidate more broadly but record why.


## PH-03 mutation-path audit

The completed PH-03 implementation routes the inspected project-edit paths through explicit boundaries:

- the main-window palette/post scalar controls use `ApplyProjectParameterMutations` with model-owned validation and selective invalidation;
- deliberate main-window camera edits use `ApplyMainWindowCameraMutation`, including coordinate text, Jump, Reset and Scout Apply while preserving compensated components supplied by camera-valued sources;
- built-in palette selection uses `ApplyProjectPaletteSelection`, remains a no-op when unchanged, and clears custom stops atomically only when the selection changes;
- preview pan/zoom gesture coalescing uses `ParameterMutationOrigin::UserGesture` and the shared `PreviewNavigation` gesture kind: pan and wheel events continue one undo block while navigation remains active, and a 500 ms monotonic inactivity gap starts a new block;
- classified whole-preset replacements use `ApplyProjectPresetReplacement` for preset load, import, Palette Editor, Equation Editor, Settings and Journey Settings;
- project dialogs edit candidate copies and commit only after acceptance; Palette and Equation candidates are domain-scoped, and the exclusive AppWindow edit session prevents concurrent candidate snapshots while their live preview, Save and Cancel paths run; rejected or mismatched replacement contexts leave the authoritative preset unchanged;
- classified broad replacements are committed through PH-05 as one atomic before/after project entry; scalar reconstruction mismatch also falls back to this representation;
- `AnimationController` journey, pan, zoom and colour-cycle evaluation continues to mutate runtime copies only and remains history-ineligible;
- `SettingsStore`, built-in construction and renderer input assembly remain loading/construction/runtime boundaries rather than interactive project mutation routes.

No independently mutable project authority or persistence schema was added. PH-04/PH-05 provide one bounded runtime history and command surface over the coordinator. `VAL-016` and `AC-011` are satisfied for the inspected PH-03 implementation scope. BR-20260924-02 adds the exclusive edit-session gate; its editor-only native fixture proves Palette, Equation and Settings block competing routes and restore the owner after close. Earlier modeless camera-retention evidence remains historical and does not authorize concurrent editor state.

## Acceptance

- **Passed in the recorded portable scope:** camera/palette snapshot round trip preserves selected fields and compensated low components.
- **Passed in the recorded portable scope:** canonical serialisation is byte-stable for equivalent state and changes for render-affecting camera, palette or output-context changes.
- **Passed in the recorded portable scope:** unknown stable parameter names fail explicitly.
- **Passed in the recorded implementation scope:** scalar, deliberate-camera and discrete palette routes are transactional, model-normalised and selectively invalidated.
- **Passed in the recorded implementation scope:** preview drag and wheel routes carry explicit user-gesture origin plus stable, bounded coalescing metadata.
- **Passed in the recorded implementation scope:** preset load/import and accepted project dialogs use classified transactional whole-preset replacement; invalid or mismatched contexts roll back.
- **Passed in the recorded implementation scope:** mutation results distinguish history-eligible user/load/import/Scout edits from replay, evaluation, migration and runtime origins.
- **Passed in the recorded portable scope:** broad replacements and implicit subtree changes become exact atomic history rather than incomplete scalar entries.
- **Passed in the recorded portable scope:** bounded scalar and structural undo/redo, coalescing, branch truncation, runtime exclusion and bounds.
- **Passed in the recorded native scope:** Palette/Equation domain-merge regressions compile and pass in the full Release suite while preserving authoritative camera and exact-camera state.
- **Pending verification:** Windows interaction checks for the complete PH-05 matrix beyond the bounded exclusive Palette/Equation/Settings gate.

## Implemented PH-06 frame-local animation evaluation

`src/Core/GeneralAnimation.*` adds a runtime-only immutable evaluation layer over the current authoritative `Preset`; it does not add a second mutable project authority. `EvaluateGeneralAnimation` receives a const base snapshot and returns a separate frame-local `Preset` plus resolved time, seed, stable applied-track order and invalidation mask.

The target registry uses stable names and typed values. Camera centres use explicit compensated high/low values and compensated interpolation. Camera scale is logarithmic; rotation follows the shortest wrapped path; integer equation powers are Step-only. Duplicate enabled Replace targets, duplicate times, unsupported interpolation, invalid value types/ranges and enabled unknown targets are rejected before evaluation. Disabled unknown target names remain inert and generate warnings.

Evaluation is classified outside project mutation: it does not call the user mutation coordinator, does not write `workingPreset_`, does not persist state and does not create undo entries. Separate runtime clocks exist for preview, wallpaper and export, but PH-06 does not yet connect them to Win32 playback. Timeline persistence remains open under DEC-014.

## Implemented PH-07 editor and preview boundary

`GeneralAnimationEditorDialog` owns a candidate `AnimationTimeline`; it does not own a mutable `Preset`. Add Current Value reads through `ReadGeneralAnimationTargetValue` from the authoritative snapshot captured when the dialog opens. Candidate edits validate before commit and do not enter `ProjectHistory`.

AppWindow stores the accepted runtime timeline and the PH-06 `AnimationClockBank`. During editor preview, the existing main render timer calls `EvaluateGeneralAnimation(workingPreset_, candidateTimeline, previewClock, seed)` and renders the returned frame-local `Preset`. `workingPreset_`, settings and the export clock are not mutated. The historical wallpaper clock is no longer connected to desktop presentation because desktop animation modes were removed. Closing the dialog disables candidate preview and restores the prior preview time.

Tracks → Journey is a managed boundary: only an exact camera-only subset is accepted. The prepared string is applied only with OK through `ApplyWorkingPresetReplacement` using the existing Journey dialog classification, producing one atomic project-history entry when the persisted Journey changes. Timeline persistence remains deferred under DEC-014.

## Proposed PH-12 exact camera authority

PH-12 now retains exact camera text in schema-3 presets. Local settings originally migrated from schema 9 to schema 10 with an original backup; schema 11 subsequently removes rendered desktop-animation modes while preserving the same original-preserving promotion contract. `mw-render-state-v1` remains readable; `mw-render-state-v2-exact-camera` is a separate exact-camera identity and is never a reinterpretation of v1 bytes.

After PH-12 acceptance:

- `ExactDecimal` owns bounded canonical decimal syntax; `ExactComplex` and `ExactCamera` own centre and positive half-height.
- Exact camera is the only mutable project camera. Current high/low/scale values are derived compatibility/render adapters with explicit loss reporting and no write-back authority.
- When an exact half-height is below the finite-double range, its legacy compatibility scale is a positive lossy placeholder and must not cause settings/preset validation to reject, round or replace the exact camera text. Legacy rendering remains an adapter-only path for that state.
- An immutable exact render snapshot captures exact camera, formula capability, precision intent/plan identity, deterministic time/seed and all render-affecting state.
- Project/preset precision intent is authoritative for the captured output; user settings supply only defaults for new projects and local presentation preferences. Device capability, chosen execution path and adaptive presentation state are transient and cannot overwrite project intent.
- Parameter identity/value handling gains explicit exact-camera types; a generic `double` value cannot claim exact/deep safety.
- UI edits, preset Save As, Journey conversion, timeline evaluation and Scout candidate promotion commit through the existing mutation/history coordinator. One accepted exact camera action is one undoable transaction; preview/evaluation/speculative work remains history-ineligible. If the scalar history representation cannot reproduce the exact result, the coordinator records an atomic full-preset snapshot rather than replaying a lossy compatibility camera.
- The implemented core `ApplyProjectExactCameraMutation` accepts parsed canonical text, derives only its legacy adapter view, and marks the full camera invalidation domain. Main-window coordinate, Jump, exact control and Scout paths that already supply an `ExactCamera` use this transaction; legacy-only preview gestures remain explicit compatibility mutations.
- Exact-keyframe animation remains deferred. The current double-valued camera tracks may operate only when the base exact camera adapts losslessly; a lossy/deep exact base fails closed rather than being reset and rebuilt from double keyframes.
- Scout may search approximately only from a lossless legacy-compatible camera. A lossy/deep exact source is refused until Scout has exact-coordinate search semantics; ordinary candidates capture canonical exact camera/fingerprint and stale candidates cannot Apply.
- A new canonical fingerprint version includes exact camera, formula capability and precision-plan identity. `mw-render-state-v1` bytes remain historical/current identity and are never reinterpreted.

PH-12 stops if exact and approximate camera can both mutate, exact text first passes through binary floating point, migration can destroy the only original, or downgrade silently drops exact data.
