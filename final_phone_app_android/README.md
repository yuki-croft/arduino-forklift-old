# Forklift Controller (Android)

Phone app for the Arduino forklift: connect to the center board's Bluetooth module, pick one load cell
(`1`-`6`) and one unload cell (`A`-`F`), press **Start**, and watch the messages the center sends back
(`SEL LOAD 3`, `JOB 3>B`, `ST TO LOAD`, `EMERGENCY`, ...). Protocol: `../PROBLEM_1PAGER.md`.

## Status (honest)
- **Never built or run on a phone.** The authoring environment had no Android SDK.
- Checked on a PC only: the pure-Java logic under `logic/` (26 unit tests, `pc_tests/app_logic/run.sh` on the `dev` / `test-pc-logic` branches) and a
  cross check against the real `final_center_board` firmware output (`pc_tests/app_vs_center/run.sh`, same branches).
- The Android classes (`MainActivity`, `BluetoothLink`, `DevicePicker`, `CellPanel`, XML resources) were only
  compiled against **hand-written stubs** of the Android API, so real-API mistakes are still possible.
- Gradle/AGP versions (`build.gradle`: AGP 8.5.2, `gradle-wrapper.properties`: Gradle 8.7) are unverified.
  If Android Studio offers an upgrade, accept it.

## Folder layout (flat on purpose)
```
AndroidManifest.xml
build.gradle, settings.gradle, gradle.properties, gradlew*, gradle/wrapper/   Gradle (single module, app = root project)
src/     Android screens: MainActivity, BluetoothLink, DevicePicker, CellPanel
logic/   pure Java, no Android classes: CommandChars, LineAssembler, CenterMessage, ControllerState
res/     layouts, strings, colors
```
The Java files keep their package names (`com.forklift.controller`, `.logic`) but are not nested in matching folders;
`build.gradle` points the source sets at `src` and `logic`. Android Studio may show a harmless "package does not match
the expected package" hint. This layout was never built (see Status).

## Open in Android Studio
1. File > Open > this `final_phone_app_android` folder, let Gradle sync (needs JDK 17+, which Android Studio bundles).
2. On the phone: Settings > Bluetooth > pair the phone-side HC-05 module first (PIN = the one set with `AT+PSWD`, see `../tool_hc05_center_setup`; the default PIN is not verified).
3. Run the app configuration (module `ForkliftController`) on the phone, press **Connect**, pick the module.
4. Unit tests are not in this folder: they live on the `dev` / `test-pc-logic` branches (`pc_tests/app_logic/`) and run on a PC with `javac`.

## First check on real hardware (suggested)
With the center board running `final_center_board` and the phone module wired as in the pin map:
tap `3`, then `B` -> the message list must show `SEL LOAD 3` and `SEL UNLOAD B`, and both cells turn blue.
Tap `3` again after a finished job -> `EMPTY 3`. If nothing comes back, check baud rate (9600), the TX/RX wiring
of the phone module, and that the phone-side HC-05 is set to slave (`AT+ROLE=0`) and not paired to something else.

## Things to know
- The Start character is `S` (`CommandChars.START`); it must match `START_CHAR` in `final_center_board.ino`.
- There is no emergency or cancel button: the protocol has none. The emergency banner appears when the
  forklift reports a rear obstacle and the center sends `EMERGENCY`.
- A cell is highlighted only after the center confirms it; the app never guesses occupancy.
- Package name `com.forklift.controller`, minSdk 26, targetSdk 34 (all changeable in `build.gradle`).
- No AndroidX or other runtime libraries are used.
