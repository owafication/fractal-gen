# Mandelbrot Live Wallpaper — Deep-Zoom Upgrade Pack

**Mode:** File generation  
**Pack status:** Proposed  
**Pack version:** 2.0  
**Source baseline:** Mandelbrot Live Wallpaper 1.13.1, archive labelled PH-09 roadmap progress  
**Application source changed:** No  
**Canonical phase status changed:** No

## Objective

Provide a source-reconciled foundation for upgrading the existing deep-zoom system from its current bounded `double`/compensated-coordinate and single-reference perturbation design to a deterministic, exact-coordinate, resource-bounded workflow.

The target is continuous deep zoom without:

- coordinate collapse or loss during copy, persistence, animation, Scout, still or video workflows;
- frozen, repeated, block-distorted or plausibly incorrect pixels;
- unexplained D3D11/OpenGL/CPU precision-selection differences;
- stale or synchronous reference-orbit work blocking or overwriting newer state;
- visible tile seams or tile-order-dependent output;
- uncontrolled worker, RAM, VRAM, cache, correction, retry or temporary-storage growth;
- unsupported formula families entering an incompatible perturbation path.

## Evidence basis

- Source archive SHA-256: `630cf8089eadfce33c4edecb99e9596793d1be8e32052823a8cffabd2f86757d`
- Supplied proposal SHA-256: `fc99cd2c0804e9080e172e61ca1458cf2b4cb30647d9815205f971338bbd1de9`
- The archive was extracted and inspected as a 256-entry source tree.
- The portable GNU 14.2 Release core build completed with warnings treated as errors.
- CTest passed 4/4: core tests, path tests, CPU visual fixture self-check and version consistency.
- `scripts/verify-source.py` failed at its PH-03 AppWindow gesture-marker check. The verifier expects separate `PreviewPan` and `PreviewWheelZoom` markers, while the current AppWindow routes both through `ParameterGestureKind::PreviewNavigation`. This baseline inconsistency is not treated as an application runtime failure, but it must be reconciled before new deep-zoom work.

## Source-specific correction to the supplied proposal

The application does **not** lack perturbation rendering. It already contains:

- `Float32`, `Float64`, split-float, double-reference perturbation and arbitrary-reference perturbation modes;
- compensated camera-centre high/low values;
- analytic quadratic and exact power-2 Tricorn perturbation profiles;
- a custom fixed-point reference-orbit generator selectable at 128, 256 or 512 bits;
- D3D11 and OpenGL reference-orbit upload and perturbation shaders;
- GPU-tiled high-resolution still output;
- deterministic CPU frame-sequence output and external FFmpeg H.264/MP4 encoding.

This pack therefore hardens and extends existing contracts. It does not propose a parallel renderer or pretend these capabilities are absent.

## Claim boundary

“Infinite zoom” is not an accepted technical or product claim.

Approved direction:

> Continuous deep zoom with dynamically planned precision, bounded resources and reported measured limits for the selected formula, backend, device and output workflow.

Every release claim must identify tested depth, formula profile, output size, backend, device class, precision plan and unresolved limitations.

## Frozen integration ranges

Before repository integration, confirm these ranges remain unused:

- phases: `PH-12`–`PH-15`;
- requirements: `REQ-021`–`REQ-040`;
- acceptance criteria: `AC-021`–`AC-040`;
- validation: `VAL-037`–`VAL-060`;
- decisions: `DEC-028`–`DEC-037`;
- risks: `RISK-013`–`RISK-030`;
- routes: `ROUTE-015`–`ROUTE-018`.

Never renumber accepted IDs. If any range is occupied, allocate a new range and update every mapping before implementation.

## Pack files

1. `README.md`
2. `PACK_MANIFEST.md`
3. `SOURCE_ASSESSMENT_AND_DIRECTION.md`
4. `DEEP_ZOOM_REQUIREMENTS_AND_SCOPE.md`
5. `DEEP_ZOOM_ARCHITECTURE_AND_CONTRACTS.md`
6. `DEEP_ZOOM_BUILD_PLAN.md`
7. `DEEP_ZOOM_VALIDATION_AND_TRACEABILITY.md`
8. `DEEP_ZOOM_RISKS_DECISIONS_AND_MIGRATION.md`
9. `CANONICAL_INTEGRATION_AND_HANDOFF.md`
10. `AUDIT_REPORT.md`

## Reading sequence

1. Repository `/AGENTS.md`.
2. `project_docs/PROJECT_INDEX.md`.
3. Current active phase and canonical owners.
4. `SOURCE_ASSESSMENT_AND_DIRECTION.md`.
5. Requirements and architecture in this pack.
6. Only the proposed phase being implemented.
7. Directly relevant source, tests, shaders, persistence and UI routes.
8. Validation, risk and migration sections affected by the change.

## Completion boundary

The deep-zoom upgrade is complete only when:

- exact camera centre **and scale** survive all durable and deterministic workflows;
- one precision planner owns algorithm selection for preview, wallpaper, still and frame export;
- formula capability is fingerprinted and fail-closed;
- asynchronous orbit results cannot reach a stale generation;
- supported perturbation output agrees with a direct high-precision reference within approved criteria;
- unstable pixels are corrected, safely rebased or explicitly rejected rather than silently rendered;
- tiled and full-frame mappings are equivalent at global sample coordinates;
- cancellation, device loss, application exit and encoder failure cannot promote incomplete output;
- long-session RAM, VRAM, cache, worker and temporary-storage growth remains within accepted budgets;
- ordinary-zoom visual fixtures remain within approved tolerances;
- native Windows D3D11, OpenGL, UI, device-loss and real-FFmpeg evidence is recorded;
- the support matrix states measured limits and contains no literal infinite/unlimited claim.
