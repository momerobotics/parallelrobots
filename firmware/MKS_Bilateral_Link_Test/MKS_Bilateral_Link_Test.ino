#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static const int PIN_SCK = 18;
static const int PIN_MISO = 19;
static const int PIN_MOSI = 23;
static const int PIN_CS = 5;
static const int PIN_ENABLE = 12;
static const int PIN_VIN_SENSE = 13;

static const float VIN_SCALE = 8.5f;
static const float UNDERVOLTAGE = 11.1f;
static const uint8_t ESPNOW_CHANNEL = 1;
static const uint32_t SEND_PERIOD_US = 2000;
static const uint32_t LINK_TIMEOUT_MS = 250;

static const uint8_t MAC_A[6] = {0x30, 0xC9, 0x22, 0x5F, 0x5D, 0x1C};
static const uint8_t MAC_B[6] = {0x30, 0xC9, 0x22, 0x5F, 0x5D, 0x24};

struct __attribute__((packed)) LinkPacket {
  uint32_t magic;
  uint32_t sequence;
  float positionRad;
  uint16_t angleRaw;
  uint16_t encoderErrors;
  float vin;
};

static const uint32_t PACKET_MAGIC = 0x50524F42; // "PROB"
SPISettings encoderSPI(1000000, MSBFIRST, SPI_MODE1);

uint8_t localMac[6] = {};
uint8_t peerMac[6] = {};
char nodeName = '?';
volatile LinkPacket receivedPacket = {};
volatile uint32_t lastReceiveMs = 0;
volatile uint32_t receivedCount = 0;
portMUX_TYPE packetMux = portMUX_INITIALIZER_UNLOCKED;

uint32_t sequenceNumber = 0;
uint32_t sentCount = 0;
uint32_t sendFailures = 0;
uint32_t lastSendUs = 0;
uint32_t lastPrintMs = 0;
uint16_t encoderErrors = 0;
uint16_t angleRaw = 0;
uint16_t previousRaw = 0;
int32_t accumulatedCounts = 0;
bool haveAngle = false;
float bootVin = 0.0f;

uint16_t addEvenParity(uint16_t word) {
  word &= 0x7FFF;
  uint16_t x = word;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return (x & 1) ? (word | 0x8000) : word;
}

bool hasEvenParity(uint16_t word) {
  uint16_t x = word;
  x ^= x >> 8;
  x ^= x >> 4;
  x ^= x >> 2;
  x ^= x >> 1;
  return (x & 1) == 0;
}

uint16_t transferEncoder(uint16_t command) {
  SPI.beginTransaction(encoderSPI);
  digitalWrite(PIN_CS, LOW);
  delayMicroseconds(1);
  uint16_t response = SPI.transfer16(command);
  delayMicroseconds(1);
  digitalWrite(PIN_CS, HIGH);
  SPI.endTransaction();
  delayMicroseconds(2);
  return response;
}

void clearEncoderError() {
  transferEncoder(addEvenParity(0x4000 | 0x0001));
  transferEncoder(0x0000);
}

bool readEncoder(uint16_t &raw) {
  for (uint8_t attempt = 0; attempt < 3; attempt++) {
    transferEncoder(addEvenParity(0x4000 | 0x3FFF));
    uint16_t frame = transferEncoder(0x0000);
    raw = frame & 0x3FFF;
    if (hasEvenParity(frame) && !(frame & 0x4000)) return true;
    clearEncoderError();
  }
  encoderErrors++;
  return false;
}

void updatePosition() {
  uint16_t raw;
  if (!readEncoder(raw)) return;
  angleRaw = raw;
  if (!haveAngle) {
    previousRaw = raw;
    haveAngle = true;
    return;
  }
  int16_t delta = (int16_t)raw - (int16_t)previousRaw;
  if (delta > 8192) delta -= 16384;
  if (delta < -8192) delta += 16384;
  accumulatedCounts += delta;
  previousRaw = raw;
}

float localPositionRad() {
  return accumulatedCounts * (TWO_PI / 16384.0f);
}

float readVin() {
  return analogReadMilliVolts(PIN_VIN_SENSE) * VIN_SCALE / 1000.0f;
}

bool sameMac(const uint8_t *a, const uint8_t *b) {
  return memcmp(a, b, 6) == 0;
}

void printMac(const uint8_t *mac) {
  for (uint8_t i = 0; i < 6; i++) {
    if (i) Serial.print(':');
    if (mac[i] < 16) Serial.print('0');
    Serial.print(mac[i], HEX);
  }
}

