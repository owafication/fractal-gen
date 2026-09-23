# Project Foundation

**Status:** Accepted current product scope; future programme items remain proposed  
**Purpose:** Own product identity, users, outcomes, scope, requirements, acceptance boundaries and non-goals  
**Owner:** Product and architecture  
**Reading trigger:** Scope, priority, feasibility or product-behaviour decisions  
**Update trigger:** Accepted requirement, scope or success-criterion change  
**Linked IDs:** REQ-001–REQ-041, AC-001–AC-041, RISK-001–RISK-032

## Objective

Maintain and extend **Mandelbrot Live Wallpaper**, a native offline Windows 10/11 x64 fractal-authoring application with file-backed desktop presentation. The application renders editable scenes in its preview and during explicit still/frame/video export jobs; the desktop host presents only saved images, image slideshows or already-exported local MP4 video and never continuously evaluates a fractal.

## Intended users

- Windows users who want a local interactive fractal-art editor, still/video exporter and low-overhead desktop presenter.
- Creative users editing cameras, equations, palettes, post-processing and journeys.
- Maintainers who need deterministic checks, recoverable settings, bounded resource use and native Windows release evidence.

## Current observed outcome

The supplied source contains a native Win32 C++20 application with:

- D3D11 rendering, OpenGL fallback and bounded CPU rendering for preview and explicit output work, not desktop presentation;
- preview editing, presets, palettes, data-only equations and deep-zoom precision controls;
- file-backed static image, slideshow and exported-video desktop modes;
- multi-monitor mirror and span layout;
- high-resolution tiled still output through PNG, JPEG, TIFF or BMP;
- adaptive resource protection, local diagnostics and local versioned JSON;
- a deterministic bounded Fractal Scout;
- CMake, core tests, release scripts and an installer definition.

This observation proves source presence and structure, not native compilation or runtime correctness.

## MVP boundary

The maintained baseline includes the schema-12 file-backed desktop scope accepted by DEC-039 and DEC-040. PH-03–PH-10 state/history, authoring animation, frame-sequence, external-video and Scout slices are implemented within their recorded evidence boundaries; PH-11 remains active, PH-12/PH-13 have bounded implemented slices, and PH-14/PH-15 remain proposed. No future phase may infer authority to restore a continuously rendered desktop mode or per-monitor preset assignment.

### In scope now

- Preserve preview editing, preset, Journey, timeline, Scout and explicit export workflows.
- Maintain only None, Static image, Slideshow and exported local MP4 Video as desktop modes.
- Keep static/slideshow/video desktop presentation file-backed and independent from fractal renderer selection, precision planning and animation evaluation.
- Establish canonical governance and traceability.
- Maintain the implemented release-version consistency gate.
- Prove the native Windows release build and startup path.
- Add production-renderer visual fixtures and render fingerprints.
- Maintain snapshot/parameter adapters without creating a second state authority.
- Maintain bounded runtime scalar and structural undo/redo while native Windows verification remains pending.

### Later programme scope

- General animation tracks and deterministic frame evaluation for preview and export authoring.
- Deterministic image-sequence export.
- External FFmpeg encoding after frame sequences are reliable; completed exports may be selected for desktop playback.
- Integration of existing Scout with shared snapshots, fingerprints and undo.
- Exact durable camera state, central precision planning, generation-safe reference orbits, validated perturbation correction and measured deep still/frame/video support under proposed PH-12–PH-15.

### Optional

- Style-only Scout exploration.
- Curated equation-family exploration.
- Lightweight Scout session persistence.
- H.265 or VP9 after the first supported encoded format.

### Excluded unless separately approved

- Network accounts, cloud sync, telemetry or analytics.
- Downloaded shader code, scripts or arbitrary equation expressions.
- Automatic desktop-file modification through Windows wallpaper settings.
- Continuous or on-the-fly fractal rendering behind desktop icons, including restoration of the removed Live or Journey desktop modes.
- Bundled FFmpeg or another codec runtime without licensing/distribution approval.
- Microservices, queues, databases, agents, vector stores or provider abstraction layers.
- Undo persistence across application restarts in the first undo release.
- Literal “infinite” or “unlimited” zoom claims. Deep support must be reported as measured limits per formula profile, backend, device class and output path.

## Requirements

### Current product requirements

