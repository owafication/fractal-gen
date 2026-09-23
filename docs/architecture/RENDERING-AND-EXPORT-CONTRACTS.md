# Rendering and Export Contracts

**Status:** Current rendering/output contract; proposed deep sections are not completion claims  
**Purpose:** Own backend, precision, deterministic render, tiling and output boundaries  
**Owner:** Rendering  
**Reading trigger:** CPU/D3D11/OpenGL, precision, post-processing, still/export or visual-test work  
**Update trigger:** Renderer input, fallback, output, tile, precision or determinism change

## Current observed renderer boundary

- `RenderRegion` carries camera, rotation, palette/custom colours, iterations, equation, colour mapping, post-processing and colours.
- `RenderOptions` carries render scale, anti-aliasing, precision and time.
- `GpuRenderer` is the facade for D3D11/OpenGL selection.
- `StillImageRenderer` exposes bounded tiled CPU rendering, progress/cancellation, tile camera mapping and overlap calculation.
- Windows WIC integration supports PNG, JPEG, TIFF and BMP; its row encoder is reusable by high-resolution still and PH-08 sequence paths.
- `FrameSequenceExport` owns deterministic PNG job identity, rational timing, manifest/receipt recovery, verification and resume.
- High-resolution GPU tile rendering exists in the UI/rendering integration; exact runtime behaviour is unproven here.

## Backend policy

1. Interactive preview automatic mode attempts D3D11 then documented OpenGL fallback. Desktop presentation never uses this fallback chain.
2. Explicit export backend selection must not silently switch unless the UI/contract explicitly allows it.
3. Production renderers are also test renderers; do not implement a visually separate test renderer.
4. Report selected and actual active backend/fallback separately.
5. One bounded recovery attempt; no restart loop.

## Desktop presentation contract

- `WallpaperController` exposes only static gallery, slideshow and exported MP4 playback. It has no continuously rendered `Start` route and no per-tick fractal render or CPU-render fallback.
- Static/slideshow images are decoded through WIC into ordinary pixels and painted behind desktop icons. Slideshow ticks only select and decode the next configured file.
- Video accepts an existing local `.mp4`, validates that Windows Media Foundation finds a selected video stream, mutes it, loops it and scales it to the desktop host. It does not invoke FFmpeg or any fractal renderer.
- Capturing the current preview as a static image may perform one explicit bounded render to create a normal image file; after capture, desktop presentation is file-backed and the capture renderer is shut down.
- Missing or invalid files fail visibly and stop. A static or video failure must not fall back to continuous fractal calculation.

## Precision contract

- Preserve compensated camera high/low components.
- Resolve precision from requested mode, scale, hardware capability and equation compatibility.
- Do not claim perturbation for unsupported formulas.
- Deterministic tests/export pin precision policy and record actual selected strategy.
- Adaptive interactive quality may not alter deterministic fixture/export output.

### Implemented legacy preview precision recovery

- Automatic preview precision is resolved from the current frame camera on every production render through the platform-neutral legacy policy. D3D11/OpenGL supply capabilities; they do not own the zoom thresholds.
- The main coordinate admission path uses the active preview renderer's capability report, allowing automatic float32, split high/low and perturbation/reference selections instead of requiring the unrelated exact-CPU export capability.
- Accepting changed precision settings performs one bounded preview-renderer reinitialisation. A renderer stopped by a prior shader or precision failure is also restarted when Settings closes, so it is not permanently disabled for the process lifetime.
- On the tested AMD OpenGL driver, the native-float64 shader retains double coordinate and recurrence arithmetic but routes transcendental helpers through float built-ins when double overloads are unavailable. This is a compatibility boundary, not a claim of full-double transcendental accuracy or a validated deep GPU envelope.

## Deterministic render contract

A deterministic request pins:

