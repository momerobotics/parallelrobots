#include <Arduino.h>
#include <SPI.h>
#include <SimpleFOC.h>
#include <esp_mac.h>

// Nonblocking replacement-pair calibration. No automatic start, radio, NVS or OTP writes.
static const int EN = 12, PWM_A = 32, PWM_B = 33, PWM_C = 25;
static const int SCK_PIN = 18, MISO_PIN = 19, MOSI_PIN = 23, CS = 5;
static const int VIN_PIN = 13, IA_PIN = 39, IB_PIN = 36;
// Historical divider scale; confirm against a meter for the replacement boards.
static const float VIN_SCALE = 8.29f, VIN_MIN = 11.1f, VIN_MAX = 13.0f;
static float TEST_V = 0.50f;
static const float MV_PER_AMP = 500.0f, DRIVER_LIMIT = 7.0f;
static const int POLE_PAIRS = 11;
static const float CAL_TWO_PI = 6.28318530718f, START_FIELD = 4.71238898038f;
static const float CURRENT_ABORT_A = 0.60f, TRAVEL_ABORT_DEG = 90.0f;
static const uint32_t SETTLE_MS = 1500, SWEEP_MS = 2500, MID_HOLD_MS = 700;
static const uint32_t MAX_TEST_MS = 12000;

BLDCDriver3PWM driver(PWM_A, PWM_B, PWM_C, EN);
SPISettings encoderSPI(1000000, MSBFIRST, SPI_MODE1);
struct Frame { uint16_t raw, data; bool valid; };
bool driverReady = false, encoderOK = false, running = false, onStage = false;
bool faultLatched = false, haveAngle = false;
uint8_t mac[6] = {}, calStage = 0;
uint16_t angleRaw = 0, previousRaw = 0;
uint32_t encoderErrors = 0, testStarted = 0, stageStarted = 0;
uint32_t lastSample = 0, lastVin = 0, lastPrint = 0, runId = 0;
int32_t travelCounts = 0;
float vin = 0, zeroA = 0, zeroB = 0, ia = 0, ib = 0;
float currentBound = 0, peakBound = 0;
const char *lastStop = "boot";
int32_t runStartRaw = 0, startCount = 0, midCount = 0;
int64_t endSum = 0;
uint32_t endSamples = 0;
bool calibrationValid = false;
int calibratedDirection = 0;
float calibratedZero = 0, forwardRad = 0, reverseRad = 0;
bool verificationMode = false, verificationValid = false;
float verificationIntegral = 0;
uint32_t verificationUpdate = 0;
static const float VERIFY_TARGETS[] = {0, 0.13962634f, 0, -0.13962634f, 0};
float verificationEndpoints[5] = {};

float normalized(float a) {
  a = fmodf(a, CAL_TWO_PI);
  return a < 0 ? a + CAL_TWO_PI : a;
}

bool evenParity(uint16_t x) {
  x ^= x >> 8; x ^= x >> 4; x ^= x >> 2; x ^= x >> 1;
  return !(x & 1);
}

uint16_t transferWord(uint16_t command) {
  SPI.beginTransaction(encoderSPI);
  digitalWrite(CS, LOW); delayMicroseconds(1);
  uint16_t raw = SPI.transfer16(command);
  delayMicroseconds(1); digitalWrite(CS, HIGH);
  SPI.endTransaction(); delayMicroseconds(2);
  return raw;
}

Frame readRegister(uint16_t address) {
  uint16_t command = 0x4000 | address;
  if (!evenParity(command)) command |= 0x8000;
  transferWord(command);
  uint16_t raw = transferWord(0);
  return {raw, uint16_t(raw & 0x3FFF), evenParity(raw) && !(raw & 0x4000)};
}

float readVin() {
  uint32_t total = 0;
  for (int i = 0; i < 16; ++i) total += analogReadMilliVolts(VIN_PIN);
  return total * VIN_SCALE / 16000.0f;
}

void outputsOff() {
  digitalWrite(EN, LOW);
  if (driverReady) driver.disable();
}

void stopTest(const char *reason, bool fault) {
  outputsOff();
  running = false; onStage = false;
  faultLatched |= fault;
  lastStop = reason;
  Serial.printf("STOP ms=%lu run=%lu reason=%s fault=%u stage=%u elapsedMs=%lu travelDeg=%.3f iA=%.3f iB=%.3f boundA=%.3f peakBoundA=%.3f VIN=%.2f encoderErrors=%lu\n",
                (unsigned long)millis(), (unsigned long)runId, reason, faultLatched,
                calStage+1, (unsigned long)(millis()-testStarted),
                travelCounts * 360.0 / 16384.0, ia, ib, currentBound, peakBound,
                vin, (unsigned long)encoderErrors);
}

