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
  sed 's/void turnRight();/void turnRight(); void reverseTurnRight(); void pivotTurnLeft();/' "$fl/hook_protos.inc" >> "$w/protos2.inc"; cp "$w/protos2.inc" "$w/protos.inc"
  # variants (2026-11-28): reversing out of a branch ends when all three sensors are black (senseMarker), not senseJunction
  sed -E 's/pulse\(gJunction,3\); expect\("main line/pulse(gMarker,3); expect("main line/' "$fl/test_forklift_logic.cpp" > "$w/test_forklift_logic.cpp"
  grep -c 'pulse(gMarker,3); expect("main line' "$w/test_forklift_logic.cpp" | grep -qx 2
  # variants (2026-11-28): slower wheels, reverse uses -LINE_BASE_SPEED (60)
  sed -i 's|expect("reverses straight", had("wheels -100 -100"));|expect("reverses straight at the slow speed", had("wheels -60 -60"));|' "$w/test_forklift_logic.cpp"
  grep -q 'had("wheels -60 -60")' "$w/test_forklift_logic.cpp"
  # variants: the turn after reversing is the backward pivot reverseTurnRight (fake logs its own name)
  sed -i -e 's|^void turnRight(){ ev.push_back("turnRight"); }|&\nvoid reverseTurnRight(){ ev.push_back("reverseTurnRight"); }\nvoid pivotTurnLeft(){ ev.push_back("pivotTurnLeft"); }|' \
         -e 's|expect("main line reached: turn right, TO_UNLOAD|expect("main line reached: backward right pivot, TO_UNLOAD|' \
         -e 's|backward right pivot, TO_UNLOAD .m., target = 2", had("turnRight")|&|' "$w/test_forklift_logic.cpp"
  python3 - "$w/test_forklift_logic.cpp" <<'PY'
import sys
p=sys.argv[1]; s=open(p).read()
a='expect("main line reached: backward right pivot, TO_UNLOAD \'m\', target = 2", had("turnRight")'
assert a in s, "pattern"
s=s.replace(a, a.replace('had("turnRight")','had("reverseTurnRight") && !had("turnRight")'))
s=s.replace('expect("main line again: RETURN \'r\'", state==S_RETURN', 'expect("main line again: backward pivot, RETURN \'r\'", had("reverseTurnRight") && state==S_RETURN')
assert 'void reverseTurnRight(){' in s and 'void pivotTurnLeft(){' in s
a2='expect("target junction reached: turn left, state APPROACH, report \'a\'", had("turnLeft")'
assert a2 in s, "pattern2"
s=s.replace(a2,'expect("target junction reached: pivot left (axis turn), state APPROACH, report \'a\'", had("pivotTurnLeft") && !had("turnLeft")')
open(p,'w').write(s)
PY
  # the state-machine test steps 10 ms at a time and expects an instant lift: run it with the lift off (LIFT_STROKE_CM = 0);
  # the real lift heights are checked separately below
  sed -i -E 's/^constexpr float LIFT_STROKE_CM( +)= [0-9.]+;/constexpr float LIFT_STROKE_CM\1= 0.0;/' "$w/main.inc"
  grep -q '^constexpr float LIFT_STROKE_CM *= 0.0;' "$w/main.inc"
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

# lift heights from the user's numbers (2026-11-28): fork 4.5 cm .. 12.5 cm over 8 s, racks 5 / 12 cm, "slightly" = 0.2 cm, pick 1 cm
for v in center_line two_lines; do
  f="$root/final_forklift_board_$v/final_forklift_board_$v.ino"
  { echo '#include <cstdio>'
    grep -E '^constexpr unsigned long LIFT_FULL_STROKE_MS' "$f"
    awk '/^constexpr float LIFT_STROKE_CM/ {p=1} p && /^const unsigned long LIFT_CARRY_MS/ {exit} p {print}' "$f"
    cat <<'CPP'
static int fails = 0;
static void expect(const char *n, bool ok) { printf("%s  %s\n", ok ? "PASS" : "FAIL", n); if (!ok) fails++; }
int main() {
  expect("lower rack: enter at 5.2 cm = 700 ms", LIFT_LOWER_MS == 700);
  expect("upper rack: enter at 12.2 cm = 7700 ms (rounded, not 7699)", LIFT_UPPER_MS == 7700);
  expect("pick 1 cm = 1000 ms (lower rack lifts to 6.2 cm = 1700 ms)", LIFT_PICK_MS == 1000 && LIFT_LOWER_MS + LIFT_PICK_MS == 1700);
  expect("upper rack pick is clipped at the top: 8000 - 7700 = 300 ms = 0.3 cm", LIFT_FULL_STROKE_MS - LIFT_UPPER_MS == 300);
  expect("drop on lower rack at 4.8 cm = 300 ms", LIFT_LOWER_DROP_MS == 300);
  expect("drop on upper rack at 11.8 cm = 7300 ms", LIFT_UPPER_DROP_MS == 7300);
  return fails ? 1 : 0;
}
CPP
  } > "$work/lift_$v.cpp"
  g++ -std=c++11 -Wall "$work/lift_$v.cpp" -o "$work/lift_$v" && "$work/lift_$v" | sed "s/^/$v: /"
done
