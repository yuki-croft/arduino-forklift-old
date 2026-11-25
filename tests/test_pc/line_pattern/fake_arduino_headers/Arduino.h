// Minimal stand-in for the Arduino core, ONLY for the PC test of the line pattern detector and forklift_sense.ino glue.
#pragma once
#include <stdint.h>
#include <cstddef>
typedef uint8_t byte;
#define A2 16
#define A3 17
#define A4 18
unsigned long millis();
int analogRead(uint8_t pin);
