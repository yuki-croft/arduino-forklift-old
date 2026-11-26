#pragma once
#include "Arduino.h"
class SoftwareSerial : public Stream { public: SoftwareSerial(uint8_t, uint8_t); void begin(long); int available() override; int read() override; size_t write(uint8_t) override; };
