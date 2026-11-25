# Arduino forklift (FRIBEE EDU, UNO R3 compatible)

Phone app -> center board (two HC-05: one set as phone-side slave, one as master) -> forklift (HC-06 slave).
Design document and open items: [PROBLEM_1PAGER.md](PROBLEM_1PAGER.md) (Korean).

## Branches (브랜치)
| Branch | Contents |
|---|---|
| `main` | **Finished code only**: `final_*` (upload/install), `tool_*` (one-time setup), `map_final/` (final map), docs |
| `test-hardware-sketches` | `main` + the 8 `test_forklift_*` board test sketches (+ their PC structure check) |
| `test-pc-logic` | `main` + `pc_tests/` (PC logic tests with fake sensors) |
| `map-resources` | `main` + `map_resources/` (earlier map / cut-plan proposals, kept for reference) |
| `dev` | the three branches above merged: everything, so `pc_tests/run_all.sh` can run all suites |

Flow: new or experimental work on a topic branch -> `dev` -> `main` once it is final.

## Folders (names tell the role; every code folder sits directly under the repo root)
| Folder | On branch | Role | Goes to | Notes |
|---|---|---|---|---|
| `final_forklift_board/` | main | **FINAL** | forklift UNO (FRIBEE EDU) | state machine + Bluetooth protocol (HC-06 on D0/D1 hardware serial), 5 .ino tabs (`final_forklift_board.ino` + `forklift_*.ino`) |
| `final_center_board/` | main | **FINAL** | center UNO | phone commands, LEDs, forwarding to the forklift, 3 .ino tabs (`final_center_board.ino` + `center_*.ino`), wiring picture |
| `final_phone_app_android/` | main | **FINAL** | Android phone | controller app (Java, Android Studio project, flat layout), **never built or run**, see its README |
| `tool_hc05_center_setup/` | main | TOOL (one-time) | any UNO + ONE HC-05 at a time | menu-driven AT setup of the two center-board HC-05 modules (A = phone slave, B = forklift-link master bound to the HC-06); wiring picture |
| `tool_hc06_forklift_setup/` | main | TOOL (one-time) | forklift board (HC-06 soldered) | no serial monitor: finds the HC-06 speed, sets name + PIN (+ 9600), reports by buzzer (long beep = OK, N short low beeps = failed step N) |
| `tool_at_bridge/` | main | TOOL (one-time) | any UNO + HC-05 / HC-06 | plain AT-command pass-through bridge (any command typed by hand) |
| `map_final/` | main | design (final) | none | final map: 150x150 pinwheel base, 100x100 line loop. Pictures `map_final_*.png`, plan `base150_*`, redraw with `python3 map_final/make_base150.py` (needs matplotlib) |
| `test_forklift_*/` | test-hardware-sketches, dev | TEST (on a board) | forklift UNO, USB serial monitor | one stand-alone sketch per part (table below); not the final program |
| `pc_tests/` | test-pc-logic, dev | TEST (on a PC) | none | one sub-folder per suite with its own `run.sh`; run all: `bash pc_tests/run_all.sh` (needs g++, python3, javac) |
| `map_resources/` | map-resources, dev | reference | none | earlier proposals: `old_layout/` (180x150 map) and `old_cutplans/` (ring / full floor cut plans) |

Naming rule: `final_*` = upload/install this; `tool_*` = one-time helper; `test_*` = checks only, never the final program.
Arduino IDE: open the `.ino` whose name equals its folder (for example `final_forklift_board/final_forklift_board.ino`); the other tabs load automatically.

### Hardware test sketches (`test_forklift_*/`, branch `test-hardware-sketches` or `dev`)
Open **one folder at a time** (each is its own sketch = **one `.ino` file**: the upper part has pins/constants/hardware functions without Serial, the lower part (after the `SERIAL TEST SECTION` banner) has the serial-monitor menu + `setup()`/`loop()`). Serial monitor 9600 bps.
| Sketch | Tests | Pins |
|---|---|---|
| `test_forklift_buzzer` | passive buzzer beep | D5 |
| `test_forklift_ultrasonic` | rear HC-SR04 distance (cm) | D6 trig, D7 echo |
| `test_forklift_infrared` | IR left/front/right (38 kHz) | D4 tx, D8 / D9 / D13 rx |
| `test_forklift_wheels` | two drive servos, 6 moves (lift wheels off the ground, press `y`) | D10 left, D11 right |
| `test_forklift_lift` | lift servo, step up/down (`u` / `d` / `q`), direction inverted after the hardware test | D12 |
| `test_forklift_photo` | photo-transistor raw/V | A0, A1 |
| `test_forklift_line` | line sensor raw values + line-steer values, wheels not moved | A2, A3, A4 |
| `test_forklift_bluetooth` | HC-06 receive for 15 s; **still assumes D2/D3 + USB monitor, does not match the soldered HC-06 on D0/D1** | D2 rx, D3 tx |

## Status (honest)
- Verified on a PC only (fake sensors/time and stub Arduino headers, not the Arduino toolchain): all `pc_tests/` suites passed on 2026-11-26 on the `dev` branch.
- **Not verified:** Arduino IDE compile, upload, any run of the final code on real boards. The hardware sketches were run on the forklift by the user (all fine except the lift direction, now inverted).
- **Not implemented:** `senseJunction`, `senseCorner`, `senseMarker` in `final_forklift_board/forklift_sense.ino` always return false until sensor measurements are available (open item 20 in the 1-pager). The forklift cannot reach a cell until they are done.
- Unmeasured values are `TODO` constants (lift heights, turn time, rear-stop and light-tracking values).

## Libraries (install in Arduino IDE)
Servo (built in), SoftwareSerial (built in), Ultrasonic by Erick Simoes (forklift), NeoSWSerial by SlashDevin (only `test_forklift_bluetooth`), LiquidCrystal_I2C (fallback display only).
Code adapted from the course PPT material is marked `EXTERNAL` in comments.
