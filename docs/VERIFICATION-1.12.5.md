# Verification — 1.12.5

## Scope

Extended Fractal Scout with target-specific scoring, deterministic multi-scale candidate cameras, and position/scale diversity selection.

## Automated coverage

- Boundary fixtures outrank filament-heavy fixtures under the Boundary target.
- Filament-heavy fixtures outrank boundary-only fixtures under the Filaments target.
- Symmetric fixtures outrank boundary-only fixtures under the Symmetry target.
- Resolved depth-band count, spread and result separation retain bounded valid requests.
- Excessive or non-finite depth/diversity requests are capped or replaced with deterministic defaults.
- Ranked searches suppress near-duplicate candidates before thumbnail rendering.
- Retained results include multiple deterministic depth bands in the bounded fixture.
- Repeated identical searches preserve scores, cameras, suppression counts and thumbnail pixels.
- Existing cancellation, result-count, thumbnail completeness, memory ceilings and unsaved-candidate guarantees remain covered.

## Build matrix

- GCC Release build with warnings treated as errors.
- GCC core tests.
- Clang Release build with warnings treated as errors.
- Clang core tests.
- Clang AddressSanitizer and UndefinedBehaviorSanitizer core tests.
- Packaged-source verifier and ZIP integrity validation.

## Limits

Native MSVC compilation and interactive Win32 dialog testing require a Windows SDK and desktop runtime and are not available in this environment.
