#!/usr/bin/env bash
# Runs every PC-side test suite (each pc_tests/<suite>/run.sh) and prints one summary line per suite.
# Needs: bash, g++, python3, javac/java. These tests do NOT use the Arduino toolchain or any board.
# Suites whose code is not on this branch (e.g. test_forklift_* sketches) are reported as SKIP.
set -uo pipefail
here="$(cd "$(dirname "$0")" && pwd)"
declare -a names=() results=()
status=0
for script in "$here"/*/run.sh; do
  suite="$(basename "$(dirname "$script")")"
  if [ "$suite" = "hw_sketch_check" ] && ! ls -d "$here"/../test_forklift_*/ >/dev/null 2>&1; then
    results+=("SKIP"); names+=("$suite (no test_forklift_* folders on this branch)"); continue
  fi
  if bash "$script" > "/tmp/pc_test_$$.log" 2>&1; then results+=("PASS"); else results+=("FAIL"); cat "/tmp/pc_test_$$.log"; status=1; fi
  names+=("$suite")
done
rm -f "/tmp/pc_test_$$.log"
for i in "${!names[@]}"; do printf '%-4s %s\n' "${results[$i]}" "${names[$i]}"; done
exit $status
