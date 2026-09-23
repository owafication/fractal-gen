# Security, Privacy and Risk

**Status:** Current canonical safeguards and open risk register  
**Purpose:** Own trust boundaries, sensitive actions, privacy limits, resource safety and risks  
**Owner:** Security and reliability  
**Reading trigger:** Imports, output paths, logs, startup, desktop integration, external tools, migrations or long-running work  
**Update trigger:** New input, integration, permission, external executable, logging or failure mode

## Security and privacy posture

### Observed

- The application source contains no intended network client, account or analytics path.
- Settings, logs and default static renders are local to the current user.
- Startup uses the current user's HKCU Run key.
- Desktop integration creates an application-owned window and does not intentionally patch Explorer or replace the configured wallpaper file.
- JSON input size and nesting are bounded.
- `EquationSettings` is a bounded numeric/data model. It includes powers, coefficients, transforms, Julia/Newton modes, colouring and post-processing, but does not accept imported shader source or executable expressions.
- Logs are size-limited and rotated; current policy excludes intentional capture of window titles, keystrokes, unrelated filenames and document contents.

### Required safeguards

- Treat every imported path and file as untrusted data.
- Validate extensions by content/codec path, not extension alone where practical.
- Never pass imported text to a shell, shader compiler, script engine or dynamic loader.
- Canonicalise output intent and avoid overwriting existing files without explicit user selection/policy.
- Use temporary output and atomic promotion; preserve prior valid state on failure.
- Bound dimensions, iterations, palette sizes, journey rows, candidate counts, retained thumbnails, memory, workers and retry loops.
- Capture only application-specific diagnostic data and allow local clearing.
- FFmpeg may be invoked only under accepted DEC-009/DEC-027: user-supplied executable, capability probes, exact `CreateProcessW` launch, fixed arguments, bounded logs, owned cancellation and verified output promotion. Do not bundle, download, remember its path or accept arbitrary templates.
- Desktop video playback accepts only an explicitly selected local regular `.mp4` file, then requires Windows Media Foundation to resolve a selected video stream. It is treated as untrusted media data and is never passed to FFmpeg, a shell or the fractal renderer.
- Avoid logging full user-selected file paths when a filename/path is not required for diagnosis; review before release.
- Treat exact coordinate text as bounded private user data: cap digits, exponent, parse work and resulting precision allocation; do not log full coordinates or include them in reports unless the user explicitly exports them.

## High-risk actions

| Action | Required control |
|---|---|
| Start with Windows | Per-user only; explicit setting; reversible |
| Attach desktop host | Bounded timeout/retry; reversible detach; no Explorer patching |
| Import preset/settings | Size/depth/type validation; no executable interpretation |
| Save settings/project | Temporary file, validation and atomic replacement |
| Render large output | Bounds, progress, cancellation, temp final and resource policy |
| Delete/clear logs or custom asset | Explicit user action and narrow target |
| External encoder | Fixed executable + argument vector; captured logs; no shell template |
| Local desktop video | Explicit regular `.mp4`; Media Foundation video-stream validation; muted playback; reversible Stop/detach |
| Migration | Original preserved; in-memory transform; deterministic defaults; validation |
| Exact coordinate / precision request | Bounded syntax, digit/exponent/allocation limits; data-only handling; no shader/command interpolation |

## Risk register