bool sampleEncoder() {
  Frame a = readRegister(0x3FFF), d = readRegister(0x3FFD);
  encoderOK = a.valid && d.valid && (d.data & 0x0100) && !(d.data & 0x0E00);
  if (!encoderOK) {
    ++encoderErrors; haveAngle = false;
    if (running) stopTest("encoder", true);
    readRegister(0x0001);
    return false;
  }
  angleRaw = a.data;
  if (running && haveAngle) {
    int32_t delta = int32_t(angleRaw) - previousRaw;
    if (delta > 8192) delta -= 16384;
    if (delta < -8192) delta += 16384;
    travelCounts += delta;
    if (fabsf(travelCounts * 360.0f / 16384.0f) > TRAVEL_ABORT_DEG)
      stopTest("travel", true);
  }
  previousRaw = angleRaw; haveAngle = true;
  return true;
}

bool zeroCurrent() {
  outputsOff();
  delay(100);
  uint32_t a = 0, b = 0;
  uint16_t minA = 65535, minB = 65535, maxA = 0, maxB = 0;
  for (int i = 0; i < 256; ++i) {
    uint16_t ma = analogReadMilliVolts(IA_PIN), mb = analogReadMilliVolts(IB_PIN);
    a += ma; b += mb;
    minA = min(minA, ma); maxA = max(maxA, ma);
    minB = min(minB, mb); maxB = max(maxB, mb);
    delayMicroseconds(500);
  }
  zeroA = a / 256.0f; zeroB = b / 256.0f;
  bool ok = zeroA > 1400 && zeroA < 1900 && zeroB > 1400 && zeroB < 1900 &&
            maxA - minA < 100 && maxB - minB < 100;
  Serial.printf("ZERO ok=%u mVA=%.2f mVB=%.2f spanA=%u spanB=%u driver=DISABLED\n",
                ok, zeroA, zeroB, maxA-minA, maxB-minB);
  return ok;
}

void sampleCurrent() {
  ia = (analogReadMilliVolts(IA_PIN) - zeroA) / MV_PER_AMP;
  ib = (analogReadMilliVolts(IB_PIN) - zeroB) / MV_PER_AMP;
  // Conservative bound on all phase magnitudes without assuming sensor polarity.
  // ADC samples are sequential, not PWM-synchronous; not a calibrated current loop.
  currentBound = fabsf(ia) + fabsf(ib);
  if (running) {
    peakBound = max(peakBound, currentBound);
    if (currentBound >= CURRENT_ABORT_A) stopTest("current", true);
  }
}

// Same inverse Park/Clarke convention as SimpleFOC SinePWM at Uq=TEST_V, Ud=0.
// Match the main controller's 7 V driver ceiling and 3.5 V common-mode center.
void applyField(float angle, float uq) {
  float alpha = -uq * sinf(angle), beta = uq * cosf(angle);
  driver.setPwm(DRIVER_LIMIT/2 + alpha,
                DRIVER_LIMIT/2 - 0.5f*alpha + 0.86602540378f*beta,
                DRIVER_LIMIT/2 - 0.5f*alpha - 0.86602540378f*beta);
}
void applyField(float angle) { applyField(angle, TEST_V); }

void finishCalibration() {
  outputsOff();
  float endCount = endSamples ? float(endSum)/endSamples : float(runStartRaw+travelCounts);
  forwardRad = (midCount-startCount)*CAL_TWO_PI/16384.0f;
  reverseRad = (endCount-midCount)*CAL_TWO_PI/16384.0f;
  float returnError = (endCount-startCount)*CAL_TWO_PI/16384.0f;
  bool moved = fabsf(forwardRad)>0.05f && fabsf(reverseRad)>0.05f;
  bool polesOK = moved &&
      fabsf(fabsf(forwardRad)*POLE_PAIRS-CAL_TWO_PI)<0.5f &&
      fabsf(fabsf(reverseRad)*POLE_PAIRS-CAL_TWO_PI)<0.5f;
  bool ok = !faultLatched && endSamples>=50 && polesOK &&
      forwardRad*reverseRad<0 && fabsf(returnError)<(2.5f*CAL_TWO_PI/360.0f);
  calibrationValid = ok;
  calibratedDirection = ok ? (forwardRad>0 ? 1 : -1) : 0;
  calibratedZero = ok ? normalized(calibratedDirection*endCount*CAL_TWO_PI/16384.0f*POLE_PAIRS) : 0;
  stopTest(ok ? "calibration-complete" : "calibration-validation", !ok);
  Serial.printf("CAL valid=%u direction=%s zeroE=%.6f forwardDeg=%.3f reverseDeg=%.3f returnErrorDeg=%.3f ppForward=%.3f ppReverse=%.3f samples=%lu alignUq=%.2f\n",
                calibrationValid, calibratedDirection==1 ? "CW" : calibratedDirection==-1 ? "CCW" : "UNKNOWN",
                calibratedZero, forwardRad*360.0f/CAL_TWO_PI, reverseRad*360.0f/CAL_TWO_PI,
                returnError*360.0f/CAL_TWO_PI, moved ? CAL_TWO_PI/fabsf(forwardRad) : 0,
                moved ? CAL_TWO_PI/fabsf(reverseRad) : 0, (unsigned long)endSamples, TEST_V);
  Serial.println("Calibration candidate only: repeat for consistency before using in main firmware. Not saved to NVS.");
}

