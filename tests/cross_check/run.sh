#!/usr/bin/env bash
# Runs a scripted scenario through the REAL master_center firmware (PC harness, fake I/O), captures what it
# sends to the phone, and feeds that text through the app's pure-Java logic. Needs g++ and javac.
set -euo pipefail
here="$(cd "$(dirname "$0")" && pwd)"; root="$here/../.."
work="$(mktemp -d)"; trap 'rm -rf "$work"' EXIT
conv() { sed -E 's/#include <([A-Za-z]+\.h)>.*/#include "\1"/' "$1" > "$2"; }
conv "$root/master_center/master_center.ino" "$work/main.inc"
conv "$root/master_center/master_hw.ino"     "$work/hw.inc"
conv "$root/master_center/master_logic.ino"  "$work/logic.inc"
cp "$root"/tests/master/*.h "$work/"
head -n 22 "$root/tests/master/harness.cpp" > "$work/xh.cpp"   # includes + run() helper of the master harness
cat >> "$work/xh.cpp" <<'CPP'
int main() {
  setup(); phone.out.clear(); Serial.out.clear();
  std::string all;
  auto grab = [&]() { all += phone.out; phone.out.clear(); };
  run("3B", ""); grab(); run("S", ""); grab();
  run("", "1"); run("", "l"); grab(); run("", "2"); run("", "a"); run("", "u"); run("", "b"); run("", "m"); grab();
  run("1", ""); grab();                                  // JOB RUNNING
  run("", "n"); grab(); run("", "d"); run("", "r"); run("", "c"); grab();
  run("3", ""); grab(); run("B", ""); grab();            // EMPTY 3, BUSY B
  run("1A", ""); run("S", ""); grab(); run("", "3"); run("", "e"); grab(); run("", "w"); grab();
  fputs(all.c_str(), stdout); return 0;
}
CPP
g++ -std=c++11 -I"$work" -DSRC_MAIN='"main.inc"' -DSRC_HW='"hw.inc"' -DSRC_LOGIC='"logic.inc"' "$work/xh.cpp" -o "$work/xh"
"$work/xh" > "$work/phone_out.txt"
mkdir "$work/j"
javac -d "$work/j" "$root"/android_app/app/src/main/java/com/forklift/controller/logic/*.java "$here/CrossCheck.java"
java -cp "$work/j" CrossCheck "$work/phone_out.txt"
