#pragma once
#include "Arduino.h"
#include <array>
#include <vector>
inline std::vector<std::array<float,3>> energizedVectors;
struct BLDCDriver3PWM {
  float voltage_power_supply=0, voltage_limit=0;
  long pwm_frequency=0;
  bool enabled=false;
  bool initResult=true;
  BLDCDriver3PWM(int,int,int,int) {}
  bool init() { return initResult; }
  void enable() { enabled=true; digitalWrite(12,HIGH); }
  void disable() { enabled=false; digitalWrite(12,LOW); }
  void setPwm(float a,float b,float c) {
    if(enabled && (a||b||c)) energizedVectors.push_back({a,b,c});
  }
};
