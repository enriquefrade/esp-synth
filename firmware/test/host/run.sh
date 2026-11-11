#!/usr/bin/env bash
# Builds and runs the host tests for the synth engine and input logic.
set -euo pipefail

HERE="$(cd "$(dirname "$0")" && pwd)"
FW="$(cd "$HERE/../.." && pwd)"
OUT="$(mktemp -d)"

c++ -std=c++17 -Wall -Wextra -Werror \
  -I "$FW/synth" \
  "$FW/synth/engine.cpp" \
  "$FW/synth/cmd.cpp" \
  "$HERE/test_engine_logic.cpp" \
  -o "$OUT/test_engine_logic"

"$OUT/test_engine_logic"
rm -rf "$OUT"
