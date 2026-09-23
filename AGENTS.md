# AGENTS.md

**Status:** Proposed governance for the supplied 1.13.1 source archive.

## Mission
Maintain Mandelbrot Live Wallpaper as a native, offline Windows fractal-authoring and file-backed desktop-presentation application. Fractal rendering and animation belong to preview, still/frame export and video export; desktop presentation is limited to saved images, image slideshows and already-exported local video. Preserve rendering correctness, deep-coordinate precision, user data, reversible desktop integration, deterministic behaviour where required, and bounded resource use.

## Authority
1. Platform, safety, privacy and data-integrity requirements.
2. Directly inspected source, tests, build files and logs.
3. Accepted requirements and decisions in `project_docs/`.
4. Historical release documents, only within their recorded scope.
5. Assumptions explicitly marked as unproven.

Do not claim a build, test, runtime flow, package, migration or visual result passed unless the corresponding command or check was run and its output recorded.

## Required reading sequence
1. Read this file.
2. Identify the active phase and task category.
3. Read `project_docs/PROJECT_INDEX.md`.
4. Load only the canonical owners mapped there.
5. Inspect directly affected source, tests, configuration and historical evidence.
6. Expand context only for a dependency, conflict or evidence gap.

## Core rules
- Inspect before editing; make narrow, reversible changes.
- Preserve current contracts unless an approved managed change requires migration.
- Keep `src/Core` platform-neutral.
- Never convert imported data into shader source, script, command text or executable behaviour.
- Preserve compensated camera coordinates and supported precision fallbacks.
- For proposed PH-12+ deep work, preserve exact camera text before any binary floating-point conversion; once accepted, exact camera state is authoritative and legacy `CameraState` is a one-way compatibility/render adapter.
- Precision algorithms, thresholds and fallback order must come from one platform-neutral planner; render backends report capabilities but do not invent policy.
- Unsupported formulas, stale generations, unresolved perturbation pixels and incomplete deep output fail closed. Orbit, correction and reference work must remain generation-safe, cancellable and bounded.
- Treat preview state, persisted settings and desktop runtime as different concerns.
- Do not reintroduce continuous or on-the-fly fractal rendering as a desktop mode. Static/slideshow desktop modes decode image files; video desktop mode plays an existing exported local file.
- Long-running work must be bounded, cancellable and failure-safe.
- Settings and output writes must avoid partial final files.
- Do not add network access, accounts, telemetry, services, package dependencies or FFmpeg bundling without an accepted requirement and decision.
- The accepted high-precision dependency is the vendored Boost.Multiprecision 1.83.0 standalone subset. Do not use a machine-global/package-manager Boost, add Boost binaries, broaden the vendored module set or change its version without updating the dependency review, manifest, hashes, notices and verification evidence.
- Do not enter playback, export, discovery progress or adaptive-performance events into undo history.
- Update only the canonical document that owns the changed contract; link historical records rather than rewriting them.

## Task categories
- **Docs/governance:** update owners, links, IDs and delivery record.
- **Core/model:** inspect persistence, validation, precision and tests.
- **Rendering/export:** inspect CPU, D3D11, OpenGL, tiling, post-processing and backend-specific checks.
- **Windows/UI:** require native MSVC evidence for completion claims.
- **Release:** reconcile every version surface, run release build/tests, inspect staged/package contents and record checks.

## Reporting
Report: inspected, changed, run, passed, failed, skipped and unproven. A source-token check proves structure only; it does not prove compilation or runtime behaviour.

## Stop conditions
Stop before mutation when required authority, destructive-action approval, migration policy, credentials, licensing decision, release version decision, or safety evidence is missing. Stop a release when native Windows validation, version consistency, data-integrity checks or required visual evidence is absent.
