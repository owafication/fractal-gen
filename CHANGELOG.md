# Changelog

## Unreleased — High-precision package review

- Pinned a source-only Boost.Multiprecision 1.83.0 standalone subset under BSL-1.0 with a complete file-hash inventory and offline integrity verifier.
- Added a project-owned independent fixed-512-bit `cpp_bin_float` reference-orbit backend and portable numerical regression coverage without replacing production precision policy or changing schemas.
- Added third-party notices to portable/install packaging and recorded native MSVC/deep-coordinate release evidence separately.

**Status:** Historical index. Detailed feature and verification records remain in `docs/` and are not rewritten as current evidence.

## Unreleased

- Proposed governance and roadmap foundation generated on 2026-07-27.
- Fixed the embedded Win32 file/product version at 1.13.1.
- Added a CMake configuration/CTest version-consistency gate across CMake, manifest, Win32 resource, release script and installer.
- Added built-executable file/product version verification to the Windows release script.
- Portable GNU C++ build and core tests passed on 2026-07-27; native Windows/MSVC verification remains unproven.
- Added the PH-02 portable production-renderer visual fixture foundation; Windows GPU/baseline verification remains pending.
- Began PH-03 with stable parameter descriptors, compensated camera and palette/post adapters, and the versioned SHA-256 canonical render fingerprint.
- Added the PH-03 transactional mutation coordinator and routed the main-window brightness, contrast, saturation and colour-offset controls through model-owned normalisation and selective invalidation.
- Routed deliberate main-window camera edits through the same transaction coordinator while preserving compensated Scout/Reset camera components and leaving preview pan/zoom runtime copy-backs outside the bounded claim.
- Added discrete transactional palette selection plus explicit mutation-origin and future-history-eligibility metadata.
- Completed PH-03 implementation with one-token preview drag coalescing, a 250 ms wheel-zoom coalescing policy, classified transactional preset/dialog replacements and candidate-copy dialog editing.
- Completed PH-04 implementation with bounded runtime camera/palette undo/redo, pan/wheel/palette-thumb coalescing, rotation editing, labelled Ctrl+Z/Ctrl+Y commands, branch truncation and entry/memory containment.
- Completed PH-05 implementation with atomic structural history for palette stops, equations, journeys, preset/import/dialog replacement, scalar-completeness fallback, full replay refresh and one-entry Scout Apply coverage; native Windows verification remains pending.
- Completed PH-06 core implementation with a bounded runtime-only timeline model, deterministic immutable evaluation, compensated/log/wrapped interpolation, duplicate-target rejection and independent preview/wallpaper/export clocks; native MSVC verification remains pending.
- Completed PH-07 implementation with a modal runtime timeline editor, track/keyframe authoring, duration/loop controls, preview-clock scrub/playback, strict Journey-to-tracks conversion and explicit loss-aware reverse conversion; the user reports this archive builds and runs on Windows.
- Completed PH-08 implementation with immutable deterministic PNG jobs, rational direct-index timing, production CPU/WIC row output, verified temporary promotion, atomic fingerprint-bound manifest/receipt resume, progress/cancellation, selected-frame tests and wallpaper pause/GPU-release policy; native Windows export verification remains pending.
- Fixed the PH-08 Visual Studio 18 2026 build failure in `HighResRenderDialog.cpp` by explicitly including `<objbase.h>` for `CoInitializeEx`, `COINIT_MULTITHREADED` and `CoUninitialize`; the user reports the corrected archive now builds and runs.
- Completed PH-09 implementation with external FFmpeg selection/`PATH` discovery, `libx264`/MP4 capability gates, complete PH-08 source verification, fixed no-shell H.264 arguments, bounded stdout/stderr logs, owned cancellation, decode-before-promotion and manifest-scoped optional frame cleanup; native PH-09 and real FFmpeg execution remain pending.

## Release records

