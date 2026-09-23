# Mandelbrot Live Wallpaper — Roadmap Recommendations

**Recommended baseline:** 1.13.1  
**Document type:** roadmap review and revision guidance  
**Source reviewed:** `Mandelbrot-Live-Wallpaper-1.12.6-Integrated-Feature-Roadmap.md`  
**Status:** recommended revision; not an implementation record

---

## 1. Purpose

This document records recommendations for updating the integrated feature roadmap covering:

1. general animation tracks;
2. delta-based undo/redo;
3. visual regression testing;
4. offline animation and video export;
5. deterministic random discovery tools.

The original roadmap is technically strong, but it was written against an uninspected, user-reported 1.12.6 baseline. The project has since advanced to 1.13.1 and already includes substantial work that overlaps with the roadmap, especially around Fractal Scout, image-output settings, desktop modes, journey controls, palette behaviour, and dialog architecture.

The roadmap should therefore be rebased before implementation continues.

---

## 2. Overall assessment

The roadmap should be retained as the long-term architectural direction.

Its strongest decisions are:

- one stable parameter-addressing contract shared by animation, undo/redo, discovery, validation, and renderer invalidation;
- deterministic evaluation for visual tests, offline frame export, and discovery;
- visual regression before large state-management changes;
- explicit separation between persistent project state and runtime-only resources;
- transaction-based undo/redo with coalescing and structural operations;
- immutable export-job snapshots;
- safe cancellation and atomic output-file handling;
- keeping structured journeys as a simpler workflow alongside general animation tracks;
- treating FFmpeg as an external executable unless a separate licensing and distribution decision is made.

The main issue is not the technical quality of the roadmap. The issue is that several assumptions are now outdated and some proposed work duplicates functionality already present in the application.

---

## 3. Required roadmap changes

### 3.1 Rebase from 1.12.6 to 1.13.1

Before accepting any proposed type, path, data model, or migration rule:

1. inspect the current 1.13.1 source;
2. identify the authoritative current models;
3. identify all current mutation paths;
4. identify current persistence schemas and migrations;
5. identify existing rendering and export interfaces;
6. reconcile every proposed source path with the real project layout;
7. update milestones so already-completed work is not implemented again.

The roadmap must not continue to use an uninspected 1.12.6 archive as its architectural baseline.

### 3.2 Add a Windows release-stabilisation phase

Recent releases exposed Windows-only compiler failures that portable core builds did not detect. Native Windows validation should become an explicit prerequisite.

Add a new phase before the shared-contract work:

## Phase -1 — Windows release stabilisation

Required checks:

- native MSVC x64 compilation;
- application resource and manifest compilation;
- Direct3D 11, DXGI, OpenGL, WIC, and Win32 linking;
- runtime D3D11 shader compilation;
- runtime OpenGL shader compilation;
- application startup and shutdown;
- preview rendering;
- desktop modes: none, static, live, slideshow, and journey;
- Settings, Equation, Palette, Journey, Quick Controller, and Fractal Scout dialogs;
- static capture and saved-image encoding;
- high-resolution tiled export;
- installer and portable package generation;
- upgrade from the preceding release;
- basic Windows 10 and Windows 11 smoke tests.

**Exit criterion:** the current release builds and starts successfully on the intended Windows release toolchain before architectural expansion begins.

### 3.3 Convert the Fractal Scout section into an integration gap analysis

The roadmap proposes Fractal Scout as future work, but the current project already includes:

- deterministic candidate generation;
- multiple search targets;
- multi-scale search;
- candidate scoring;
- thumbnail generation;
- near-duplicate suppression;
- cancellation;
- refinement around a selected result;
- temporary candidate preview;
- save-as-preset handoff;
- bounded candidate, iteration, thumbnail, and memory limits.

The revised roadmap should not recreate Scout from scratch.

Replace the existing discovery programme with:

## Existing Scout integration work

- adapt Scout candidate state to the shared project snapshot contract;
- assign stable candidate fingerprints;
- integrate candidate application with undo/redo as one transaction;
- integrate Scout thumbnails with visual regression fixtures;
- optionally add style-only and curated equation-family exploration;
- optionally persist lightweight search sessions;
- preserve the rule that previewing a candidate never mutates the active project;
- preserve deterministic seed behaviour.

### 3.4 Avoid introducing a second authoritative state model

The proposed `ProjectSnapshot` is useful, but it must not become a competing copy of current state.

Use this migration rule:

> Introduce snapshot and parameter adapters over the current authoritative models first. Do not create a second independent source of truth.

Recommended progression:

1. existing current models remain authoritative;
2. `ProjectSnapshot` initially acts as a serialisable view or immutable capture;
3. `ParameterRegistry` reads and writes selected existing fields through adapters;
4. migrate one domain at a time;
5. remove direct mutations only after all callers for that domain are inspected and converted;
6. consider deeper model consolidation only after the adapter migration is complete and tested.

