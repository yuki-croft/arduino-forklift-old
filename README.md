# Arduino forklift (FRIBEE EDU, UNO R3 compatible)

Phone app -> center board (two HC-05: one set as phone-side slave, one as master) -> forklift (HC-06 slave).
Design document and open items: [PROBLEM_1PAGER.md](PROBLEM_1PAGER.md) (Korean).

## Which code goes where (names tell the role)
| Folder | Role | Goes to | Notes |
|---|---|---|---|
| `final_forklift_board/` | **FINAL** | forklift UNO (FRIBEE EDU) | state machine + Bluetooth protocol, 5 .ino tabs (`final_forklift_board.ino` + `forklift_*.ino`) |
| `final_center_board/` | **FINAL** | center UNO | phone commands, LEDs, forwarding to the forklift, 3 .ino tabs (`final_center_board.ino` + `center_*.ino`) |
| `final_phone_app_android/` | **FINAL** | Android phone | controller app (Java, Android Studio project), **never built or run**, see its README |
| `test_forklift_board_hardware/` | TEST (on a board) | forklift UNO, USB serial monitor | one stand-alone sketch per part, each in its own sub-folder (see below); not the final program |
| `tool_bluetooth_hc05_setup/` | TOOL (one-time) | any UNO + ONE HC-05 at a time | menu-driven AT setup of the two center-board HC-05 modules (A = phone slave, B = forklift-link master bound to the HC-06); wiring picture `wiring_hc05_setup.png` |
| `tool_bluetooth_hc06_forklift_setup/` | TOOL (one-time) | FORKLIFT board (HC-06 soldered) | no serial monitor: finds the HC-06 speed, sets name + PIN (+ 9600), reports by buzzer (long beep = OK, N short low beeps = failed step N) |
| `tool_bluetooth_at_setup/` | TOOL (one-time) | any UNO + HC-05 / HC-06 | plain AT-command pass-through bridge (any command typed by hand) |
| `test_pc/` | TEST (on a PC) | none (runs on this computer) | fake-sensor logic tests: `test_pc/run_all_pc_tests.sh` (needs g++, python3, javac) |
| `map/` | design | none | map layout proposal: images + generator (`python3 map/make_map.py`, needs matplotlib) |

Naming rule: `final_*` = upload/install this; `test_*` = checks only, never the final program; `tool_*` = one-time helper.
Arduino IDE: open the `.ino` whose name equals its folder (for example `final_forklift_board/final_forklift_board.ino`); the other tabs load automatically.

### Hardware test sketches (`test_forklift_board_hardware/`)
Open **one sub-folder at a time** (each is its own sketch = **one `.ino` file**: the upper part has pins/constants/hardware functions without Serial, the lower part (after the `SERIAL TEST SECTION` banner) has the serial-monitor menu + `setup()`/`loop()`). Serial monitor 9600 bps.
| Sketch | Tests | Pins |
|---|---|---|
| `test_forklift_buzzer` | passive buzzer beep | D5 |
| `test_forklift_ultrasonic` | rear HC-SR04 distance (cm) | D6 trig, D7 echo |
| `test_forklift_infrared` | IR left/front/right (38 kHz) | D4 tx, D8 / D9 / D13 rx |
| `test_forklift_wheels` | two drive servos, 6 moves (lift wheels off the ground, press `y`) | D10 left, D11 right |
| `test_forklift_lift` | lift servo, step up/down (`u` / `d` / `q`) | D12 |
| `test_forklift_photo` | photo-transistor raw/V (extra, split from the old combined menu) | A0, A1 |
| `test_forklift_line` | line sensor raw values + line-steer values, wheels not moved (extra) | A2, A3, A4 |
| `test_forklift_bluetooth` | HC-06 receive for 15 s (extra) | D2 rx, D3 tx |

## Status (honest)
- Verified: `test_pc/run_all_pc_tests.sh` (PC, fake sensors/time and stub Arduino headers, not the Arduino toolchain; includes a structure/syntax check of the 8 hardware test sketches). Forklift 35 checks and center board 30 checks passed on 2026-11-25.
- **Not verified:** Arduino IDE compile, upload, any run on real boards.
- **Not implemented:** `senseJunction`, `senseCorner`, `senseMarker` in `final_forklift_board/forklift_sense.ino` always return false until sensor measurements are available (open item 20 in the 1-pager). The forklift cannot reach a cell until they are done.
- Unmeasured values are `TODO` constants (lift heights, turn time, rear-stop and light-tracking values).

## Libraries (install in Arduino IDE)
Servo (built in), SoftwareSerial (built in), Ultrasonic by Erick Simoes, NeoSWSerial by SlashDevin, LiquidCrystal_I2C (fallback display only).
Code adapted from the course PPT material is marked `EXTERNAL` in comments.
