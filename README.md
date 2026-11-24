# Arduino forklift (FRIBEE EDU, UNO R3 compatible)

Phone app -> center board (HC-05 master + phone-side slave module) -> forklift (HC-06 slave).
Design document and open items: [PROBLEM_1PAGER.md](PROBLEM_1PAGER.md) (Korean).

| Folder | Board | Purpose |
|---|---|---|
| `forklift_main/` | forklift UNO | state machine + Bluetooth protocol (5 .ino tabs) |
| `master_center/` | center UNO | phone commands, LEDs, forwarding to the forklift (3 tabs) |
| `forklift_test/` | forklift UNO | serial-monitor hardware test menu (kept separate from the main sketch) |
| `bt_at_setup/` | any UNO | AT-command bridge for configuring HC-05 / HC-06 |
| `android_app/` | Android phone | controller app (Java, Android Studio project) - **never built or run**, see its README |
| `tests/` | PC | fake-sensor logic tests (`tests/run_tests.sh`, needs g++ and python3; `tests/cross_check/run.sh` also needs javac) |

## Status (honest)
- Verified: `tests/run_tests.sh` (PC, fake sensors/time, not the Arduino toolchain). Forklift 35 checks and center board 30 checks passed on 2026-11-25.
- **Not verified:** Arduino IDE compile, upload, any run on real boards.
- **Not implemented:** `senseJunction`, `senseCorner`, `senseMarker` in `forklift_main/fk_sense.ino` always return false until sensor measurements are available (open item 20 in the 1-pager). The forklift cannot reach a cell until they are done.
- Unmeasured values are `TODO` constants (lift heights, turn time, rear-stop and light-tracking values).

## Libraries (install in Arduino IDE)
Servo (built in), SoftwareSerial (built in), Ultrasonic by Erick Simoes, NeoSWSerial by SlashDevin, LiquidCrystal_I2C (fallback display only).
Code adapted from the course PPT material is marked `EXTERNAL` in comments.
