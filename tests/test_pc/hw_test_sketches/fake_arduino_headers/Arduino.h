// Minimal stand-in for the Arduino core, ONLY to syntax-check the test sketches on a PC (g++ -fsyntax-only).
// Signatures cover just what test_forklift_*/ uses; this is NOT the real Arduino API.
#pragma once
#include <cstdlib>
using std::abs;   // the real Arduino core provides abs()
#include <stdint.h>
#include <stddef.h>
typedef uint8_t byte;
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
#define F(x) (x)
void pinMode(uint8_t, uint8_t);
void digitalWrite(uint8_t, uint8_t);
int digitalRead(uint8_t);
int analogRead(uint8_t);
void delay(unsigned long);
unsigned long millis();
void tone(uint8_t, unsigned int, unsigned long = 0);
struct Stream {  // real core: HardwareSerial and SoftwareSerial both derive from Stream
  virtual int available() = 0;
  virtual int read() = 0;
  virtual size_t write(uint8_t) = 0;
  virtual ~Stream() {}
};
struct FakeSerial : Stream {
  void begin(long);
  int available() override;
  int read() override;
  size_t write(uint8_t) override;
  size_t print(const char *); size_t print(char); size_t print(unsigned char); size_t print(int); size_t print(unsigned int);
  size_t print(long); size_t print(unsigned long); size_t print(double); size_t print(double, int);  // real Print has print(double, digits)
  size_t println(const char *); size_t println(char); size_t println(unsigned char); size_t println(int); size_t println(unsigned int);
  size_t println(long); size_t println(unsigned long); size_t println(double); size_t println();  // real Print has println(void)
};
extern FakeSerial Serial;
