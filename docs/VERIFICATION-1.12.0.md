# Verification — 1.12.0

## Scope

Phase 1 of the fractal-style rendering build plan:

- Tricorn and Multicorn templates
- Palette frequency, gamma, and mapping mode
- Stripe-average colouring
- CPU/OpenGL/Direct3D 11 parameter plumbing
- Tuned cyan-fire palette and scene

## Automated coverage

Core tests cover:

- Settings round-trip for palette and stripe controls
- Finite, normalised stripe-average orbit output
- Tricorn and Multicorn template identity
- Presence and settings of the tuned Tricorn scene
- Existing equation, preset, animation, deep-zoom, and high-resolution behaviour

## Commands

```text
python3 scripts/verify-source.py
cmake -S . -B build-core -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build-core -- -j2
ctest --test-dir build-core --output-on-failure
cmake -S . -B build-core-clang -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build-core-clang -- -j2
ctest --test-dir build-core-clang --output-on-failure
```

## Environment limits

The current verification environment does not include the Windows SDK, an interactive Win32 desktop, Direct3D 11 runtime validation, or an offline GLSL/HLSL compiler. The Windows UI and shader changes are therefore source-checked here but require native MSVC compilation and visual backend comparison on Windows before claiming full runtime verification.

## Results in this environment

Passed:

- Source structure and 1.12.0 feature verifier
- GCC 14 Release build with `-Werror`
- GCC 14 core test suite: 1/1
- Clang 17 Release build with `-Werror`
- Clang 17 core test suite: 1/1
- Clang 17 AddressSanitizer and UndefinedBehaviorSanitizer core test run: 1/1
- Balanced-delimiter inspection of the embedded GLSL/HLSL shader sources and modified Windows-only Palette Editor source
