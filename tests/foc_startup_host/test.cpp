// Actual calibration control flow with ideal encoder/field tracking.
// This checks conventions and fault handling, not motor physics or ADC accuracy.
#include <cassert>
#include <iostream>
#include "../../firmware/MKS_FOC_Startup_Calibration/MKS_FOC_Startup_Calibration.ino"
void stopped(const char* why) {
  assert(!running && !enablePin && !driver.enabled);
  assert(std::string(lastStop)==why);
}
void fresh() {
  fakeUs=0; enablePin=0; adcA=1665; adcB=1667; adcVin=1450;
  fakeAngle=16350; fakeEncoderFault=false; Serial.input.clear();
  energizedVectors.clear(); driver.enabled=false; driver.initResult=true;
  driverReady=encoderOK=running=onStage=faultLatched=haveAngle=false;
  calStage=angleRaw=previousRaw=0;
  encoderErrors=testStarted=stageStarted=lastSample=lastVin=lastPrint=runId=0;
  travelCounts=0; vin=zeroA=zeroB=ia=ib=currentBound=peakBound=0;
  runStartRaw=startCount=midCount=0; endSum=0; endSamples=0;
  TEST_V=0.50f; verificationMode=verificationValid=false; verificationIntegral=0;
  calibrationValid=false; calibratedDirection=0; calibratedZero=0; lastStop="boot";
  setup(); assert(!running && !enablePin && energizedVectors.empty());
}
void ticks(unsigned ms) { for(unsigned i=0;i<ms;++i) { delay(1); loop(); } }
void start() { Serial.input="c\n"; loop(); assert(running && enablePin); }
void track(int direction) {
  float field=START_FIELD, previous=START_FIELD;
  float initial=fakeAngle;
  while(running) {
    auto pwm=energizedVectors.back();
    for(auto phase:pwm) assert(phase>=-1e-6f && phase<=DRIVER_LIMIT+1e-6f);
    float alpha=pwm[0]-DRIVER_LIMIT/2;
    float beta=(pwm[1]-pwm[2])/1.73205080756f;
    float angle=atan2f(-alpha,beta);
    float delta=remainderf(angle-previous,CAL_TWO_PI);
    field+=delta; previous=angle;
    int count=lroundf(initial+direction*(field-START_FIELD)*16384.0f/(CAL_TWO_PI*POLE_PAIRS));
    fakeAngle=(count%16384+16384)%16384;
    delay(1); loop();
  }
}
int main() {
  for(int direction:{-1,1}) {
    fresh(); ticks(1000); assert(energizedVectors.empty());
    float expected=normalized(direction*fakeAngle*CAL_TWO_PI/16384.0f*POLE_PAIRS);
    start(); track(direction); stopped("calibration-complete");
    assert(calibrationValid && calibratedDirection==direction);
    assert(fabsf(remainderf(calibratedZero-expected,CAL_TWO_PI))<0.005f);
    assert(endSamples>=50 && !faultLatched);
    auto n=energizedVectors.size(); ticks(1000); assert(energizedVectors.size()==n);
  }
  fresh(); Serial.input="v\n"; loop(); assert(!running && energizedVectors.empty());
  fresh(); start(); track(1); Serial.input="v\n"; loop(); assert(running && verificationMode);
  ticks(2100); stopped("verification-position"); assert(!verificationValid);
  fresh(); start(); ticks(9000); stopped("calibration-validation"); assert(!calibrationValid);
  fresh(); Serial.input="cx\n"; loop(); stopped("manual"); assert(energizedVectors.empty());
  fresh(); adcVin=0; Serial.input="c\n"; loop(); assert(!running && !enablePin);
  fresh(); adcVin=1700; Serial.input="c\n"; loop(); assert(!running && !enablePin);
  fresh(); adcA=1700; start(); assert(zeroA==1700);
  adcA=2100; ticks(5); stopped("current"); assert(faultLatched);
  adcA=1700; Serial.input="c\n"; loop(); assert(!running);
  fresh(); start(); fakeEncoderFault=true; ticks(5); stopped("encoder");
  fresh(); start(); adcVin=0; ticks(25); stopped("supply");
  fresh(); start(); fakeAngle=5000; ticks(5); stopped("travel");
  fresh(); start(); delay(12001); loop(); stopped("test-timeout");
  fresh(); start(); Serial.input="x\n"; loop(); stopped("manual");
  fresh(); adcA=100; Serial.input="c\n"; loop(); stopped("current-zero");
  std::cout<<"PASS: CW/CCW and wraparound calibration, bounded PWM, no autostart/repeat, stalled-rotor rejection, stop priority, supply/current/encoder/travel/timeout/zero faults and latch.\n";
}