### 3.5 Separate project, user, and runtime state

The revised architecture should define three state roots.

```text
ProjectSnapshot
  Render-affecting artistic state
  Camera and rotation
  Equation and precision state
  Palette and colouring
  Post-processing
  Structured journey
  General animation timeline
  Project schema version

UserSettings
  Output folders
  Default image format and compression/quality
  Default desktop mode
  Performance preferences
  Startup behaviour
  UI preferences
  Recent locations where appropriate

RuntimeSession
  Playback clocks
  Current applied desktop mode
  GPU resources
  Window handles
  Active jobs and workers
  Temporary previews
  Frame timing and adaptive-performance state
  Export and discovery progress
```

Undo/redo should normally affect `ProjectSnapshot`, not `UserSettings` or `RuntimeSession`.

### 3.6 Use stable object IDs instead of indices

The proposed `ParameterPath` includes an optional numeric index. Indices are unsafe identities for reorderable structures.

Use stable IDs for:

- palette stops;
- journey waypoints;
- timeline tracks;
- keyframes;
- saved equation components where structural editing is supported;
- discovery-session candidates if they are persisted.

Recommended shape:

```cpp
struct ParameterPath {
    ParameterDomain domain;
    ParameterKey key;
    std::optional<ObjectId> objectId;
    std::optional<ParameterKey> childKey;
};
```

Numeric indices may still be derived for display and iteration, but they should not be persisted as identity.

### 3.7 Use a hybrid undo model

Do not force every edit into scalar deltas.

Use:

- typed scalar deltas for camera, palette values, equation parameters, and effects;
- insert/remove/reorder operations for stable-ID collections;
- subtree replacement for preset loads, candidate application, imports, and other broad changes.

Recommended operation family:

```cpp
using ProjectOperation = std::variant<
    ParameterChangeOperation,
    InsertOperation,
    RemoveOperation,
    ReorderOperation,
    ReplaceSubtreeOperation
>;
```

This keeps normal history compact while making structural changes reversible and maintainable.

### 3.8 Treat visual-comparison thresholds as measured configuration

Example thresholds in the original roadmap are useful illustrations, not final release criteria.

Thresholds should be calibrated separately for:

- canonical CPU output on a pinned toolchain;
- D3D11 WARP output where used as a reproducible Windows reference;
- hardware D3D11 output;
- OpenGL output by supported environment where necessary;
- raw fractal passes;
- post-processed output;
- tiled-versus-full seam strips.

Use exact comparison only where repeatability is demonstrated. GPU comparisons should combine:

- maximum per-channel error;
- differing-pixel ratio;
- structural similarity;
- stricter tile-boundary checks;
- explicit backend exceptions where equivalent output is not expected.

Do not solve instability by globally loosening all thresholds.

### 3.9 Define the Windows GPU fixture strategy

The revised roadmap should explicitly choose how GPU visual fixtures are rendered.

Recommended approach:

- CPU renderer as the canonical portable fixture path;
- D3D11 WARP as the preferred reproducible Windows GPU-like reference where compatible;
- hidden or offscreen D3D11 render targets for automated fixtures;
- hidden OpenGL context or the existing production tiled-export path for OpenGL fixtures;
- hardware-renderer checks as environment-specific release or scheduled tests;
- production renderer code paths only, never a duplicate test renderer.

### 3.10 Split the roadmap into maintained documents

The original roadmap combines product direction, architecture, implementation details, testing, security, performance, milestones, and risk management in one long file.

Recommended structure:

```text
docs/roadmaps/INTEGRATED-CREATIVE-ROADMAP.md
docs/architecture/PROJECT-STATE-AND-PARAMETERS.md
docs/testing/VISUAL-REGRESSION.md
docs/features/UNDO-REDO-PLAN.md
docs/features/ANIMATION-TRACKS-PLAN.md
docs/features/OFFLINE-EXPORT-PLAN.md
docs/features/FRACTAL-SCOUT-STATUS.md
```

The master roadmap should contain:

- programme status;
- phase order;
- dependencies;
- release gates;
- links to detailed specifications.

The detailed documents should own data models, APIs, UI behaviour, test cases, and migration rules.

---

## 4. Recommended revised programme order

### Phase -1 — Windows release stabilisation

Build and run the current app successfully under the release MSVC environment. Validate D3D11, OpenGL, WIC, dialogs, desktop modes, Scout, export, and packaging.

### Phase 0 — Visual regression foundation

Implement a small production-renderer fixture harness before changing state architecture.

Initial fixtures:

1. standard Mandelbrot;
2. Tricorn Cyan Fire Ring;
3. rotated bloom scene;
4. deep Mandelbrot perturbation;
5. deep Tricorn perturbation;
6. tiled-versus-full seam case.

