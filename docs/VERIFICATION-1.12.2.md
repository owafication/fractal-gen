# Verification — 1.12.2

## Scope

Completed Phase 3 of `docs/FRACTAL-STYLE-BUILD-PLAN.md`: deep perturbation for the exact power-2 Tricorn profile.

## Core mathematical checks

- Double Tricorn reference orbit applies conjugation before squaring.
- Arbitrary-precision Tricorn reference orbit applies the same recurrence.
- Perturbation profile resolver accepts the exact Tricorn and analytic quadratic profiles.
- Higher Multicorn powers remain rejected.
- Tricorn perturbation matches direct escape/iteration results at scales `1e-4`, `1e-8`, and `1e-12` for the tested sample.
- Excessive perturbation deltas trigger deterministic reference refresh.

## Renderer contract checks

- OpenGL split-float recurrence applies conjugation.
- OpenGL perturbation uses conjugated reference and delta values.
- Direct3D 11 split-float recurrence applies conjugation.
- Direct3D 11 perturbation uses the same scaled recurrence.
- Both GPU shaders contain a relative-delta guard and safe direct fallback.
- Reference orbit cache keys include equation and camera identity; high-resolution tile cameras therefore generate tile-local reference orbits.

## Verification commands

- `python3 scripts/verify-source.py`
- GCC Release configuration/build/tests with warnings treated as errors.
- Clang Release configuration/build/tests with warnings treated as errors.
- Clang AddressSanitizer and UndefinedBehaviorSanitizer build/tests.
- ZIP integrity and clean packaged-tree checks.

## Limits

Native MSVC compilation, runtime GLSL/HLSL compilation, GPU visual comparison, and ultra-deep image comparison beyond the CPU fixtures remain unverified in this environment because it does not provide the Windows SDK or a GPU rendering context.
