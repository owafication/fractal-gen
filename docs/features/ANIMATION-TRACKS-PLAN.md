# Animation Tracks Plan

**Status:** PH-07 implementation complete; bounded native editor/preview-clock and Journey conversion interaction passed, with visual playback and DPI/keyboard gates pending  
**Purpose:** Own timeline model, deterministic evaluation, interpolation, clocks, editor behaviour and Journey compatibility  
**Owner:** Core animation/UI  
**Reading trigger:** General animation, playback, scrubbing, export-time evaluation or Journey conversion  
**Update trigger:** Track target, interpolation, clock, persistence or editor change  
**Linked IDs:** REQ-016, PH-06, PH-07, AC-014, VAL-023–VAL-026

## Product boundary

PH-06 deterministic evaluator implementation complete: the bounded runtime model, interpolation rules and independent clocks remain the foundation used by this phase. Its recorded boundary remains runtime-only with compensated high/low values, Clamp / Loop / PingPong timing, disabled unknown future target retention, VAL-023–VAL-025 and DEC-014. Native MSVC compilation and BR-20260910-04 preview-clock control interaction pass; rendered visual playback remains pending.

General tracks extend rather than replace existing `AnimationMode` and structured Journey. Journey remains the simpler persisted camera-route workflow for preview and export authoring. The general timeline remains runtime-only under DEC-014 and is not written to settings schema 11, presets, startup state or undo history. Neither workflow is a desktop-presentation mode.

PH-07 uses a modal editor rather than a modeless second authority or a new main navigation page. The dialog owns a candidate timeline, previews it through the existing AppWindow render timer and the PH-06 preview clock, commits the runtime timeline only on OK, and restores the prior preview clock/state on Cancel or close.

## Implemented model and targets

`src/Core/GeneralAnimation.*` defines bounded stable timeline, track and keyframe IDs, Clamp / Loop / PingPong timing, Step / Linear / Smoothstep interpolation, deterministic evaluation and independent clock storage. Preview and export are the supported active animation domains. `AnimationClockDomain::Wallpaper` remains only as inert compatibility/test state after DEC-039 and is not connected to `WallpaperController` or any desktop mode. Safety bounds remain 256 tracks, 4096 total keyframes, 80 characters per object ID, 120 characters per target name and seven days per timeline.

Implemented targets are compensated camera centre X/Y, logarithmic camera scale, wrapped rotation, palette offset/frequency/gamma, brightness/contrast/saturation, stripe/bloom/edge-light strength, equation quadratic real/imaginary coefficients, step-only equation powers and equation bailout radius. Duplicate enabled Replace targets are rejected. Disabled unknown future targets remain inert warnings; enabled unknown targets fail validation.

## Evaluation contract

`EvaluateGeneralAnimation(baseSnapshot, timeline, time, seed)` validates both inputs, resolves timing, evaluates enabled tracks in stable track-ID order and returns a frame-local `Preset`. It never mutates `workingPreset_`, persists data or creates per-frame history. Camera centres retain compensated high/low interpolation, scale uses logarithmic interpolation and rotation uses the shortest wrapped path.

## Implemented PH-07 authoring editor

`src/App/GeneralAnimationEditorDialog.*` provides:

- track list, enabled state, registry target picker, add and remove;
- keyframe table with time, typed value and interpolation;
- Add Current Value using the authoritative project snapshot;
- selected keyframe time/interpolation update and removal;
- duration and loop-mode editing with conflict rejection;
- scrubber plus Play, Pause and Stop;
- validation/conflict display and disabled-unknown warnings;
- Journey → Tracks and Tracks → Journey commands;
- OK/Cancel candidate boundaries and restored preview state on close.

The AppWindow preview uses the candidate evaluated `Preset` for camera, equation, palette and post-processing fields. Candidate change, scrub and playback callbacks force an immediate preview render while the modal editor is open; export time and the inert compatibility wallpaper clock are not changed. Timeline editing itself does not enter project undo history because the timeline is runtime-only. A prepared Tracks → Journey conversion changes the persisted `Preset` only after OK and then uses the existing atomic Journey project replacement/history path. Desktop presentation never evaluates this timeline.

## Journey adapter contract

### Journey → Tracks

The adapter strictly parses every non-comment row. It accepts exactly:

```text
centreX,centreY,scale,transitionSeconds[,holdSeconds]
```

Malformed, excessive, non-finite or out-of-range rows reject the whole conversion; rows are never silently skipped. Conversion creates deterministic camera centre X/Y and scale tracks. The first keyframe is the current project camera, transitions are Smoothstep, holds are Step, and the resulting timeline uses Clamp timing for an exact one-pass representation. This deliberately avoids claiming the existing Journey's subsequent cyclic return-to-first behaviour can be represented without an additional pre-roll/cycle model.

### Tracks → Journey

Reverse conversion succeeds only when:

- the timeline uses Clamp timing;
- enabled tracks are exactly camera centre X, centre Y and scale;
- all three tracks share the same times and interpolation grid;
- the first frame exactly matches the current project camera;
- transition and hold timing fits Journey limits;
- each movement is Smoothstep and each hold is Step;
- waypoint compensated low components are zero, because Journey text cannot preserve them.

Any unsupported target, mismatched grid, non-representable interpolation, lossy compensated value or timing mismatch returns an explanation and produces no Journey text. Tracks are never silently dropped.

## Persistence and compatibility

No settings or preset schema changed. Runtime-generated IDs are bounded and unique only within the current timeline session; persisted ID generation remains open under DEC-014. Unknown target raw compatibility across file versions is not claimed because no timeline file format exists yet.

## Validation evidence

Portable tests cover PH-06 determinism plus PH-07 strict parsing, one-pass timing, exact hold evaluation, supported Journey/track round trips, explicit unsupported-target refusal, compensated-value loss refusal, Add Current Value, duplicate-time rollback and unchanged output on failed conversion. Source-audit checks cover the modal editor controls, single preview clock authority, no schema change, AppWindow evaluated-preset propagation and atomic Journey application.

AC-014 and VAL-023–VAL-026 pass in the recorded portable/core/structural scope. BR-20260910-04 opens the production editor, adds a track and authoritative current-value keyframes, scrubs/plays/stops the preview clock, proves Cancel discards its candidate without project/history mutation, and proves OK retains one runtime track on reopen without entering project history. BR-20260910-05 proves native Journey-to-Tracks produces the three camera tracks, Tracks-to-Journey exposes its success confirmation, and Cancel preserves project/history state. DPI/keyboard smoke and rendered visual preview playback remain pending.

## Proposed PH-12/PH-15 exact camera integration

- Camera keyframes own canonical exact centre and half-height values; current compensated/double values become derived compatibility values only.
- Evaluation derives each frame directly from immutable keyframes and exact deterministic time. Continuous zoom and long Journey motion must not accumulate rounded per-frame steps.
- Exact scale interpolation and its evaluation version are part of render/export identity.
- Journey conversion preserves exact values under a versioned accepted format or refuses the lossy conversion; it never silently drops digits.
- Preview remains frame-local and history-ineligible. An accepted conversion/project edit remains one atomic history action.
- VAL-044 covers Journey/timeline/Scout adapters; VAL-058 covers exact seek, pause/resume, long motion and frame-index timing.

No timeline schema, exact keyframe type or Journey syntax is implemented by BR-20260730-01; DEC-014 and exact-camera schema authority remain prerequisites.
