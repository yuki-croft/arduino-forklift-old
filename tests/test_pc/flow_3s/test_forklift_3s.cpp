// PC test of tests/test_3s_flow/test_final_forklift_3s.ino with a fake clock, fake servos and fake line sensors.
#include <cstdio>
#include <string>
#include <vector>
#include "Arduino.h"
#include "Servo.h"
unsigned long g_ms = 0;
std::vector<BeepRec> g_beeps;
std::vector<PulseRec> g_pulses;
HardwareSerial Serial;
static int g_line[3] = {26, 40, 26};   // A2, A3, A4
int analogRead(uint8_t pin) { return g_line[pin - A2]; }
void setup(); void loop();
#include SRC_FORKLIFT
static int checks = 0, fails = 0;
static void expect(const char *what, bool ok) { checks++; if (!ok) fails++; printf("%s  %s\n", ok ? "PASS" : "FAIL", what); }
// first/last time a pulse value was written on a pin; returns duration of "us on pin" until the next write on that pin
static long held(int pin, int us) {
  for (size_t i = 0; i < g_pulses.size(); i++)
    if (g_pulses[i].pin == pin && g_pulses[i].us == us)
      for (size_t j = i + 1; j < g_pulses.size(); j++)
        if (g_pulses[j].pin == pin) return (long)(g_pulses[j].at - g_pulses[i].at);
  return -1;
}
static void reset() { g_ms = 0; g_beeps.clear(); g_pulses.clear(); Serial.in.clear(); Serial.out.clear(); loadChar = unloadChar = -1; }
int main() {
  // A: the line is seen during the forward move (center sensor on black)
  reset(); g_line[0] = 26; g_line[1] = 33; g_line[2] = 26;
  setup();
  expect("setup reports 'w' once", Serial.out == "w");
  Serial.in = "3"; loop(); expect("load char alone does not start", Serial.out == "w" && g_pulses.size() <= 4);
  Serial.in = "X"; loop(); expect("'X' is ignored", Serial.out == "w");
  Serial.in = "B"; loop();
  expect("report order 1 l a u m d c w", Serial.out == "w1laumdcw");
  long fwdL = held(10, 1500 + 100), fwdR = held(11, 1500 - 100);
  expect("left wheel pulse 1600 held about 3000 ms", fwdL >= 2990 && fwdL <= 3100);
  expect("right wheel pulse 1400 held about 3000 ms", fwdR >= 2990 && fwdR <= 3100);
  expect("lift up pulse 1300 (inverted) held about 2000 ms", held(12, 1300) == 2000);
  expect("lift down pulse 1700 held about 2000 ms", held(12, 1700) == 2000);
  bool stopped = false; for (size_t i = g_pulses.size(); i-- > 0;) if (g_pulses[i].pin == 10) { stopped = g_pulses[i].us == 1500; break; }
  expect("wheels end at the stop pulse 1500", stopped);
  expect("line seen: exactly one long beep (600 ms)", g_beeps.size() == 1 && g_beeps[0].ms == 600);
  expect("job state cleared (no second run)", loadChar == -1 && unloadChar == -1);
  // B: line never seen, floor all white -> 'n' once + two short beeps
  reset(); g_line[0] = g_line[1] = g_line[2] = 26;
  Serial.in = "1A"; loop();
  expect("white floor: 'n' reported once, between 'l' and 'a'", Serial.out == "1lnaumdcw");
  expect("line not seen: two short beeps (150 ms)", g_beeps.size() == 2 && g_beeps[0].ms == 150 && g_beeps[1].ms == 150);
  // C: values 30 are neither white (<= 29) nor black (>= 31): no 'n', and the line is not counted as seen
  reset(); g_line[0] = g_line[1] = g_line[2] = 30;
  Serial.in = "1A"; loop();
  expect("values 30: no 'n', line not seen (two short beeps)", Serial.out == "1laumdcw" && g_beeps.size() == 2);
  printf("%d checks, %d failed\n", checks, fails);
  return fails ? 1 : 0;
}
