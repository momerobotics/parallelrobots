#include <Arduino.h>

static const int MOTOR_ENABLE_PIN = 12;
static const int CURRENT_A_PIN = 39;
static const int CURRENT_B_PIN = 36;

static const float SHUNT_OHMS = 0.01f;
static const float AMPLIFIER_GAIN = 50.0f;
static const float MILLIVOLTS_PER_AMP = SHUNT_OHMS * AMPLIFIER_GAIN * 1000.0f;

struct ChannelSample {
  uint32_t rawSum = 0;
  uint32_t mvSum = 0;
  uint16_t rawMin = 4095;
  uint16_t rawMax = 0;
};

float offsetMvA = 0.0f;
float offsetMvB = 0.0f;

ChannelSample sampleChannel(int pin, int count) {
  ChannelSample sample;
  for (int i = 0; i < count; ++i) {
    uint16_t raw = analogRead(pin);
    uint16_t mv = analogReadMilliVolts(pin);
    sample.rawSum += raw;
    sample.mvSum += mv;
    sample.rawMin = min(sample.rawMin, raw);
    sample.rawMax = max(sample.rawMax, raw);
    delayMicroseconds(100);
  }
  return sample;
}

void printMac() {
  uint64_t mac = ESP.getEfuseMac();
  for (int i = 5; i >= 0; --i) {
    if (i != 5) Serial.print(':');
    uint8_t value = (mac >> (8 * i)) & 0xFF;
    if (value < 16) Serial.print('0');
    Serial.print(value, HEX);
  }
}

void setup() {
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN, LOW);

  Serial.begin(115200);
  delay(800);
  analogReadResolution(12);
  analogSetPinAttenuation(CURRENT_A_PIN, ADC_11db);
  analogSetPinAttenuation(CURRENT_B_PIN, ADC_11db);

  Serial.println("MKS ESP32 FOC Mega current-sense diagnostic");
  Serial.print("MAC: ");
  printMac();
  Serial.println();
  Serial.println("Motor driver disabled; no phase switching.");
  Serial.print("Sensitivity: ");
  Serial.print(MILLIVOLTS_PER_AMP, 1);
  Serial.println(" mV/A");

  delay(200);
  ChannelSample a = sampleChannel(CURRENT_A_PIN, 1000);
  ChannelSample b = sampleChannel(CURRENT_B_PIN, 1000);
  offsetMvA = a.mvSum / 1000.0f;
  offsetMvB = b.mvSum / 1000.0f;

  Serial.print("Zero offsets: A=");
  Serial.print(offsetMvA, 2);
  Serial.print("mV B=");
  Serial.print(offsetMvB, 2);
  Serial.println("mV");
  Serial.println("rawA,mvA,noiseA,ampsA,rawB,mvB,noiseB,ampsB");
}

void loop() {
  ChannelSample a = sampleChannel(CURRENT_A_PIN, 128);
  ChannelSample b = sampleChannel(CURRENT_B_PIN, 128);
  float rawA = a.rawSum / 128.0f;
  float rawB = b.rawSum / 128.0f;
  float mvA = a.mvSum / 128.0f;
  float mvB = b.mvSum / 128.0f;

  Serial.print(rawA, 1);
  Serial.print(',');
  Serial.print(mvA, 2);
  Serial.print(',');
  Serial.print(a.rawMax - a.rawMin);
  Serial.print(',');
  Serial.print((mvA - offsetMvA) / MILLIVOLTS_PER_AMP, 4);
  Serial.print(',');
  Serial.print(rawB, 1);
  Serial.print(',');
  Serial.print(mvB, 2);
  Serial.print(',');
  Serial.print(b.rawMax - b.rawMin);
  Serial.print(',');
  Serial.println((mvB - offsetMvB) / MILLIVOLTS_PER_AMP, 4);

  delay(250);
}
