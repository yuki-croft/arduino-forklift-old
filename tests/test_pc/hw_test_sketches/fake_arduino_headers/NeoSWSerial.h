#pragma once
#include "Arduino.h"
class NeoSWSerial { public: NeoSWSerial(uint8_t, uint8_t); void begin(long); int available(); int read(); size_t write(uint8_t); };
