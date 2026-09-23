# Visual Regression

**Status:** In progress; portable CPU foundation and a bounded D3D11 WARP production-readback self-check are implemented. Approved baselines, hardware D3D11, OpenGL and cross-environment threshold calibration remain open.
**Purpose:** Own deterministic fixture design, baseline approval, comparison metrics and artifacts
**Owner:** Rendering quality
**Reading trigger:** Any render, equation, colour, precision, post-processing, tiling or export change
**Update trigger:** Fixture, backend, metric, threshold or approval change
**Linked IDs:** REQ-012, REQ-013, AC-007–AC-009, VAL-009–VAL-013

## Implemented portable foundation

PH-02 now includes `MandelbrotVisualFixtures`, a bounded command-line harness that invokes the production `RenderStillImageTiled` CPU path. It does not contain a second fractal renderer.

Implemented fixtures:

1. `standard-mandelbrot` — full-view Mandelbrot mapping and palette output.
2. `tricorn-cyan-fire-ring` — conjugate recurrence, distance colouring, stripes, mathematical edge lighting and smooth palette interpolation.
3. `rotated-bloom-state` — rotated mapping, anti-aliasing, edge lighting and bloom-overlap state.
4. `deep-mandelbrot-perturbation-state` — compensated deep camera state and high-iteration Mandelbrot boundary output.
5. `tiled-versus-full-seams` — full-width versus scanline-tiled output plus dedicated tile-boundary strips.
6. `deliberate-visual-mutation` — palette-offset and depth mutation that must produce a non-zero diagnostic diff.
7. `fractal-scout-thumbnails` — two bounded production Scout searches whose three retained exact-camera candidate identities and non-uniform thumbnails must match exactly while remaining visually distinct from one another.

The CPU renderer does not apply the GPU screen-space bloom blur and does not execute the GPU perturbation shader. Those fixture names preserve the production scene/state being carried forward; D3D11/OpenGL fixture expansion must validate the backend-specific effects and precision path before PH-02 can complete.

## Running the fixtures

Portable:

```bash
./scripts/run-visual-fixtures.sh
```

Windows PowerShell or CMD:

```powershell
.\scripts\run-visual-fixtures.ps1
.\scripts\run-visual-fixtures.cmd
```

Run one fixture:

```powershell
.\scripts\run-visual-fixtures.ps1 -Fixture tricorn-cyan-fire-ring
```

Compare manually approved baselines:

```powershell
.\scripts\run-visual-fixtures.ps1 `
  -BaselineDirectory .\tests\baselines\visual `
  -BaselineMaximumChannelError 0 `
  -BaselineMaximumDifferingRatio 0 `
  -BaselineMinimumSsim 1
```

The baseline layout is:

```text
tests/baselines/visual/<fixture>/cpu/baseline.ppm
```

No command copies candidate output into that location. Baseline promotion remains an explicit review action.

## Current artifact layout

The portable command writes deterministic binary PPM images and JSON metadata:

```text
test_artifacts/visual/<fixture>/cpu/candidate/
  current.ppm
  repeat.ppm
  diff.ppm
  metrics.json
  environment.json
  state.json
  render-state.canonical

test_artifacts/visual/tiled-versus-full/cpu/candidate/
  full.ppm
  tiled.ppm
  diff.ppm
  metrics.json
  seam-strips/
    full-seams.ppm
    tiled-seams.ppm
    diff.ppm
    metrics.json

test_artifacts/visual/deliberate-mutation/cpu/candidate/
  expected.ppm
  mutated.ppm
  diff.ppm
  metrics.json
  expected-state.json
  mutated-state.json
  expected-render-state.canonical
  mutated-render-state.canonical

test_artifacts/visual/fractal-scout-thumbnails/cpu/candidate/
  state.json
  rank-1..3/
    current.ppm
    repeat.ppm
    diff.ppm
    metrics.json
```

Generated artifacts are ignored by Git. Approved compact baselines and their review metadata may be committed later under `tests/baselines/visual/`.

The Scout fixture records the resolved bounded request, compiler/system identity, V2 exact-camera candidate identities, scores, metrics, exact/legacy cameras and image hashes. It invokes `RunFractalScout`, whose thumbnails use the production `RenderStillImageTiled` path; it does not add a test-only search or renderer. Candidate generation remains separate from baseline approval.

## Metrics

The implemented comparator reports:

- maximum per-channel absolute error;
- differing-pixel count and ratio above the selected error floor;
- mean absolute channel error;
- root-mean-square channel error;
- global luminance structural similarity;
- fixture-local image hashes;
- exact dimension and pixel identity.