void updateCalibration(uint32_t now) {
  uint32_t elapsed = now-stageStarted;
  if (calStage==0) {
    applyField(START_FIELD);
    if (elapsed>=SETTLE_MS) {
      startCount=runStartRaw+travelCounts; calStage=1; stageStarted=now;
    }
  } else if (calStage==1) {
    float fraction=min(1.0f, elapsed/float(SWEEP_MS));
    applyField(START_FIELD+CAL_TWO_PI*fraction);
    if (elapsed>=SWEEP_MS) { calStage=2; stageStarted=now; }
  } else if (calStage==2) {
    applyField(START_FIELD+CAL_TWO_PI);
    if (elapsed>=MID_HOLD_MS) {
      midCount=runStartRaw+travelCounts; calStage=3; stageStarted=now;
    }
  } else if (calStage==3) {
    float fraction=min(1.0f, elapsed/float(SWEEP_MS));
    applyField(START_FIELD+CAL_TWO_PI*(1.0f-fraction));
    if (elapsed>=SWEEP_MS) { calStage=4; stageStarted=now; endSum=0; endSamples=0; }
  } else if (calStage==4) {
    applyField(START_FIELD);
    if (elapsed>=SETTLE_MS) finishCalibration();
  }
}

void updateVerification(uint32_t now) {
  float position = calibratedDirection*travelCounts*CAL_TWO_PI/16384.0f;
  float error = VERIFY_TARGETS[calStage]-position;
  float dt = min(0.02f, (now-verificationUpdate)*0.001f);
  verificationUpdate = now;
  verificationIntegral = max(-0.20f, min(0.20f, verificationIntegral+error*dt));
  float uq = max(-TEST_V, min(TEST_V, 5.0f*error+verificationIntegral));
  float electrical = normalized(calibratedDirection*angleRaw*CAL_TWO_PI/16384.0f*POLE_PAIRS-calibratedZero);
  applyField(electrical, uq);
  if (now-stageStarted >= 1000) {
    verificationEndpoints[calStage] = position;
    if (fabsf(error)>0.05235988f) {
      stopTest("verification-position", true);
      Serial.printf("VERIFY valid=0 stage=%u targetDeg=%.3f actualDeg=%.3f\n", calStage+1,
                    VERIFY_TARGETS[calStage]*360/CAL_TWO_PI, position*360/CAL_TWO_PI);
      return;
    }
    ++calStage; stageStarted=now; verificationIntegral=0;
    if (calStage==5) {
      verificationValid=true; stopTest("verification-complete", false);
      Serial.printf("VERIFY valid=1 home1Deg=%.3f positiveDeg=%.3f home2Deg=%.3f negativeDeg=%.3f home3Deg=%.3f\n",
        verificationEndpoints[0]*360/CAL_TWO_PI, verificationEndpoints[1]*360/CAL_TWO_PI,
        verificationEndpoints[2]*360/CAL_TWO_PI, verificationEndpoints[3]*360/CAL_TWO_PI,
        verificationEndpoints[4]*360/CAL_TWO_PI);
    }
  }
}

void startTest(bool verify = false) {
  if (verify && !calibrationValid) { Serial.println("VERIFY REFUSED: no valid calibration in RAM."); return; }
  if (running || faultLatched || !driverReady) {
    Serial.println("START REFUSED: running, latched fault, or driver unavailable."); return;
  }
  vin = readVin();
  if (!isfinite(vin) || vin < VIN_MIN || vin > VIN_MAX) {
    Serial.printf("START REFUSED: VIN=%.2f; expected 11.1..13.0V.\n", vin); return;
  }
  if (!sampleEncoder()) { Serial.println("START REFUSED: encoder."); return; }
  if (!zeroCurrent()) { stopTest("current-zero", true); return; }
  // A queued stop takes priority over a start, including during zero sampling.
  bool cancel = false;
  while (Serial.available()) { char c = Serial.read(); if (c == 'x' || c == 'X') cancel = true; }
  if (cancel) { stopTest("cancelled-before-start", false); return; }
  if (!sampleEncoder()) { Serial.println("START REFUSED: encoder."); return; }
  vin = readVin();
  if (!isfinite(vin) || vin < VIN_MIN || vin > VIN_MAX) {
    Serial.println("START REFUSED: supply changed."); return;
  }
  driver.voltage_power_supply = vin;
  travelCounts = 0; peakBound = 0; calStage = 0; ++runId;
  runStartRaw = angleRaw; endSum = 0; endSamples = 0;
  verificationMode = verify; verificationValid = false; verificationIntegral=0;
  if (!verify) { calibrationValid = false; calibratedDirection = 0; calibratedZero = 0; }
  Serial.printf("START run=%lu VIN=%.2f testV=%.2f maxMs=%lu\n",
                (unsigned long)runId, vin, TEST_V, (unsigned long)MAX_TEST_MS);
  testStarted = stageStarted = verificationUpdate = millis(); running = onStage = true;
  driver.enable();
  if (verify) applyField(0, 0); else applyField(START_FIELD);
}

