# Offline Export Plan

**Status:** PH-08 deterministic PNG frame-sequence implementation complete with bounded native success/resume/refusal/cancellation evidence; PH-09 has real-encoder fixture evidence plus bounded native dialog summary/validation/failure and verified MP4 success evidence  
**Purpose:** Own deterministic frame jobs, resume, cancellation and external encoding  
**Owner:** Rendering/export  
**Reading trigger:** Animation frame output, job persistence, progress, cancellation or FFmpeg work  
**Update trigger:** Frame timing, manifest, output format, encoder or cleanup policy change  
**Linked IDs:** REQ-017, REQ-018, PH-08, PH-09, AC-015–AC-017, VAL-027–VAL-033


## PH-08 implementation boundary

PH-08 now implements the first supported offline output as a deterministic PNG sequence:

- `FrameSequenceExportJob` owns an immutable normalised `Preset`, runtime timeline copy, rational frame rate, dimensions, DPI, tile size, fixed seed, renderer/evaluator/application identifiers, naming policy and output directory.
- Frame times are derived directly from `start + index * denominator / numerator`; an end frame is added only when the duration lies on the rational frame grid. No cumulative time stepping is used.
- The production CPU `RenderStillImageTiled` path is used with preset iterations/AA pinned and adaptive resolution quality disabled.
- Frames stream through the reusable WIC row encoder into metadata-directory `.part` files, are reopened for dimension validation, digested, receipted and promoted only after verification.
- `.mw-frame-sequence/manifest.json` is updated through temporary/backup promotion. Resume requires the exact job fingerprint and matching typed manifest fields, then revalidates every completed frame by dimensions, byte length and digest.
- Cancellation stops new work, removes only the current incomplete `.part` file and preserves verified frames, receipts and manifest. The worker is joined before dialog state is destroyed.
- The Win32 route exposes output folder, dimensions, DPI, rational frame-rate choices, frame-grid endpoint policy, filename prefix, matching-manifest resume, progress, cancellation and output-folder access.
- Active file-backed desktop presentation is held paused while either modal export route is open. Automatic lifecycle recovery and Tray Resume cannot release that hold. Closing re-evaluates current automatic-pause conditions and preserves a pre-existing user pause; a stopped background is not restarted. Desktop presentation owns no fractal GPU renderer; renderer resources belong to preview/export.
- General timelines remain runtime-only under DEC-014; PH-08 does not add a persisted timeline field to settings schema 11 or enter project undo history.

Portable tests cover VAL-027–VAL-030 in the recorded scope: frame-count/time mapping, deterministic job identity, cancellation preservation, matching resume, mismatch refusal, malformed metadata rejection, and deterministic start/middle/end production CPU frames. BR-20260910-06 adds bounded native interaction through the production dialog: a 32x24 WIC PNG is reopened and validated before promotion, its typed manifest is retained, a second dialog run reuses the verified frame as `0 rendered, 1 resumed`, and separate 1024x1024 runs are cancelled through the visible Cancel action and the window close action with their workers joined and no `.part` file left. The close-while-active route retains the dialog after cancellation so the result remains visible until a final close. The same route exposes and dismisses production errors for a mismatched manifest and an untracked final PNG, preserving both existing files. The fixture uses isolated application data/output and applies no desktop mode. Open Folder launch, file-backed desktop pause/resume, DPI, keyboard and accessibility remain pending.

## Stage 1 — Frame sequence

PNG sequence is the first supported output because it is lossless and independently verifiable.

## Immutable job snapshot

Capture before work:

- project snapshot and render fingerprint;
- timeline and evaluator version;
- start/end or duration;
- frame rate as rational/integer policy;
- dimensions, DPI and format;
- backend/precision/AA/post-processing;
- fixed seed;
- output directory/name pattern;
- application/source version and manifest schema.

Editing the live project after launch does not alter the job.

## Frame timing

Use exact frame-index mapping. Avoid cumulative floating-point time increments.

```text
time(frameIndex) = start + frameIndex * frameDuration
```

Represent frame rate/time so common rates remain stable and final frame inclusion is explicitly defined.

## Output workflow

1. Validate bounds/path and create a job manifest.
2. For each frame, evaluate a frame-local snapshot.
3. Render through production still/tiled paths with deterministic options.
4. Write a temporary frame.
5. Validate expected dimensions/format and close it.
6. Promote to the final frame name.
7. Update the manifest atomically.
8. Report progress through a thread-safe channel.

## Cancellation/failure

- Stop scheduling new work.
- Join/close workers safely.
- Remove only incomplete temporary files.
- Preserve verified completed frames and manifest.
- Never leave a partial frame under a final name.
- App close uses an explicit cancel/wait policy; no detached worker accessing destroyed UI/state.

