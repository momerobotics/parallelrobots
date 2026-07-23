#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <SimpleFOC.h>

// Historical first-generation prototype for the FOC_ESP32_V1.1 board.
// This pin mapping and MAC pair do not match the current MKS ESP32 FOC Mega
// hardware. Use firmware/MKS_Parallel_Mirror for the current prototype.

// ==============================
// PEER MAC (egyiket kommenteld)
// Board A: 88:57:21:BB:F3:20
// Board B: 88:57:21:BC:7C:00
// ==============================

// Board A-n fut: peer = Board B
uint8_t peerAddress[] = { 0x88, 0x57, 0x21, 0xBC, 0x7C, 0x00 };
// Board B-n fut: peer = Board A
// uint8_t peerAddress[] = { 0x88, 0x57, 0x21, 0xBB, 0xF3, 0x20 };

static const uint8_t ESPNOW_CHANNEL = 1;

// ===== AliExpress FOC_ESP32_V1.1 pinout =====
static const int PWM_A   = 16;
static const int PWM_B   = 17;
static const int PWM_C   = 5;
static const int EN_PIN  = 4;

static const int SPI_SCK  = 18;
static const int SPI_MISO = 19;
static const int SPI_MOSI = 23;
static const int SPI_CS   = 13;

// ===== packet =====
typedef struct __attribute__((packed)) {
  float angle; // rad
} MotorData;

volatile MotorData lastIncoming;
volatile uint32_t lastRxMs = 0;

// ===== SimpleFOC =====
MagneticSensorSPI sensor = MagneticSensorSPI(AS5048_SPI, SPI_CS);
BLDCMotor motor = BLDCMotor(11);
BLDCDriver3PWM driver = BLDCDriver3PWM(PWM_A, PWM_B, PWM_C, EN_PIN);

// ===== Feel / performance tuning =====
static const float SUPPLY_VOLTAGE = 12.0f;

// Max “torque” in torque-voltage mode (valójában fesz limit):
static const float VOLTAGE_LIMIT  = 7.0f;     // ha erős: 6.0, ha gyenge: 8–9

// Rugó erősség (nyomaték = K * delta_eff):
static const float SPRING_K       = 2.0f;     // kisebb = szabadabb. próbáld 1.2–3.0

// Holtjáték: ekkora delta alatt 0 nyomaték => szabad mozgás
static const float DEAD_BAND_RAD  = 0.08f;    // ~4.6° (0.05–0.15 jó tartomány)

// Ha szeretnél még puhább “közel nulla” érzetet:
static const float SOFTEN_EXP     = 1.4f;     // 1.0 = lineáris, 1.2–2.0 = finomabb

// Irány invert ha kell
static const int MIRROR_SIGN      = +1;       // ha fordítva: -1

// Link timeout
static const uint32_t TIMEOUT_MS  = 250;

// Küldési freki (kapcsolat “gyorsasága”):
static const uint32_t SEND_PERIOD_US = 2000;  // 2000us = 500Hz, 1000us=1kHz (csak ha bírja)

// ===== ESP-NOW recv =====
void onReceiveData(const esp_now_recv_info* info, const uint8_t* incoming, int len) {
  (void)info;
  if (len != (int)sizeof(MotorData)) return;

  MotorData tmp;
  memcpy(&tmp, incoming, sizeof(MotorData));
  memcpy((void*)&lastIncoming, &tmp, sizeof(MotorData));
  lastRxMs = millis();
}

static void initEspNow() {
  WiFi.mode(WIFI_MODE_NULL);
  delay(20);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.disconnect(true, true);
  delay(50);

  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  delay(20);

  if (esp_now_init() != ESP_OK) {
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onReceiveData);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerAddress, 6);
  peerInfo.channel = ESPNOW_CHANNEL;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    while (true) delay(1000);
  }
}

static void initFoc() {
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  sensor.init();
  motor.linkSensor(&sensor);

  driver.voltage_power_supply = SUPPLY_VOLTAGE;
  driver.init();
  motor.linkDriver(&driver);

  motor.controller = MotionControlType::torque; // torque-voltage
  motor.voltage_limit = VOLTAGE_LIMIT;

  motor.init();
  motor.initFOC();
}

static inline float fast_abs(float x) { return x < 0 ? -x : x; }

// Deadband + “soft” rugó karakterisztika
static float springTorqueFromDelta(float delta) {
  float a = fast_abs(delta);
  if (a <= DEAD_BAND_RAD) return 0.0f;

  // deadband utáni hatásos delta
  float d = a - DEAD_BAND_RAD;

  // finomabb érzet (nemlineáris)
  // d_eff = d^SOFTEN_EXP  (SOFTEN_EXP=1 -> lineáris)
  float d_eff = powf(d, SOFTEN_EXP);

  float t = SPRING_K * d_eff;
  return (delta < 0) ? -t : t;
}

void setup() {
  // Serial nélkül: gyorsabb/kevesebb zavar
  initEspNow();
  initFoc();
}

void loop() {
  motor.loopFOC();

  const uint32_t nowMs = millis();
  const bool linkOk = (lastRxMs != 0) && ((nowMs - lastRxMs) <= TIMEOUT_MS);

  // ---- send at fixed high rate ----
  static uint32_t nextSendUs = 0;
  const uint32_t nowUs = micros();
  if ((int32_t)(nowUs - nextSendUs) >= 0) {
    nextSendUs = nowUs + SEND_PERIOD_US;

    MotorData out;
    out.angle = motor.shaft_angle;
    esp_now_send(peerAddress, (uint8_t*)&out, sizeof(out));
  }

  // ---- control ----
  if (linkOk) {
    MotorData in;
    memcpy(&in, (const void*)&lastIncoming, sizeof(MotorData));

    const float remoteAngle = (float)MIRROR_SIGN * in.angle;
    const float delta = remoteAngle - motor.shaft_angle;

    motor.move(springTorqueFromDelta(delta));
  } else {
    motor.move(0.0f);
  }
}
