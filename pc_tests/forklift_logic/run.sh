#!/usr/bin/env bash
# PC-only logic test of final_forklift_board (state machine + comms) with fake sensors/time.
# This is NOT an Arduino IDE compile and does NOT run on a board.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
conv() { sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$1" > "$2"; }   # <Lib.h> -> "Lib.h" (use the fake headers)
conv "$root/final_forklift_board/final_forklift_board.ino" "$work/m.inc"
conv "$root/final_forklift_board/forklift_comm.ino"       "$work/c.inc"
conv "$root/final_forklift_board/forklift_logic.ino"      "$work/l.inc"
python3 "$here/make_prototypes.py" "$work/c.inc" "$work/l.inc" > "$work/protos.inc"
cat "$here/hook_protos.inc" >> "$work/protos.inc"
cp "$here"/*.h "$here/test_forklift_logic.cpp" "$work/"
g++ -std=c++11 -I"$work" -DSRC_MAIN='"m.inc"' -DSRC_COMM='"c.inc"' -DSRC_LOGIC='"l.inc"' \
    "$work/test_forklift_logic.cpp" -o "$work/test_forklift_logic"
"$work/test_forklift_logic"
