// PC logic test of tools/tool_bluetooth_hc05_factory_reset.ino with a fake HC-05 (NOT an Arduino compile, no board involved).
#include <cstdio>
#include "Arduino.h"
#include "SoftwareSerial.h"
static unsigned long fakeNow = 0;
unsigned long millis() { return ++fakeNow; }
void delay(unsigned long ms) { fakeNow += ms; }
void pinMode(uint8_t, uint8_t) {}
std::vector<Beep> beeps;
void tone(uint8_t pin, unsigned int hz, unsigned long ms) { beeps.push_back({pin, hz, ms}); }
// prototypes the Arduino IDE would generate automatically
bool waitOk(unsigned long); bool sendAt(const char*); bool wakeAt(); uint8_t runFactoryReset();
void beep(unsigned int, unsigned long); void playResult(uint8_t); void setup(); void loop();
#include SRC_MAIN
static int fails = 0;
static void expect(const char *name, bool ok) { printf("%s  %s\n", ok ? "PASS" : "FAIL", name); if (!ok) fails++; }
static void reset() { btModule = SoftwareSerial(2, 3); beeps.clear(); result = RESET_OK; }
static bool sent(const std::vector<std::string> &w) { return btModule.cmds == w; }
static bool beepsAre(unsigned int hz, size_t n) { if (beeps.size() != n) return false; for (auto &b : beeps) if (b.hz != hz || b.pin != PIN_BUZZER) return false; return true; }
int main() {
  reset(); setup(); loop();
  expect("module at AT baud 38400, buzzer D5 on the forklift setting", btModule.baud == 38400 && PIN_BUZZER == 5);
  expect("normal: AT, ORGL, AT, UART=9600,0,0 (in this order)", sent({"AT", "AT+ORGL", "AT", "AT+UART=9600,0,0"}));
  expect("normal: result OK -> one long high beep", result == RESET_OK && beepsAre(OK_HZ, 1) && beeps[0].ms == 1000);
  reset(); btModule.atErrorsFirst = 2; setup(); loop();
  expect("first two AT answer ERROR:(0) -> third AT works, reset continues", result == RESET_OK && btModule.cmds.size() >= 4 && btModule.cmds[3] == "AT+ORGL");
  reset(); btModule.silent = true; setup(); loop();
  expect("silent module -> 3 AT tries, nothing else, 1 low beep", sent({"AT", "AT", "AT"}) && result == FAIL_NO_AT && beepsAre(FAIL_HZ, 1));
  reset(); btModule.failOn = "AT+ORGL"; setup(); loop();
  expect("ORGL refused -> stops, 2 low beeps", sent({"AT", "AT+ORGL"}) && result == FAIL_ORGL && beepsAre(FAIL_HZ, 2));
  reset(); btModule.failOn = "AT+UART=9600,0,0"; setup(); loop();
  expect("UART refused -> 3 low beeps", result == FAIL_UART && beepsAre(FAIL_HZ, 3));
  reset(); setup(); loop(); loop();
  expect("result repeats every loop", beepsAre(OK_HZ, 2));
  printf("%s\n", fails ? "SOME TESTS FAILED" : "all passed");
  return fails ? 1 : 0;
}
