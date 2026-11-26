// Minimal stand-in for the Arduino core, ONLY for the PC test of test_final_forklift_3s.ino (NOT the real API).
#pragma once
#include <stdint.h>
#include <cstddef>
#include <string>
#include <vector>
#define OUTPUT 1
#define A2 16
#define A3 17
#define A4 18
extern unsigned long g_ms;                 // fake clock: every millis() call advances 1 ms so busy loops end
inline unsigned long millis() { return g_ms++; }
inline void delay(unsigned long ms) { g_ms += ms; }
inline void pinMode(uint8_t, uint8_t) {}
int analogRead(uint8_t pin);               // provided by the test
struct BeepRec { unsigned long at; unsigned int hz, ms; };
extern std::vector<BeepRec> g_beeps;
inline void tone(uint8_t, unsigned int hz, unsigned long ms) { g_beeps.push_back({g_ms, hz, (unsigned int)ms}); }
struct HardwareSerial {
  std::string in, out;
  void begin(long) {}
  int available() { return (int)in.size(); }
  int read() { int c = (unsigned char)in[0]; in.erase(0, 1); return c; }
  size_t write(uint8_t c) { out.push_back((char)c); return 1; }
};
extern HardwareSerial Serial;
