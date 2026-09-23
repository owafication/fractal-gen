# UI Workflows and Routes

**Status:** Current workflow and route map; proposed deep routes retain separate gates  
**Purpose:** Own user flows, screen responsibilities, state boundaries, permissions, fallbacks and route validation  
**Owner:** Win32 UI  
**Reading trigger:** AppWindow, dialogs, Quick Controller, tray or workflow changes  
**Update trigger:** Screen, command, navigation, state ownership or fallback change

There are no HTTP routes. `ROUTE-###` identifies desktop user workflows.

## State terms

- **Working preview state:** editable `workingPreset_` and preview runtime.
- **Persisted user settings:** `AppSettings` and custom assets stored through `SettingsStore`.
- **Desktop runtime:** `WallpaperController`, active file-backed desktop mode, decoded image pixels or Windows Media Foundation video playback. It does not own a continuous fractal renderer.
- **Temporary job state:** Scout, high-resolution render and deterministic frame-sequence progress/cancellation; PH-08 verified frame manifests live beside user-selected output, not in settings.

## Route map

| Route | Flow and screens | State touched | Permission / high-risk action | Fallback | Validation |
|---|---|---|---|---|---|
| ROUTE-001 | Launch → settings load → main preview | settings, preview runtime | Local files only; HKCU startup only when configured | Defaults + corrupt-file preservation | VAL-005, VAL-006, VAL-015 |
| ROUTE-002 | Preview edit: camera, rotation, palette, equation, performance-visible controls; bounded Undo/Redo | working preview state through scalar/gesture/replacement/history boundaries | No desktop mutation until Apply | transactional rejection; validation/clamping; labelled Ctrl+Z/Ctrl+Y; renderer error surface | VAL-006, VAL-009–VAL-016, VAL-019, VAL-020, VAL-022 |
| ROUTE-003 | Desktop page: choose None, Static image, Slideshow or Video file → Apply; choose Mirror or Span layout; Video may also choose an exported MP4 directly | desktop runtime; default mode, layout and selected local video path | Starts/stops desktop host; video is muted and looped | missing/invalid media, asynchronous playback failure or slideshow exhaustion fails visibly and stops; never falls back to fractal rendering | VAL-006, VAL-007, VAL-061 |
| ROUTE-004 | Journey Settings → edit candidate → accept/reject replacement | preset journey data; runtime animation | Bounded row text, times and count | reject/cancel preserves authoritative preset | VAL-005, VAL-006, VAL-016, VAL-026 |
| ROUTE-013 | Animation Timeline → edit candidate → scrub/play preview → OK/Cancel; optional lossless Tracks → Journey preparation | runtime-only timeline; preview clock; preset Journey only after accepted reverse conversion | Candidate preview must not mutate `workingPreset_`; unsupported conversion is refused | close restores prior preview clock/state; invalid timeline disables OK; Journey change uses atomic replacement | VAL-023–VAL-026 |
| ROUTE-005 | Palette Editor → domain candidate live preview → Save/Cancel replacement | palette/stripe fields only; one exclusive AppWindow edit session | Bounded colour count/data | Main/Quick Controller and competing routes are disabled until close; Cancel discards the candidate | VAL-006, VAL-014, VAL-016, VAL-019 |
| ROUTE-006 | Equation Editor → domain candidate live preview → Save/Cancel replacement | bounded `EquationSettings` only; one exclusive AppWindow edit session | Imported data must never become code | Main/Quick Controller and competing routes are disabled until close; Cancel discards the candidate | VAL-006, VAL-016, VAL-018, VAL-019 |
| ROUTE-007 | Fractal Scout → search → preview → Save as New / Apply later | temporary job and candidate copy | Bounded worker and memory | cancel/discard partial; no persisted mutation on preview | VAL-017, VAL-034–VAL-036 |
| ROUTE-008 | Render Hi-Res → choose settings/path → render/save | immutable render request should be preferred; output file | Long-running local output | cancel; temporary output; CPU/backend fallback as explicitly allowed | VAL-012, VAL-017 |
| ROUTE-014 | Preview → Export Frames... → choose output/timing/size → Start/Cancel/Resume → Open Folder | immutable preset/timeline/settings job; output PNGs and `.mw-frame-sequence` metadata | Long-running local writes; active file-backed desktop presentation is temporarily paused | reject mismatched/untracked output; preserve verified frames/manifest; joined worker; explicit error | VAL-027–VAL-030 |
| ROUTE-009 | Slideshow manager → add/import/reorder/select | static wallpaper settings and local paths | Reads selected image files | reject decode failures; retain previous list/current | VAL-006, VAL-007, VAL-015 |
| ROUTE-010 | Diagnostics → open/copy/clear | local logs and summary | Clear is destructive to logs only | confirmation policy as implemented; no unrelated data | VAL-006, VAL-018 |
| ROUTE-011 | Quick Controller / tray → preview and desktop commands | preview or desktop runtime according to command | Explicit desktop actions | main window remains recovery surface | VAL-006, VAL-007 |
| ROUTE-012 | Stop/exit → detach/destroy desktop window | runtime only; last state persistence where defined | Must preserve configured Windows wallpaper | reveal prior wallpaper; bounded cleanup | VAL-006, VAL-007 |
| ROUTE-015 | Edit/Copy exact coordinates | exact project camera and derived legacy view | Bounded exact parsing; no initial binary conversion | Parse/planner refusal and no partial commit; unsupported preview preserves current state | VAL-038, VAL-039, VAL-041 |
| ROUTE-016 | Configure precision intent → preview resolves strategy from the current camera and backend capability | persisted intent plus transient resolved strategy | Precision/resource allocation | One bounded renderer restart/fallback or explicit unsupported reason | VAL-045–VAL-047 |
| ROUTE-017 | Continuous zoom / Journey / timeline / Scout candidate promotion | proposed exact camera plus render generation | Cancellable reference/correction work | Last valid frame; stale discard; exact Apply as one transaction | VAL-044, VAL-049, VAL-058 |
| ROUTE-018 | Deep still / frame / video export | proposed immutable exact job, plan and validity state | Long-running local output and external FFmpeg after source validation | Temp output; preserve source frames; no invalid promotion | VAL-057, VAL-059, VAL-060 |

