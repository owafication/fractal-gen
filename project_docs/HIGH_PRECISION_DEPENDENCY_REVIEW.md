# High-Precision Dependency Review

**Status:** Accepted, source-integrated and native Release compile/test verified; production-route acceptance pending  
**Decision:** DEC-029  
**Delivery:** BR-20260730-03  
**Reviewed package:** Boost.Multiprecision standalone source subset 1.83.0  
**Licence:** Boost Software License 1.0 (`BSL-1.0`)

## Decision

Use a project-vendored, source-only subset of Boost.Multiprecision 1.83.0 with the Boost-provided `cpp_bin_float` backend behind the platform-neutral precision boundary.

The reviewed numerical type is fixed at 512 binary digits with expression templates disabled:

```cpp
boost::multiprecision::number<
    boost::multiprecision::backends::cpp_bin_float<
        512,
        boost::multiprecision::backends::digit_base_2>,
    boost::multiprecision::et_off>
```

This delivery adds an independent reference-orbit implementation. It does not replace the current production precision planner, persisted camera authority, renderer fallback order or existing `BuildReferenceOrbitArbitrary` path. The project wrapper rejects unsupported perturbation profiles and non-finite camera/equation input.

## Concrete package

The package is stored at:

```text
third_party/boost-multiprecision-1.83.0/
```

Package contents are deliberately limited to:

- `boost/multiprecision/**`;
- `boost/config/**` and `boost/config.hpp` required by standalone mode;
- `boost/version.hpp`;
- `LICENSE_1_0.txt`;
- package provenance and policy in `PACKAGE.json` and `README.md`;
- a complete reviewed file inventory in `FILES.SHA256`.

`PACKAGE.json` records the imported source provenance as Debian `libboost1.83-dev` package version `1.83.0-4.2`, the corresponding upstream release archive name and published archive digest, and the exact digest of this package's local content inventory. The local files were not independently proven byte-for-byte identical to the upstream archive; the local file hashes are the authority for this source package.

## Build and deployment policy

- Compile with `BOOST_MP_STANDALONE`.
- Treat the vendored include directory as a CMake `SYSTEM` include path.
- Do not use `find_package(Boost)`, vcpkg, Conan, NuGet or configure-time network acquisition for this dependency.
- Do not ship a Boost DLL, static library, service, installer component or separately loadable runtime artifact.
- Keep all Boost types private to `src/Core/Precision`; public project contracts expose project-owned types only.
- Include `THIRD_PARTY_NOTICES.md` in portable and installer packages.
- Verify `PACKAGE.json`, `FILES.SHA256` and every vendored file with `scripts/verify-third-party.py` before building or packaging.
- Any version or file-set change requires a new dependency review, regenerated hashes, licence review, portable warnings-as-errors builds, native MSVC/x64 evidence and numerical fixture comparison.

## Licence review

The accepted licence is the Boost Software License 1.0.

The project packaging policy is:

- retain the complete Boost licence text beside the vendored source;
- retain source copyright and licence notices;
- include an application-level third-party notice in source, portable and installer outputs;
- do not imply that Boost contributors endorse this application;
- treat any future addition of GMP, MPFR, MPIR, proprietary code or another Boost component as a separate licence and packaging decision.

The selected subset is header-only. It contributes compiled template code to the application executable but introduces no independent runtime binary. This review is a project packaging decision, not jurisdiction-specific legal advice.

## Technical review

### Accepted properties

- Fixed 512-bit binary precision aligns with the project's current bounded 128/256/512-bit precision intent.
- Fixed storage avoids unbounded precision growth for this reference backend.
- Header-only integration avoids native ABI, DLL search-path and installer-registration risks.
- Standalone mode limits the source dependency to Multiprecision plus Boost.Config.
- Expression templates are disabled to keep evaluation order easier to review in the independent numerical oracle.
- The implementation consumes compensated camera high/low components without first collapsing them into one `double`.
- Iteration count remains bounded to the existing 32–4096 range.
- Unsupported formula profiles and non-finite input fail closed at the project wrapper.

### Rejected alternatives for this slice

- **GMP/MPFR:** not selected because they add native libraries, ABI/runtime deployment, additional licensing analysis and a larger Windows packaging surface.
- **System/package-manager Boost:** not selected because it would make builds depend on machine state and would weaken source-package reproducibility.
- **Extending only the existing embedded fixed-point engine:** not accepted as the independent oracle because it would share too much bespoke implementation risk with the path being checked.
- **Automatically tracking the newest Boost release:** not accepted because dependency upgrades must be based on inspected source bytes and repeatable evidence.

## Implemented integration

- `src/Core/Precision/HighPrecisionBackend.h/.cpp` defines the project-owned boundary and independent 512-bit orbit builder.
- `CMakeLists.txt` pins the directory and Boost version macro, defines `BOOST_MP_STANDALONE`, and links the source-only interface privately into `MandelbrotCore`.
- `tests/CoreTests.cpp` checks package metadata, fixed-resource descriptors, orbit agreement against the existing arbitrary-reference path, Tricorn conjugation, unsupported-profile refusal and non-finite-input refusal. It also exercises the exact-camera entry point: it agrees with the equivalent legacy reconstruction when all canonical values fit the fixed 512-bit envelope, and rejects longer significands rather than silently rounding them.
- `scripts/verify-third-party.py` rejects missing, added, modified or incorrectly described package files.

## Review outcome

**Accepted for source integration.** The package, licence and source/installer policy stop condition in DEC-029 is resolved for this pinned dependency.

This does not by itself approve PH-12 exact-camera migration or production deep rendering. Those remain gated by:

- expanded numerical fixtures at deep coordinates and escape boundaries;
- measured CPU time and memory bounds;
- cancellation evidence when connected to a long-running production route;
- explicit migration and persistence implementation before exact camera state changes;
- release-package inspection confirming the notice and absence of unintended Boost runtime files.

## Verification record

See BR-20260730-03 in `DELIVERY_REPORT.md`. Against the integrated root:

- the 178-file package-integrity verifier passed;
- the complete source/governance verifier passed;
- Visual Studio 18 2026 x64 Release compiled and linked the backend, application, tests and fixture targets with warnings-as-errors;
- native Release CTest passed 6/6, including the independent numerical/refusal tests;
- a CMake install-stage inspection included `THIRD_PARTY_NOTICES.md` and contained no Boost DLL or static/import library.

This is compile/link/test and package-surface evidence, not production-route runtime, performance, cancellation or deep-release acceptance.

The independent backend now accepts `ExactCamera` directly. Because a fixed 512-bit binary significand preserves at most 154 decimal significant digits, this entry point fails closed beyond that limit. The larger DEC-028 persistence grammar remains valid project data; a future planner may select a different execution route, but it must not relabel this fixed backend as supporting it.

## Rollback

Remove:

- `third_party/boost-multiprecision-1.83.0/`;
- `src/Core/Precision/HighPrecisionBackend.h/.cpp`;
- the CMake interface target and source entry;
- the independent backend tests;
- the third-party verifier and notice packaging entries.

No settings schema, preset schema, user data or released binary format was changed in this dependency-review slice.
