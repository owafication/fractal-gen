# Integrated Creative Roadmap

**Status:** Current programme roadmap; PH-11 active and PH-14/PH-15 proposed  
**Purpose:** Own programme order, dependencies, release gates and active status  
**Owner:** Product/architecture  
**Reading trigger:** Roadmap, sequencing or feature-priority work  
**Update trigger:** Phase status, dependency or accepted scope change  
**Linked IDs:** REQ-011–REQ-041, PH-01–PH-15

## Direction

Retain the original architectural direction: one stable parameter contract, deterministic evaluation, visual regression before broad state changes, explicit project/user/runtime boundaries, transactional undo, immutable export jobs and safe external encoding. Fractal rendering and animation are authoring/output capabilities; desktop presentation consumes saved images or exported video only. Rebase the programme onto observed 1.13.1 rather than an uninspected 1.12.6 state.

## Baseline adjustments

1. Native Windows release stabilisation precedes new architecture.
2. Visual regression precedes state migration.
3. Snapshots and parameters begin as adapters over current models.
4. Stable object IDs replace indices as persisted identity.
5. Undo uses scalar, structural and subtree operations.
6. Thresholds are measured per renderer/environment.
7. The existing Fractal Scout is integrated, not recreated.
8. Frame sequences precede FFmpeg encoding.
9. Detailed contracts live in smaller canonical documents.
10. DEC-039 permanently separates renderer-backed preview/export from file-backed desktop presentation unless a later explicit product decision supersedes it.

## Programme

| Phase | Objective | Status | Primary specification |
|---|---|---|---|
| PH-00 | Repository/governance baseline | Historical working-repository baseline passed; this archive has no usable Git metadata or supplied bundle | `project_docs/IMPLEMENTATION_PLAN.md` |
| PH-01 | Native Windows release stabilisation | In progress; native build/tests, historical package checks and isolated install/launch/reinstall/uninstall pass; preceding-release upgrade and broader matrix remain | repository + validation docs |
| PH-02 | Production-renderer visual fixtures | Complete at the documented scope; approved CPU baselines and native CPU, D3D11 WARP/hardware and OpenGL production fixtures pass | `docs/testing/VISUAL-REGRESSION.md` |
| PH-03 | Camera/palette adapters and fingerprint | Complete at documented scope; native compile/core evidence and recorded navigation/dialog interaction pass | `docs/architecture/PROJECT-STATE-AND-PARAMETERS.md` |
| PH-04 | Camera/palette undo | Complete at documented scope; camera/palette Undo/Redo and redo-branch interaction confirmed | `docs/features/UNDO-REDO-PLAN.md` |
| PH-05 | Complete project undo | Complete at automated native scope; Palette/Equation, preset Load/Import, Settings, Journey and Scout Apply history replay pass | undo plan |
| PH-06 | Deterministic animation evaluator | Implementation/native tests and preview-clock interaction pass; rendered visual playback remains | `docs/features/ANIMATION-TRACKS-PLAN.md` |
| PH-07 | Basic editor and Journey adapter | Implementation/native tests, editor state/clock and Journey conversion interaction pass; visual playback and DPI/keyboard remain | animation plan |
| PH-08 | Deterministic frame sequences | Implemented; native WIC success/resume/refusal/cancellation pass; modal desktop pause/restoration passes; Open Folder, DPI/keyboard and active-output pause remain | `docs/features/OFFLINE-EXPORT-PLAN.md` |
| PH-09 | External FFmpeg encoding | Implemented; FFmpeg 8.1.1 and bounded native dialog success/cancellation evidence pass; other encoder builds remain | offline export plan |
| PH-10 | Existing Scout integration | Complete at documented bounded/native scope; deterministic identity/thumbnails, selection/Close isolation and Apply/Undo/Redo pass | `docs/features/FRACTAL-SCOUT-STATUS.md` |
| PH-11 | Integration hardening | Active; bounded desktop media/failure, display-message/host recovery, native tab/name, 60-second static resource and isolated installer checks pass; broader/manual matrix remains | validation plan |
| PH-12 | Baseline repair and exact camera foundation | Exact camera, schema migration, v2 identity, UI/export adapters and exact history are implemented; deep execution remains later work | state/persistence owners |
| PH-13 | Central precision planner and orbit service | Planner-owned exact CPU tiers, legacy GPU selection and bounded planner-matched reference service are implemented; measured encoding limits and validated perturbation rendering remain | rendering contracts |
| PH-14 | Perturbation validity, correction and bounded multi-reference | Proposed | rendering contracts + deep fixtures |
| PH-15 | Deep export and authoring integration hardening | Proposed; desktop validation is limited to playback of already-exported files | export/validation/support matrix |

