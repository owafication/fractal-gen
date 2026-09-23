> **Historical implemented rendering plan:** Current forward roadmap is `docs/roadmaps/INTEGRATED-CREATIVE-ROADMAP.md`; existing Scout integration status is `docs/features/FRACTAL-SCOUT-STATUS.md`.

# Fractal Style Rendering Build Plan

## Objective

Extend Mandelbrot Live Wallpaper so it can produce highly detailed Tricorn and related anti-holomorphic fractal scenes with fine filament texture, tightly repeated cyan-to-orange boundary colours, deep zoom precision, controlled glow, and seam-free large-format export.

The work is staged so each release remains testable, reversible, and compatible with existing presets. New persisted fields use defaults that preserve older preset appearance.

## Phase 1 — Fractal families and colour texture

**Status: Implemented in 1.12.0**

- Expose named Tricorn/Mandelbar and Multicorn power-3/power-4 equation templates through the existing equation model.
- Add editable palette frequency, gamma, and linear/smooth phase interpolation.
- Add optional stripe-average orbit colouring with density, phase, strength, and start-iteration controls.
- Persist and validate all new settings.
- Apply the same controls in the CPU still renderer, OpenGL renderer, and Direct3D 11 renderer.
- Add a `Cyan Fire Ring` palette and a tuned `Tricorn Cyan Fire Ring` scene preset.
- Add round-trip, template, stripe-range, and preset regression tests.

### Phase 1 acceptance criteria

- Older presets load with frequency 8, gamma 1, linear mapping, and stripe colouring disabled.
- New settings survive settings export/import without changing their values.
- CPU results expose a finite stripe average in the range 0–1.
- GPU render inputs receive the same persisted parameters as CPU still rendering.
- The tuned Tricorn scene is available without requiring manual equation entry.

## Phase 2 — Conjugate distance estimation and edge lighting

**Status: Implemented in 1.12.1**

- Tracks the two real Jacobian columns for the exact power-2 Tricorn parameter map instead of applying an invalid analytic derivative.
- Uses the Jacobian's largest singular value as the local stretch in the distance estimate.
- Restricts support to the validated Tricorn profile; higher Multicorn powers and transformed conjugate formulas remain on the safe direct path with no fabricated distance value.
- Separates `edgeLightingStrength` from screen-space `glowStrength` bloom across persistence, validation, CPU still rendering, OpenGL, and Direct3D 11.
- Migrates older presets by copying their previous glow value into the new edge-light field once, preserving prior appearance while allowing independent editing afterward.
- Adds a finite-difference reference fixture and unsupported-profile regression coverage.
- Tunes `Tricorn Cyan Fire Ring` to use distance colouring, narrow edge light, and lower independent bloom.

### Exit criteria

- Distance values remain finite outside the set at tested scales.
- Edge lighting works for Tricorn without silently falling back to orbit-trap distance.
- Existing Mandelbrot distance-estimation output remains unchanged.

## Phase 3 — Deep Tricorn perturbation

**Status: Implemented in 1.12.2**

- Generates double and 128–512-bit fixed-point reference orbits for the exact power-2 Tricorn profile, applying conjugation before the quadratic recurrence.
- Adds an explicit perturbation-profile resolver so analytic quadratic maps and the validated Tricorn map are selected intentionally; higher Multicorn powers and transformed conjugate formulas remain unsupported.
- Implements the scaled conjugate perturbation recurrence in OpenGL and Direct3D 11:
  `q(n+1) = 2 conjugate(Z(n)) conjugate(q(n)) + scale conjugate(q(n))^2 + pixelOffset`.
- Extends split-float direct rendering to apply the Tricorn conjugation, providing a guarded fallback when perturbation deltas become non-finite or too large relative to the reference orbit.
- Refreshes the CPU reference orbit at the target coordinate when the perturbation guard is exceeded. GPU references are already regenerated when the camera or high-resolution tile centre changes, so each tile acts as a bounded rebase region.
- Adds direct-versus-perturbation regression tests at scales from `1e-4` through `1e-12`, reference-orbit recurrence fixtures, unsupported-profile tests, and explicit guard/refresh coverage.

### Exit criteria

- The renderer selects perturbation only for explicitly supported equation profiles.
- Deep Tricorn iteration and escape results match direct samples at the tested progressively deeper scales.
- Excessive deltas are detected and routed to a bounded fallback or refreshed reference instead of continuing with unstable perturbation values.
- Unsupported conjugate formulas continue to use a safe direct precision path.