void printStatus() {
  Serial.printf("ms=%lu MAC=%02X:%02X:%02X:%02X:%02X:%02X run=%lu drive=%u stage=%u VIN=%.2f encoder=%s angleDeg=%.3f travelDeg=%.3f iA=%.3f iB=%.3f boundA=%.3f peakBoundA=%.3f fault=%u lastStop=%s\n",
                (unsigned long)millis(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                (unsigned long)runId, running && onStage, calStage+1, vin,
                encoderOK ? "OK" : "CHECK", angleRaw * 360.0 / 16384.0,
                travelCounts * 360.0 / 16384.0, ia, ib, currentBound, peakBound,
                faultLatched, lastStop);
}

void setup() {
  const int pins[] = {EN, PWM_A, PWM_B, PWM_C};
  for (int pin : pins) { digitalWrite(pin, LOW); pinMode(pin, OUTPUT); }
  Serial.begin(115200); delay(800);
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  digitalWrite(CS, HIGH); pinMode(CS, OUTPUT);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS);
  readRegister(0x0001);
  analogReadResolution(12);
  analogSetPinAttenuation(VIN_PIN, ADC_11db);
  analogSetPinAttenuation(IA_PIN, ADC_11db);
  analogSetPinAttenuation(IB_PIN, ADC_11db);
  driver.voltage_power_supply = 12.6f;
  driver.voltage_limit = DRIVER_LIMIT; driver.pwm_frequency = 40000;
  driverReady = driver.init(); outputsOff();
  Serial.println("MKS FOC STARTUP CALIBRATION v3: disabled at boot, no automatic motion.");
  Serial.println("c = calibration; v = FOC +/-8deg verification; l/h = 0.5/0.7V while stopped; x = stop; ? = status. Faults latch until reset.");
  if (!driverReady) { stopTest("driver-init", true); return; }
  if (!zeroCurrent()) stopTest("current-zero", true);
  vin = readVin(); sampleEncoder(); printStatus();
}

void loop() {
  bool start = false, stop = false, status = false, verify = false;
  float requestedVoltage = 0;
  while (Serial.available()) {
    char c = Serial.read();
    start |= c == 'c' || c == 'C'; stop |= c == 'x' || c == 'X'; status |= c == '?';
    verify |= c == 'v' || c == 'V';
    if(c=='l') requestedVoltage=0.50f;
    if(c=='h') requestedVoltage=0.70f;
  }
  if (stop) stopTest("manual", false);
  else if (requestedVoltage && !running) {
    TEST_V=requestedVoltage; calibrationValid=false; verificationValid=false;
    Serial.printf("Alignment voltage=%.2f; calibration invalidated.\n", TEST_V);
  }
  else if (start) startTest();
  else if (verify) startTest(true);
  uint32_t now = millis();
  if (running && now-testStarted >= MAX_TEST_MS) stopTest("test-timeout", true);
  if (now-lastSample >= 2) {
    lastSample = now;
    sampleCurrent(); sampleEncoder();
    if (running && !verificationMode && calStage==4 && now-stageStarted>=SETTLE_MS-300) {
      endSum += runStartRaw+travelCounts; ++endSamples;
    }
  }
  if (now-lastVin >= 20) {
    lastVin = now; vin = readVin();
    if (running && (!isfinite(vin) || vin < VIN_MIN || vin > VIN_MAX)) stopTest("supply", true);
  }
  if (running) { if(verificationMode) updateVerification(now); else updateCalibration(now); }
  if (!running) outputsOff();
  if (status || now-lastPrint >= (running ? 100U : 500U)) {
    lastPrint = now;
    // Emit verbose telemetry only while outputs are disabled. Per-vector
    // endpoints and the peak-current bound are retained for review.
    if (!running || !onStage) printStatus();
  }
}