## Immediate bounded slice

- **Completed:** correct and validate release version surfaces, including a deliberate mismatch test.
- **Historical:** local `main` baseline recorded in the original working repository; no usable Git metadata or bundle is present in this archive.
- **Completed:** add canonical MSVC presets, isolated application-data support, path-isolation tests, portable package inspection, automated startup/shutdown smoke and report generation.
- **Open PH-01 gate:** complete the broader manual Windows matrix and preceding-release upgrade; isolated install/reinstall/uninstall now pass in BR-20260923-03. Automated native Release build/CTest, bounded Palette/Equation interaction, isolated startup/shutdown, portable-package inspection and installer production already pass.
- **Implemented:** CPU production-renderer fixture command with four bounded fixtures, exact repeatability, tiled/full seam strips, diagnostic artifacts and deliberate-mutation detection.
- **Completed PH-02 verification:** strict reviewed CPU baselines and native CPU, D3D11 WARP/hardware and OpenGL production readback fixtures pass. Backend evidence remains environment-specific and does not imply general cross-backend pixel equivalence.
- **Implemented PH-03 foundation:** canonical `mw-render-state-v1` serialisation, SHA-256 fingerprint, bounded parameter descriptors, camera/palette adapters and round-trip tests.
- **Implemented PH-03 coordinator slice:** transactional model-normalised mutation batches and selective invalidation now own the main-window brightness, contrast, saturation and colour-offset controls.
- **Implemented PH-03 deliberate-camera slice:** coordinate-triplet edits, separate centre/scale controls, Jump, Reset and Scout Apply use the same compensated camera transaction path.
- **Implemented PH-03 palette/origin slice:** built-in palette selection now uses a discrete transaction and mutation results carry explicit origin/future-history eligibility without creating history.
- **Completed PH-03 gesture/replacement slice:** preview pan and wheel use one `PreviewNavigation` token until a 500 ms inactivity gap starts a new undo block, and preset load/import plus Palette, Equation, Settings and Journey dialogs use classified transactional whole-preset replacement.
- **Completed PH-04 implementation:** runtime-only typed camera/palette history, atomic undo/redo replay, drag/wheel/palette-control coalescing, rotation editing, labelled commands/shortcuts, redo-branch truncation and configurable entry/memory bounds.
- **Completed PH-05 implementation and automated native interaction:** atomic structural history for palette stops, equations, journeys, preset/import/dialog replacement and scalar-completeness fallback, with one-entry Scout Apply coverage. BR-20260909-03 proves native Palette/Equation structural labels and Equation Undo/Redo; BR-20260909-04 adds native preset Load/Undo/Redo with exact camera and identity restoration; BR-20260909-05 adds Settings accept/Undo/Redo and main-window camera-control synchronisation; BR-20260909-06 adds native Journey accept/Undo/Redo with exact structured text restoration; BR-20260910-01 adds production-serialised preset Import/Undo/Redo through the native Open dialog; BR-20260910-02 completes the matrix with production Scout search and exact camera-only Apply/Undo/Redo replay.
- **Completed PH-06 implementation:** bounded runtime-only timeline data, stable targets/IDs, immutable deterministic evaluation, compensated centre/log-scale/wrapped-angle interpolation, loop modes and VAL-023–VAL-025 portable coverage. Preview/export are the supported active clock domains; the historical wallpaper clock is inert compatibility state and is not connected to desktop presentation.
- **Completed PH-07 implementation:** modal runtime timeline editor, authoritative Add Current Value, keyframe/time/interpolation editing, preview-clock scrubbing/playback, strict Journey conversion, loss-aware reverse conversion and VAL-026 portable/structural coverage; user reports the PH-07 archive builds and runs on Windows.
- **Completed PH-08 implementation:** immutable deterministic PNG jobs, rational direct-index timing, CPU tiled/WIC row output, verified temporary promotion, atomic manifest/receipt recovery, exact resume refusal, cancellation preservation, progress UI, selected-frame evidence and file-backed desktop pause/resume policy; bounded native WIC success/resume/refusal/cancellation interaction passes; BR-20260924-01 adds bounded modal file-backed desktop pause/restoration; Open Folder, DPI/keyboard and desktop pause during active output jobs remain open.
- **Completed bounded PH-10 scope:** deterministic identity/thumbnails, native candidate selection/Close isolation and one-transaction exact Apply/Undo/Redo pass (BR-20260910-02/03).
- **Historical native release baseline:** Visual Studio 18 2026 x64 Release compiled and linked the application plus all test/fixture targets on 2026-09-09. Release CTest passed 8/8, including core, path, CPU visual/Scout thumbnails, D3D11 WARP, OpenGL, version consistency and the process-scoped native Palette/Equation interaction fixture. BR-20260909-01 separately ran the opt-in real-FFmpeg success, owned-process-cancellation, integrated-export-cancellation and encoder-failure target, with evidence in `artifacts/p9-v7/`. BR-20260909-02 records the cumulative clean package/installer workflow. This does not close the remaining UI, installer runtime, upgrade, lifecycle or deep-GPU numerical gates.
- **Historical PH-11 automated release evidence (2026-09-09):** the process-scoped Windows validator passed source policy, a clean native rebuild, CTest 8/8, executable metadata, portable-package inspection, isolated D3D11 startup/render/clean shutdown and Inno Setup 6.7.3 installer production/metadata/hash inspection on Windows 10 build 19045. The interaction gate additionally proved live Palette/Equation camera retention, Automatic preview precision switching and bounded structural history/Equation Undo/Redo. Authenticode remains `NotSigned`; cross-version, remaining-manual-interaction, desktop, display, lifecycle, installer install/upgrade/uninstall and soak gates remain open.
- Preserve current preview, preset, Journey, Scout, export and coordinate behaviour, plus only the accepted file-backed desktop modes. Do not preserve or reintroduce removed rendered Live/Journey desktop behaviour.
- **Deep-programme sequencing:** implemented PH-12/PH-13 slices do not waive their remaining validation gates. PH-13 completion still precedes PH-14 validity/correction/multi-reference, and PH-14 completion precedes PH-15 deep export claims. Scoped PH-12 work may remain ahead of PH-10/PH-11 completion without marking either predecessor complete.

The current BR-20260923-03 delivery adds bounded desktop-media, failure, 60-second static resource and isolated installer lifecycle evidence; earlier dated build/package records above retain their original scope. PH-11 remains active.

## Release gates

A phase completes only when:

- native MSVC passes for affected Windows code;
- portable checks pass where applicable;
- affected unit/integration checks pass;
- visual baselines show no unexplained change;
- tiled seam checks pass for rendering changes;
- migration fixtures load correctly;
- cancellation is verified for long-running work;
- failure does not damage user data or final output;
- schemas and canonical docs are updated;
- unverified claims remain explicitly unproven.

## Non-goals

No network/cloud platform, runtime AI, shader scripting, broad state rewrite, bundled encoder, microservice, formal distributed evaluation infrastructure or continuously rendered fractal desktop mode is justified by current requirements. Literal infinite/unlimited zoom claims are also excluded; support must be measured.