## Phase 4 — Post-processing and composition

**Status: Implemented in 1.12.3**

- Builds on the independent edge-light and bloom strengths completed in Phase 2.
- Adds editable bloom threshold, soft knee, and radius fields with compatibility defaults matching the former fixed 3×3 bloom extraction.
- Replaces the fixed square bloom pass in OpenGL and Direct3D 11 with bounded horizontal and vertical separable passes supporting radii from 0–16 pixels.
- Calculates high-resolution GPU tile overlap from the active bloom radius plus the anti-alias reconstruction allowance, then crops the overlap before streaming image rows.
- Adds persisted camera rotation and applies the same rotated global-pixel mapping to preview, desktop, CPU still rendering, CPU static fallback, Direct3D 11/OpenGL rendering, tiled GPU export, drag panning, wheel zoom anchoring, and visual-change revision tracking.
- Keeps coordinate strings compatible as `centreX,centreY,scale`; rotation is an independent preset setting in Settings.

### Exit criteria

- Adjacent high-resolution tiles receive enough cropped overlap for the configured bloom kernel and anti-alias reconstruction allowance.
- Rotation preserves the selected complex-plane centre and output scale.
- CPU and both GPU backends use the same top-down global-pixel rotation convention.
- Rotated tile centres match the corresponding global output-pixel mapping in regression fixtures.

## Phase 5 — Fractal Scout

**Status: Implemented in 1.12.4**

- Adds a bounded deterministic golden-angle candidate search around the current preview camera.
- Scores a larger candidate pool using boundary mix, normalised iteration variance, edge density, detail, and an optional horizontal/rotational symmetry indicator.
- Sorts candidates deterministically and renders only the highest-scoring bounded result set as CPU thumbnails using the current equation, palette, rotation, and visual settings.
- Adds Fast, Balanced, and Detailed search budgets with hard ceilings for candidate count, sample grids, iterations, thumbnail dimensions, and retained BGRA memory.
- Runs searches on a cancellable worker thread and discards partial candidates after cancellation.
- Lets users refine around a selected candidate, apply it temporarily to the preview, or hand it to the existing Save as New workflow.
- Preserves saved preset identity and settings until the user explicitly chooses Use & Save as New.

### Exit criteria

- Search never modifies saved presets until the user explicitly saves a candidate.
- Candidate scoring and thumbnail ordering are deterministic for the same inputs.
- Thumbnail jobs respect cancellation and configured performance limits.
- Refinement starts from the selected candidate camera without changing its equation or visual style.

## Phase 6 — Targeted multi-scale Scout

**Status: Implemented in 1.12.5**

- Adds explicit Balanced, Boundary, Filaments, and Symmetry search targets with deterministic metric weights.
- Samples up to three logarithmically spaced depth bands around each golden-angle search position instead of forcing every candidate to the same scale.
- Applies greedy position-and-scale separation to the score-sorted pool so retained thumbnails are less likely to show near-identical locations.
- Relaxes the separation threshold deterministically only when required to return the bounded requested result count.
- Exposes the search target in the Fractal Scout dialog and reports how many near-duplicate candidates were suppressed.
- Keeps all candidate, thumbnail, iteration, cancellation, and retained-memory ceilings from Phase 5.

### Exit criteria

- Hand-authored metric fixtures rank differently under Boundary, Filaments, and Symmetry targets.
- Identical multi-scale requests produce identical cameras, scores, suppression counts, ordering, and thumbnail pixels.
- A bounded search retains candidates from more than one depth band when the scored pool permits it.
- Diversity selection preserves descending score order and never reduces the requested result count below the resolved resource limit.

## Verification strategy

Each phase should include:

- Model validation and settings round-trip tests.
- CPU reference tests for new mathematical output.
- Renderer contract checks covering CPU, OpenGL, and Direct3D 11 inputs.
- Build with warnings treated as errors under the available GCC and Clang toolchains.
- Native Windows/MSVC and shader-runtime validation when a Windows SDK and GPU test environment are available.
- High-resolution seam comparison tests when post-processing radius becomes configurable.

## Compatibility and rollback

- Persisted additions are optional and default to prior behaviour.
- Existing equation IDs and preset IDs remain stable.
- New equation templates are appended or intentionally indexed with tests guarding the current library mapping.
- Each phase is independently removable without requiring a mandatory settings migration because all new fields have compatibility defaults.
