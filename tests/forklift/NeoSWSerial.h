#pragma once
#include "Arduino.h"
struct NeoSWSerial { std::string in, out; NeoSWSerial(uint8_t,uint8_t){} void begin(long){}
  int available(){ return (int)in.size(); } int read(){ int c=(unsigned char)in[0]; in.erase(0,1); return c; }
  size_t write(uint8_t c){ out.push_back((char)c); return 1; } };
