# Verification — 1.11.4

## Implemented checks

- Hover-menu preset-name edit and rename controls are created, laid out, shown and included in the normal z-order restoration path.
- The edit is bounded to 120 characters and synchronises with the working preset name.
- Rename validation rejects blank and oversized persisted names.
- Built-in presets remain read-only.
- Custom renames preserve the preset ID, save through `SaveSettings`, and roll back after a failed save.
- Version metadata is consistent at 1.11.4.

## Ran and passed

- `python3 scripts/verify-source.py`
- GCC 14 core configuration and build with `MW_WARNINGS_AS_ERRORS=ON`
- GCC 14 `MandelbrotCoreTests`
- Clang 17 core configuration and build with `MW_WARNINGS_AS_ERRORS=ON`
- Clang 17 `MandelbrotCoreTests`

## Verification limit

This environment does not provide the Windows SDK or a Win32 desktop session. Native MSVC compilation and interactive testing of the new edit/button controls were not performed here.
