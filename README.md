# Arduino forklift (FRIBEE EDU, UNO R3 compatible)

Phone app -> center board (HC-05 master + phone-side slave module) -> forklift (HC-06 slave).
Design document and open items: [PROBLEM_1PAGER.md](PROBLEM_1PAGER.md) (Korean).

## Which code goes where (names tell the role)
| Folder | Role | Goes to | Notes |
|---|---|---|---|
| `final_forklift_board/` | **FINAL** | forklift UNO (FRIBEE EDU) | state machine + Bluetooth protocol, 5 .ino tabs (`final_forklift_board.ino` + `forklift_*.ino`) |
| `final_center_board/` | **FINAL** | center UNO | phone commands, LEDs, forwarding to the forklift, 3 .ino tabs (`final_center_board.ino` + `center_*.ino`) |
| `final_phone_app_android/` | **FINAL** | Android phone | controller app (Java, Android Studio project), **never built or run**, see its README |
| `test_forklift_board_hardware/` | TEST (on a board) | forklift UNO, USB serial monitor | per-sensor / per-actuator checks; not the final program |
| `tool_bluetooth_at_setup/` | TOOL (one-time) | any UNO + HC-05 / HC-06 | AT-command bridge to configure the modules |
| `test_pc/` | TEST (on a PC) | none (runs on this computer) | fake-sensor logic tests: `test_pc/run_all_pc_tests.sh` (needs g++, python3, javac) |
| `map/` | design | none | map layout proposal: images + generator (`python3 map/make_map.py`, needs matplotlib) |

Naming rule: `final_*` = upload/install this; `test_*` = checks only, never the final program; `tool_*` = one-time helper.
Arduino IDE: open the `.ino` whose name equals its folder (for example `final_forklift_board/final_forklift_board.ino`); the other tabs load automatically.

## Status (honest)
- Verified: `test_pc/run_all_pc_tests.sh` (PC, fake sensors/time, not the Arduino toolchain). Forklift 35 checks and center board 30 checks passed on 2026-11-25.
- **Not verified:** Arduino IDE compile, upload, any run on real boards.
- **Not implemented:** `senseJunction`, `senseCorner`, `senseMarker` in `final_forklift_board/forklift_sense.ino` always return false until sensor measurements are available (open item 20 in the 1-pager). The forklift cannot reach a cell until they are done.
- Unmeasured values are `TODO` constants (lift heights, turn time, rear-stop and light-tracking values).

## Libraries (install in Arduino IDE)
Servo (built in), SoftwareSerial (built in), Ultrasonic by Erick Simoes, NeoSWSerial by SlashDevin, LiquidCrystal_I2C (fallback display only).
Code adapted from the course PPT material is marked `EXTERNAL` in comments.
