#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build="$root/build-visual"
output="${MW_VISUAL_OUTPUT_DIR:-$root/test_artifacts/visual}"

cmake -S "$root" -B "$build" \
  -DMW_BUILD_TESTS=ON \
  -DMW_BUILD_VISUAL_FIXTURES=ON \
  -DMW_WARNINGS_AS_ERRORS=ON
cmake --build "$build" --parallel --target MandelbrotVisualFixtures
"$build/MandelbrotVisualFixtures" --output-dir "$output" "$@"
