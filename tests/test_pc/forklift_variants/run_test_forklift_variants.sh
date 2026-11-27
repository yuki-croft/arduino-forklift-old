#!/usr/bin/env bash
# PC-only checks of final_forklift_board_center_line (A) and final_forklift_board_two_lines (B), 2026-11-28.
# 1) every tab joined like the Arduino IDE does + syntax check with fake headers, 2) same state-machine test as final_forklift_board,
# 3) detector rules on synthetic values in the user's ranges, 4) variant B steering. NOT an Arduino IDE compile, no board.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../../.."; fl="$here/../forklift_logic"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
conv() { sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$1" > "$2"; }
for v in center_line two_lines; do
  d="$root/final_forklift_board_$v"; w="$work/$v"; mkdir -p "$w"
  cp "$fl"/fake_arduino_headers/*.h "$d/forklift_line_pattern.h" "$w/"
  conv "$d/final_forklift_board_$v.ino" "$w/main.inc"
  others=$(ls "$d"/forklift_*.ino | sort)
  for f in $others; do conv "$f" "$w/$(basename "$f" .ino).inc"; done
  python3 "$fl/make_prototypes.py" "$w"/main.inc "$w"/forklift_*.inc > "$w/protos.inc"
  { echo '#include "Arduino.h"   // the Arduino IDE adds this automatically'; echo '#include <cstdlib>'; echo 'using std::abs;'
    awk '{print} /^#include "forklift_line_pattern.h"/ {exit}' "$w/main.inc"   # comments + includes
    cat "$w/protos.inc"                                                          # the IDE generates these
    awk 'f {print} /^#include "forklift_line_pattern.h"/ {f=1}' "$w/main.inc"
    for f in $others; do cat "$w/$(basename "$f" .ino).inc"; done
    echo 'HardwareSerial Serial;'
  } > "$w/all.cpp"
  g++ -std=c++11 -fsyntax-only -Werror=invalid-constexpr -I"$w" "$w/all.cpp"
  echo "$v: all tabs syntax OK"
  # state machine test (logic/comm tabs are the same as final_forklift_board; run it on this folder's copy)
  python3 "$fl/make_prototypes.py" "$w/forklift_comm.inc" "$w/forklift_logic.inc" > "$w/protos2.inc"
  cat "$fl/hook_protos.inc" >> "$w/protos2.inc"; cp "$w/protos2.inc" "$w/protos.inc"
  # variants (2026-11-28): reversing out of a branch ends when all three sensors are black (senseMarker), not senseJunction
  sed -E 's/pulse\(gJunction,3\); expect\("main line/pulse(gMarker,3); expect("main line/' "$fl/test_forklift_logic.cpp" > "$w/test_forklift_logic.cpp"
  grep -c 'pulse(gMarker,3); expect("main line' "$w/test_forklift_logic.cpp" | grep -qx 2
  g++ -std=c++11 -I"$w" -DSRC_MAIN='"main.inc"' -DSRC_COMM='"forklift_comm.inc"' -DSRC_LOGIC='"forklift_logic.inc"' "$w/test_forklift_logic.cpp" -o "$w/tl"
  "$w/tl" > "$w/tl.log" && echo "$v: state machine test passed ($(grep -c '^PASS' "$w/tl.log" || true) checks)" || { cat "$w/tl.log"; exit 1; }
done
cp "$root/final_forklift_board_center_line/forklift_line_pattern.h" "$work/pattern_a.h"
cp "$root/final_forklift_board_two_lines/forklift_line_pattern.h" "$work/pattern_b.h"
cp "$here/test_line_variants.cpp" "$here/test_lane_steer.cpp" "$work/"
g++ -std=c++11 -Wall -Wextra -I"$work" "$work/test_line_variants.cpp" -o "$work/ta" && "$work/ta"
g++ -std=c++11 -Wall -Wextra -DVARIANT_B=1 -I"$work" "$work/test_line_variants.cpp" -o "$work/tb" && "$work/tb"
awk '/^int laneLastNet/ {f=1} f {print} f && /^}/ {exit}' "$root/final_forklift_board_two_lines/forklift_hw.ino" > "$work/lane_steer.inc"
g++ -std=c++11 -Wall -Wextra -I"$work" "$work/test_lane_steer.cpp" -o "$work/ts" && "$work/ts"