Same-process repeatability and the current CPU seam fixture require exact equality. Baseline thresholds are explicit command inputs and are recorded in the invocation; there is no guessed project-wide tolerance.

`environment.json` now records the PH-03 canonical SHA-256 render fingerprint and `mw-render-state-v1` contract. `render-state.canonical` contains the exact hashed bytes. Fixture image hashes remain FNV-1a diagnostics and are not substitutes for the render-state fingerprint.

## Harness requirements

- Invoke production still/render code.
- Run headlessly through a bounded fixture command.
- Pin dimensions, time, scene state, anti-aliasing and output mapping.
- Emit current/repeat or baseline/current images, difference image, metric JSON, human-readable state, canonical render-state bytes and environment metadata.
- Return non-zero on render, repeatability, seam, baseline or mutation-check failure.
- Avoid timestamps and encoder metadata in pixel identity.
- Keep candidate generation separate from baseline approval.

## Backend strategy

- Canonical portable path: CPU production renderer — implemented.
- Reproducible Windows GPU-like path: D3D11 WARP hidden-window production render/readback — implemented; exact same-process repeatability at 256x144 is enforced for fixed standard, bloom-enabled and explicit-perturbation scenes by `MandelbrotD3D11WarpFixtures`.
- Hardware D3D11: environment-specific production readback evidence recorded on AMD Radeon RX 7900 XT / Direct3D 11.1; approved baseline comparison remains open.
- OpenGL: hidden production context/readback fixture passed on the recorded AMD OpenGL 4.6 environment; approved baseline comparison remains open.
- Never compare output across backends as exact unless repeatability and equivalence are demonstrated.

## Calibration and approval

1. Produce repeated renders on the same pinned environment.
2. Measure natural variation.
3. Investigate variation before selecting thresholds.
4. Set the narrowest fixture/backend/environment threshold justified by evidence.
5. Run the deliberate mutation and prove it is rejected.
6. Review current, candidate baseline and diff images.
7. Record reviewer acceptance, environment, metrics and linked requirement/decision.
8. Copy the reviewed candidate to the baseline tree in a separate explicit change.

Do not auto-approve baselines in CI.

## Remaining PH-02 gates

- Run the portable fixture command under native MSVC and record repeatability evidence.
- Approve compact CPU baselines after visual review on the selected reference environment.
- Calibrate and expand the D3D11 WARP fixture set beyond its bounded standard, bloom and perturbation repeatability scenes.
- Implement supported OpenGL offscreen fixtures and calibrate them separately.
- Validate actual GPU bloom blur and perturbation execution, not only their portable state.
- Decide whether later artifacts should additionally use PNG; PPM currently avoids encoder metadata and external dependencies.
- Record AC-007–AC-009 and VAL-009–VAL-013 evidence at their final required scope.

## Failure triage

1. Compare the canonical render fingerprint and `render-state.canonical` before comparing fixture-local image hashes.
2. Confirm backend, compiler, precision policy, dimensions, time and tile width.
3. Separate raw fractal, colour, post-processing and tile composition.
4. Inspect dedicated seam strips.
5. Reproduce on the canonical CPU path.
6. Reject threshold changes until the difference is explained.

## Proposed deep-zoom fixture extension

Before PH-12 changes visible behaviour, preserve ordinary/current-limit CPU, D3D11 and OpenGL captures and add versioned fixtures:

- `DZ-FIX-001` ordinary Mandelbrot; `DZ-FIX-002` compensated Seahorse Valley; `DZ-FIX-003` analytic quadratic near the current limit;
- `DZ-FIX-004` exact camera beyond the current double-scale product cap;
- `DZ-FIX-005`/`006` current/deeper exact power-2 Tricorn;
- `DZ-FIX-007` high-iteration filament and `008` periodic/near-critical region;
- `DZ-FIX-009` natural single-reference instability and `010` injected non-finite/error-mask;
- `DZ-FIX-011` multi-reference stress and `012` correction-budget boundary;
- `DZ-FIX-013` unsupported Julia and `014` unsupported absolute/Burning Ship refusal;
- `DZ-FIX-015` odd portrait tiles and `016` ultrawide rotated bloom/halo tiles;
- `DZ-FIX-017` long deterministic continuous zoom and `018` pause/resume/exact seek;
- `DZ-FIX-019` Scout exact promote/Apply and `020` device-loss/cancelled export recovery.

Each fixture records exact camera, formula/plan/fingerprint versions, dimensions/rotation/AA/post-processing, iterations/time/seed, precision stages, reference/encoding, backend/device/driver, thresholds, provenance and digest. Classification disagreement is not hidden by colour similarity; unresolved pixels fail except in a fixture explicitly testing failure reporting. Tile order and worker count must not change deterministic export output.
