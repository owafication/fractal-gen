# Privacy and Security

**Status:** Current compatibility summary. Canonical policy and risks are in `project_docs/SECURITY_PRIVACY_AND_RISK.md`.

- The intended application is offline and local-only; no account or analytics is required.
- Settings, logs and default render output are stored under the current user's local application data unless the user selects another output location.
- Imported JSON is size/depth bounded, validated and treated as data.
- `EquationSettings` is a bounded data model supporting coefficients, powers, transforms, Julia/Newton modes, colouring and post-processing. Imported content is not compiled as shader source or executed as script.
- Startup uses the current user's HKCU Run key and does not require administrator privileges.
- Desktop integration creates an app-owned window; it does not intentionally patch Explorer or replace the configured wallpaper file.
- Logs are local, size-limited and rotating. They must not intentionally capture keystrokes, unrelated window titles, document contents or unnecessary full paths.
- Large rendering and Scout work must remain bounded and cancellable.
- Future FFmpeg invocation, if approved, must use a fixed executable and argument vector without shell interpolation or arbitrary command templates.

Runtime privacy, network and failure-injection checks remain required before broad assurance claims.