- project/render snapshot and fingerprint;
- output dimensions and full/tile mapping;
- backend and precision policy;
- maximum iterations and AA pattern;
- fixed animation/coefficient time;
- random seed and evaluation order;
- palette ordering and stable IDs;
- post-processing and colour-space assumptions;
- metadata excluded from pixel comparison.

## Tiling contract

- Tile camera mapping uses global top-down output coordinates.
- Rotation convention matches full-frame output.
- Overlap includes active bloom radius and AA reconstruction allowance.
- Cropped tiles compose to the same expected viewport as full output.
- Seam strips receive stricter review than global metrics.
- Large output is streamed/banded; do not require unbounded full-frame memory.

## Output safety

- Validate dimensions, DPI, format, quality and path before work.
- Render to temporary frame/output paths.
- Flush/close and validate expected dimensions/format before promotion.
- Promote atomically where possible.
- On cancellation/failure, keep the previous final file intact.
- Frame-sequence jobs preserve verified completed frames, refuse untracked finals, and use a typed fingerprint-bound manifest plus per-frame prepared receipts.

## Current and future output

### Current observed

- Static capture and slideshow images.
- High-resolution still output: PNG/JPEG/TIFF/BMP through WIC.
- Tiled GPU/CPU strategies described and represented in source/docs.

### Implemented PH-08

- Deterministic PNG frame sequences through the production CPU tiled renderer.
- Rational direct-index timing, pinned preset quality, temporary/validated promotion, atomic manifest and exact resume.

### Proposed

- Optional TIFF/BMP sequence only if justified.
- External FFmpeg H.264/MP4 after frame validation.
- H.265/VP9 optional and separately accepted.

## Renderer change acceptance

- Applicable CPU/D3D11/OpenGL checks pass.
- Visual fixtures show intended changes only.
- Deep precision and equation-compatibility fixtures pass.
- Tiled/full seam evidence passes.
- Failure/fallback path is bounded and reported.
- Output cancellation and prior-file preservation pass.

## PH-09 external video contract

- `FrameSequenceManifest` and verified receipts remain the only accepted source authority; video export does not rerender or infer missing frames.
- `ExternalVideoExportJob` is immutable after preparation and binds source job identity, rational frame rate/count, encoder capability version line, fixed codec/pixel format, CRF, preset, paths and cleanup policy.
- Process execution is a Windows-integration concern. The core produces argument vectors and validates outcomes; `ExternalProcess` owns `CreateProcessW`, pipes, bounded captures, timeout/cancellation and exit status.
- The encoder writes only to an adjacent owned temporary MP4. A fixed decode probe must succeed before promotion; an existing final output is never overwritten.
- Fixed yuv420p output admits only even source width and height; an incompatible verified PNG sequence is rejected during immutable job construction rather than delegated to FFmpeg failure.
- Failures and cancellation preserve all verified PNG frames. Optional cleanup is a separate post-promotion step limited to manifest-tracked files.
- FFmpeg remains external, optional and runtime-selected. No executable path, encoder binary or video job enters project/settings persistence or undo history.

## Proposed PH-13–PH-15 deep-render contract

BR-20260730-01 changes governance only. Current D3D11/OpenGL/CPU code, precision thresholds, orbit encoding and export formats remain production behaviour until their owning phases pass.

### Central plan and formula capability

One platform-neutral deterministic `PrecisionPlanner` consumes exact camera, versioned formula capability, persisted intent, output class, backend/device capability and enforced resource budget. Its immutable result records algorithm/backend/fallback order, camera/reference/orbit-upload/delta precision, reference/tile/correction policy versions, reproducibility class and reason. D3D11 and OpenGL consume the plan; they do not invent thresholds or fallback order. Their legacy `CameraState` compatibility selection also uses the same planner: backends report Float64/split/perturbation/reference capabilities only, while the `1e6`, `1e13` and `1e14` compatibility thresholds live centrally. The accepted near-unlimited direction is adaptive CPU exact precision: a bounded ladder selects the smallest adequate tier (512, 2,048, 8,192 or 16,384 bits) and refuses rather than round if no tier can meet a request. Persisted exact text remains broader than executable precision; a valid stored camera is not a promise of an unbounded export.

