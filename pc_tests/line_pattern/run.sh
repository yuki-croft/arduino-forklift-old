#!/usr/bin/env bash
# PC test of the line-pattern detector (forklift_line_pattern.h) and the forklift_sense.ino glue on SYNTHETIC traces.
# The sensor values are assumptions, not measurements. NOT an Arduino IDE compile, no board.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
cp "$root/final_forklift_board/forklift_line_pattern.h" "$root/final_forklift_board/forklift_sense.ino" "$work/"
mv "$work/forklift_sense.ino" "$work/forklift_sense.inc"
cp "$here/Arduino.h" "$here/test_line_pattern.cpp" "$work/"
g++ -std=c++11 -Wall -Wextra -I"$work" -DSRC_SENSE='"forklift_sense.inc"' "$work/test_line_pattern.cpp" -o "$work/test_line_pattern"
"$work/test_line_pattern"
