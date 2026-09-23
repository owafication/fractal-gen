# Data and Persistence

**Status:** Current canonical data contract; future state separation remains explicitly proposed  
**Purpose:** Own persisted state, formats, migrations, file safety and future state separation  
**Owner:** Core models and settings  
**Reading trigger:** Models, SettingsStore, import/export, presets, output defaults, timeline or migration work  
**Update trigger:** Field, schema, path, migration or identity change

## Observed current data roots

### `AppSettings` — settings schema 12

Observed fields include selected preset, performance/precision/adaptive settings, general/startup settings, default desktop mode, static/slideshow output settings, a bounded local exported-video path, Mirror/Span monitor mode, custom presets, custom palette/equation presets and last-running state. Schema 12 removes the inert per-monitor preset-assignment map.

### `Preset`

Observed render-affecting fields include compensated camera, rotation, zoom/journey behaviour, iterations, bounded equation settings, built-in/custom palette data, colour mapping and post-processing, animation mode, journey text, frame limit, render scale and anti-aliasing.

### Preset serialisation

`SettingsStore::SerialisePreset` emits schema version 3 with canonical exact-camera text. Application version, settings schema and preset schema are separate version domains.

Schema-3 load derives the legacy `CameraState` only as a compatibility view. A positive exact half-height below double range is retained verbatim and derives a marked lossy positive placeholder; validation must not reject that placeholder or overwrite the exact text. A legacy renderer still cannot claim to execute this state exactly.

### Local paths

Observed path owner: `src/Infrastructure/Paths.cpp`.

```text
%LOCALAPPDATA%\MandelbrotLiveWallpaper\settings.json
%LOCALAPPDATA%\MandelbrotLiveWallpaper\logs\
%LOCALAPPDATA%\MandelbrotLiveWallpaper\static-renders\
```

User-selected output folders, slideshow entries and the optional exported MP4 wallpaper may point elsewhere.

## Observed file-safety behaviour

- JSON parser input is bounded by size and nesting depth.
- Numeric values are validated/normalised.
- Settings save uses a temporary file and rename-based replacement.
- Invalid settings are preserved with a `.corrupt-<timestamp>.json` suffix before defaults are used.
- Imported equation/preset data is treated as data, not executable code.

These observations require runtime and failure-injection validation before release claims.

## Proposed future state roots

### `ProjectSnapshot`

Render-affecting artistic state, camera/rotation, equation/precision choices that belong to a project, palette/colouring, post-processing, structured journey, general timeline and project schema version.

### `UserSettings`

Output folders, default image format/quality, default desktop mode, performance/startup/UI preferences and recent locations where appropriate.

### `RuntimeSession`

Preview/export playback clocks, active file-backed desktop mode, preview/export GPU resources, media handles, jobs/workers, temporary previews, frame timing, adaptive decisions and progress. Desktop presentation does not own fractal scene state, an animation clock or renderer resources.

**Migration rule:** current models remain authoritative initially. A snapshot is an immutable capture/view, not a second mutable authority. Once PH-12 is implemented, render-affecting precision intent belongs to the project/preset; user settings provide defaults for new projects and local presentation preferences only.

## Runtime undo history

PH-04/PH-05 history is process-memory only. It stores typed scalar/camera deltas plus bounded exact before/after `Preset` snapshots for structural transactions. It does not store project files, paths, logs or user settings, and it is cleared when the application exits. Preset load/import application and accepted project dialogs remain one project operation without changing any settings or preset schema. Persisted history remains outside scope pending an explicit format, privacy, size and migration requirement.

## Identity rules

Stable object IDs are required before reorderable structures enter general history or timeline persistence:

- palette stops;
- journey waypoints;
- animation tracks;
- keyframes;
- structurally edited equation components where applicable;
- persisted Scout candidates, only if session persistence is approved.

Indices are display/iteration positions, not persisted identity.

## Migration procedure

1. Record source schema and preserve the original file.
2. Parse with existing bounds.
3. Migrate in memory through explicit ordered steps.
4. Apply deterministic defaults only for absent fields.
5. Validate the complete result.
6. Save to a temporary file.
7. Re-read and validate when the change is high risk.
8. Atomically promote where the platform permits.
9. Keep the original or corrupt copy until success is proven.
10. Record migration fixtures and unknown-field behaviour.

## Compatibility rules

- Older supported files remain readable.
- New optional fields default to previous behaviour.
- Unknown executable or unsupported typed values never execute.
- Unknown future timeline targets disable only the affected track and produce a warning; they do not invalidate unrelated project state.
- Built-in IDs remain stable; custom IDs are never silently reused.
- Broad replacements such as preset load or Scout Apply are one project operation when undo is available.

## Data acceptance

- Settings/preset round trip preserves supported values.
- Camera low components survive snapshot and persistence paths that claim deep-coordinate safety.
- Malformed, excessive, non-finite or unsupported input is rejected or normalised according to the owning contract.
- Failed saves leave the last valid final file intact.
- Output cancellation leaves no partial file under the final name.

## Open decisions

