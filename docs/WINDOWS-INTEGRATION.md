# Windows Desktop Integration

## Mechanism

The application uses the common WorkerW/Progman desktop-host technique:

1. Find `Progman`.
2. Send message `0x052C` with a timeout to request the WorkerW desktop hierarchy.
3. Enumerate top-level windows to locate `SHELLDLL_DefView` and the associated WorkerW behind it.
4. Attach the application-owned wallpaper window as a non-activating child behind the desktop icon view.

This behaviour is isolated in `DesktopHost` because WorkerW is undocumented and may change between Explorer versions.

## Reversibility

The application never calls `SystemParametersInfo` to replace the wallpaper image. It only creates and attaches its own window. Stop and shutdown hide, detach and destroy that window. The user’s previous wallpaper remains configured throughout.

## Explorer restart

Every two seconds while the wallpaper is active, the controller verifies that:

- The desktop host still exists.
- The wallpaper window still has that host as its parent.

If not, it locates the new WorkerW/Progman hierarchy and reattaches. Reattachment failures are logged but do not trigger an uncontrolled loop.

## Display changes

`WM_DISPLAYCHANGE` and DPI changes cause monitor enumeration, virtual-desktop bounds, rendering regions and monitor animation assignments to be rebuilt. One render host spans the virtual desktop, including negative monitor coordinates.

## Compatibility risk

WorkerW is not a documented public API contract. The fallback to Progman improves resilience but cannot guarantee compatibility with every Explorer build, third-party shell replacement or policy-managed desktop environment. This is the main Windows-specific runtime item requiring physical Windows 10 and Windows 11 verification.

## Virtual desktop coordinate mapping

A wallpaper window becomes a child of WorkerW or Progman after `SetParent`. Screen coordinates can no longer be passed directly to `SetWindowPos`, especially when monitors extend left or above the primary display. `DesktopHost::MapDesktopRectToHost` maps both corners of the virtual desktop from desktop coordinates into the selected host's client coordinates. Creation, Explorer reattachment and display-change handling all use the mapped rectangle.

Span mode then renders one region from `(0,0)` to the virtual desktop width and height. Mirror and Independent modes create display regions relative to the virtual desktop origin.
