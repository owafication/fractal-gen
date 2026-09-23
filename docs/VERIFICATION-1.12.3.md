# Verification — 1.12.3

## Scope

Phase 4 adds configurable separable bloom, radius-aware GPU tile overlap, and persisted camera rotation across rendering and interaction paths.

## Automated coverage

Core regression coverage verifies:

- Bloom threshold, soft knee, radius, and rotation settings round-trip through JSON.
- A 90-degree rotation maps the right output edge toward positive imaginary coordinates.
- A rotated tile camera centre matches the corresponding global output-pixel mapping.
- Disabled bloom requires no overlap at 1× anti-aliasing.
- Bloom radius contributes directly to overlap.
- Multi-sample anti-aliasing adds the bounded reconstruction allowance.
- Existing unrotated tile-camera quadrant mapping remains unchanged.
- Deep compensated tile offsets remain retained.

## Source-contract checks

The source verifier checks:

- New model fields, validation, and persistence.
- Equation Editor and Settings controls.
- Rotation propagation through preview, wallpaper, CPU fallback, and high-resolution regions.
- Separable OpenGL and Direct3D 11 bloom resources and shader controls.
- Dynamic high-resolution overlap and rotated tile-camera mapping.
- Phase status and release metadata.

## Verification limits

This environment cannot compile or execute Win32, Direct3D 11, OpenGL, or HLSL/GLSL runtime paths. Native MSVC compilation, shader-driver compilation, visual bloom comparison, and rendered seam-image comparison require a Windows GPU environment. Platform-independent model, mapping, persistence, quality, and tile-camera contracts are exercised here.
