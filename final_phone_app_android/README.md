# Forklift Controller (Android)

Phone app for the Arduino forklift: connect to the center board's Bluetooth module, pick one load cell
(`1`-`6`) and one unload cell (`A`-`F`), press **Start**, and watch the messages the center sends back
(`SEL LOAD 3`, `JOB 3>B`, `ST TO LOAD`, `EMERGENCY`, ...). Protocol: `../PROBLEM_1PAGER.md`.

## Status (honest)
- **Never built or run on a phone.** The authoring environment had no Android SDK.
- Checked on a PC only: the pure-Java logic under `logic/` (26 unit tests via `tools/run_test_app_logic.sh`) and a
  cross check against the real `final_center_board` firmware output (`../test_pc/app_vs_center/run_test_app_vs_center.sh`).
- The Android classes (`MainActivity`, `BluetoothLink`, `DevicePicker`, `CellPanel`, XML resources) were only
  compiled against **hand-written stubs** of the Android API, so real-API mistakes are still possible.
- Gradle/AGP versions (`build.gradle`: AGP 8.5.2, `gradle-wrapper.properties`: Gradle 8.7) are unverified.
  If Android Studio offers an upgrade, accept it.

## Open in Android Studio
1. File > Open > this `final_phone_app_android` folder, let Gradle sync (needs JDK 17+, which Android Studio bundles).
2. On the phone: Settings > Bluetooth > pair the HC-06 module first (default PIN is usually 1234 or 0000).
3. Run the `app` configuration on the phone, press **Connect**, pick the module.
4. Unit tests: right-click `app/src/test` > Run (real JUnit 4).

## First check on real hardware (suggested)
With the center board running `final_center_board` and the phone module wired as in the pin map:
tap `3`, then `B` -> the message list must show `SEL LOAD 3` and `SEL UNLOAD B`, and both cells turn blue.
Tap `3` again after a finished job -> `EMPTY 3`. If nothing comes back, check baud rate (9600), the TX/RX wiring
of the phone module, and that the module is HC-06 (slave) and not paired to something else.

## Things to know
- The Start character is `S` (`CommandChars.START`); it must match `START_CHAR` in `final_center_board.ino`.
- There is no emergency or cancel button: the protocol has none. The emergency banner appears when the
  forklift reports a rear obstacle and the center sends `EMERGENCY`.
- A cell is highlighted only after the center confirms it; the app never guesses occupancy.
- Package name `com.forklift.controller`, minSdk 26, targetSdk 34 (all changeable in `app/build.gradle`).
- No AndroidX or other runtime libraries are used.