- [1.13.1](docs/FEATURES-1.13.1.md) — feature record; see [verification](docs/VERIFICATION-1.13.1.md).
- [1.13.0](docs/FEATURES-1.13.0.md) — feature record; see [verification](docs/VERIFICATION-1.13.0.md).
- [1.12.6](docs/FEATURES-1.12.6.md) — feature record; see [verification](docs/VERIFICATION-1.12.6.md).
- [1.12.5](docs/FEATURES-1.12.5.md) — feature record; see [verification](docs/VERIFICATION-1.12.5.md).
- [1.12.4](docs/FEATURES-1.12.4.md) — feature record; see [verification](docs/VERIFICATION-1.12.4.md).
- [1.12.3](docs/FEATURES-1.12.3.md) — feature record; see [verification](docs/VERIFICATION-1.12.3.md).
- [1.12.2](docs/FEATURES-1.12.2.md) — feature record; see [verification](docs/VERIFICATION-1.12.2.md).
- [1.12.1](docs/FEATURES-1.12.1.md) — feature record; see [verification](docs/VERIFICATION-1.12.1.md).
- [1.12.0](docs/FEATURES-1.12.0.md) — feature record; see [verification](docs/VERIFICATION-1.12.0.md).
- [1.11.7](docs/FEATURES-1.11.7.md) — feature record; see [verification](docs/VERIFICATION-1.11.7.md).
- [1.11.6](docs/FEATURES-1.11.6.md) — feature record; see [verification](docs/VERIFICATION-1.11.6.md).
- [1.11.5](docs/FEATURES-1.11.5.md) — feature record; see [verification](docs/VERIFICATION-1.11.5.md).
- [1.11.4](docs/FEATURES-1.11.4.md) — feature record; see [verification](docs/VERIFICATION-1.11.4.md).
- [1.11.3](docs/FEATURES-1.11.3.md) — feature record; see [verification](docs/VERIFICATION-1.11.3.md).
- [1.11.2](docs/FEATURES-1.11.2.md) — feature record; see [verification](docs/VERIFICATION-1.11.2.md).
- [1.11.1](docs/FEATURES-1.11.1.md) — feature record; see [verification](docs/VERIFICATION-1.11.1.md).
- [1.11.0](docs/FEATURES-1.11.0.md) — feature record; see [verification](docs/VERIFICATION-1.11.0.md).
- [1.10.3](docs/FEATURES-1.10.3.md) — feature record; see [verification](docs/VERIFICATION-1.10.3.md).
- [1.10.1](docs/FEATURES-1.10.1.md) — feature record; see [verification](docs/VERIFICATION-1.10.1.md).
- [1.10.0](docs/FEATURES-1.10.0.md) — feature record; see [verification](docs/VERIFICATION-1.10.0.md).
- [1.9.4](docs/FEATURES-1.9.4.md) — feature record.
- [1.9.0](docs/FEATURES-1.9.0.md) — feature record; see [verification](docs/VERIFICATION-1.9.0.md).
- [1.8.0](docs/FEATURES-1.8.0.md) — feature record; see [verification](docs/VERIFICATION-1.8.0.md).
- [1.7.1](docs/FEATURES-1.7.1.md) — feature record; see [verification](docs/VERIFICATION-1.7.1.md).
- [1.7.0](docs/FEATURES-1.7.0.md) — feature record; see [verification](docs/VERIFICATION-1.7.0.md).
- [1.6.0](docs/FEATURES-1.6.0.md) — feature record; see [verification](docs/VERIFICATION-1.6.0.md).
- [1.5.0](docs/FEATURES-1.5.0.md) — feature record; see [verification](docs/VERIFICATION-1.5.0.md).
- [1.4.0](docs/FEATURES-1.4.0.md) — feature record; see [verification](docs/VERIFICATION-1.4.0.md).
- [1.3.0](docs/FEATURES-1.3.0.md) — feature record.
- [1.2.0](docs/FEATURES-1.2.0.md) — feature record.
- [1.1.0](docs/FEATURES-1.1.0.md) — feature record.

## Build-fix history

See `docs/BUILD-FIXES-1.0.1.md` through `docs/BUILD-FIXES-1.0.5.md`.
