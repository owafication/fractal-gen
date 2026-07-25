#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cmake -S "$root" -B "$root/build-core" -DMW_BUILD_TESTS=ON -DMW_WARNINGS_AS_ERRORS=ON
cmake --build "$root/build-core" --parallel
ctest --test-dir "$root/build-core" --output-on-failure
