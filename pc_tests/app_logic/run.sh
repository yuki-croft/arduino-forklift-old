#!/usr/bin/env bash
# Compiles the app's pure-Java logic (final_phone_app_android/logic, no Android classes) and its tests with javac,
# runs them with the JUnit stand-in (Assert.java, Test.java, RunTests.java in this folder).
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; logic="$here/../../final_phone_app_android/logic"
out="$(mktemp -d)"; trap 'rm -rf "$out"' EXIT
javac -Xlint:all -d "$out" "$here"/*.java "$logic"/*.java
java -cp "$out" RunTests \
  com.forklift.controller.logic.LineAssemblerTest com.forklift.controller.logic.CommandCharsTest \
  com.forklift.controller.logic.CenterMessageTest com.forklift.controller.logic.ControllerStateTest