- **REQ-001:** Run as a native Windows 10/11 x64 desktop application without administrator privilege.
- **REQ-002:** Keep user content and diagnostics local; no network client or account is required.
- **REQ-003:** Render previews and explicit output jobs through production CPU/D3D11/OpenGL paths with explicit bounded fallback behaviour; desktop presentation is outside this renderer fallback chain.
- **REQ-004:** Preserve compensated camera coordinates and supported deep-zoom precision strategies.
- **REQ-005:** Accept only bounded data models for imported settings, presets, palettes, equations and journeys.
- **REQ-006:** Keep desktop attachment reversible and preserve the configured Windows wallpaper.
- **REQ-007:** Superseded by REQ-041; retained as the historical None/Static/Live/Slideshow/Journey desktop-mode requirement.
- **REQ-008:** Persist settings and user assets through versioned, validated, failure-safe local files.
- **REQ-009:** Bound and cancel long-running Scout and high-resolution render work.
- **REQ-010:** Support local diagnostics without intentionally collecting unrelated user activity.

### Foundation and roadmap requirements

- **REQ-011:** Establish a native Windows release gate before architectural expansion.
- **REQ-012:** Add deterministic visual regression using production renderers.
- **REQ-013:** Add canonical render-state serialisation and a stable fingerprint.
- **REQ-014:** Introduce snapshot and parameter adapters over current authoritative models.
- **REQ-015:** Provide bounded transactional undo/redo with structural operations and coalescing.
- **REQ-016:** Provide deterministic general animation tracks while retaining Journey as a simpler workflow.
- **REQ-017:** Export deterministic frame sequences from immutable job snapshots with safe cancellation and resumability.
- **REQ-018:** Treat FFmpeg as an external executable and invoke it only through safe fixed argument construction.
- **REQ-019:** Integrate the existing Fractal Scout rather than recreate its candidate engine.
- **REQ-020:** Maintain native Windows, migration, visual, cancellation and resource evidence for affected releases.
- **REQ-021:** Parse and retain exact camera centre and half-height without first converting through binary floating point.
- **REQ-022:** Maintain one authoritative exact camera; approximate camera values are derived adapters and cannot be independently edited.
- **REQ-023:** Version canonical render identity so exact camera, formula capability and precision-plan identity affect fingerprints and export manifests.
- **REQ-024:** Use one deterministic platform-neutral precision planner for algorithm, precision, backend and fallback selection.
- **REQ-025:** Separate persisted artistic precision intent from transient device capability and resolved execution plan.
- **REQ-026:** Preserve the current analytic quadratic and exact power-2 Tricorn perturbation profiles as explicit versioned capabilities.
- **REQ-027:** Reject unsupported formula/profile combinations rather than entering an incompatible perturbation recurrence.
- **REQ-028:** Generate reference orbits through immutable, cancellable, bounded requests and typed results.
- **REQ-029:** Fence orbit, correction and upload results by generation and fingerprint so stale work cannot alter the current render.
- **REQ-030:** Report camera, reference, orbit-upload and delta precision separately; do not label an encoded GPU path by reference precision alone.
- **REQ-031:** Classify non-finite, unstable and unresolved perturbation output before a frame is accepted.
- **REQ-032:** Rebase or directly correct invalid perturbation work while preserving exact camera identity, or reject it.
- **REQ-033:** Add multiple references only through deterministic bounded planning after single-reference validity and correction pass.
- **REQ-034:** Derive full-frame, tile and sample coordinates from one exact global mapping with explicit halo/crop semantics.
- **REQ-035:** Enforce limits for precision bits, workers, cache, references, corrections, subdivisions, retries, iterations, memory, disk and cancellation.
- **REQ-036:** Preserve ordinary/current-limit output and performance within accepted fixtures while deep paths evolve.
- **REQ-037:** Evaluate continuous zoom, Journey, timeline and Scout promotion for preview/export authoring from immutable exact state and deterministic time rather than cumulative double stepping; these evaluators do not drive desktop presentation.
- **REQ-038:** Never promote incomplete or invalid still, frame or video output; preserve current temporary-file, source-frame and FFmpeg safety boundaries.
- **REQ-039:** Expose bounded deep diagnostics sufficient to reproduce camera, plan, formula, generation, validity, resources and render fingerprint.
- **REQ-040:** Publish only measured formula/backend/depth/resource limits and prohibit literal infinite/unlimited support wording.
- **REQ-041:** Desktop presentation supports only None, Static image, Slideshow and exported local MP4 Video, with Mirror or Span multi-monitor layout. Static/slideshow runtime decodes ordinary image files, video runtime uses the Windows media pipeline, and no desktop mode continuously evaluates the fractal. Runtime media failure stops presentation and is surfaced visibly.

## Acceptance boundaries

