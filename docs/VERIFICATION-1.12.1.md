# Verification — 1.12.1

## Scope

Implemented Phase 2 of `docs/FRACTAL-STYLE-BUILD-PLAN.md`: exact power-2 Tricorn Jacobian distance estimation and independent mathematical edge lighting.

## Automated coverage

- Exact Tricorn support-gate test.
- Finite, positive outside-set distance test.
- Central finite-difference comparison of both real Jacobian columns.
- Relative distance tolerance check against the finite-difference reference.
- Unsupported power-3 Multicorn support-gate and zero-distance regression.
- Independent bloom/edge persistence round trip.
- Pre-1.12.1 preset migration from combined glow to edge lighting.
- Tuned scene regression for distance colouring, edge strength, bloom strength, palette controls, and anti-aliasing.
- Existing analytic Mandelbrot distance output pinned to its pre-change value.
- CPU still-render fixture confirming Tricorn edge light changes output while bloom is disabled.

## Build matrix

Passed in the available environment:

- GCC 14 Release build with `-Wall -Wextra -Wpedantic -Werror`.
- GCC core tests: 1/1 passed.
- Clang 17 Release build with `-Wall -Wextra -Wpedantic -Werror`.
- Clang core tests: 1/1 passed.
- Clang AddressSanitizer and UndefinedBehaviorSanitizer core tests: 1/1 passed.

## Static renderer checks

The source verifier checks that:

- the CPU uses the real two-axis Jacobian and largest singular value;
- OpenGL float and native-float64 shaders contain conjugate Jacobian tracking;
- Direct3D 11 receives the strict support flag and independent edge-light constant;
- bloom continues to be supplied only to the post-process pass;
- the Equation Editor exposes separate Bloom and Edge light controls.

## Limits

The current environment does not include the Windows SDK, a Direct3D device, or an OpenGL desktop context. Native MSVC compilation, runtime HLSL/GLSL compilation, and visual CPU/GPU parity remain unverified here.
