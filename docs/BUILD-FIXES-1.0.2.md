# Build fixes 1.0.2

- Disabled MSVC linker-generated manifests because `src/App/resources.rc` already embeds `src/App/app.manifest` as resource type `RT_MANIFEST`, ID 1. This resolves `CVT1100: duplicate resource. type: MANIFEST, name: 1`.
- Reworked UTF-8/UTF-16 fallback conversion to use Windows conversion APIs with replacement semantics rather than lossy iterator narrowing. This resolves MSVC warning C4244 in `AppWindow::ToUtf8`.
- Enabled `/WX` for the Windows application target when `MW_WARNINGS_AS_ERRORS=ON`.
- Added `--clean-first` to the release build to reduce stale-object and stale-resource issues.
