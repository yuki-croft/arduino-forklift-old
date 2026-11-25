#!/usr/bin/env bash
# PC-only logic test of final_center_board (phone commands, LEDs, occupancy) with fake I/O.
# This is NOT an Arduino IDE compile and does NOT run on a board.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
conv() { sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$1" > "$2"; }
conv "$root/final_center_board/final_center_board.ino" "$work/main.inc"
conv "$root/final_center_board/center_hw.ino"     "$work/hw.inc"
conv "$root/final_center_board/center_logic.ino"  "$work/logic.inc"
cp "$here"/*.h "$here/test_center_logic.cpp" "$work/"
g++ -std=c++11 -I"$work" -DSRC_MAIN='"main.inc"' -DSRC_HW='"hw.inc"' -DSRC_LOGIC='"logic.inc"' \
    "$work/test_center_logic.cpp" -o "$work/test_center_logic"
"$work/test_center_logic"
