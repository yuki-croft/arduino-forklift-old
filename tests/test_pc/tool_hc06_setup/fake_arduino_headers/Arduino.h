// Minimal stand-in for the Arduino core + a fake HC-06, ONLY for the PC logic test of tool_bluetooth_hc06_forklift_setup (NOT the real API).
#pragma once
#include <stdint.h>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#define OUTPUT 1
unsigned long millis();                 // fake clock: +1 ms per call, delay() adds its argument
void delay(unsigned long ms);
void pinMode(uint8_t, uint8_t);
void tone(uint8_t pin, unsigned int hz, unsigned long ms);   // records the beep

struct FakeHc06 {                       // behaves like the HC-06 described by martyncurrey: uppercase, no line ending, replies without line ending
  long baud = 9600;                     // speed the module listens at
  bool silent = false;                  // never replies (e.g. paired/connected, or dead)
  bool deadAfterBaudChange = false;     // module stops answering after AT+BAUD (new speed not reachable)
  std::string failOn;                   // "NAME" / "PIN" / "BAUD" -> reply ERROR for that command
  std::string name = "HC-06", pin = "1234";
  std::vector<std::string> received;    // commands that arrived at the right speed
  bool badFormat = false;               // saw CR/LF or a lowercase letter in a command
  std::string rx;                       // bytes waiting for the sketch to read
};
struct Port {
  long hostBaud = 0;
  void begin(long b) { hostBaud = b; }
  int available();
  int read();
  void flush() {}
  size_t print(const char *s);
};
extern FakeHc06 hc06;
extern Port Serial;
struct Beep { unsigned int hz; unsigned long ms; };
extern std::vector<Beep> beeps;
