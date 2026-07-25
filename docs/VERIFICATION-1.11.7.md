# Verification — 1.11.7

## Scope

Removed the preview hover menu completely and removed Play and Stop Desktop from the Quick Controller.

## Source checks

The packaged-source verifier confirms that no preview-overlay IDs, fields, methods, construction, layout, z-order, mouse-display, status-update, or command-routing code remains. It also confirms that the Quick Controller no longer contains Play or Stop Desktop and that its remaining commands still use the established handlers.

## Build checks

The platform-independent source verifier and core tests are run under GCC and Clang.

## Limit

The current environment does not provide the Windows SDK or an interactive Win32 desktop, so native MSVC compilation and visual Quick Controller testing remain unverified.
