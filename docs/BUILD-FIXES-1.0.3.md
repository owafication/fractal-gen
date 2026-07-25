# Build and runtime fixes 1.0.3

- Replaced the GLSL 1.20-incompatible integer `clamp` overload with an explicit float clamp and integer conversion.
- Added deterministic preview painting when GPU startup fails, preventing stale child-window contents from appearing as duplicated controls.
- Retained the static CPU wallpaper fallback while keeping the failure visible in diagnostics.
