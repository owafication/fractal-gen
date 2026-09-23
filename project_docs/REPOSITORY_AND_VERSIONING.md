# Repository and Versioning

**Status:** Current version and release policy; source archive has no usable Git metadata  
**Purpose:** Own repository baseline, version authority, release procedure, changelog and rollback rules  
**Owner:** Release engineering  
**Reading trigger:** Git, build, package, version or release work  
**Update trigger:** Repository topology, version scheme, release command or package change  
**Linked IDs:** PH-00, PH-01, REQ-011, AC-002–AC-004, VAL-001–VAL-008, RISK-001

## Repository state

- Project root is the directory containing `CMakeLists.txt`.
- Historical repository-execution evidence records branch `main` with no added remote.
- Historical baseline commit `8f82119` includes the imported source, canonical governance and release records; this archive cannot verify that commit locally.
- Generated `build*`, `dist`, `artifacts`, IDE state and local logs are ignored.
- This source archive has no usable `.git` or accompanying bundle. The baseline commit/branch above are historical evidence, not current checkout state.
- Primary source directories: `src/Core`, `src/Rendering`, `src/WindowsIntegration`, `src/App`, `src/Infrastructure`.
- Tests: `MandelbrotCoreTests`, `MandelbrotPathTests` and `VersionConsistency` through CTest.
- Build/release inputs: `CMakeLists.txt`, `CMakePresets.json`, `scripts/build-release.ps1`, `scripts/validate-windows-release.ps1`, CMD wrappers, `scripts/run-core-tests.sh`, `scripts/verify-source.py`, and `installer/MandelbrotWallpaper.iss`.

## Version surfaces

| Surface | Observed value | Status |
|---|---:|---|
| CMake project | 1.13.1 | Observed |
| Application manifest identity | 1.13.1.0 | Observed |
| Release PowerShell script | 1.13.1 | Observed |
| Inno Setup definition | 1.13.1 | Observed |
| Win32 version resource | 1.13.1 | Passed source consistency |
| README current release text | 1.13.1 | Observed |

**Implemented:** `src/App/resources.rc` is aligned to 1.13.1. `cmake/VerifyVersionConsistency.cmake` checks every listed surface during configuration and through CTest. `scripts/build-release.ps1` rejects a built executable whose embedded file or product version differs and discovers machine-wide, per-user or `PATH`-provided Inno Setup. BR-20260909-02 records native executable/package/installer artifact verification; install, upgrade, uninstall and signing remain separate release gates.

## Current version authority

- `CMakeLists.txt` is the human-edited canonical application version.
- Manifest, resource, release-script and installer values remain explicit release surfaces and must match it. Configuration fails on drift.
- The Windows release build must verify the version embedded in the produced executable before packaging.
- Settings schema and preset schema are independent compatibility versions; do not equate them with application SemVer.
- Any release changes one accepted version in all surfaces within the same change set.

## Local repository baseline

**Implemented 2026-07-27:** the confirmed root was initialised on `main`; imported historical documents remain tracked because they are evidence records; no remote was added; baseline commit is `8f82119`. Generated build/package/report state is ignored.

If the historical delivery bundle is separately obtained (it is absent here), its recorded restoration command is:

```text
git clone Mandelbrot-Live-Wallpaper-1.13.1-roadmap-progress.bundle Mandelbrot-Live-Wallpaper
```

A plain source ZIP remains usable but does not itself prove branch, commits, remotes or working-tree state.

## Change procedure

1. Read `AGENTS.md` and the mapped owners.
2. Record affected REQ/AC/VAL/RISK IDs.
3. Inspect relevant source, tests, build files and historical evidence.
4. Make the smallest reversible change.
5. Update canonical owners; append a release record only when a release is actually prepared.
6. Run proportionate checks and record exact commands/results.
7. Review diff and staged files.
8. Commit only when authorised.

## Release procedure

1. Confirm clean working tree and intended branch.
2. Confirm all version surfaces with VAL-002.
3. Run source-policy checks; treat them as structural only.
4. Run `scripts/validate-windows-release.ps1`, which uses the canonical MSVC preset, build, CTest, version checks, package inspection and isolated startup/shutdown smoke.
5. Review its Markdown/JSON/log report and preserve the report with the delivery.
6. Complete the manual dialog, desktop-mode, GPU, display and lifecycle matrix in `docs/testing/WINDOWS-RELEASE-VALIDATION.md`.
7. Build portable package and installer where supported.
8. Inspect package contents; reject build outputs or user data not intended for distribution.
9. Validate ZIP/installer integrity and upgrade behaviour.
10. Write a release report listing passed, failed, skipped and unproven items.

## Isolated installer validation

Run `scripts/validate-isolated-installer.ps1 -ReportDirectory artifacts/<fresh-directory>` after a native build, with no application running. It builds a distinct test AppId/name/directory, isolates application data, checks install and launch, verifies running-app uninstall refusal, and checks same-version reinstall/uninstall while preserving settings and an untracked output. Never substitute the normal AppId or interpret reinstall as preceding-release upgrade. Production setup/uninstall use the application mutex, do not forcibly close applications, and remove only tracked installed files.

## Changelog policy

- Root `CHANGELOG.md` is a compact release index.
- Detailed historical release content stays in `docs/FEATURES-<version>.md` and `docs/VERIFICATION-<version>.md`.
- Never rewrite a historical verification result to imply a check was rerun.

## Rollback

- Code rollback target: last accepted commit before the phase.
- Settings migration rollback: retain original file until in-memory migration validates and atomic save succeeds.
- Output rollback: write temporary files and promote only verified output.
- Release rollback: retain prior installer/portable package and document incompatibilities before distributing a replacement.
