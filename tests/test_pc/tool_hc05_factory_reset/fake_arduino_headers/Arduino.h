// Minimal stand-in for the Arduino core, ONLY for the PC logic test of tool_bluetooth_hc05_factory_reset (NOT the real API).
#pragma once
#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#define OUTPUT 1
unsigned long millis();
void delay(unsigned long ms);
void pinMode(uint8_t, uint8_t);
void tone(uint8_t pin, unsigned int hz, unsigned long ms);
struct Beep { uint8_t pin; unsigned int hz; unsigned long ms; };
extern std::vector<Beep> beeps;
