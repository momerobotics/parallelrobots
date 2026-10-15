#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstddef>
#include <string>
using std::min;
using std::max;
using std::isfinite;
constexpr int LOW=0, HIGH=1, OUTPUT=1, ADC_11db=3;
inline uint64_t fakeUs=0;
inline int enablePin=0;
inline uint16_t adcA=1665, adcB=1667, adcVin=1450;
inline uint32_t millis() { return fakeUs/1000; }
inline void delay(unsigned long ms) { fakeUs+=ms*1000; }
inline void delayMicroseconds(unsigned int us) { fakeUs+=us; }
inline void pinMode(int,int) {}
inline void digitalWrite(int pin,int value) { if(pin==12) enablePin=value; }
inline void analogReadResolution(int) {}
inline void analogSetPinAttenuation(int,int) {}
inline uint16_t analogReadMilliVolts(int pin) {
  return pin==39?adcA:pin==36?adcB:adcVin;
}
struct FakeSerial {
  std::string input;
  void begin(int) {}
  int available() { return input.size(); }
  char read() { char c=input.front(); input.erase(0,1); return c; }
  void println(const char*) {}
  template<class... Args> void printf(const char*,Args...) {}
};
inline FakeSerial Serial;
