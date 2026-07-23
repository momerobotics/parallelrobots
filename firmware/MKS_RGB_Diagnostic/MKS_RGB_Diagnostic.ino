#include <Arduino.h>
#include <esp32-hal-rmt.h>

static const int MOTOR_ENABLE_PIN=12;
static const int DEFAULT_LED_PIN=2;
static const uint32_t RMT_FREQUENCY=20000000;

struct Timing {
  const char* name;
  uint16_t zeroHigh;
  uint16_t zeroLow;
  uint16_t oneHigh;
  uint16_t oneLow;
};

// Durations are 50 ns RMT ticks.
static const Timing TIMINGS[]={
  {"WS2812-800",8,17,16,9},
  {"XINGLIGHT-LONG",6,34,18,22},
  {"WS2812-ALT",7,18,14,11}
};

static const int SCAN_PINS[]={0,2,4,14,15,16,17,21,22,26,27};
rmt_data_t ledData[24];
int activePin=-1;

bool selectPin(int pin){
  if(activePin==pin)return true;
  if(activePin>=0)rmtDeinit(activePin);
  activePin=-1;
  if(!rmtInit(pin,RMT_TX_MODE,RMT_MEM_NUM_BLOCKS_1,RMT_FREQUENCY))return false;
  rmtSetEOT(pin,LOW);
  activePin=pin;
  return true;
}

bool sendRgb(int pin,const Timing&timing,uint8_t red,uint8_t green,uint8_t blue){
  if(!selectPin(pin))return false;
  const uint8_t colors[3]={green,red,blue};
  int symbol=0;
  for(int color=0;color<3;color++){
    for(int bit=7;bit>=0;bit--){
      bool one=colors[color]&(1U<<bit);
      ledData[symbol].level0=1;
      ledData[symbol].duration0=one?timing.oneHigh:timing.zeroHigh;
      ledData[symbol].level1=0;
      ledData[symbol].duration1=one?timing.oneLow:timing.zeroLow;
      symbol++;
    }
  }
  bool ok=rmtWrite(pin,ledData,24,RMT_WAIT_FOR_EVER);
  delayMicroseconds(200);
  return ok;
}

void testDefault(int timingIndex){
  const Timing&timing=TIMINGS[timingIndex];
  bool ok=sendRgb(DEFAULT_LED_PIN,timing,255,255,255);
  Serial.printf("GPIO2 %s WHITE rmt=%s\n",timing.name,ok?"OK":"ERROR");
}

void allOff(){
  for(const Timing&timing:TIMINGS)sendRgb(DEFAULT_LED_PIN,timing,0,0,0);
  Serial.println("GPIO2 OFF sent with all timings");
}

void scanAll(){
  Serial.println("SCAN START: watch for a one-second white flash");
  for(int pin:SCAN_PINS){
    for(const Timing&timing:TIMINGS){
      Serial.printf("SCAN GPIO%d %s WHITE\n",pin,timing.name);
      sendRgb(pin,timing,255,255,255);
      delay(1000);
      sendRgb(pin,timing,0,0,0);
      delay(250);
    }
  }
  Serial.println("SCAN COMPLETE");
}

void setStaticLevel(bool high){
  if(activePin>=0){rmtDeinit(activePin);activePin=-1;}
  pinMode(DEFAULT_LED_PIN,OUTPUT);
  digitalWrite(DEFAULT_LED_PIN,high?HIGH:LOW);
  Serial.printf("GPIO2 STATIC %s\n",high?"HIGH (about 3.3 V)":"LOW (about 0 V)");
}

void setup(){
  pinMode(MOTOR_ENABLE_PIN,OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN,LOW);
  Serial.begin(115200);
  delay(800);
  Serial.println("MKS RGB diagnostic - MOTOR DRIVER DISABLED");
  Serial.println("1/2/3=LED timing 0=off s=scan h=GPIO2 high l=GPIO2 low");
}

void loop(){
  digitalWrite(MOTOR_ENABLE_PIN,LOW);
  if(!Serial.available())return;
  char command=Serial.read();
  while(Serial.available())Serial.read();
  if(command>='1'&&command<='3')testDefault(command-'1');
  else if(command=='0')allOff();
  else if(command=='s'||command=='S')scanAll();
  else if(command=='h'||command=='H')setStaticLevel(true);
  else if(command=='l'||command=='L')setStaticLevel(false);
}
