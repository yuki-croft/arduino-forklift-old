#!/usr/bin/env bash
# PC test of tests/test_3s_flow/: forklift test sketch (fake clock/servos/sensors) + center single-file copy (fake I/O) + "center copy == final tabs" check.
# NOT an Arduino IDE compile, no board.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../../.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
cp "$here"/fake_arduino_headers/*.h "$here/test_forklift_3s.cpp" "$work/"
cp "$root/tests/test_3s_flow/test_final_forklift_3s.ino" "$work/forklift.inc"
g++ -std=c++11 -Wall -Wextra -I"$work" -DSRC_FORKLIFT='"forklift.inc"' "$work/test_forklift_3s.cpp" -o "$work/t_forklift"
"$work/t_forklift"
mkdir -p "$work/c"
cp "$here/../center_logic/fake_arduino_headers"/*.h "$work/c/"
cp "$here/test_center_with_forklift_3s.cpp" "$work/c/"
sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$root/tests/test_3s_flow/test_final_center_3s.ino" > "$work/c/center.inc"
g++ -std=c++11 -Wall -Wextra -I"$work/c" -DSRC_CENTER='"center.inc"' "$work/c/test_center_with_forklift_3s.cpp" -o "$work/t_center"
"$work/t_center"
# the single-file copy must contain the three final center tabs unchanged (comment header block of each tab excluded)
python3 -I - "$root" <<'PY'
import re, sys
root = sys.argv[1]
copy = open(root + "/tests/test_3s_flow/test_final_center_3s.ino", encoding="utf-8").read()
ok = True
for name in ("final_center_board.ino", "center_hw.ino", "center_logic.ino"):
    s = open(root + "/final_center_board/" + name, encoding="utf-8").read()
    s = re.sub(r"^/\*.*?\*/\n", "", s, count=1, flags=re.S)      # top block comment
    s = s[s.index("#include"):] if name == "final_center_board.ino" else s
    s = s.strip("\n")
    if s not in copy:
        print("FAIL  center copy differs from final_center_board/" + name); ok = False
    else:
        print("PASS  center copy contains final_center_board/" + name + " unchanged")
sys.exit(0 if ok else 1)
PY
