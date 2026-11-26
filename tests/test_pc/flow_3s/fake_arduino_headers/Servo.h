#pragma once
#include "Arduino.h"
struct PulseRec { unsigned long at; int pin; int us; };
extern std::vector<PulseRec> g_pulses;
struct Servo {
  int pin = -1; bool on = false;
  void attach(int p) { pin = p; on = true; }
  void detach() { on = false; g_pulses.push_back({g_ms, pin, -1}); }
  void writeMicroseconds(int us) { g_pulses.push_back({g_ms, pin, us}); }
};
