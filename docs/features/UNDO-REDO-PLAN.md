# Undo and Redo Plan

**Status:** PH-05 complete at the documented automated native scope; PH-03/PH-04 interaction confirmed and all bounded structural replacement routes evidenced  
**Purpose:** Own history scope, operations, transactions, coalescing, UI and validation
**Owner:** Core state/UI
**Reading trigger:** Mutation coordinator, editor, preset/import, journey or Scout Apply work
**Update trigger:** History eligibility, operation type, coalescing or UI change
**Linked IDs:** REQ-015, PH-04, PH-05, AC-012, AC-013, AC-018, VAL-019–VAL-022, VAL-036

## Implemented scope

`src/Core/ProjectHistory.h` and `.cpp` provide one runtime-only bounded history over the caller-owned authoritative `Preset`.

Two entry forms are used:

- `ParameterChangeOperation` entries for complete registered scalar/camera mutations. These retain PH-04 gesture coalescing and replay through `ApplyProjectParameterMutations` with `UndoRedo` origin.
- `PresetReplacementOperation` entries containing exact authoritative before/after `Preset` snapshots for structural or broad transactions that cannot be represented completely by the registered scalar keys.

The complete bounded scope includes:

- compensated camera coordinates, scale, rotation and camera-associated starting-scale/animation-mode metadata;
- palette selection, offset, frequency, gamma, interpolation and post-processing scalars;
- custom palette-stop insertion, removal, reorder and replacement;
- multi-field equation and equation-post-processing edits;
- journey text/row insertion, removal and reorder;
- preset load, imported preset application and accepted Palette, Equation, Settings and Journey dialogs;
- Scout Apply as one labelled camera transaction;
- labelled Undo/Redo buttons plus Ctrl+Z and Ctrl+Y;
- exact redo-branch truncation after a new edit;
- configurable hard containment defaults of 256 entries and 4 MiB estimated operation memory.

A scalar mutation is first reconstructed from its stable-key deltas and camera metadata. When that reconstruction does not exactly equal the committed authoritative result, history automatically records one atomic structural snapshot instead of accepting partial history. This covers implicit subtree changes such as selecting a built-in palette that clears custom stops.

## Eligibility and authority

Undo/redo affects project/preview artistic state. It does not own `UserSettings`, persisted history or `RuntimeSession` state.

Eligible origins are user controls, user gestures, preset load/import and Scout Apply. Replay, preview/export animation evaluation, file-backed desktop playback, export evaluation, migration, renderer/device recovery and system/runtime operations remain excluded. There is no desktop fractal-animation evaluation domain under DEC-039.

History records only after a successful coordinator commit. It never becomes a second mutable project authority. Startup preset selection is explicitly marked history-ineligible.

## Transactions and coalescing

- One accepted cross-dialog edit, import, preset load or Scout Apply creates one entry.
- Empty/no-op and failed operations create no entry.
- Parameter operations coalesce only when origin, gesture kind, non-zero token and target set match.
- Structural snapshot entries never coalesce.
- Preview pan and wheel navigation share one `PreviewNavigation` token while events remain within the explicit 500 ms inactivity window.
- One palette-thumb gesture creates one entry.
- New edits after undo remove the redo branch.
- Undo/redo while a preview drag or palette-thumb gesture is active is rejected.

## Replay and UI refresh

Scalar entries replay through the mutation coordinator. Structural entries replace the caller-owned `Preset` from the stored validated snapshot. Both paths report invalidation; structural replay requests a full project render.

After replay the application resynchronises preset selection/name, camera, rotation, iterations, palette, colour/post-processing controls, equation summary, animation input, monitor assignment/status and Undo/Redo command labels.

## Persistence and containment

History is runtime-only and is not written to settings or preset JSON. Persisting history across restarts remains excluded until a separate format, privacy, size and migration requirement is accepted.

Memory accounting includes labels, scalar operations and dynamic snapshot data. An individual entry larger than the configured memory limit is rejected rather than silently evicting the entire prior history to retain an oversized transaction.

## Validation status

Portable tests cover:

- exact scalar apply/undo/redo and camera metadata restoration;
- pan, wheel and palette-control coalescing;
- branch truncation, entry bounds and estimated-memory bounds;
- runtime/background/replay exclusion;
- custom-stop-deleting palette selection fallback to atomic history;
- palette-stop insert/remove/reorder exact round trip;
- multi-field equation replacement exact round trip;
- journey row insertion/reorder exact round trip;
- labelled preset-load replacement;
- oversized structural-entry rejection;
- one-entry Scout Apply undo/redo.

VAL-019, VAL-020, VAL-021, VAL-022 and VAL-036 pass in the recorded portable/unit/structural scope. Native MSVC compilation is recorded; user-confirmed Windows interaction covers PH-04 camera/palette undo/redo and redo-branch truncation. BR-20260909-03 additionally proves accepted Palette and Equation replacements expose their expected native structural history labels and that Equation Undo/Redo restores powers 2/3 while preserving the camera changed during the open editor. BR-20260909-04 proves built-in preset Load/Undo/Redo restores exact before/after camera text and selected preset identity. BR-20260909-05 proves Settings accept/Undo/Redo restores its before/after project rotation and synchronises the main-window camera controls. BR-20260909-06 proves Journey accept/Undo/Redo restores exact before/after structured waypoint text. BR-20260910-01 proves a production-serialised preset selected through the native Open dialog restores exact before/after camera text and preset identity through Import/Undo/Redo. BR-20260910-02 proves production Scout search and Use in Preview create one reversible `Apply Scout Camera` entry with exact camera replay and unchanged preset identity, completing the automated native PH-05 replacement matrix.