Analytic quadratic and exact power-2 Tricorn remain distinct supported capability fingerprints. Unsupported formula/profile combinations fail closed. The existing four-float orbit upload is now named `mw-orbit-float4-expansion/v1` (four 32-bit float components per coordinate). Its descriptor explicitly records that no validated precision ceiling exists yet; it is not evidence of end-to-end 512-bit output. BR-20260731-01 adds deterministic WARP evidence that the production immutable `R32G32B32A32_FLOAT` real/imaginary textures retain their uploaded float components bit-for-bit through staging readback. BR-20260803-01 additionally measures the ordered float reconstruction used by both perturbation shaders against the Boost-512 producer for two bounded legacy-camera fixtures. BR-20260803-02 makes an absent validated envelope explicit in the central plan: a declared float4 transport with zero validated bits requires direct correction and cannot imply GPU-safe deep execution. This proves the declared transport payload and fixture-local reconstruction/error policy only; it does not establish a deep-precision ceiling, validate a hardware adapter or satisfy end-to-end perturbation comparison.

Current core identifiers are `analytic-quadratic-mandelbrot/v1` and `tricorn-power2/v1`. `ReferenceOrbitService` verifies that the equation still resolves to the profile embedded in its immutable plan, refusing a stale or substituted formula before reference work begins. These identifiers are not yet part of a renderer upload, job manifest or measured GPU support claim.

### Reference orbit service

Reference work uses immutable typed requests/results keyed by exact camera, formula capability, iteration/precision/encoding and planner version. A bounded worker pool supports cancellation, request coalescing and byte-accounted cache eviction. Every result carries a generation/fingerprint and uploads only on the matching render generation. Shutdown stops new work, advances generation, cancels/drains within policy and destroys GPU resources only after owned work is safe.

The current foundation remains deliberately narrow: `ReferenceOrbitService` accepts only a planner-selected Boost CPU tier whose backend and selected bits match one of 512, 2,048, 8,192 or 16,384 bits. It verifies the current `mw-precision-plan-v1`, registered formula profile, current `mw-orbit-float4-expansion/v1` encoding and 32–4096 iteration bound, supports cooperative cancellation, byte/entry-bounded LRU reuse, and stamps the caller's generation on a cache hit. Cache keys and worker coalescing include the plan/encoding identities; a result returns the full immutable plan, plan/encoding versions and formula capability. Coalesced subscribers share only numerical orbit work: each delivery restores its subscriber's plan, including `requiresDirectCorrection`, rather than inheriting policy from the first request. `ReferenceOrbitWorker` adds one bounded platform-neutral worker (eight independent queued keys and 64 retained subscribers per key by default), deterministic equal-key coalescing, per-subscriber cancellation, and generation stamping at delivery. A zero or exhausted subscriber limit refuses work without retaining it. Its shutdown boundary refuses new work, cancels all subscribers, joins the owned worker and releases queued/active request state before returning; completion callbacks must not synchronously invoke shutdown on their own worker thread. The float4 transport is still unvalidated for deep GPU consumption, and no CPU/GPU perturbation or correction renderer consumes the service yet.

### Perturbation validity and correction

Both backends share recurrence/index/bailout/smoothing semantics and emit validity/classification data before final colour acceptance. The platform-neutral perturbation result now distinguishes `Stable`, `Rebased` and `Unresolved`: an instability with reference refresh disabled is explicitly unresolved, while a refreshed reference is classified separately from an ordinary stable sample. Non-finite, unstable and unresolved pixels are rebased, directly corrected within budget, or rejected. Multiple references may be introduced only after single-reference correction passes; reference count, corrected pixels, subdivision, retries, memory and time are deterministic and bounded. Worker/tile completion order cannot change reference assignment or deterministic output.

