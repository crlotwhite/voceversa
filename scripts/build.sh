#!/usr/bin/env bash
set -euo pipefail

PRESET="${1:-debug}"

cmake --preset "$PRESET"
cmake --build --preset "$PRESET" -j
ctest --preset "$PRESET" --output-on-failure
