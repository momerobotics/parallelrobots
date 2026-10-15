// Exercises the actual sketch's control flow with simulated IO and time.
// Does not validate ESP32 PWM, analog accuracy, electrical wiring, or mechanics.
#include <cassert>
#include <iostream>
#include "../../firmware/MKS_Phase_Startup_Test/MKS_Phase_Startup_Test.ino"

void ticks(unsigned ms) { for(unsigned i=0;i<ms;++i) { delay(1); loop(); } }
void fresh() {
  fakeUs=0; enablePin=0; adcA=1665; adcB=1667; adcVin=1450;
  fakeAngle=1000; fakeEncoderFault=false; Serial.input.clear();
  energizedVectors.clear(); driver.enabled=false; driver.initResult=true;
  driverReady=encoderOK=running=onStage=faultLatched=haveAngle=false;
  vectorIndex=angleRaw=previousRaw=0;
  encoderErrors=testStarted=stageStarted=lastSample=lastVin=lastPrint=runId=0;
  travelCounts=0; vin=zeroA=zeroB=ia=ib=currentBound=peakBound=0; lastStop="boot";
  setup();
  assert(!running && !enablePin && energizedVectors.empty());
}
void start() { Serial.input="w\n"; loop(); assert(running && enablePin); }
void stopped(const char* reason) {
  assert(!running && !enablePin && !driver.enabled);
  assert(std::string(lastStop)==reason);
}

int main() {
  fresh(); ticks(1000); assert(energizedVectors.empty()); // No automatic start.
  start(); ticks(3800); stopped("cycle-complete");
  assert(!faultLatched && energizedVectors.size()==6);
  for(size_t i=0;i<6;++i) for(int j=0;j<3;++j)
    assert(energizedVectors[i][j]==VECTORS[i][j]*TEST_V);
  ticks(5000); assert(energizedVectors.size()==6); // No automatic repeat.

  fresh(); Serial.input="wx\n"; loop(); stopped("manual");
  assert(energizedVectors.empty()); // Stop wins over buffered start.

  fresh(); adcVin=0; Serial.input="w\n"; loop(); assert(!running && !enablePin);
  fresh(); adcVin=1700; Serial.input="w\n"; loop(); assert(!running && !enablePin);

  fresh(); adcA=1700; adcB=1701; start();
  assert(zeroA==1700 && zeroB==1701); // Rezero with the current supply.
  adcA=1900; ticks(5); stopped("current"); assert(faultLatched);
  adcA=1700; auto before=energizedVectors.size(); Serial.input="w\n"; loop();
  assert(!running && energizedVectors.size()==before); // Fault stays latched.

  fresh(); start(); fakeEncoderFault=true; ticks(5); stopped("encoder");
  fresh(); start(); adcVin=0; ticks(25); stopped("supply");
  fresh(); start(); fakeAngle=6000; ticks(5); stopped("travel");
  fresh(); start(); delay(5001); loop(); stopped("test-timeout");
  fresh(); start(); Serial.input="x\n"; loop(); stopped("manual");
  fresh(); adcA=100; Serial.input="w\n"; loop(); stopped("current-zero");
  assert(energizedVectors.empty());
  std::cout<<"PASS: no autostart/repeat; six bounded vectors; stop priority; supply, encoder, current, travel, timeout and zero checks; fault latch.\n";
}