## UI contract rules

1. Selecting a desktop mode does not apply it until the explicit Apply action.
2. Preview changes do not silently mutate persisted custom assets or desktop runtime.
3. State-editing auxiliary routes use one AppWindow edit session; cancellation restoration and save boundaries stay inside that session.
4. Long-running work exposes progress and cancellation without blocking safe shutdown.
5. Error messages identify the failed operation and preserve prior valid state.
6. Reduced motion and pause policies affect runtime playback, not persisted artistic data unless explicitly saved.
7. Bounded undo/redo operates on registered scalar/camera state and accepted atomic project replacements, not user preferences or runtime frames.
8. Palette, Equation, Settings and other auxiliary editors remain owner-associated tool windows, but AppWindow permits only one edit session at a time. The active session disables the main window and any Quick Controller; competing posted, tray and delayed routes are ignored until commit or cancel returns control. Candidate preview and Save therefore cannot race another in-memory snapshot.
9. Deep-coordinate text and camera operations must preserve compensated components or use an explicit managed conversion.
10. Keyboard, DPI, high contrast and small-screen behaviour remain part of Windows acceptance.
11. Coordinate text, Jump, Reset, Scout Apply and built-in palette selection use the bounded project mutation coordinator with explicit origins.
12. Preview drag/wheel camera edits use user-gesture origin and stable bounded coalescing metadata; runtime animation evaluation remains outside project history.
13. Preset load/import and accepted Palette, Equation, Settings and Journey dialogs use classified whole-preset replacement and create one atomic PH-05 history entry when changed.
14. Preview Undo/Redo buttons and Ctrl+Z/Ctrl+Y reflect one core history; active drag or palette-thumb gestures reject replay until the gesture ends.
15. The PH-07 timeline editor is modal and candidate-owned. Scrubbing/playback uses only `AnimationClockDomain::Preview`; export time, the inert compatibility wallpaper clock and `workingPreset_` remain unchanged. No timeline clock drives desktop presentation.
16. Runtime timeline edits do not enter project history. A prepared Tracks → Journey conversion enters history only after OK through the existing Journey replacement boundary.
17. PH-08 frame export is modal but executes on one joined worker. Close while active requests cancellation; final dialog destruction cannot detach work.
18. Frame export never overwrites an untracked final PNG and resumes only a matching typed manifest/fingerprint after revalidating completed files.
19. Proposed PH-12+ exact coordinate input is parsed and canonicalised before binary conversion; unsupported values fail without changing the authoritative camera.
20. Precision UI stores only accepted artistic intent. Device capability, selected plan, fallback reason and precision stages are transient diagnostics.
21. Deep navigation presents the last valid frame while generation-safe work is pending; stale orbit/correction results cannot update the preview.
22. Deep export validates every source frame before FFmpeg and preserves the current PH-08/PH-09 temporary-output, source-preservation and atomic-promotion rules.
23. Desktop backgrounds are file-backed. Static/slideshow decode ordinary image files; Video decodes a selected local MP4. Preview animation, Journey and timeline remain preview/export authoring features and cannot be applied as continuously rendered desktop modes.
24. Schema-10 `live-image`, `journey` and legacy `startWallpaperOnLaunch=true` migrate fail-closed to None. Schema-11 Independent monitor mode migrates to Mirror and its per-monitor assignment map is discarded. Static-gallery load failure, later slideshow exhaustion and asynchronous video failure all stop visibly rather than starting a renderer.