### Exact mapping, resources and diagnostics

Full-frame, tile and sample coordinates derive from one exact global mapping with explicit anti-aliasing, halo and crop semantics. Resource pressure may cancel stale work, evict cache, reduce in-flight/prefetch/interactive presentation quality or select a safe plan, but never silently reduce required numerical precision. Diagnostics record exact camera summary/digits, capability/plan versions and reason, backend, precision stages, reference/cache/validity/correction/tile metrics, CPU/RAM/VRAM/temp use, cancellation/fallback and render fingerprint without logging full coordinates by default.

The current Boost-512 direct primitive consumes an unrotated `ExactStillRenderSample` (canonical camera plus rational global-pixel offsets) and returns only bounded escape classification. It rejects rotation, unsupported profiles, invalid iteration/rational input and cancellation. It is not a colour/tile/export renderer and cannot be used to certify perturbation correction or full-frame deep output.

`RenderExactDirectStillImage` now composes the direct primitive with the existing CPU colour contract into streamed output rows. It supports planner-selected Boost-512, Boost-2048, Boost-8192 or Boost-16384, unrotated AA 1–4 stills with 32–4096 iterations and static equation coefficients. Its request may declare full-frame dimensions plus a tile origin; a bounded crop then evaluates the same global exact pixels as the corresponding full image and fails when it exceeds the declared frame. Its one-row working allocation is admitted only up to 64 MiB, preventing an oversized exact request from allocating without bound. Cancellation is checked before each row and within high-precision iteration; a cancellation before a row completes publishes no partial row. Every AA subpixel is represented by an exact rational half-height factor; no camera/subpixel coordinate is first converted to binary floating point. A representable small fixture is equivalent to ordinary CPU output; deterministic AA-2 and bounded AA-4 fixtures pass, while `1e-200`, `1e-1000` and `1e-3000` fixtures prove the 2048-bit, 8192-bit and 16384-bit routes complete without legacy adaptation. The exact evaluator has no coefficient-time phase, so animated coefficients fail closed at sample, still/high-resolution and frame job/per-frame boundaries. High-resolution and frame-sequence UI selection can request these CPU renderers; selection and tier are immutable in the job/manifest and unsupported input fails without fallback. When the high-resolution route is selected, its status identifies the exact CPU direct evaluator rather than the compatibility tiled CPU path. This does not establish a UI-integrated deep tile scheduler, rotated, time-varying-equation, GPU, correction or video output.

### Still, frame and FFmpeg boundary

Deep still/frame jobs are immutable and bind exact snapshot, plan, validity, mapping and user-selected renderer versions. Frame jobs use schema 3 and `mw-render-state-v3-exact-precision-plan`, which records the planner version, capability, reason, required bits, selected bits and execution backend alongside exact camera text. After per-frame validation, the frame callback receives that immutable `PrecisionPlan`; the Windows exact CPU route consumes its selected bit tier from the plan rather than re-deriving it from renderer text. A schema-1/v1 or schema-2/v2 manifest is rejected rather than resumed under a possibly different precision route. Exact CPU frame jobs reject rotation and AA outside 1–4 during job creation; the same check runs after each timeline evaluation before that frame's renderer callback. Before export, the user selects CPU-reference or GPU rendering. The job/manifest records that selection and its renderer classification; GPU export is available only for a validated profile and fails rather than silently falling back to CPU. The current executable registry contains only `cpu-production-still` and the four exact CPU Boost tiers; an unregistered renderer ID is refused during immutable job construction, manifest save/load, and the Windows callback rather than invoking the CPU path. A frame with unresolved pixels beyond policy is invalid and cannot enter a manifest or FFmpeg source set. FFmpeg remains only an external encoder after every source frame passes validity; fixed vectors, bounded logs, decode probe, source preservation and atomic promotion remain mandatory.
