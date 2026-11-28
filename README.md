# Arduino forklift (FRIBEE EDU, UNO R3 compatible)

Phone app -> center board (two HC-05: one set as phone-side slave, one as master) -> forklift (HC-06 slave).
Design document and open items: [PROBLEM_1PAGER.md](PROBLEM_1PAGER.md) (Korean).

## Which code goes where (names tell the role)
Test code is under `tests/`, one-time tools under `tools/`. Single-file sketches are plain `.ino` files, not wrapped in a folder of the same name.
| Path | Role | Goes to | Notes |
|---|---|---|---|
| `final_forklift_board/` | **FINAL** | forklift UNO (FRIBEE EDU) | state machine + Bluetooth protocol (HC-06 on D0/D1 hardware serial), 5 .ino tabs (`final_forklift_board.ino` + `forklift_*.ino`) + `forklift_line_pattern.h` |
| `final_forklift_board_center_line/` | FINAL (variant A, 2026-11-28) | forklift | same as `final_forklift_board/` but the line rules use the user's ranges (white 25-29, black 30-34): center sensor rides the single black line; branch/corner/marker threshold 30 (the old 40 could never fire); spin stops when the center reads 30 or more. Right event = corner, left branch = load/unload on both side segments, lift heights from cm (racks 5 / 12 cm; needs LIFT_STROKE_CM). PC-checked only |
| `final_forklift_board_two_lines/` | FINAL (variant B, 2026-11-28) | forklift | for a map with two parallel black lines: left sensor on the left line, center on white between, right sensor on the right line. Steering from the left-right difference; branch = one line broken, corner = right line broken then the outer line ahead, marker = all three black; spin stops when left black + center white + right black. Line gap about 19 mm (user). Right-line break = corner (user), left-line break = branch to load/unload; marker rule and steering direction are assumptions; lift heights from cm (racks 5 / 12 cm; needs LIFT_STROKE_CM). PC-checked only |
| `final_forklift_board_white_lines/` | FINAL (variant C, 2026-11-28) | forklift | variant B with the colours swapped: two white lines on a black floor (left and right sensors on the white lines, center on the black floor between). `lineRead()` mirrors each line-sensor value (59 - raw), so the B rules run unchanged. Assumes the white tape / black floor read in the same ranges as foam / black tape. PC-checked only |
| `final_center_board/` | **FINAL** | center UNO | phone commands, LEDs, forwarding to the forklift, 3 .ino tabs (`final_center_board.ino` + `center_*.ino`), wiring picture |
| `final_phone_app_android/` | **FINAL** | Android phone | controller app (Java, Android Studio project), **never built or run**, see its README |
| `tools/` | TOOL (one-time) | see rows below | `tool_bluetooth_hc05_setup.ino` (menu-driven AT setup of the two center-board HC-05 modules; menu 6 binds module B to the forklift module the user reported (name Yuki, address 98:DA:60:01:B1:60, PIN 1234 -> `AT+BIND=98DA,60,01B160` + `AT+PSWD=1234`); menu 3 also accepts a 6-byte MAC; menu 7 sets module B as slave for the swapped-role test; PC-tested with a fake module only, not on a real HC-05; wiring picture `wiring_hc05_setup.png`), `tool_bluetooth_hc06_forklift_setup.ino` (forklift board, HC-06 soldered: no serial monitor, reports by buzzer: long beep = OK, N short low beeps = failed step N), `tool_bluetooth_hc05_forklift_setup.ino` (forklift side HC-05 on D2 (module TXD) / D3 (module RXD): slave setup + shows its address, also in the comma form for the center BIND; for a board without button/KEY it tells to tie EN to 3.3V before power-up (forum report); menu 5 makes it MASTER bound to center module B 2025,05,0086A4, menu 6 sends INIT/PAIR/LINK, for the swapped-role test), `tool_bluetooth_hc05_factory_reset.ino` (HC-05 factory reset with no serial monitor: runs on upload with the module on D2/D3 and EN to 3.3V; AT, AT+ORGL, then AT+UART=9600,0,0; buzzer D5 (forklift) or D13 (center): 1 s high = OK, 1/2/3 short low = no AT / ORGL refused / UART refused; PC-tested with a fake module only), `tool_forklift_lift_adjust.ino` (move the forklift lift from the Arduino IDE serial monitor over USB, Bluetooth module unplugged from D0/D1: u/d 0.1 cm, U/D 1 cm, 1/2/3/4 = 5.2 / 12.2 / 6.2 / 12.5 cm, b = bottom, z = set zero, p = show; height estimated by time, 4.5-12.5 cm over 8 s; syntax-checked only), `tool_forklift_line_values.ino` (prints A2/A3/A4 once per second on the serial monitor, 1 s average, B/W by 30, min/max every 10 s; syntax-checked only), `tool_bluetooth_at_setup.ino` (plain AT pass-through bridge) |
| `tests/test_forklift_board_hardware/` | TEST (on a board) | forklift UNO, USB serial monitor | one stand-alone `.ino` per part (table below; 12 files); not the final program |
| `tests/test_center_board_hardware/` | TEST (on a board) | center UNO, no USB monitor | `test_center_link.ino`: link module B on D0/D1 both ways with the center buzzer on D13 only (pair with `test_forklift_link.ino`); not yet run on a real board |
| `tests/test_3s_flow/` | TEST (on boards) | forklift UNO + center UNO | pair for a flow check before the map: `test_final_forklift_3s.ino` (3 s forward, line sensors only recognize, then load = fork up, unload = fork down, same Bluetooth letters as final) and `test_final_center_3s.ino` (single-file copy of the final center code) |
| `tests/test_pc/` | TEST (on a PC) | none (runs on this computer) | fake-sensor logic tests: `bash tests/test_pc/run_all_pc_tests.sh` (needs g++, python3, javac) |
| `map/` | design | none | `map/` = first layout proposal (images + generator `python3 map/make_map.py`, needs matplotlib); `map/map_final/` = final map (150x150 pinwheel base, 100x100 line loop; redraw with `python3 map/map_final/make_base150.py`); `map/old_cutplans/` = earlier cut plans |