## Open UI decisions

- PH-08 uses a modal progress/cancel surface and cancel/wait close policy. A future background job centre would be an intentional managed UX change.

## Current native interaction evidence

BR-20260924-02 adds the exclusive editor-session gate. The editor-only native fixture opens Palette, Equation and Settings through the production routes, verifies that the main window is disabled, posts a competing editor command and observes no competing window for 750 ms, then closes the active editor and verifies owner restoration. This corrects the earlier concurrent-camera test assumption: a second candidate window is no longer permitted to coexist.

BR-20260909-03 adds a serial, process-scoped `WindowsInteractionFixture` over the real Release application and isolated `MW_APPDATA_DIR`. It opens Palette and Equation through their main-window commands, changes the authoritative exact camera while each editor remains open, changes and accepts an editor-owned setting, and verifies the newer canonical camera survives. On the recorded D3D11 run, the same live camera changes moved the preview precision from `GPU float32` to `Split high/low float` and back under requested Automatic mode. Accepted Palette/Equation changes exposed their structural history labels; Equation Undo/Redo restored powers 2/3 without moving the camera. This is ROUTE-002/005/006/016 interaction evidence only, not keyboard, DPI, accessibility, all-dialog or desktop-runtime acceptance.

BR-20260909-04 extends the same isolated native fixture through the real preset combo selection notification. Loading the second built-in preset exposes `Load Preset`; Undo restores the exact pre-load camera and working preset, and Redo restores both the loaded camera and `Seahorse Valley` identity. The fixture now waits for complete dialog control construction and for the main-window history label that signals the accepted dialog candidate has committed, removing an observed harness race without changing production behavior. Settings, Journey, import and Scout Apply remain open.

BR-20260909-05 extends the fixture through Settings accept and the production `Edit Project Settings` transaction. The first focused run reproduced a stale main-window rotation field after the accepted preset state had changed. The accepted Settings path now synchronises every main-window camera control from the authoritative working preset; native Undo/Redo restores rotation `0`/`17.5` through one structural entry. Journey, import and Scout Apply remain open.

BR-20260909-06 drives the real Journey Settings dialog with a valid two-row structured route. Acceptance exposes one `Edit Journey` history entry; reopening after acceptance, Undo and Redo proves exact before/after waypoint text restoration. The isolated fixture applies no desktop mode, so this proves project replacement/history behavior only; Journey desktop playback was subsequently removed by DEC-039. Import and Scout Apply remain open.

BR-20260910-01 creates a bounded import candidate with the production preset serialiser, round-trip validates it, submits its isolated path through the native Open dialog and requires one `Import Preset` entry. Undo restores the exact pre-import camera and preset identity; Redo restores the imported camera and `Fixture Imported` identity. Scout Apply remains open.

BR-20260910-02 waits for the production Fractal Scout search to expose a selectable result, invokes `Use in Preview`, and requires one `Apply Scout Camera` entry. Undo and Redo restore the exact before/after camera text while the selected preset identity remains unchanged, completing the automated native PH-05 replacement matrix.

BR-20260910-03 first closes the production Scout dialog after its selected candidate and thumbnail are ready, proving the project camera, preset identity and history label remain unchanged. It then reopens Scout and retains the BR-20260910-02 Apply replay checks, completing VAL-035/VAL-036 and PH-10 at the documented bounded/native scope.

BR-20260910-04 drives the production Animation Timeline through Add Track, Add Current Value, midpoint scrub, Play and Stop. Cancel discards the candidate track and restores project/history state; a second edit accepted with OK remains present on reopen while the project camera, preset identity and history label stay unchanged. Visual playback and Journey conversion interaction remain open.

BR-20260910-05 supplies a production-serialised preset with a valid Journey, requires Journey → Tracks to expose three camera tracks, then requires Tracks → Journey to expose its successful preparation confirmation. Cancel leaves project camera, preset identity and history unchanged; rendered visual playback and DPI/keyboard remain open.

