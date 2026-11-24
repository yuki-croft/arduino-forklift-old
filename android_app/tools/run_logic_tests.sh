#!/usr/bin/env bash
# Compiles the pure-Java logic (no Android classes) and its tests with javac, runs them with the shim.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; app="$here/../app/src"
out="$(mktemp -d)"; trap 'rm -rf "$out"' EXIT
javac -Xlint:all -d "$out" "$here"/junit_shim/org/junit/*.java "$here/junit_shim/RunTests.java" \
  "$app"/main/java/com/forklift/controller/logic/*.java "$app"/test/java/com/forklift/controller/logic/*.java
java -cp "$out" RunTests \
  com.forklift.controller.logic.LineAssemblerTest com.forklift.controller.logic.CommandCharsTest \
  com.forklift.controller.logic.CenterMessageTest com.forklift.controller.logic.ControllerStateTest
