# Debugging and Maintenance

**Status:** Current diagnostic and maintenance policy  
**Purpose:** Own fault isolation, diagnostic evidence, maintenance boundaries and update procedure  
**Owner:** Maintainers  
**Reading trigger:** Regression, crash, visual difference, data issue, GPU failure, desktop integration failure or release maintenance  
**Update trigger:** New fault class, diagnostic surface or maintenance procedure

## Debugging order

1. Identify the user-visible symptom and last known accepted checkpoint.
2. Classify the layer: persistence, core math, animation, renderer facade, D3D11, OpenGL, still/tile output, Win32 UI, desktop host, system-state policy, package or installer.
3. Capture environment: application/source revision, Windows version, GPU/driver/backend, precision mode, dimensions, DPI, monitor layout and render fingerprint where applicable.
4. Reproduce with the smallest existing fixture or add a bounded regression fixture.
5. Inspect direct dependencies only; expand context for a concrete evidence gap.
6. Fix the canonical owner and all affected call sites without unrelated refactoring.
7. Run the layer's check plus cross-cutting release gates affected by the fix.
8. Record failed, passed, skipped and unproven evidence.

## Common fault domains

### Native build/version

- Check every version surface first.
- For MSVC access/type failures, inspect public/private contract boundaries rather than exposing internal representations unnecessarily.
- Resource compilation and manifest duplication are separate from C++ compilation.

### Renderer startup

- Record D3D11 creation/HLSL result, OpenGL context/GLSL result and facade fallback decision.
- Do not infer GPU success from a visible CPU fallback image.
- Preserve the one-recovery-attempt rule; no restart loop.

### Visual regression

- Compare fingerprint and fixture environment before pixels.
- Separate raw fractal, colouring, post-processing and tiling differences.
- Inspect seam strips for tiled output.
- Do not approve a broad threshold increase to hide one unexplained backend difference.

### Deep zoom

Use this order so a late colour symptom is not mistaken for an early identity/policy fault:

1. exact camera canonical text, digit count and fingerprint;
2. formula capability fingerprint and central plan/reason;
3. generation ID and typed orbit key;
4. reference calculation precision and orbit encoding;
5. shader validity/classification output;
6. rebase, reference assignment and direct correction;
7. exact global sample/tile/halo mapping;
8. frame validity, temporary promotion and encoder boundary;
9. device/lifecycle state and CPU/RAM/VRAM/cache/worker/disk metrics.

For current pre-PH-12 builds, preserve `centreXLow`/`centreYLow`, record the selected precision strategy, check formula compatibility before perturbation and compare direct/reference/perturbation fixtures progressively. Do not describe an ordinary-double CPU image as an independent deep oracle.

### Desktop integration

- Verify WorkerW/Progman discovery, mapped host coordinates, parent relationship and Explorer restart behaviour.
- Stop must detach/destroy the app-owned window and reveal the prior wallpaper.
- First classify the active file-backed mode: WIC-decoded static image, WIC-decoded slideshow or Media Foundation video. Do not investigate desktop failures through D3D11/OpenGL precision or animation code unless the symptom is the separate one-time static capture operation.
- Missing, invalid or unsupported desktop media must fail visibly and stop; it must never activate a CPU/GPU fractal fallback.
- MFPlay is retained as technical debt for the current bounded exported-MP4 route. It is a legacy Windows API; a future media expansion should migrate to `IMFMediaEngine` or the current Windows media-player API under a separate accepted decision and lifecycle validation batch.

### Persistence

- Preserve the original failing file.
- Record schema and parse/validation error.
- Reproduce with a minimal fixture, not the user's only copy.
- Fault-inject temporary-write and promotion failures where safe.

### Long-running jobs

- Record resolved bounds, job snapshot/fingerprint, progress and cancellation point.
- Check worker lifetime during dialog/app close.
- Final output names must not contain partial data.

## Logging policy

Current logs are local, rotating and size-limited. Add logs only when they reduce diagnosis uncertainty. Avoid secrets, unrelated user activity, file contents, keystrokes and unnecessary full paths. Diagnostic summaries should distinguish selected backend from actual active backend/fallback.

## Maintenance rules

- Keep `src/Core` platform-neutral.
- Keep Windows SDK and COM ownership explicit.
- Keep renderer/animation ownership out of `WallpaperController`; desktop integration presents files only.
- Retain bounded input and resource limits when extending features.
- Add migrations instead of parallel persistence formats.
- Remove obsolete direct mutations after a domain is fully migrated and validated.
- Update historical release records only by adding a new release document.
- Review compiler warnings as defects or documented intentional cases.

## Support report minimum

```text
Observed symptom
Application/source version surfaces
Windows/GPU/driver/backend
Steps to reproduce
Expected vs actual
Relevant bounded logs
Settings/preset fixture with sensitive paths removed
Checks run and results
Workaround/rollback
Unproven items
```
