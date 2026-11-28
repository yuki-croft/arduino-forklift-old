#pragma once
#include "Arduino.h"
class Servo { public: uint8_t attach(int); void detach(); bool attached(); void writeMicroseconds(int); };
