#!/usr/bin/env bash
# PC-only logic tests (fake sensors/time). This is NOT an Arduino IDE compile and does NOT run on a board.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT

# <Lib.h> -> "Lib.h" (use the stub headers), strip nothing else
conv() { sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$1" > "$2"; }

# --- forklift ---
conv "$root/forklift_main/forklift_main.ino" "$work/m.inc"
conv "$root/forklift_main/fk_comm.ino"       "$work/c.inc"
conv "$root/forklift_main/fk_logic.ino"      "$work/l.inc"
python3 "$here/forklift/protos.py" "$work/c.inc" "$work/l.inc" > "$work/protos.inc"
cat "$here/forklift/hook_protos.inc" >> "$work/protos.inc"
cp "$here"/forklift/*.h "$here/forklift/fkharness.cpp" "$work/"
g++ -std=c++11 -I"$work" -DSRC_MAIN='"m.inc"' -DSRC_COMM='"c.inc"' -DSRC_LOGIC='"l.inc"' \
    "$work/fkharness.cpp" -o "$work/fkharness"
"$work/fkharness"

# --- master (center board) ---
conv "$root/master_center/master_center.ino" "$work/main.inc"
conv "$root/master_center/master_hw.ino"     "$work/hw.inc"
conv "$root/master_center/master_logic.ino"  "$work/logic.inc"
cp "$here"/master/*.h "$here/master/harness.cpp" "$work/"
g++ -std=c++11 -I"$work" -DSRC_MAIN='"main.inc"' -DSRC_HW='"hw.inc"' -DSRC_LOGIC='"logic.inc"' \
    "$work/harness.cpp" -o "$work/harness"
"$work/harness"