Naming rule: `final_*` = upload/install this; `test_*` = checks only, never the final program; `tool_*` = one-time helper.
Arduino IDE: the IDE wants every `.ino` inside a folder of the same name. Multi-tab sketches (`final_*`) already are (open `final_forklift_board/final_forklift_board.ino`; the other tabs load automatically). For a single-file `.ino` in `tools/` or `tests/`, the IDE offers to create that folder when you open the file.

### Hardware test sketches (`tests/test_forklift_board_hardware/`)
Open **one file at a time** (each is its own sketch = **one `.ino` file**: the upper part has pins/constants/hardware functions without Serial, the lower part (after the `SERIAL TEST SECTION` banner) has the serial-monitor menu + `setup()`/`loop()`). Serial monitor 9600 bps.
| Sketch | Tests | Pins |
|---|---|---|
| `test_forklift_buzzer` | passive buzzer beep | D5 |
| `test_forklift_ultrasonic` | rear HC-SR04 distance (cm) | D6 trig, D7 echo |
| `test_forklift_infrared` | IR left/front/right (38 kHz) | D4 tx, D8 / D9 / D13 rx |
| `test_forklift_wheels` | two drive servos, 6 moves (lift wheels off the ground, press `y`) | D10 left, D11 right |
| `test_forklift_shuffle` | drives forward/backward in place by itself (no serial monitor; buzzer countdown, 5 cycles), for a check before placing on the map | D10 left, D11 right, D5 buzzer |
| `test_forklift_link` | Bluetooth link both ways with the buzzer only (no USB monitor): beeps once at boot, sends `w` every 2 s (phone should show `ST WAIT`), one short beep per byte received from the center; wheels and lift do not move; not yet run on a real board | D0/D1 link module, D5 buzzer |
| `test_forklift_all_in_place` | in-place check of everything except line sensors and wheels, started from the phone (pick a load and an unload cell, then Start): beep + red LED D0, lift up/down 2 s (1300/1700 us), IR / ultrasonic / photo 5 s each with a beep on detection; each stage shows on the phone (ST TO LOAD, LIFT UP, UNLOAD, APPROACH, REVERSE, RETURN, DONE); Bluetooth NeoSWSerial D2/D3; not yet run on a real board | D2/D3 Bluetooth, D0 red LED, D5 buzzer, D12 lift, D4/D8/D9/D13 IR, D6/D7 ultrasonic, A0/A1 photo |
| `test_forklift_line_bt` | streams A2/A3/A4 as `ms,L,C,R` lines to a phone Bluetooth terminal (no USB monitor); phone sends p = print only (push by hand), d = slow drive with the final steering rule + print, s = stop, m = min/max, r = reset; not yet run on a real board | D0/D1 link module, A2-A4 line, D10/D11 wheels, D5 buzzer |
| `test_forklift_lift` | lift servo, step up/down (`u` / `d` / `q`), direction inverted after the hardware test | D12 |
| `test_forklift_photo` | photo-transistor raw/V | A0, A1 |
| `test_forklift_line` | line sensor raw values + line-steer values, wheels not moved | A2, A3, A4 |
| `test_forklift_bluetooth` | HC-06 receive for 15 s; **still assumes D2/D3 + USB monitor, does not match the soldered HC-06 on D0/D1** | D2 rx, D3 tx |

## Status (honest)
- The junction / corner / marker detection (`final_forklift_board/forklift_line_pattern.h`) is a temporary rule built on assumed sensor values, tested only on synthetic traces (`tests/test_pc/line_pattern/`). Measured so far: black line 31 or more (max about 35), white floor about 24-29; real A2/A3/A4 recordings at the straight line, T-junction and start marker are still missing.
- Verified on a PC only (fake sensors/time and stub Arduino headers, not the Arduino toolchain): `bash tests/test_pc/run_all_pc_tests.sh` passed (11 suites) on 2026-11-26.
- **Not verified:** Arduino IDE compile, upload, any run of the final code on real boards. The hardware sketches were run on the forklift by the user (all fine except the lift direction, now inverted).
- **Not implemented for real:** `senseJunction`, `senseCorner`, `senseMarker` in `final_forklift_board/forklift_sense.ino` use the temporary rule above, and its reference value (40) is above the measured black maximum, so on the real sensor it would detect nothing until measurements replace it (open item 20 in the 1-pager).
- Unmeasured values are `TODO` constants (lift heights, turn time, rear-stop and light-tracking values).

## Libraries (install in Arduino IDE)
Servo (built in), SoftwareSerial (built in), Ultrasonic by Erick Simoes (forklift), NeoSWSerial by SlashDevin (only `test_forklift_bluetooth`), LiquidCrystal_I2C (fallback display only).
Code adapted from the course PPT material is marked `EXTERNAL` in comments.
