// PC logic test: the real sketch text is included and run against a fake HC-06. Not an Arduino IDE compile, no board.
#include "Arduino.h"
#include <cstdlib>
#include <map>

FakeHc06 hc06;
Port Serial;
std::vector<Beep> beeps;
static unsigned long g_clock = 0;
unsigned long millis() { return g_clock++; }
void delay(unsigned long ms) { g_clock += ms; }
void pinMode(uint8_t, uint8_t) {}
void tone(uint8_t, unsigned int hz, unsigned long ms) { beeps.push_back({hz, ms}); }

int Port::available() { return (int)hc06.rx.size(); }
int Port::read() { int c = (unsigned char)hc06.rx[0]; hc06.rx.erase(0, 1); return c; }
size_t Port::print(const char *s) {
  std::string cmd(s);
  if (hostBaud != hc06.baud || hc06.silent) return cmd.size();   // wrong speed or no answer
  for (char c : cmd) if (c == '\r' || c == '\n' || (c >= 'a' && c <= 'z')) hc06.badFormat = true;
  hc06.received.push_back(cmd);
  static const std::map<char, long> rates = {{'1', 1200}, {'2', 2400}, {'3', 4800}, {'4', 9600}, {'5', 19200}, {'6', 38400}, {'7', 57600}, {'8', 115200}};
  if (cmd == "AT") hc06.rx += "OK";
  else if (cmd.rfind("AT+NAME", 0) == 0) { if (hc06.failOn == "NAME") hc06.rx += "ERROR"; else { hc06.name = cmd.substr(7); hc06.rx += "OKsetname"; } }
  else if (cmd.rfind("AT+PIN", 0) == 0) { if (hc06.failOn == "PIN") hc06.rx += "ERROR"; else { hc06.pin = cmd.substr(6); hc06.rx += "OKsetPIN"; } }
  else if (cmd.rfind("AT+BAUD", 0) == 0 && cmd.size() == 8) {
    if (hc06.failOn == "BAUD" || !rates.count(cmd[7])) hc06.rx += "ERROR";
    else { long r = rates.at(cmd[7]); hc06.rx += "OK" + std::to_string(r); hc06.baud = r; if (hc06.deadAfterBaudChange) hc06.silent = true; }
  }
  return cmd.size();
}

#include SRC_MAIN

static int fails = 0, checks = 0;
static void expect(const char *what, bool ok) { checks++; if (!ok) fails++; printf("%s  %s\n", ok ? "PASS" : "FAIL", what); }
static void reset() { hc06 = FakeHc06(); Serial = Port(); beeps.clear(); g_clock = 0; resultCode = 255; }
static int countHz(unsigned int hz) { int n = 0; for (auto &b : beeps) if (b.hz == hz) n++; return n; }
static bool eq(const std::vector<std::string> &a, std::vector<std::string> b) { return a == b; }

int main() {
  reset(); setup();
  expect("default 9600 module: success code 0", resultCode == 0);
  expect("default 9600 module: AT, NAME, PIN only (no baud command)", eq(hc06.received, {"AT", "AT+NAMEFORKLIFT", "AT+PIN1234"}));
  expect("name and PIN stored in the module", hc06.name == "FORKLIFT" && hc06.pin == "1234");
  expect("start beep is the only sound during setup", beeps.size() == 1 && beeps[0].hz == 2000);
  beeps.clear(); loop();
  expect("success sound = one long 3000Hz beep", beeps.size() == 1 && beeps[0].hz == 3000 && beeps[0].ms == 1000);
  expect("commands: uppercase, no CR/LF", !hc06.badFormat);

  reset(); hc06.baud = 38400; setup();
  expect("module at 38400: found, success", resultCode == 0);
  expect("module at 38400: AT, NAME, PIN, AT+BAUD4, AT", eq(hc06.received, {"AT", "AT+NAMEFORKLIFT", "AT+PIN1234", "AT+BAUD4", "AT"}));
  expect("module ends at 9600 and sketch follows", hc06.baud == 9600 && Serial.hostBaud == 9600);

  reset(); hc06.baud = 115200; setup();
  expect("module at 115200: found after other speeds, success", resultCode == 0 && hc06.baud == 9600);

  reset(); hc06.silent = true; setup(); beeps.clear(); loop();
  expect("no answer at any speed -> code 1, one low beep", resultCode == FAIL_NO_AT && beeps.size() == 1 && beeps[0].hz == 800);

  reset(); hc06.failOn = "NAME"; setup(); beeps.clear(); loop();
  expect("NAME error -> code 2, two low beeps", resultCode == FAIL_NAME && countHz(800) == 2 && countHz(3000) == 0);

  reset(); hc06.failOn = "PIN"; setup(); beeps.clear(); loop();
  expect("PIN error -> code 3, three low beeps", resultCode == FAIL_PIN && countHz(800) == 3);

  reset(); hc06.baud = 38400; hc06.failOn = "BAUD"; setup(); beeps.clear(); loop();
  expect("BAUD error -> code 4, four low beeps", resultCode == FAIL_BAUD && countHz(800) == 4);

  reset(); hc06.baud = 38400; hc06.deadAfterBaudChange = true; setup();
  expect("no answer after speed change -> code 4", resultCode == FAIL_BAUD);

  reset(); hc06.baud = 38400; hc06.failOn = "NAME"; setup();
  expect("failure before BAUD: baud command is not sent", hc06.received.size() == 2);

  expect("name length <= 20 (martyncurrey)", strlen(HC06_NAME) <= 20 && strlen(HC06_NAME) > 0);
  bool pinOk = strlen(HC06_PIN) == 4; for (const char *p = HC06_PIN; *p; p++) pinOk = pinOk && *p >= '0' && *p <= '9';
  expect("PIN is 4 digits", pinOk);
  expect("target speed 9600 matches code 4", TARGET_BAUD == 9600 && strcmp(TARGET_BAUD_CMD, "AT+BAUD4") == 0);

  printf("%d checks, %d failed\n", checks, fails);
  if (fails) return 1;
  puts("all passed");
  return 0;
}
