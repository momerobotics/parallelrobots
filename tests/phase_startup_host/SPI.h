#pragma once
#include <cstdint>
constexpr int MSBFIRST=1, SPI_MODE1=1;
inline uint16_t fakeAngle=1000;
inline bool fakeEncoderFault=false;
struct SPISettings { SPISettings(int,int,int) {} };
struct FakeSPI {
  uint16_t address=0;
  void begin(int,int,int,int) {}
  void beginTransaction(SPISettings) {}
  void endTransaction() {}
  uint16_t transfer16(uint16_t command) {
    if(command) { address=command&0x3fff; return 0; }
    uint16_t data=address==0x3fff?fakeAngle:address==0x3ffd?(fakeEncoderFault?0:0x100):0;
    if(__builtin_popcount(data)&1) data|=0x8000;
    return data;
  }
};
inline FakeSPI SPI;