- [Decision required: whether future timelines persist inside settings, inside preset/project files, or in a new project document; affects PH-06–PH-08.]
- [Accepted DEC-015: project/preset precision intent overrides the user default; hardware capability, execution selection and adaptive presentation remain transient.]
- [Decision required: stable ID representation and generation policy; affects PH-03, PH-05 and PH-06.]

## PH-06 timeline persistence boundary

PH-06 introduces no persisted timeline field, project document or settings migration. `AnimationTimeline`, tracks and keyframes are runtime data passed explicitly to the platform-neutral evaluator. IDs are caller-supplied bounded stable tokens for the lifetime of the timeline; no generator or persisted identity format is claimed.

Target names are stored as stable strings so an unknown disabled future target can remain inert in memory. Safe raw-value retention across file versions cannot be claimed until DEC-014 selects a timeline persistence location and schema. PH-07 introduced no timeline persistence or schema change; current settings schema 12 likewise adds no timeline field. The accepted editor timeline remains process-runtime data; only an explicitly prepared, lossless Tracks → Journey conversion can update the existing persisted Journey string through the established atomic project replacement path.

## PH-09 video export data boundary

- FFmpeg executable paths, capability results, CRF/preset choices and video jobs are not persisted as application settings or project data. The schema-11-added `videoWallpaper.filePath` remains a local presentation preference referring to an already exported MP4.
- A video job reads an existing PH-08 manifest/receipt set and writes bounded local process logs beneath that sequence's `.mw-frame-sequence/video-export` metadata area.
- The final MP4 is user-selected output. Its adjacent temporary file is application-owned only for the active job and is removed on failure/cancellation.
- Optional source cleanup uses only the verified manifest and receipts; arbitrary directory enumeration is not an ownership claim, and untracked files remain untouched.
- No video-export data enters undo/redo history or timeline persistence.

## Proposed PH-12 exact-camera migration boundary

The durable formats are now settings schema 12 and preset schema 3. Schema-3 presets retain canonical exact centre/half-height strings while preserving a derived legacy `CameraState` for current renderers. Schema-2 presets and schema-11-or-earlier settings remain migration inputs only; no downgrade writer exists. Journey and several UI paths still parse decimal text through `double`; `mw-render-state-v1` remains the historical IEEE-754 identity.

The current durable formats are settings schema 12 and preset schema 3. This unreleased, single-instance application adopts a forward-only policy: no earlier application version is supported to open or write the new formats. The current application still preserves the pre-migration file before its one-way local migration.

Once PH-12 has accepted schema authority:

- exact centre and half-height are one canonical project-owned representation; legacy `CameraState` fields are derived compatibility/render values and cannot be saved back over exact values silently;
- bounded exact text is canonicalised before any binary floating-point conversion;
- legacy migration reconstructs the exact mathematical values of the stored IEEE-754 high/low/scale fields, not unknown original decimal text;
- settings/preset migration runs in memory, writes a temporary file, validates/re-reads it, promotes only after success and preserves the original;
- built-in presets remain immutable; Save As creates a new stable user ID and retains exact camera, equation, palette, animation and precision intent;
- invalid exact fields reject the whole import/migration without partial preset creation;
- no downgrade writer exists; an unsupported older application format is not produced by this application;
- `mw-render-state-v1` remains a historical/current identity; a new exact fingerprint has a distinct version and mixed-version resume fails unless a specific tested rule is accepted.

Settings schema 12/preset schema 3, forward-only migration and DEC-015/DEC-039/DEC-040 are accepted governance. DEC-028 accepts the ASCII-only normalized-scientific grammar, 16 KiB input bound, 8,192 significant-digit bound and normalized exponent magnitude no greater than 1,000,000. Legacy settings promotion preserves a timestamped original, atomically writes schema 12, re-reads/validates it, and restores the original if verification fails. Schema 11 added `videoWallpaper.filePath`; schema-10 Live/Journey desktop defaults and the legacy launch flag migrate to None rather than starting a renderer. Schema 12 removes per-monitor preset assignments; schema-11 `monitorMode="independent"` migrates to Mirror and `monitorPresetAssignments` is ignored and omitted on rewrite. `mw-render-state-v2-exact-camera` hashes canonical exact camera text as a distinct contract and fails if exact state is absent; v1 bytes remain historical/current. New frame-sequence export jobs use the forward-only `mw-render-state-v3-exact-precision-plan` identity and schema-3 manifests: they bind the exact camera, planner version, selected backend/tier, formula capability and reason, so schema-1/v1 and schema-2/v2 export work cannot resume under a different precision route. The main-window coordinate triplet, X/Y/scale fields and Jump-to-Coordinates parse and retain canonical exact text, deriving a legacy preview camera only through the fail-closed adapter; preset loading preserves canonical text in controls. High-resolution export retains the exact camera in its immutable request and derives the same adapter before current CPU/GPU rendering begins. The platform-neutral still mapper represents a global pixel centre as exact camera text plus rational half-height factors; non-zero rotation is explicitly marked for a later precision adapter rather than silently rounded. DEC-029 accepts the pinned Boost.Multiprecision 1.83.0 source-only dependency and BSL-1.0 packaging policy. Exact-camera authority across animation and renderer execution paths and numerical/resource gates remain PH-12 work.