## Resume

Resume only when job manifest, evaluator version, project/timeline fingerprint, output settings and frame naming match. Mismatch refuses resume and offers a new job; it never mixes frames from different state.

## Resource policy

- Reuse existing tiling/banding.
- Pin deterministic quality; disable adaptive changes.
- Define file-backed desktop playback pause/resume policy before work; do not create a renderer-sharing policy because desktop presentation has no fractal renderer.
- Bound worker count and retained frame memory.
- Keep progress/reporting independent from render determinism.

## Stage 2 — External FFmpeg

**Status:** PH-09 implementation complete; portable/core/structural and one real FFmpeg 8.1.1 production-path success/owned-process-cancellation/integrated-cancellation fixture passed. BR-20260918-01 adds bounded native dialog summary, validation, missing-executable failure containment, verified MP4 success and owned-process cancellation with the same external build; broader encoder compatibility remains pending.

### Accepted boundary

- The user supplies FFmpeg by selecting an executable or using `PATH` discovery. The application does not bundle, download or remember the executable path and does not add video-export fields to settings schema 11.
- Compatibility is capability-based: the executable must complete a version probe and expose the `libx264` encoder plus MP4 muxer. The first line of the reported version is included in the immutable job identity and local log.
- The source must be a complete PH-08 sequence whose manifest, contiguous receipts, dimensions, sizes and digests all verify before process launch.
- The application starts the exact selected executable through `CreateProcessW` with a fixed argument vector. It does not invoke a shell or accept command templates.
- Initial output is H.264/AVC in MP4 using `libx264`, `yuv420p`, a bounded CRF and a bounded preset selected from the application-owned list.
- Because fixed `yuv420p` requires even chroma dimensions, video job construction rejects an odd source width or height before starting FFmpeg. PH-08 PNG sequences remain free to use valid odd dimensions; only MP4 conversion is refused.
- Encoding writes to an application-owned temporary MP4 adjacent to the selected final path. A second fixed FFmpeg invocation decodes one video frame before the temporary file may be atomically promoted.
- Stdout and stderr are captured separately into bounded local logs. Progress is parsed from FFmpeg's machine-readable progress stream and does not affect render/encode determinism.
- Cancellation terminates only the owned process, removes only the owned temporary MP4 and preserves the complete source frame sequence plus logs.
- Optional post-success cleanup deletes only files named by the verified PH-08 manifest/receipts. Untracked files are never removed, and cleanup cannot run before verified final promotion.

### Distribution and compatibility wording

FFmpeg remains an optional external dependency for MP4 export. Users are responsible for obtaining a build suitable for their environment and for any licence obligations attached to that build. The application package contains no FFmpeg binary or download mechanism and does not claim compatibility by version number alone.

## Validation

- exact frame count/timing;
- repeated frame fingerprint equality;
- cancellation at early/mid/final stages;
- crash/interruption leaves resumable verified frames;
- fingerprint mismatch refuses resume;
- selected-frame visual baselines;
- encoder command-injection tests;
- encoder failure/cancel preserves frames;
- cleanup occurs only after verified successful output.

### Real encoder fixture — BR-20260909-01

- `MandelbrotExternalVideoFixture` is an opt-in Windows test target. It requires an explicitly supplied external executable and a new artifact directory; it is not a default CTest and does not bundle, discover, download or persist FFmpeg.
- The fixture uses production `RunFrameSequenceExport`, WIC PNG validation, `BuildExternalVideoExportJob`, `RunOwnedProcess` and `RunExternalVideoExport` paths rather than a duplicate encoder workflow.
- A native Release run with FFmpeg 8.1.1 generated and revalidated three 96x64 PNG frames, passed version/libx264/MP4 capability probes, encoded H.264/yuv420p MP4, decoded one video frame, atomically promoted a 3,426-byte final and preserved the verified source frames and receipts. It then replayed that verified MP4 in a fixture-owned indefinite real-time loop, requested cancellation after 200 ms and observed owned termination in 251 ms with exit code 1223, no surviving FFmpeg process and no additional output file.
- The same run built a second valid 96x64 production sequence, completed production preflight, temporarily moved only that fixture-owned sequence directory immediately before the exact FFmpeg launch, and required the real encoder to fail. The production boundary retained bounded stdout/stderr logs, removed the temporary/final MP4, and the restored three PNGs, receipts and manifest revalidated exactly. This exercises a source race/failure after preflight without changing the production argument vector.
- The privacy-safe report records only `ffmpeg.exe`, the version line, capabilities, dimensions, full immutable job fingerprint, filenames and success-boundary results. It does not record the selected executable directory.
- Integrated cancellation uses a production-rendered 640x360 seed and 120 verified same-volume linked frames, starts the production `veryslow` encode, and arms cancellation only after the owned process runner begins. The child reported cancellation with exit code 1223 in 116 ms; no final or temporary MP4 remained, both bounded logs existed, no FFmpeg process remained, and all source frames, receipts and the manifest revalidated.
- Evidence: `artifacts/p9-v7/report.json` and `media-inspection.json` plus the success, cancellation and failure manifests/receipt sets, bounded stdout/stderr logs, source PNGs and verified MP4. Independent inspection reports H.264, yuv420p, 96x64, 3/1 fps, three frames and one-second duration.
- This closes the real success-path, owned-process termination, integrated `RunExternalVideoExport` cancellation cleanup and post-preflight encoder-failure containment portions of VAL-031–VAL-033 for that exact build. It does not prove Video Export dialog interaction, other FFmpeg builds or the user-selected build's licence suitability.