| ID | Risk | Severity | Evidence/status | Containment |
|---|---|---:|---|---|
| RISK-001 | Release version surfaces can drift | High | Mitigated 2026-07-27; VAL-002 passed in portable environment | CMake configure/CTest consistency gate plus built-executable metadata check; retain native release evidence |
| RISK-002 | Portable checks miss MSVC/Win32 regressions | High | Native automation and bounded interaction plus isolated installer lifecycle pass; Windows 11 and broader manual matrix remain | Canonical MSVC preset, report-producing PH-01 runner, isolated startup smoke and manual Windows matrix |
| RISK-003 | WorkerW/Progman is undocumented/version-sensitive | High | Current-topology display handling and simulated owned-host reattachment pass; actual Explorer restart and cross-version behavior remain unproven | Runtime matrix, reversible fallback, no compatibility guarantee |
| RISK-004 | Current models, snapshots, UI and runtime become competing authorities | High | Future risk | Adapters first; domain migration; VAL-016 |
| RISK-005 | Deep coordinates lose low components or precision during generic state/animation | High | PH-06 core/native and compensated/exact refusal evidence pass; exact keyframe execution remains open | Explicit compensated animation values/interpolation; VAL-024 deep fixtures; never flatten to one double |
| RISK-006 | GPU/driver variation causes false visual failures or hides regressions | Medium | Future risk | CPU canonical; WARP where suitable; backend thresholds; metadata |
| RISK-007 | Playback/export/background operations enter undo history | High | Scalar/structural exclusion and native Timeline/export/Scout isolation evidenced; broader runtime matrix remains open | Origin/eligibility gate, UndoRedo replay origin, startup suppression and structural runtime exclusion |
| RISK-008 | Cancellation or encoder failure corrupts/deletes output | High | Core, real FFmpeg and native frame/video cancellation/failure containment evidenced at bounded scope | Temp files, manifest, decode verification, atomic promotion, preserve frames |
| RISK-009 | FFmpeg licensing/distribution or command injection | High | Fixed-vector external FFmpeg 8.1.1 and native dialog evidence pass; other builds and distribution licensing remain separately gated | External user-supplied executable; no bundle/download/path persistence; capability gate; exact process launch; fixed args; bounded logs |
| RISK-010 | Settings/project migration damages user data | High | Cross-cutting; startup smoke now isolated | Original preservation, in-memory migration, fixtures, atomic save; `MW_APPDATA_DIR` prevents validation from loading normal user settings |
| RISK-011 | Diagnostics expose user paths or unrelated activity | Medium | Policy present; runtime review unproven | Narrow logging and release privacy review |
| RISK-012 | Large renders/Scout/animation exhaust CPU, GPU or memory | Medium | Resource bounds plus 60-second static-presentation counters pass in BR-20260923-03; long-duration playback/export soak remains unproven | 256 tracks, 4096 keyframes, seven-day duration, bounded IDs, 250 ms maximum timer advance; later cancellation/soak tests |
| RISK-013 | Long decimal coordinates are rounded before exact state | Critical | Bounded exact parser/camera and native exact-entry slices implemented; broader PH-12 audit remains | Bounded exact parser; prohibit initial `double` conversion; canonical round-trip tests |
| RISK-014 | Exact and legacy camera fields become independently mutable | Critical | Exact transaction and one-way adapter slices implemented; remaining authority integration stays gated | One exact authority; one-way adapters; full camera-write audit |
| RISK-015 | Scale remains a double/float bottleneck after centre migration | Critical | Exact half-height and 512–16384-bit direct CPU routes implemented; GPU encoding envelope remains unvalidated | Exact half-height; versioned delta/scale encoding; depth fixtures |
| RISK-016 | “512-bit” wording overstates four-float GPU orbit/delta accuracy | High | DEC-032 accepted; float4 transport/reconstruction measurements are bounded and provide no validated deep-GPU ceiling | Report precision stages separately; measure encoding limits |
| RISK-017 | D3D11 and OpenGL choose different precision semantics | High | Legacy GPU thresholds centralized in planner; validated deep production perturbation remains open | Central plan; backend policy audit; captured plan identity |
| RISK-018 | Unsupported equation enters an incompatible perturbation recurrence | Critical | Core planner/reference/direct routes reject unsupported profiles; PH-14 GPU recurrence validation remains open | Versioned formula fingerprint and fail-closed planner |
| RISK-019 | Synchronous high-precision orbit generation stalls UI/rendering | High | Bounded cancellable reference worker implemented; current renderers do not yet consume the new service | Cancellable bounded worker service; last-valid-frame presentation |
| RISK-020 | Stale orbit/correction work overwrites current generation | Critical | Service/worker generation and immutable-plan evidence pass; renderer upload/commit integration remains open | Generation and fingerprint checks before upload/commit |
| RISK-021 | Single-reference instability yields plausible incorrect colour | Critical | Current correctness gap | Validity mask; direct reference; rebase/correct/reject |
| RISK-022 | Multi-reference or correction grows without bound | High | Proposed PH-14 | Enforced reference/pixel/subdivision/retry/memory/time budgets |
| RISK-023 | Tile camera derivation diverges from exact global mapping | High | Exact direct CPU global mapping/AA/tile-origin slices implemented; deep scheduler and PH-15 integration remain open | Exact global sample fixtures, reversed tile order and seam strips |
| RISK-024 | Migration or downgrade discards exact values | Critical | Schema 3/12 forward migration and original preservation implemented; no downgrade writer is authorized | Preserve originals; version fields; visible read-only/downgrade policy |
| RISK-025 | Ordinary/current-limit views regress during deep changes | High | Cross-phase | Freeze compatibility fixtures; cumulative visual checks |
| RISK-026 | Ordinary-double CPU output is used as a false deep oracle | Critical | Vendored independent Boost reference and exact direct tiers implemented; production perturbation comparison remains open | Independent direct high-precision samples/tiles |
| RISK-027 | Orbit/cache/temp storage grows with session duration | High | Reference cache/worker bounds and cancellation evidenced; static resource check is not a deep-render/cache soak | Byte accounting, eviction, disk reserve, cancellation and soak |
| RISK-028 | Device loss/cancel promotes partial deep output | Critical | Existing output controls require extension | Validity-aware temp/promotion rules and fault injection |
| RISK-029 | Structural verifier and accepted source behaviour diverge | High | Mitigated by BR-20260730-01 | Keep source audit aligned and green on the exact source state |
| RISK-030 | Product copy claims infinite support beyond evidence | High | Prohibited | Measured support matrix and release wording audit |
| RISK-031 | Malformed or unsupported local desktop media causes hangs or unsafe fallback | High | Static/slideshow exhaustion, invalid MP4, mute/loop and injected asynchronous stop/detach pass; readiness race corrected; codec/physical lifecycle matrix remains | Media Foundation stream validation, no rendered fallback, visible failure, reversible Stop/detach, lifecycle and soak validation |
| RISK-032 | MFPlay is a legacy Windows playback API and may accumulate compatibility or maintenance risk | Medium | Accepted technical debt under DEC-041 | Keep the current route bounded to explicit local exported MP4; surface asynchronous errors and stop safely; evaluate `IMFMediaEngine` or the current Windows media-player API before expanding playback scope |

