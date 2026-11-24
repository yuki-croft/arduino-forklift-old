#pragma once
#include <stdint.h>
#include <cstddef>
#include <string>
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
#define constrain(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
void pinMode(uint8_t,uint8_t); int digitalRead(uint8_t); int analogRead(uint8_t);
void delay(unsigned long); unsigned long millis(); void tone(uint8_t,unsigned int,unsigned long=0);
