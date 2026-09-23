# PH-02 Visual Baseline Approval

**Approved by:** User
**Date:** 2026-07-29
**Scope:** CPU production-renderer baseline images for the four canonical fixtures.

The approved fixture baselines use exact thresholds on the recorded native MSVC environment:

- maximum channel error: 0
- maximum differing ratio: 0
- minimum structural similarity: 1

The baseline promotion contains only reviewed `baseline.ppm` images. D3D11 WARP, hardware D3D11 and OpenGL retain same-process repeatability evidence; they are not asserted to be pixel-equivalent to CPU baselines.
