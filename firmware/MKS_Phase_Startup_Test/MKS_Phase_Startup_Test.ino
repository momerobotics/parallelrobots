#include <Arduino.h>
#include <SPI.h>
#include <SimpleFOC.h>
#include <esp_mac.h>

// New-pair phase check: no automatic start, radio, stored FOC, or OTP writes.
static const int EN = 12, PWM_A = 32, PWM_B = 33, PWM_C = 25;
static const int SCK_PIN = 18, MISO_PIN = 19, MOSI_PIN = 23, CS = 5;
static const int VIN_PIN = 13, IA_PIN = 39, IB_PIN = 36;
// Historical divider scale; confirm against a meter for the replacement boards.
static const float VIN_SCALE = 8.29f, VIN_MIN = 11.1f, VIN_MAX = 13.0f;
static const float TEST_V = 0.5f, MV_PER_AMP = 500.0f;
static const float CURRENT_ABORT_A = 0.35f, TRAVEL_ABORT_DEG = 90.0f;
static const uint32_t ON_MS = 350, OFF_MS = 250, MAX_TEST_MS = 5000;
static const uint8_t VECTORS[6][3] = {
  {1,0,0}, {1,1,0}, {0,1,0}, {0,1,1}, {0,0,1}, {1,0,1}
};

BLDCDriver3PWM driver(PWM_A, PWM_B, PWM_C, EN);
SPISettings encoderSPI(1000000, MSBFIRST, SPI_MODE1);
struct Frame { uint16_t raw, data; bool valid; };
bool driverReady = false, encoderOK = false, running = false, onStage = false;
bool faultLatched = false, haveAngle = false;
uint8_t mac[6] = {}, vectorIndex = 0;
uint16_t angleRaw = 0, previousRaw = 0;
uint32_t encoderErrors = 0, testStarted = 0, stageStarted = 0;
uint32_t lastSample = 0, lastVin = 0, lastPrint = 0, runId = 0;
int32_t travelCounts = 0;
float vin = 0, zeroA = 0, zeroB = 0, ia = 0, ib = 0;
float currentBound = 0, peakBound = 0;
const char *lastStop = "boot";

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
  Serial.printf("STOP ms=%lu run=%lu reason=%s fault=%u vector=%u elapsedMs=%lu travelDeg=%.3f iA=%.3f iB=%.3f boundA=%.3f peakBoundA=%.3f VIN=%.2f encoderErrors=%lu\n",
                (unsigned long)millis(), (unsigned long)runId, reason, faultLatched,
                vectorIndex+1, (unsigned long)(millis()-testStarted),
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

void energizeVector() {
  driver.enable();
  driver.setPwm(VECTORS[vectorIndex][0] * TEST_V,
                VECTORS[vectorIndex][1] * TEST_V,
                VECTORS[vectorIndex][2] * TEST_V);
  onStage = true; stageStarted = millis();
}

void startTest() {
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
  travelCounts = 0; peakBound = 0; vectorIndex = 0; ++runId;
  Serial.printf("START run=%lu VIN=%.2f testV=%.2f maxMs=%lu\n",
                (unsigned long)runId, vin, TEST_V, (unsigned long)MAX_TEST_MS);
  testStarted = millis(); running = true;
  energizeVector();
}

void printStatus() {
  Serial.printf("ms=%lu MAC=%02X:%02X:%02X:%02X:%02X:%02X run=%lu drive=%u vector=%u VIN=%.2f encoder=%s angleDeg=%.3f travelDeg=%.3f iA=%.3f iB=%.3f boundA=%.3f peakBoundA=%.3f fault=%u lastStop=%s\n",
                (unsigned long)millis(), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
                (unsigned long)runId, running && onStage, vectorIndex+1, vin,
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
  driver.voltage_limit = TEST_V; driver.pwm_frequency = 40000;
  driverReady = driver.init(); outputsOff();
  Serial.println("MKS PHASE STARTUP v2: disabled at boot, no automatic motion.");
  Serial.println("w = one 6-vector 0.5V test; x = stop; ? = status. Faults latch until reset.");
  if (!driverReady) { stopTest("driver-init", true); return; }
  if (!zeroCurrent()) stopTest("current-zero", true);
  vin = readVin(); sampleEncoder(); printStatus();
}

void loop() {
  bool start = false, stop = false, status = false;
  while (Serial.available()) {
    char c = Serial.read();
    start |= c == 'w' || c == 'W'; stop |= c == 'x' || c == 'X'; status |= c == '?';
  }
  if (stop) stopTest("manual", false);
  else if (start) startTest();
  uint32_t now = millis();
  if (running && now-testStarted >= MAX_TEST_MS) stopTest("test-timeout", true);
  if (now-lastSample >= 2) {
    lastSample = now;
    sampleCurrent(); sampleEncoder();
  }
  if (now-lastVin >= 20) {
    lastVin = now; vin = readVin();
    if (running && (!isfinite(vin) || vin < VIN_MIN || vin > VIN_MAX)) stopTest("supply", true);
  }
  if (running && onStage && now-stageStarted >= ON_MS) {
    outputsOff(); onStage = false; stageStarted = now;
    printStatus();
  } else if (running && !onStage && now-stageStarted >= OFF_MS) {
    if (++vectorIndex >= 6) { vectorIndex = 5; stopTest("cycle-complete", false); }
    else energizeVector();
  }
  if (!running) outputsOff();
  if (status || now-lastPrint >= (running ? 100U : 500U)) {
    lastPrint = now;
    // Emit verbose telemetry only while outputs are disabled. Per-vector
    // endpoints and the peak-current bound are retained for review.
    if (!running || !onStage) printStatus();
  }
}
