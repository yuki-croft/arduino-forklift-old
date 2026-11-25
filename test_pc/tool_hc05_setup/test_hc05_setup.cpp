// PC logic test of tool_bluetooth_hc05_setup.ino with a fake HC-05 (NOT an Arduino compile, no board involved).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "Arduino.h"
PortStub Serial;
static unsigned long fakeNow = 0;
unsigned long millis() { return ++fakeNow; }
// prototypes the Arduino IDE would generate automatically
bool waitReply(unsigned long); void drainModule(unsigned long); bool sendAt(const char*, unsigned long);
bool sendAtFmt(const char*, const char*); bool sendUartCmd(); bool pollUsbLine(); bool readLineWait(unsigned long);
bool normalizeAddress(const char*, char*, size_t); bool setupPhoneModule(); bool scanForForklift(); bool bindToForklift(const char*);
void runPhoneSetup(); void printFailHint(); void askAddressAndBind(); void showSettings(); void printMenu(); void handleLine(const char*); void setup(); void loop();
#include SRC_MAIN
static int fails = 0;
static void expect(const char *name, bool ok) { printf("%s  %s\n", ok ? "PASS" : "FAIL", name); if (!ok) fails++; }
static void reset() { Serial.in.clear(); Serial.out.clear(); btModule.in.clear(); btModule.pending.clear(); btModule.cmds.clear(); btModule.failOn.clear(); btModule.silent = false; lineLen = 0; }
static void typeLine(const std::string &s) { Serial.in += s + "\n"; for (int i = 0; i < 6; i++) loop(); }
static bool sent(const std::vector<std::string> &want) { return btModule.cmds == want; }
int main() {
  setup();
  expect("module starts at AT baud 38400", btModule.baud == 38400);
  expect("menu printed at start", Serial.out.find("HC-05 setup") != std::string::npos);
  reset(); typeLine("1");
  expect("1: module A = AT, ROLE=0, UART 9600, NAME",
         sent({"AT", "AT+ROLE=0", "AT+UART=9600,0,0", "AT+NAME=FORKLIFT_PHONE"}) && Serial.out.find("Module A configured") != std::string::npos);
  reset(); btModule.failOn = "AT+ROLE=0"; typeLine("1");
  expect("1: stops at the first failing command and reports FAILED",
         sent({"AT", "AT+ROLE=0"}) && Serial.out.find("Module A setup FAILED") != std::string::npos);
  expect("failure prints the button hint", Serial.out.find("keep the on-board button pressed") != std::string::npos);
  reset(); typeLine("2");
  expect("2: scan = AT, RMAAD, ROLE=1, INIT, INQ and the +INQ line is shown",
         sent({"AT", "AT+RMAAD", "AT+ROLE=1", "AT+INIT", "AT+INQ"}) && Serial.out.find("+INQ:98D3:34:905D3F") != std::string::npos);
  reset(); btModule.failOn = "AT+INIT"; typeLine("2");
  expect("2: INIT error is ignored, scan still runs", btModule.cmds.size() == 5 && btModule.cmds[4] == "AT+INQ");
  reset(); Serial.in = "3\n98d3:34:905d3f\n"; for (int i = 0; i < 10; i++) loop();
  expect("3: colon address -> comma BIND, ROLE=1, CMODE=0, UART, NAME (in this order)",
         sent({"AT", "AT+ROLE=1", "AT+CMODE=0", "AT+BIND=98d3,34,905d3f", "AT+UART=9600,0,0", "AT+NAME=FORKLIFT_LINK"})
         && Serial.out.find("Module B configured") != std::string::npos);
  reset(); Serial.in = "3\n98D3,34,905D3F\n"; for (int i = 0; i < 10; i++) loop();
  expect("3: comma address accepted as typed", btModule.cmds.size() == 6 && btModule.cmds[3] == "AT+BIND=98D3,34,905D3F");
  reset(); Serial.in = "3\nnot-an-address\n"; for (int i = 0; i < 10; i++) loop();
  expect("3: bad address -> nothing sent to module", btModule.cmds.empty() && Serial.out.find("Bad address") != std::string::npos);
  reset(); Serial.in = "3\n98d3:34\n"; for (int i = 0; i < 10; i++) loop();
  expect("3: address with only 2 groups rejected", btModule.cmds.empty());
  reset(); Serial.in = "3\n:34:905d3f\n"; for (int i = 0; i < 10; i++) loop();
  expect("3: empty first group rejected", btModule.cmds.empty());
  reset(); typeLine("4");
  expect("4: show settings queries", sent({"AT", "AT+VERSION?", "AT+ADDR?", "AT+ROLE?", "AT+UART?", "AT+NAME?"}));
  reset(); typeLine("5"); expect("5: AT+RESET", sent({"AT+RESET"}));
  reset(); typeLine("AT+PSWD=1234");
  expect("raw AT line is sent as typed, CR+LF added by the sketch", sent({"AT+PSWD=1234"}));
  reset(); Serial.in = "AT+NAME?\r\n"; for (int i = 0; i < 6; i++) loop();
  expect("Windows line ending (CR+LF) from the monitor does not send an extra empty command", sent({"AT+NAME?"}));
  reset(); typeLine("9"); expect("unknown menu key", btModule.cmds.empty() && Serial.out.find("Unknown cmd") != std::string::npos);
  reset(); btModule.silent = true; typeLine("AT");
  expect("silent module -> timeout message, no hang", Serial.out.find("timeout") != std::string::npos);
  reset(); typeLine("at");
  expect("lowercase 'at' is not sent", btModule.cmds.empty());
  printf("%s\n", fails ? "SOME TESTS FAILED" : "all passed");
  return fails ? 1 : 0;
}