BR-20260910-06 drives ROUTE-014 through the production modal dialog with fixture-owned output. Start produces one WIC-encoded 32x24 PNG plus its typed manifest with no `.part` file; reopening the same job reports `0 rendered, 1 resumed`. Separate 1024x1024 jobs are cancelled through the visible Cancel button and the window close action; both workers join, preserve resumable metadata and leave no partial file. Close while active retains the cancelled dialog until the user closes it again. Closing the completed/cancelled dialogs restores the main owner without changing camera, preset identity or history. Native mismatched-manifest and untracked-final errors are displayed and preserve their existing files. Open Folder launch, file-backed desktop pause/resume, DPI, keyboard and accessibility remain open.

## PH-09 MP4 export route

**Entry:** Preview page → **Encode MP4...**.

1. Select a completed PH-08 PNG sequence directory.
2. Select an FFmpeg executable or use **Detect PATH**. The selection is not remembered.
3. Select a new `.mp4` destination, bounded H.264 preset/CRF and optional post-success frame cleanup.
4. The dialog validates FFmpeg version/`libx264`/MP4 capabilities, then verifies every manifest-tracked source frame before encoding.
5. Progress and bounded logs are reported while the owned process runs. **Cancel** terminates only that process and preserves source frames.
6. The temporary MP4 is decode-probed and promoted only after success. **Open Output** becomes available for the final file.

BR-20260918-01 exercises the production dialog with the verified one-frame PH-08 fixture sequence. Typing the sequence directory now refreshes the manifest summary consistently with Browse selection. Missing required paths and CRF 52 expose their bounded validation messages; a nonexistent FFmpeg path is refused before process creation, returns control to the dialog and leaves no final or temporary MP4. With an explicitly supplied FFmpeg 8.1.1 path, two opt-in runs complete capability checking, encoding, decode verification and promotion; Open Output becomes enabled, the source PNG remains and no temporary MP4 survives. Two additional runs use the visible Cancel action during a verified 120-frame encode; the owned process terminates, every source frame revalidates and no output survives. Closing restores the main owner without changing project camera, selected preset identity or history. Open Output launch, file-backed desktop pause/resume, DPI, keyboard and accessibility remain open.

The route is modal. If file-backed desktop presentation is active, AppWindow pauses it before the operation and resumes it through the existing policy after the dialog closes. Desktop presentation has no fractal GPU resources to release. No project mutation, undo entry or persisted executable path is produced.

## Proposed deep-route status

ROUTE-015 has a bounded implemented slice: main-window coordinate text is parsed as exact decimals and commits through the exact-camera transaction only when the current preview planner can execute it safely; copy uses canonical text. Deep navigation remains unavailable rather than silently rendering a lossy preview. ROUTE-016 has an implemented legacy-preview slice: automatic mode resolves again from the live camera on every render, and accepted precision changes restart a failed or incompatible preview renderer. The PH-13 deep-plan UI and ROUTE-017–ROUTE-018 remain governance mappings only until their owning phases pass.

## Modal export desktop pause evidence — BR-20260924-01

The native fixture checks static images, saved-image slideshows and an explicitly supplied local MP4 across both export dialogs. Each route covers a running background, a pre-existing user pause, a pre-existing automatic pause that clears while the dialog is open, and a dialog closed while suspension remains reported: **24 cases passed**. Synthetic power messages and Tray Resume do not release the modal hold. Closing restores running status only when appropriate; a remaining suspension clears only after the subsequent resume notification. Desktop host HWND, camera text and Undo label are preserved.

The reproduced defect was automatic recovery resuming desktop presentation inside the still-open frame export dialog. AppWindow now tracks the export hold separately, keeps automatic-pause state current and re-evaluates it on close. Export-open commands arriving after owner enablement but before the modal loop returns are deferred rather than discarded.

This is native controller-status/host-continuity evidence from idle modal dialogs, not physical sleep/resume, visual/video-timestamp pause proof or an export-job-plus-desktop workload test. Existing WIC success/resume/refusal/cancellation checks passed separately in the same cumulative run with desktop presentation stopped. Real FFmpeg opt-ins were skipped. Open Folder/Open Output, active-job desktop pause, DPI, keyboard and accessibility coverage remain open. DEC-039 still excludes all renderer-backed desktop modes.

See [evidence ledger](../artifacts/ph11-20260924-01/report.md).
