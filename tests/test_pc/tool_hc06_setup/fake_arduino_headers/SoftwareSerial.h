// Only for the PC syntax check of the HC06_ON_D0_D1 = 0 branch.
#pragma once
#include "Arduino.h"
struct SoftwareSerial : Port { SoftwareSerial(uint8_t, uint8_t) {} };
