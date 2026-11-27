#!/usr/bin/env bash
# PC-only logic test of tool_bluetooth_hc05_factory_reset with a fake HC-05 (NOT an Arduino IDE compile, no board).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../../.."
src="$root/tools/tool_bluetooth_hc05_factory_reset.ino"
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
# no serial monitor: the sketch must not use Serial at all (comments removed first)
if perl -0pe 's{/\*.*?\*/}{}gs; s{//[^\n]*}{}g' "$src" | grep -nE '\bSerial\.'; then echo "FAIL: sketch uses Serial"; exit 1; fi
sed -E 's/#include <(SoftwareSerial\.h)>.*/#include "\1"/' "$src" > "$work/main.inc"
cp "$here"/fake_arduino_headers/*.h "$here/test_hc05_factory_reset.cpp" "$work/"
g++ -std=c++11 -Wall -Wextra -I"$work" -DSRC_MAIN='"main.inc"' "$work/test_hc05_factory_reset.cpp" -o "$work/t"
"$work/t"
# center setting (buzzer D13) also compiles
sed 's/FORKLIFT_BOARD      = true/FORKLIFT_BOARD      = false/' "$work/main.inc" > "$work/main_center.inc"
g++ -std=c++11 -Wall -Wextra -fsyntax-only -I"$work" -DSRC_MAIN='"main_center.inc"' "$work/test_hc05_factory_reset.cpp"
echo "center setting: syntax OK"