### Native dialog validation — BR-20260918-01

- Typing or pasting a verified PH-08 sequence folder now refreshes the same manifest summary as Browse selection; the prior edit path left the summary stale.
- The isolated production dialog displays and dismisses its missing-input and out-of-range CRF errors, then refuses a nonexistent FFmpeg path at the executable preflight boundary.
- No final or `.part.mp4` output is created, closing restores the main owner, and exact camera text, selected preset identity and Undo state remain unchanged.
- With `MW_TEST_FFMPEG_PATH` explicitly set, the same production dialog completes capability probing, one-frame H.264/MP4 encoding, decode verification and final promotion twice through FFmpeg 8.1.1. Each run preserves the verified PNG, leaves no `.part.mp4`, enables Open Output and closes with project/history isolation. With `MW_TEST_VIDEO_CANCEL_SEQUENCE` also set to the existing verified 120-frame fixture sequence, two further runs reach owned encoding and cancel through the visible dialog action; the sequence revalidates and no final or temporary MP4 remains. The default CTest route keeps both external inputs optional.
- This does not prove Open Output launch through the dialog, file-backed desktop pause/resume, DPI, keyboard, accessibility, other FFmpeg builds or licensing suitability.

## Proposed PH-15 deep export extension

PH-15 deep rendering remains limited to preview and explicit output jobs. The desktop may play a successfully exported file, but it cannot consume a live deep-render state or invoke the precision planner.

- Every still/frame job captures immutable exact camera, formula capability, precision plan/version, global sample mapping, deterministic time/seed and resource policy.
- Deep frame manifests record validity, reference/correction/unresolved counts, backend/device evidence class and fingerprint versions.
- A frame with unresolved pixels beyond accepted policy is not promotable and cannot be passed to FFmpeg.
- Full/tiled/reordered output must share exact global mapping and accepted seam/halo semantics.
- Current deterministic CPU PNG frames remain authoritative until DEC-036 accepts a separately classified GPU deep-frame path after direct-reference equivalence.
- FFmpeg remains an external encoder only. It cannot select precision, render, repair invalid frames or alter timing/count.
- Existing fixed argument vectors, bounded logs, decode probe, source-frame preservation, temporary output and atomic promotion remain unchanged.
- Preflight and execution enforce CPU/RAM/VRAM/cache/worker/disk/cancellation budgets; resource pressure cannot silently reduce required numerical precision.

BR-20260730-01 adds no export fields or format version. Exact job/manifests and deep-frame authority remain proposed under PH-15 and VAL-057–VAL-060.

## File-backed desktop export pause evidence — BR-20260924-01

The native fixture checks static images, saved-image slideshows and an explicitly supplied local MP4 across both export dialogs. Each route covers a running background, a pre-existing user pause, a pre-existing automatic pause that clears while the dialog is open, and a dialog closed while suspension remains reported: **24 cases passed**. Synthetic power messages and Tray Resume do not release the modal hold. Closing restores running status only when appropriate; a remaining suspension clears only after the subsequent resume notification. Desktop host HWND, camera text and Undo label are preserved.

The reproduced defect was automatic recovery resuming desktop presentation inside the still-open frame export dialog. AppWindow now tracks the export hold separately, keeps automatic-pause state current and re-evaluates it on close. Export-open commands arriving after owner enablement but before the modal loop returns are deferred rather than discarded.

This is native controller-status/host-continuity evidence from idle modal dialogs, not physical sleep/resume, visual/video-timestamp pause proof or an export-job-plus-desktop workload test. Existing WIC success/resume/refusal/cancellation checks passed separately in the same cumulative run with desktop presentation stopped. Real FFmpeg opt-ins were skipped. Open Folder/Open Output, active-job desktop pause, DPI, keyboard and accessibility coverage remain open. DEC-039 still excludes all renderer-backed desktop modes.