Initial outputs:

- current render;
- approved baseline;
- difference image;
- metric summary;
- render-state fingerprint.

### Phase 1 — Parameter adapter and render fingerprint

Introduce stable parameter identity for a narrow first domain:

- camera centre;
- camera scale;
- camera rotation;
- palette offset;
- palette frequency;
- palette gamma;
- stripe strength;
- bloom strength;
- edge-light strength.

Keep current models authoritative. Add an immutable snapshot adapter and canonical render-state serialisation.

### Phase 2 — Camera and palette undo/redo

Implement:

- transaction API;
- pan and zoom coalescing;
- rotation edits;
- palette-control edits;
- immediate-preview edits;
- selective renderer invalidation;
- Undo and Redo UI and shortcuts;
- bounded history by entry count and memory.

### Phase 3 — Complete project undo/redo

Migrate:

- equation parameters;
- post-processing;
- palette-stop structures;
- preset application;
- journey waypoint edits;
- Scout candidate application;
- imports and broad replacements.

Use subtree replacement for operations that should remain one user-visible history entry.

### Phase 4 — General animation evaluator

Implement the timeline data model and deterministic evaluator before a complex editor.

First supported targets:

- camera centre;
- logarithmic camera scale;
- rotation;
- palette offset;
- palette frequency;
- stripe strength;
- bloom strength;
- selected validated equation values.

Requirements:

- immutable base snapshot;
- frame-local evaluation;
- stable track order;
- duplicate-target rejection in Replace mode;
- deep-coordinate-safe interpolation;
- separate preview, wallpaper, and export clocks;
- no runtime frame changes in undo history.

### Phase 5 — Basic animation editor and journey adapter

Add:

- track list;
- target selection;
- keyframe table;
- duration and loop mode;
- scrubber;
- play, pause, and stop;
- add keyframe from current value;
- journey-to-track conversion;
- tracks-to-journey conversion only for the supported camera-only subset.

Keep Journey Settings available as the simpler workflow.

### Phase 6 — Deterministic frame-sequence export

Reuse the existing high-resolution tiled renderer.

Implement:

- immutable export job snapshot;
- exact frame-index-to-time calculation;
- PNG sequence export first;
- temporary frame files and atomic promotion;
- cancellation;
- progress events;
- job manifest;
- fingerprint-validated resume;
- explicit wallpaper pause or throttle policy;
- selected-frame visual fixtures.

### Phase 7 — FFmpeg encoding

Add external FFmpeg discovery and validation after frame-sequence export is stable.

Initial codecs:

- H.264 in MP4;
- optional H.265 in MP4;
- optional VP9 in WebM.

Requirements:

- safe argument construction;
- no arbitrary command templates;
- captured logs;
- cancellation;
- completed source frames preserved on failure;
- output verified before optional cleanup.

### Phase 8 — Existing Scout integration

Connect the existing Fractal Scout to the shared contracts:

- project snapshots;
- parameter registry;
- render fingerprints;
- undo transaction on Apply;
- visual thumbnail fixtures;
- optional style variation;
- optional curated equation-family exploration;
- optional session persistence.

Do not replace the existing deterministic candidate engine unless direct inspection proves a narrower integration is inadequate.

### Phase 9 — Integration hardening

Complete:

- native MSVC and Windows GPU matrix;
- multi-monitor coexistence;
- sleep/resume and device-loss testing;
- long-running animation and export soak tests;
- installer upgrade tests;
- memory and resource profiling;
- migration documentation;
- final user documentation and examples.

---

## 5. Recommended immediate bounded slice

The next implementation slice should provide safety infrastructure without changing visible application behaviour.

### Scope

1. Confirm the current 1.13.1 native Windows build.
2. Add a CPU visual-fixture command using the production still renderer.
3. Add four initial fixtures:
   - standard Mandelbrot;
   - Tricorn Cyan Fire Ring;
   - rotated bloom scene;
   - deep perturbation scene.
4. Add one tiled-versus-full comparison.
5. Add canonical render-state serialisation.
6. Add a render fingerprint.
7. Add camera and palette parameter adapters only.
8. Add snapshot round-trip tests for those fields.
9. Preserve all current UI, preset, journey, desktop, export, and coordinate behaviour.

### Definition of done

- native Windows build succeeds;
- existing tests pass;
- the four visual fixtures pass repeatedly;
- the tiled-versus-full fixture has no unexplained seam difference;
- a deliberate palette change produces a clear visual-test failure;
- snapshot round trip preserves selected camera and palette fields;
- no second authoritative state copy is introduced;
- no visible workflow changes are added;
- fixture output identifies renderer, dimensions, precision, and render fingerprint.

---

## 6. Updated architecture rules

### 6.1 One mutation path per migrated domain

