#!/usr/bin/env bash
# Runs every PC-side test of the project and prints one summary line per suite.
# Needs: bash, g++, python3, javac/java. These tests do NOT use the Arduino toolchain or any board.
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
declare -a names=() results=()

run_suite() {   # run_suite "<name>" <command...>
  local name="$1"; shift
  if "$@" > "/tmp/pc_test_$$.log" 2>&1; then results+=("PASS"); else results+=("FAIL"); cat "/tmp/pc_test_$$.log"; fi
  names+=("$name")
}

run_suite "final_forklift_board + final_center_board logic" bash "$here/run_test_firmware_logic.sh"
run_suite "final_phone_app_android logic (unit tests)"      bash "$here/app_logic/run_test_app_logic.sh"
run_suite "app vs real center-board output"                 bash "$here/app_vs_center/run_test_app_vs_center.sh"
run_suite "hardware test sketches (structure + stub syntax check)" python3 -I "$here/hw_test_sketches/check_test_sketches.py"
run_suite "tool_bluetooth_hc05_setup (fake HC-05)"           bash "$here/tool_hc05_setup/run_test_hc05_setup.sh"
run_suite "tool_bluetooth_hc06_forklift_setup (fake HC-06)"  bash "$here/tool_hc06_setup/run_test_hc06_setup.sh"
run_suite "tool_bluetooth_hc05_forklift_setup (fake HC-05)"  bash "$here/tool_hc05_forklift_setup/run_test_hc05_forklift_setup.sh"
run_suite "tool_bluetooth_hc05_factory_reset (fake HC-05)"   bash "$here/tool_hc05_factory_reset/run_test_hc05_factory_reset.sh"
run_suite "map layout geometry"                             bash -c "cd '$root/map' && python3 -m unittest test_layout"
run_suite "line pattern detector (synthetic traces)"        bash "$here/line_pattern/run_test_line_pattern.sh"
run_suite "final map plan (map/map_final)"                  bash "$here/map_final/run_test_map_final.sh"
run_suite "3 s forward test pair (tests/test_3s_flow)"      bash "$here/flow_3s/run_test_flow_3s.sh"

rm -f "/tmp/pc_test_$$.log"
status=0
for i in "${!names[@]}"; do
  printf '%-4s %s\n' "${results[$i]}" "${names[$i]}"
  [ "${results[$i]}" = "PASS" ] || status=1
done
exit $status
