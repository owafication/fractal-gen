# Contributing

**Status:** Proposed contributor procedure for the supplied 1.13.1 codebase.

## Before changing anything

1. Read `AGENTS.md`.
2. Read `project_docs/PROJECT_INDEX.md` and the canonical owners for the task.
3. Identify affected REQ, AC, VAL, RISK and ROUTE IDs.
4. Inspect the directly relevant source, tests, build files and historical reports.
5. Confirm the current phase and stop conditions.

## Build and checks

Windows x64 Release is the release authority for Win32-facing changes:

```powershell
python .\scripts\verify-source.py
cmake --preset windows-msvc-release
cmake --build --preset windows-msvc-release --parallel
ctest --preset windows-msvc-release
```

The release script packages the app and optionally builds the installer:

```powershell
.\scripts\build-release.ps1
```

The PH-01 evidence workflow runs the same release path, inspects the package, isolates app data, performs startup/shutdown smoke and writes Markdown/JSON/log evidence:

```powershell
.\scripts\validate-windows-release.ps1
```

Record exact tool versions, commands and results. Source checks prove structure only. Portable core checks do not prove Win32, GPU, WIC, WorkerW or installer behaviour. Complete the manual matrix in `docs/testing/WINDOWS-RELEASE-VALIDATION.md` before a release-ready claim.

Run the PH-02 CPU production-renderer fixtures for rendering, equation, palette, precision, post-processing or tiling changes:

```powershell
.\scripts\run-visual-fixtures.ps1
```

Candidate images and metrics are written below `test_artifacts/visual/`. Do not copy them into `tests/baselines/visual/` without reviewing current, baseline and diff artifacts and recording the environment and accepted thresholds.

## Change rules

- Keep `src/Core` platform-neutral.
- Preserve existing data formats unless the change includes an additive, recoverable migration.
- Preserve compensated camera precision and explicit equation compatibility.
- Use production renderers for visual tests.
- Keep long-running work bounded, cancellable and output-safe.
- Do not add network access, telemetry, executable imported content or shell command templates.
- Avoid unrelated refactors.
- Update the canonical owner and traceability status; do not rewrite historical verification reports.

## Pull/commit report

Include:

- objective and affected IDs;
- inspected and changed files;
- commands run;
- passed, failed, skipped and unproven checks;
- visual/migration artifacts where applicable;
- rollback point and known limitations.

## Release-version gate

CMake configuration runs `cmake/VerifyVersionConsistency.cmake` and fails when the CMake, manifest, Win32 resource, release-script or installer version differs. `scripts/build-release.ps1` also checks the version embedded in the built executable. Do not bypass either check.
