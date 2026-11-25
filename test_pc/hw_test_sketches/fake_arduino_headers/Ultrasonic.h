#pragma once
#include "Arduino.h"
class Ultrasonic { public: Ultrasonic(uint8_t, uint8_t); long read(); };
