# Privacy and Security

- The application contains no network client and requires no account.
- Presets and settings remain under the current user’s local application-data directory.
- Imported preset files are capped in size, parsed by a bounded JSON parser and treated as data only.
- Unsupported enum values and malformed structures are rejected.
- Unknown fields do not execute or dynamically load code.
- The application never downloads shaders or scripts.
- Startup uses only the current user’s `HKCU` Run key.
- Desktop integration creates an application-owned window and does not patch Explorer.
- Logs contain application lifecycle, renderer, attachment, display and configuration errors only.
- Logs do not intentionally record window titles, keystrokes, desktop filenames or document contents.

## Custom equations

Custom equations use one fixed renderer program and a bounded numeric model: `A*z² + B*z + C*c + D`. Presets store only finite numeric coefficients and two Boolean absolute-value flags. The application does not evaluate expression strings, compile imported shader source, or execute scripts from presets.
