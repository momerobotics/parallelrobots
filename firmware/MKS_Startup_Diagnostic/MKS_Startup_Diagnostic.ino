#include <Arduino.h>
#include <SPI.h>
#include <esp_mac.h>

// Replacement-pair stage 1. No motor library, PWM, radio, or drive commands.
static const int EN = 12, PWM_A = 32, PWM_B = 33, PWM_C = 25;
static const int SCK_PIN = 18, MISO_PIN = 19, MOSI_PIN = 23, CS = 5;
static const uint16_t ANGLE = 0x3FFF, DIAG = 0x3FFD, MAG = 0x3FFE;
SPISettings encoderSPI(1000000, MSBFIRST, SPI_MODE1);

struct Frame {
  uint16_t raw, data;
  bool parity, error;
};
uint8_t mac[6] = {};
bool macOK = false, havePrevious = false;
uint16_t previousAngle = 0, angleRaw = 0;
int64_t travelCounts = 0;
uint32_t samples = 0, badFrames = 0, badParity = 0, errorFlags = 0;
uint32_t unhealthySamples = 0, lastSample = 0, lastPrint = 0;
Frame angleFrame = {}, diagFrame = {}, magFrame = {};

bool evenParity(uint16_t word) {
  word ^= word >> 8;
  word ^= word >> 4;
  word ^= word >> 2;
  word ^= word >> 1;
  return !(word & 1);
}

uint16_t transferWord(uint16_t command) {
  SPI.beginTransaction(encoderSPI);
  digitalWrite(CS, LOW);
  delayMicroseconds(1);
  uint16_t result = SPI.transfer16(command);
  delayMicroseconds(1);
  digitalWrite(CS, HIGH);
  SPI.endTransaction();
  delayMicroseconds(2);
  return result;
}

Frame readRegister(uint16_t address) {
  uint16_t command = 0x4000 | address;
  if (!evenParity(command)) command |= 0x8000;
  transferWord(command);
  uint16_t raw = transferWord(0);
  return {raw, uint16_t(raw & 0x3FFF), evenParity(raw), bool(raw & 0x4000)};
}

bool valid(Frame frame) { return frame.parity && !frame.error; }

void countFrame(Frame frame) {
  if (!valid(frame)) ++badFrames;
  if (!frame.parity) ++badParity;
  if (frame.error) ++errorFlags;
}

bool healthy() {
  // OCF must be set. COF and both comparator warning flags must be clear.
  // This also rejects a disconnected MISO returning all zeroes with valid parity.
  return valid(angleFrame) && valid(diagFrame) && valid(magFrame) &&
         (diagFrame.data & 0x0100) && !(diagFrame.data & 0x0E00);
}

void printStatus() {
  Serial.printf("ms=%lu MAC=", (unsigned long)millis());
  if (macOK) {
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else Serial.print("READ_FAILED");
  Serial.printf(" motor=DISABLED encoder=%s angleDeg=%.3f travelDeg=%.3f",
                healthy() ? "OK" : "CHECK", angleRaw * 360.0 / 16384.0,
                (double)travelCounts * 360.0 / 16384.0);
  Serial.printf(" angleFrame=%04X diagFrame=%04X magFrame=%04X AGC=%u OCF=%u COF=%u compHigh=%u compLow=%u magnitude=%u",
                angleFrame.raw, diagFrame.raw, magFrame.raw, diagFrame.data & 255,
                (diagFrame.data >> 8) & 1, (diagFrame.data >> 9) & 1,
                (diagFrame.data >> 11) & 1, (diagFrame.data >> 10) & 1, magFrame.data);
  Serial.printf(" samples=%lu badFrames=%lu parityErrors=%lu errorFlags=%lu unhealthySamples=%lu\n",
                (unsigned long)samples, (unsigned long)badFrames, (unsigned long)badParity,
                (unsigned long)errorFlags, (unsigned long)unhealthySamples);
}

void setup() {
  digitalWrite(EN, LOW);
  pinMode(EN, OUTPUT);
  const int phases[] = {PWM_A, PWM_B, PWM_C};
  for (int pin : phases) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  Serial.begin(115200);
  delay(800);
  macOK = esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK;
  digitalWrite(CS, HIGH);
  pinMode(CS, OUTPUT);
  SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, CS);
  readRegister(0x0001); // Clear startup SPI error; never program encoder OTP.
  Serial.println("MKS STARTUP DIAGNOSTIC v1 | motor permanently DISABLED | SPI 1MHz MODE1");
  Serial.println("Commands: r = reset counters/travel; ? = status. No command enables drive.");
  Serial.println("Angle/travel retain the last healthy sample on faults; travel cannot bridge invalid samples.");
}

void loop() {
  digitalWrite(EN, LOW);
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'r') {
      samples = badFrames = badParity = errorFlags = unhealthySamples = 0;
      travelCounts = 0;
      havePrevious = false;
      Serial.println("Counters reset; motor DISABLED.");
    } else if (c == '?') printStatus();
  }
  uint32_t now = millis();
  if (now - lastSample >= 10) {
    lastSample = now;
    angleFrame = readRegister(ANGLE);
    diagFrame = readRegister(DIAG);
    magFrame = readRegister(MAG);
    ++samples;
    countFrame(angleFrame);
    countFrame(diagFrame);
    countFrame(magFrame);
    if (healthy()) {
      angleRaw = angleFrame.data;
      if (havePrevious) {
        int32_t delta = int32_t(angleRaw) - previousAngle;
        if (delta > 8192) delta -= 16384;
        if (delta < -8192) delta += 16384;
        travelCounts += delta;
      }
      previousAngle = angleRaw;
      havePrevious = true;
    } else {
      ++unhealthySamples;
      havePrevious = false;
      if (!valid(angleFrame) || !valid(diagFrame) || !valid(magFrame)) readRegister(0x0001);
    }
  }
  if (now - lastPrint >= 500) {
    lastPrint = now;
    printStatus();
  }
}
