// Minimal stand-in for the Arduino core, ONLY for the PC logic test of tool_hc05_center_setup (NOT the real API).
#pragma once
#include <stdint.h>
#include <cstddef>
#include <cstdio>
#include <string>
#define F(s) (reinterpret_cast<const __FlashStringHelper*>(s))
class __FlashStringHelper;
unsigned long millis();            // fake clock: +1 ms per call, so every wait loop ends
struct PortStub {                  // USB Serial: in = what the "user" typed, out = what the sketch printed
  std::string in, out;
  void begin(long){}
  int available(){ return (int)in.size(); }
  int read(){ int c=(unsigned char)in[0]; in.erase(0,1); return c; }
  size_t write(uint8_t c){ out.push_back((char)c); return 1; }
  size_t print(const char*s){ out+=s; return 1; }
  size_t print(const __FlashStringHelper*s){ out+=(const char*)s; return 1; }
  size_t println(const char*s){ out+=s; out+="\n"; return 1; }
  size_t println(const __FlashStringHelper*s){ out+=(const char*)s; out+="\n"; return 1; }
};
extern PortStub Serial;