## Installer process and file boundary

Setup and uninstall check the application single-instance mutex and require a running application to close; they never use global process-name termination or automatic application closure. Uninstall removes only installation-tracked files and preserves untracked user outputs. BR-20260923-03 verifies this with an isolated AppId, directory and data root; normal user installation and preceding-release upgrade remain unproven.

## Incident handling

1. Stop the affected operation; do not retry indefinitely.
2. Preserve the last valid settings/project/output and relevant bounded logs.
3. Record exact environment, inputs, backend and fingerprint without collecting unrelated data.
4. Reproduce using the smallest safe fixture.
5. Add a regression check before fixing when feasible.
6. Validate the fix within the fault's actual layer and all affected release gates.

## Unproven items

- Actual network behaviour was not executed or packet-inspected.
- Log content across all runtime paths was not exhaustively reviewed.
- Atomic rename behaviour under Windows failures was not fault-injected.
- Current-topology WorkerW attachment and simulated recovery have bounded native evidence in BR-20260923-03; actual Explorer restart, GPU device-loss fallback and startup registration remain outside that run.

## PH-08 output and resume boundary

- Export remains local and offline. The filename prefix is limited to 64 ASCII letters, digits, hyphen and underscore; frame numbers are bounded and generated internally.
- The user selects an output directory, but the core never accepts arbitrary final file names from timeline/project data and refuses to overwrite untracked final frames.
- Temporary frames, receipts and manifest are confined to `.mw-frame-sequence` under the selected output directory. Cancellation removes only the active incomplete temporary frame.
- Resume requires schema/evaluator/application/project/timeline/renderer/timing/output identity and revalidates dimensions, file size and digest. Malformed, duplicate or mismatched metadata fails closed.
- The export worker is joined before its UI state can be destroyed. No shell, network, external executable or persisted secret is introduced in PH-08.
- FNV-1a64 receipts are corruption/identity checks within a SHA-256-bound job, not cryptographic authenticity claims; hostile local filesystem modification is detected on resume but not prevented.

## PH-09 external encoder boundary

- The selected executable path is process-runtime input only. It is not saved to settings, presets, manifests or project history.
- The exact executable is launched directly; command interpreters, shell expansion, environment-authored argument templates and user-supplied raw arguments are excluded.
- Capability probes and encoding use application-owned bounded vectors. Captured stdout/stderr are capped at 1 MiB each and written under the frame sequence metadata directory.
- Final output promotion is withheld until process success, non-empty temporary output and a successful decode probe. Existing final files and untracked sequence files are never overwritten/deleted.
- Fixed yuv420p encoding rejects odd source dimensions before external launch, avoiding a known encoder failure that can be classified locally without weakening PH-08 PNG support.
- BR-20260909-01 adds real FFmpeg 8.1.1 success-path evidence through the production process/export boundary, a bounded real-time replay proving owned process termination, a full production export cancellation run proving no partial/final output and complete source revalidation, and a post-preflight source-race run proving real encoder failure leaves no partial/final output and preserves revalidated source artifacts. Its report deliberately retains the executable filename and version line but not the selected installation directory. This does not establish Win32 dialog behavior, compatibility with other builds, or licence suitability.
- Cancellation targets only the process created for the current operation. Failure and cancellation remove only the owned temporary MP4 and preserve verified source frames and diagnostic logs.
- Optional cleanup is opt-in and executes after verified promotion; it enumerates only manifest-tracked frames/receipts and refuses broader directory deletion.

## Vendored high-precision dependency boundary

- The accepted dependency is the source-only Boost.Multiprecision 1.83.0 standalone subset under BSL-1.0.
- Configure and build steps must not download dependencies or resolve a machine-global Boost installation.
- `scripts/verify-third-party.py` must verify the package manifest, content-index digest and every vendored file before source acceptance or packaging.
- The dependency contributes no DLL, service, executable, network client, installer registration or runtime search path.
- Boost types remain private to the platform-neutral precision implementation; persistence and public project interfaces remain project-owned.
- Package upgrades are managed supply-chain changes requiring inspected source, regenerated hashes, licence review, compiler evidence and numerical verification.
- Release packages must include `THIRD_PARTY_NOTICES.md`; source packages must retain the full Boost licence beside the vendored files.
