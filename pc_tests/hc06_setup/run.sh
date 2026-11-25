#!/usr/bin/env bash
# PC-only logic test of tool_hc06_forklift_setup with a fake HC-06 (NOT an Arduino IDE compile, no board).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
src="$root/tool_hc06_forklift_setup/tool_hc06_forklift_setup.ino"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
# the sketch must not print to the USB monitor (HC-06 is on the same port)
# (comments are removed first, so only real code is checked)
if perl -0pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' "$src" | grep -nE 'Serial\.(print|println|write)'; then echo "FAIL: sketch writes to Serial directly"; exit 1; fi
sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$src" > "$work/main.inc"
cp "$here"/*.h "$here/test_hc06_setup.cpp" "$work/"
g++ -std=c++11 -Wall -Wextra -I"$work" -DSRC_MAIN='"main.inc"' "$work/test_hc06_setup.cpp" -o "$work/test_hc06_setup"
"$work/test_hc06_setup"
# syntax check of the D2/D3 branch (HC06_ON_D0_D1 = 0)
sed 's/#define HC06_ON_D0_D1 1/#define HC06_ON_D0_D1 0/' "$work/main.inc" > "$work/main_soft.inc"
g++ -std=c++11 -Wall -Wextra -fsyntax-only -I"$work" -DSRC_MAIN='"main_soft.inc"' "$work/test_hc06_setup.cpp"
echo "D2/D3 branch: syntax OK"
