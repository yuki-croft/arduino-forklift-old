// Fake HC-05 in AT mode: records each "\r\n"-terminated command and answers OK / ERROR:(n).
#pragma once
#include "Arduino.h"
struct SoftwareSerial {
  std::string in, pending;
  std::vector<std::string> cmds;
  std::string failOn;          // command answered with ERROR:(17)
  int atErrorsFirst = 0;       // the first N "AT" get ERROR:(0) (forum report)
  bool silent = false;
  bool badEnding = false;      // a command arrived without CR+LF before the next one
  long baud = 0;
  SoftwareSerial(uint8_t, uint8_t) {}
  void begin(long b) { baud = b; }
  int available() { return (int)in.size(); }
  int read() { int c = (unsigned char)in[0]; in.erase(0, 1); return c; }
  size_t print(const char *s) {
    pending += s;
    size_t p;
    while ((p = pending.find("\r\n")) != std::string::npos) {
      std::string c = pending.substr(0, p); pending.erase(0, p + 2); cmds.push_back(c);
      if (silent) continue;
      if (c == "AT" && atErrorsFirst > 0) { atErrorsFirst--; in += "ERROR:(0)\r\n"; }
      else if (c == failOn) in += "ERROR:(17)\r\n";
      else in += "OK\r\n";
    }
    return 1;
  }
};
