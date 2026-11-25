#!/usr/bin/env bash
# PC-only logic tests (fake sensors/time). This is NOT an Arduino IDE compile and does NOT run on a board.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT

# <Lib.h> -> "Lib.h" (use the stub headers), strip nothing else
conv() { sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$1" > "$2"; }

# --- final_forklift_board: state machine + comms (fake sensors) ---
conv "$root/final_forklift_board/final_forklift_board.ino" "$work/m.inc"
conv "$root/final_forklift_board/forklift_comm.ino"       "$work/c.inc"
conv "$root/final_forklift_board/forklift_logic.ino"      "$work/l.inc"
python3 "$here/forklift_logic/make_prototypes.py" "$work/c.inc" "$work/l.inc" > "$work/protos.inc"
cat "$here/forklift_logic/hook_protos.inc" >> "$work/protos.inc"
cp "$here"/forklift_logic/fake_arduino_headers/*.h "$here/forklift_logic/test_forklift_logic.cpp" "$work/"
g++ -std=c++11 -I"$work" -DSRC_MAIN='"m.inc"' -DSRC_COMM='"c.inc"' -DSRC_LOGIC='"l.inc"' \
    "$work/test_forklift_logic.cpp" -o "$work/test_forklift_logic"
"$work/test_forklift_logic"

# --- final_center_board: phone commands, LEDs, occupancy (fake I/O) ---
conv "$root/final_center_board/final_center_board.ino" "$work/main.inc"
conv "$root/final_center_board/center_hw.ino"     "$work/hw.inc"
conv "$root/final_center_board/center_logic.ino"  "$work/logic.inc"
cp "$here"/center_logic/fake_arduino_headers/*.h "$here/center_logic/test_center_logic.cpp" "$work/"
g++ -std=c++11 -I"$work" -DSRC_MAIN='"main.inc"' -DSRC_HW='"hw.inc"' -DSRC_LOGIC='"logic.inc"' \
    "$work/test_center_logic.cpp" -o "$work/test_center_logic"
"$work/test_center_logic"