- **AC-001:** Current visible workflows remain unchanged by the first infrastructure slice.
- **AC-002:** Every release-facing version field resolves to one accepted version.
- **AC-003:** Native x64 MSVC build, resource compilation and linking complete without errors.
- **AC-004:** Application starts, shows preview, opens required dialogs and exits cleanly on supported Windows versions.
- **AC-005:** Existing settings and presets load without destructive migration.
- **AC-006:** Superseded by AC-041; retained as the historical continuously rendered desktop acceptance boundary.
- **AC-007:** Four canonical visual fixtures repeat within measured thresholds.
- **AC-008:** Tiled and full-frame output has no unexplained seam difference.
- **AC-009:** A deliberate render-affecting change produces an actionable fixture failure.
- **AC-010:** Camera and palette snapshot round trips preserve selected fields and low camera components.
- **AC-011:** No independent second source of truth is introduced during adapter migration.
- **AC-012:** Undo excludes runtime playback, export, discovery progress and adaptive decisions.
- **AC-013:** Broad user actions are one history entry and can be reversed without partial state.
- **AC-014:** Animation evaluation is deterministic for snapshot, timeline, time and seed.
- **AC-015:** Frame export uses exact frame-index timing and never promotes a partial final frame.
- **AC-016:** Cancellation preserves completed verified frames and leaves no corrupt final output.
- **AC-017:** FFmpeg failure preserves source frames and captured logs.
- **AC-018:** Applying a Scout candidate creates one undoable user action; preview alone does not mutate persisted project state.
- **AC-019:** Required governance owners and traceability are updated with each accepted contract change.
- **AC-020:** Release reports distinguish source inspection, compilation, tests, runtime checks, visual acceptance and unproven items.
- **AC-021:** Exact centre and half-height parse and canonicalise without binary floating-point conversion.
- **AC-022:** Copy, save, load and copy return identical canonical exact camera values.
- **AC-023:** Existing settings schema 9 and preset schema 2 files load without destructive camera change.
- **AC-024:** Legacy migration preserves the exact mathematical value of stored IEEE-754 fields and does not invent lost source digits.
- **AC-025:** Exact and approximate camera representations cannot be independently mutated.
- **AC-026:** Built-in Save As creates a new editable user preset retaining complete exact state.
- **AC-027:** Equivalent exact values fingerprint identically and any render-affecting exact digit change changes the versioned fingerprint.
- **AC-028:** Identical planner inputs produce the same plan, reason and fallback order.
- **AC-029:** D3D11 and OpenGL consume the central plan and retain no independent precision-policy thresholds.
- **AC-030:** Formula profiles accept only their defined versioned recurrence fingerprints.
- **AC-031:** Reference work is cancellable and bounded, and stale results cannot upload.
- **AC-032:** Diagnostics distinguish camera, reference, orbit-upload and delta precision.
- **AC-033:** Supported perturbation samples/tiles agree with accepted direct high-precision references.
- **AC-034:** Non-finite, unstable and injected-invalid perturbation output is classified before acceptance.
- **AC-035:** Rebase and correction preserve exact camera and avoid unexplained visible discontinuities.
- **AC-036:** Multi-reference planning terminates within reference, subdivision, correction, retry, memory and time budgets.
- **AC-037:** Unresolved pixels beyond policy fail the frame/job rather than being silently coloured.
- **AC-038:** Full-frame, tiled and reordered-tile output use equivalent global sample mapping and accepted seam tolerance.
- **AC-039:** Cancellation, device loss, exit, write failure and FFmpeg failure cannot promote incomplete final output.
- **AC-040:** Release documentation reports measured profile/backend/depth limits and contains no literal infinite/unlimited claim.
- **AC-041:** None, Static image, Slideshow and exported MP4 Video can be started/stopped without replacing the configured Windows wallpaper; removed Live/Journey settings migrate to None, removed Independent monitor settings migrate to Mirror, asynchronous video or exhausted-slideshow failure stops visibly, and no failure path starts a fractal desktop renderer.

## Success criteria

The programme succeeds when the baseline builds and runs under the intended Windows release toolchain, version metadata is consistent, deterministic visual fixtures protect preview/export rendering, state changes have one controlled path, exports are failure-safe, desktop presentation remains file-backed and reversible, and every completion claim points to proportionate evidence.

## Constraints and assumptions

- **Observation:** The originally supplied archive has no Git metadata.
- **Implemented:** The repository-execution working copy uses the root containing `CMakeLists.txt`, branch `main`, baseline commit `8f82119`, and no remote.
- **Observation:** Windows-facing code depends on the Windows SDK and system libraries listed in CMake.
- **Observation:** The current settings schema is 11 and preset serialisation emits schema 3.
- **Assumption:** Existing public UI terminology and file formats should remain stable unless a managed change is approved.
- **Accepted decision:** Retain application version 1.13.1 while correcting and validating metadata; no new release number is invented by the stabilisation work.
