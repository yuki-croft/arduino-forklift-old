#pragma once
#include "Arduino.h"
#include <vector>
// Fake HC-05 on the module side. Every line the sketch sends (ending "\r\n") is recorded in cmds and answered.
struct SoftwareSerial {
  std::string in, pending;
  std::vector<std::string> cmds;
  std::string failOn;               // command that gets "ERROR:(17)"
  int failTimes = 1000000;          // how many times failOn fails before it answers OK again
  bool silent = false;              // true = module never answers
  long baud = 0;
  SoftwareSerial(uint8_t, uint8_t) {}
  void begin(long b){ baud = b; }
  int available(){ return (int)in.size(); }
  int read(){ int c=(unsigned char)in[0]; in.erase(0,1); return c; }
  size_t print(const char*s){ return feed(s); }
  size_t print(const __FlashStringHelper*s){ return feed((const char*)s); }
  size_t feed(const char*s){
    pending += s;
    size_t p;
    while ((p = pending.find("\r\n")) != std::string::npos) {
      std::string cmd = pending.substr(0, p); pending.erase(0, p + 2); cmds.push_back(cmd);
      if (silent) continue;
      if (cmd == failOn && failTimes-- > 0) in += "ERROR:(17)\r\n";
      else if (cmd == "AT+INQ") in += "+INQ:98D3:34:905D3F,1F00,7FFF\r\nOK\r\n";
      else if (cmd == "AT+ADDR?") in += "+ADDR:98d3:34:905d3f\r\nOK\r\n";
      else in += "OK\r\n";
    }
    return 1;
  }
};