void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int length) {
  if (length != sizeof(LinkPacket) || !sameMac(info->src_addr, peerMac)) return;
  LinkPacket packet;
  memcpy(&packet, data, sizeof(packet));
  if (packet.magic != PACKET_MAGIC) return;
  portENTER_CRITICAL(&packetMux);
  memcpy((void *)&receivedPacket, &packet, sizeof(packet));
  lastReceiveMs = millis();
  receivedCount++;
  portEXIT_CRITICAL(&packetMux);
}

void onSend(const esp_now_send_info_t *, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) sendFailures++;
}

bool initRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_get_mac(WIFI_IF_STA, localMac);

  if (sameMac(localMac, MAC_A)) {
    nodeName = 'A';
    memcpy(peerMac, MAC_B, 6);
  } else if (sameMac(localMac, MAC_B)) {
    nodeName = 'B';
    memcpy(peerMac, MAC_A, 6);
  } else {
    return false;
  }

  if (esp_now_init() != ESP_OK) return false;
  esp_now_register_recv_cb(onReceive);
  esp_now_register_send_cb(onSend);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, peerMac, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  return esp_now_add_peer(&peer) == ESP_OK;
}

void sendPosition() {
  LinkPacket packet = {
    PACKET_MAGIC,
    sequenceNumber++,
    localPositionRad(),
    angleRaw,
    encoderErrors,
    bootVin
  };
  if (esp_now_send(peerMac, (uint8_t *)&packet, sizeof(packet)) == ESP_OK) sentCount++;
  else sendFailures++;
}

void printStatus() {
  LinkPacket remote;
  uint32_t rxMs;
  uint32_t rxCount;
  portENTER_CRITICAL(&packetMux);
  memcpy(&remote, (const void *)&receivedPacket, sizeof(remote));
  rxMs = lastReceiveMs;
  rxCount = receivedCount;
  portEXIT_CRITICAL(&packetMux);

  uint32_t age = rxMs ? millis() - rxMs : UINT32_MAX;
  Serial.print("node="); Serial.print(nodeName);
  Serial.print(" local="); Serial.print(localPositionRad(), 3);
  Serial.print(" remote="); Serial.print(remote.positionRad, 3);
  Serial.print(" link="); Serial.print(age <= LINK_TIMEOUT_MS ? "OK" : "LOST");
  Serial.print(" ageMs=");
  if (age == UINT32_MAX) Serial.print("never"); else Serial.print(age);
  Serial.print(" tx="); Serial.print(sentCount);
  Serial.print(" rx="); Serial.print(rxCount);
  Serial.print(" txFail="); Serial.print(sendFailures);
  Serial.print(" encErr="); Serial.print(encoderErrors);
  Serial.print(" bootVin="); Serial.println(bootVin, 2);
}

void setup() {
  Serial.begin(115200);
  delay(800);
  pinMode(PIN_ENABLE, OUTPUT);
  digitalWrite(PIN_ENABLE, LOW);
  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_CS);
  analogReadResolution(12);
  bootVin = readVin(); // GPIO13 is ADC2 and cannot be sampled after Wi-Fi starts.

  for (uint8_t i = 0; i < 5; i++) updatePosition();
  bool radioOk = initRadio();

  Serial.println("Parallel Robots MKS bilateral link test - motors disabled");
  Serial.print("Local MAC: "); printMac(localMac);
  Serial.print(" node="); Serial.println(nodeName);
  Serial.print("Peer MAC:  "); printMac(peerMac); Serial.println();
  Serial.print("AS5048A: "); Serial.println(haveAngle ? "OK" : "ERROR");
  Serial.print("ESP-NOW: "); Serial.println(radioOk ? "OK" : "ERROR");
  Serial.print("Boot VIN: "); Serial.println(bootVin, 2);
  if (!radioOk || !haveAngle || bootVin < UNDERVOLTAGE) {
    Serial.println("Startup check failed; motor remains disabled.");
  }
}

void loop() {
  updatePosition();
  uint32_t nowUs = micros();
  if (nowUs - lastSendUs >= SEND_PERIOD_US) {
    sendPosition();
    lastSendUs = nowUs;
  }
  uint32_t nowMs = millis();
  if (nowMs - lastPrintMs >= 500) {
    printStatus();
    lastPrintMs = nowMs;
  }
}
