# Verification — 1.11.0

This document records checks executed in the packaging environment. It does not claim a native Windows GUI or OpenGL runtime test unless explicitly stated.

## Automated core coverage

The core test suite checks:

- all 45 equation presets validate and produce finite sampled results;
- the seven new reference variants retain their intended coefficients and flags;
- `z + c²` uses a powered parameter term and is not evaluated as linear `c`;
- powered parameter terms survive settings JSON round trips;
- all 30 built-in palette IDs are unique and every palette has valid colour stops;
- all 36 complete scene IDs are unique and validate;
- the supplied `z + c²` plus Crimson Web scene is present;
- tiled still-render tests and all existing core regression tests remain active.

## Source regression coverage

`scripts/verify-source.py` checks:

- `parameterPower` model, persistence, CPU math, OpenGL uniforms and editor controls;
- perturbation compatibility guard for powered parameter terms;
- supplied equation and palette markers;
- built-in palette read-only/custom-tail handling;
- preset-save rollback and success reporting;
- complete hover-menu high-resolution command wiring;
- 1.11.0 version metadata across CMake, manifest, installer and release script.

## Platform limit

The packaging environment can compile and execute the platform-neutral core but cannot compile or visually exercise the Win32 application. Final Windows verification must still cover:

- equation and palette combo layout at supported DPI scales;
- OpenGL compilation of the new `uParameterPower` shader uniforms;
- loading every equation and palette in the dialogs;
- saving and reloading a custom preset from `%LOCALAPPDATA%`;
- all hover-menu buttons;
- installer and portable release packaging under MSVC.

## Packaging-environment results

Executed successfully:

```text
python3 scripts/verify-source.py
GCC 14 Release build with warnings treated as errors
Clang 17 Release build with warnings treated as errors
CTest: MandelbrotCoreTests passed under GCC and Clang
Clang AddressSanitizer + UndefinedBehaviorSanitizer build and test
```
