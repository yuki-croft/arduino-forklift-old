#!/usr/bin/env bash
# PC-only logic test of tool_bluetooth_hc05_setup with a fake HC-05 (NOT an Arduino IDE compile, no board).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$root/tool_bluetooth_hc05_setup/tool_bluetooth_hc05_setup.ino" > "$work/main.inc"
cp "$here"/fake_arduino_headers/*.h "$here/test_hc05_setup.cpp" "$work/"
g++ -std=c++11 -Wall -Wextra -I"$work" -DSRC_MAIN='"main.inc"' "$work/test_hc05_setup.cpp" -o "$work/test_hc05_setup"
"$work/test_hc05_setup"
