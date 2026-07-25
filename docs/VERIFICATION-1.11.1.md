# Verification — 1.11.1

This document records checks executed in the packaging environment. It does not claim a native Windows GUI or OpenGL runtime test unless explicitly stated.

## Source regression coverage

`scripts/verify-source.py` checks that:

- preview and desktop zoom/colour controls have distinct control IDs, state variables and command handlers;
- preview toggles do not call the wallpaper controller;
- desktop toggles do call the wallpaper controller;
- the exact visible preview is copied into a transient live preset by **Send Preview to Desktop**;
- the hover status renderer contains no centre/scale formatting;
- the new controls are included in layout, visibility and z-order groups;
- 1.11.1 version metadata is consistent.

## Automated core coverage

The existing platform-neutral core test suite remains unchanged and is run with warnings treated as errors.

## Platform limit

The packaging environment cannot compile or visually exercise the Win32 application. Final Windows verification should confirm button text and wrapping at supported DPI scales, independent preview/desktop toggling, exact-frame transfer, desktop restart behaviour and all existing hover actions.

## Packaging-environment results

Executed successfully after the final patch:

```text
python3 scripts/verify-source.py
GCC 14 core build with warnings treated as errors
CTest: MandelbrotCoreTests passed under GCC
Clang 17 Release core build with warnings treated as errors
CTest: MandelbrotCoreTests passed under Clang
```