Once a parameter domain is migrated to the coordinator, direct UI mutation for that domain should be removed.

### 6.2 Runtime evaluation never modifies base project state

Animation playback, wallpaper playback, frame export, and candidate preview operate on runtime or frame-local snapshots.

### 6.3 Undo history excludes runtime operations

Do not record:

- animation frames;
- wallpaper playback frames;
- export evaluation;
- discovery search progress;
- thumbnail generation;
- adaptive-performance decisions;
- background loading progress.

### 6.4 Broad user actions remain single history entries

Examples:

- load preset;
- apply Scout candidate;
- import project subtree;
- convert journey to tracks;
- commit current animation frame;
- replace palette.

### 6.5 Production renderers are the test renderers

Visual tests must invoke the same CPU, D3D11, OpenGL, perturbation, post-processing, and tiled-export paths used by the application.

### 6.6 Deterministic mode must disable adaptive quality changes

Visual tests and offline export must pin:

- renderer backend;
- precision policy;
- anti-aliasing pattern;
- frame time;
- random seed;
- candidate order;
- evaluation order;
- output metadata that would otherwise vary.

Adaptive interactive behaviour may remain enabled for wallpaper playback.

### 6.7 Persistence migration remains additive and recoverable

- old formats remain readable;
- missing fields receive deterministic defaults;
- migration occurs in memory first;
- original files are not partially overwritten;
- save uses atomic replacement where possible;
- unknown timeline targets disable only the affected track and produce a warning.

---

## 7. Revised release gates

A programme phase may be considered complete only when:

- the native Windows release build passes for Windows-facing code;
- portable GCC and Clang checks still pass where applicable;
- affected unit and integration tests pass;
- approved visual baselines show no unexplained change;
- tiled-seam checks pass for rendering changes;
- migration fixtures load correctly;
- cancellation is verified for new long-running work;
- user data and existing output files remain intact after failure;
- documentation and versioned schemas are updated;
- unverified runtime claims are stated explicitly.

---

## 8. Primary risks and recommended containment

### Competing state authorities

**Risk:** current models, snapshots, UI state, timelines, and renderers retain independent copies.

**Containment:** use adapters first; migrate domain by domain; remove old mutation paths only after inspection.

### Windows-only regressions

**Risk:** portable core builds pass while Win32 dialogs or integration fail under MSVC.

**Containment:** add native Windows compilation and smoke tests to every Windows-facing release gate.

### Deep-coordinate precision loss

**Risk:** parameter variants or animation interpolation reduce camera coordinates to ordinary doubles.

**Containment:** define an explicit high-precision parameter value type or camera-specific adapter compatible with the existing compensated coordinate model.

### False visual-regression failures

**Risk:** driver variation creates unstable GPU comparisons.

**Containment:** canonical CPU fixtures, D3D11 WARP where suitable, backend-specific tolerances, exact plus structural checks, and explicit environment metadata.

### Undo history corruption

**Risk:** runtime playback or background jobs enter user history.

**Containment:** explicit change origins and history eligibility rules at the coordinator boundary.

### Export data loss

**Risk:** cancellation or encoding failure leaves corrupt final files or removes completed frames.

**Containment:** temporary names, atomic promotion, resumable manifests, and cleanup only after verified output.

### Excessive architecture before evidence

**Risk:** broad state rewrite creates long-lived parallel systems.

**Containment:** bounded first slice, no visible workflow change, camera and palette adapters only, and direct fixture evidence before expansion.

---

## 9. Recommended roadmap maintenance format

Each milestone should record:

```text
Status
Objective
Inspected evidence
Affected contracts
Implemented changes
Migration impact
Verification run
Verification passed
Native Windows status
Visual baseline impact
Known limitations
Next bounded slice
```

Avoid marking a phase complete based only on source-token checks. Completion should distinguish:

- source structure verified;
- portable core build passed;
- native MSVC build passed;
- runtime shader compilation passed;
- interactive UI flow tested;
- visual output accepted.

---

## 10. Final recommendation

Retain the original roadmap’s architectural direction, but revise it before further implementation.

The revised roadmap should:

1. use 1.13.1 as the inspected baseline;
2. begin with Windows release stabilisation;
3. implement visual regression before state refactoring;
4. introduce snapshot and parameter adapters without creating a second authority;
5. separate project, user, and runtime state;
6. use stable IDs for structural objects;
7. use hybrid undo operations;
8. build the deterministic evaluator before a complex animation UI;
9. implement frame-sequence export before FFmpeg encoding;
10. integrate the existing Fractal Scout instead of recreating it;
11. split the master roadmap into smaller maintained specifications;
12. require native Windows and visual evidence for Windows-rendering release claims.

With those changes, the roadmap becomes a practical implementation programme rather than only a strong conceptual architecture.
