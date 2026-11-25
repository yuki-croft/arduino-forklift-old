#pragma once
#include <stdint.h>
#include <cstddef>
#include <string>
#define HIGH 1
#define LOW 0
#define OUTPUT 1
class __FlashStringHelper;
#define F(s) (reinterpret_cast<const __FlashStringHelper*>(s))
void pinMode(uint8_t,uint8_t); void digitalWrite(uint8_t,uint8_t);
struct PortStub {            // stands in for HardwareSerial / SoftwareSerial in the PC test only
  std::string in, out;
  void begin(long){}
  int available(){ return (int)in.size(); }
  int read(){ int c=(unsigned char)in[0]; in.erase(0,1); return c; }
  size_t write(uint8_t c){ out.push_back((char)c); return 1; }
  size_t print(const __FlashStringHelper* s){ out += (const char*)s; return 1; }
};
extern PortStub Serial;
