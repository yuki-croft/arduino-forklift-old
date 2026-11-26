// PC logic test of tools/tool_bluetooth_hc05_forklift_setup.ino with a fake HC-05 (NOT an Arduino compile, no board involved).
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "Arduino.h"
PortStub Serial;
static unsigned long fakeNow = 0;
unsigned long millis() { return ++fakeNow; }
// prototypes the Arduino IDE would generate automatically
void drainModule(unsigned long); bool waitReply(unsigned long); bool sendAt(const char*, unsigned long);
bool sendAtFmt(const char*, const char*); bool sendUartCmd(); bool pollUsbLine();
bool wakeAt(); bool parseAddress(char*, char*, size_t); void showAddress(); bool setupForkliftModule(); void printFailHint();
bool setupForkliftMaster(); void runForkliftMaster(); void pairAndLinkCenter(); void runForkliftSetup(); void showSettings(); void printMenu(); void handleLine(const char*); void setup(); void loop();
#include SRC_MAIN
static int fails = 0;
static void expect(const char *name, bool ok) { printf("%s  %s\n", ok ? "PASS" : "FAIL", name); if (!ok) fails++; }
static void reset() { Serial.in.clear(); Serial.out.clear(); btModule.in.clear(); btModule.pending.clear(); btModule.cmds.clear(); btModule.failOn.clear(); btModule.failTimes = 1000000; btModule.silent = false; lineLen = 0; }
static void typeLine(const std::string &s) { Serial.in += s + "\n"; for (int i = 0; i < 6; i++) loop(); }
static bool sent(const std::vector<std::string> &want) { return btModule.cmds == want; }
int main() {
  setup();
  expect("module starts at AT baud 38400", btModule.baud == 38400);
  expect("pins: Arduino RX = D2 (module TXD), Arduino TX = D3 (module RXD)", PIN_MODULE_TXD == 2 && PIN_MODULE_RXD == 3);
  reset(); typeLine("1");
  expect("1: AT, ROLE=0, UART 9600, NAME, PSWD, then ADDR? (in this order)",
         sent({"AT", "AT+ROLE=0", "AT+UART=9600,0,0", "AT+NAME=FORKLIFT_HC05", "AT+PSWD=1234", "AT+ADDR?"})
         && Serial.out.find("Forklift module configured") != std::string::npos);
  expect("1: address shown in colon form and comma (BIND) form",
         Serial.out.find("Forklift module address: 98d3:34:905d3f") != std::string::npos
         && Serial.out.find("menu 3): 98d3,34,905d3f") != std::string::npos);
  reset(); btModule.failOn = "AT+ROLE=0"; typeLine("1");
  expect("1: stops at the first failing command, reports FAILED + EN hint",
         sent({"AT", "AT+ROLE=0"}) && Serial.out.find("setup FAILED") != std::string::npos
         && Serial.out.find("EN to 3.3V (NOT 5V)") != std::string::npos);
  reset(); btModule.failOn = "AT"; btModule.failTimes = 1; typeLine("1");
  expect("1: first AT answers ERROR -> AT is sent once more, then setup continues",
         btModule.cmds.size() == 7 && btModule.cmds[0] == "AT" && btModule.cmds[1] == "AT" && btModule.cmds[2] == "AT+ROLE=0"
         && Serial.out.find("Forklift module configured") != std::string::npos);
  reset(); btModule.failOn = "AT"; typeLine("1");
  expect("1: AT fails twice -> FAILED after exactly two AT, nothing else sent",
         sent({"AT", "AT"}) && Serial.out.find("setup FAILED") != std::string::npos);
  reset(); typeLine("2");
  expect("2: only AT+ADDR? is sent and the address is shown",
         sent({"AT+ADDR?"}) && Serial.out.find("98d3,34,905d3f") != std::string::npos);
  reset(); btModule.failOn = "AT+ADDR?"; typeLine("2");
  expect("2: module error -> no address printed, hint shown",
         Serial.out.find("Forklift module address") == std::string::npos && Serial.out.find("No address") != std::string::npos);
  reset(); btModule.silent = true; typeLine("2");
  expect("2: silent module -> timeout, no hang", Serial.out.find("timeout") != std::string::npos);
  { char a[24], b[24];
    strcpy(replyBuf, "+ADDR:2:13:ABCDEF\r\nOK\r\n");
    expect("parse: short groups kept as the module prints them", parseAddress(a, b, 24) && !strcmp(a, "2:13:ABCDEF") && !strcmp(b, "2,13,ABCDEF"));
    strcpy(replyBuf, "+ADDR:98d3:34\r\nOK\r\n");
    expect("parse: 2 groups rejected", !parseAddress(a, b, 24));
    strcpy(replyBuf, "OK\r\n");
    expect("parse: no +ADDR: rejected", !parseAddress(a, b, 24));
    strcpy(replyBuf, "+ADDR:98d3:34:90zz3f\r\nOK\r\n");
    expect("parse: non-hex rejected", !parseAddress(a, b, 24)); }
  reset(); typeLine("3");
  expect("3: show settings queries", sent({"AT", "AT+VERSION?", "AT+ADDR?", "AT+ROLE?", "AT+UART?", "AT+NAME?", "AT+PSWD?", "AT+CMODE?", "AT+BIND?"}));
  reset(); typeLine("5");
  expect("5: master = AT, RMAAD, ROLE=1, CMODE=0, BIND=2025,05,0086A4, PSWD, UART, NAME (in this order)",
         sent({"AT", "AT+RMAAD", "AT+ROLE=1", "AT+CMODE=0", "AT+BIND=2025,05,0086A4", "AT+PSWD=1234", "AT+UART=9600,0,0", "AT+NAME=FORKLIFT_MASTER"})
         && Serial.out.find("configured as MASTER") != std::string::npos);
  reset(); btModule.failOn = "AT+BIND=2025,05,0086A4"; typeLine("5");
  expect("5: BIND failure stops and reports FAILED", btModule.cmds.size() == 5 && Serial.out.find("master setup FAILED") != std::string::npos);
  reset(); typeLine("6");
  expect("6: AT, INIT, PAIR=addr,9, LINK=addr then OK", sent({"AT", "AT+INIT", "AT+PAIR=2025,05,0086A4,9", "AT+LINK=2025,05,0086A4"}) && Serial.out.find("PAIR + LINK OK") != std::string::npos);
  reset(); btModule.failOn = "AT+INIT"; typeLine("6");
  expect("6: INIT error is ignored, PAIR and LINK still sent", sent({"AT", "AT+INIT", "AT+PAIR=2025,05,0086A4,9", "AT+LINK=2025,05,0086A4"}));
  reset(); btModule.failOn = "AT+PAIR=2025,05,0086A4,9"; typeLine("6");
  expect("6: PAIR failure -> no LINK, PAIR FAILED shown", sent({"AT", "AT+INIT", "AT+PAIR=2025,05,0086A4,9"}) && Serial.out.find("PAIR FAILED") != std::string::npos);
  reset(); typeLine("4"); expect("4: AT+RESET", sent({"AT+RESET"}));
  reset(); typeLine("AT+NAME?"); expect("raw AT line sent as typed", sent({"AT+NAME?"}));
  reset(); typeLine("9"); expect("unknown menu key", btModule.cmds.empty() && Serial.out.find("Unknown cmd") != std::string::npos);
  printf("%s\n", fails ? "SOME TESTS FAILED" : "all passed");
  return fails ? 1 : 0;
}
