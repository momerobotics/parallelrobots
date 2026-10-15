#include <Arduino.h>
#include <SPI.h>
#include <SimpleFOC.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>

static const int PWM_A=32, PWM_B=33, PWM_C=25, EN=12;
static const int CURRENT_A_PIN=39, CURRENT_B_PIN=36;
static const int SCK_PIN=18, MISO_PIN=19, MOSI_PIN=23, CS_PIN=5, VIN_PIN=13;
// iPower GM3506 specification: 24N/22P, therefore 11 rotor pole pairs.
static const int POLE_PAIRS=11;
// Calibrated against both prototype boards at their input terminals.
static const float VIN_SCALE=8.29f, MIN_VIN=11.1f;
static const float LIPO_EMPTY_VOLTAGE=10.5f, LIPO_FULL_VOLTAGE=12.6f;
static const float ALIGN_VOLTAGE=1.0f;
static const float CALIBRATION_ALIGN_VOLTAGE=1.5f;
static const float POWER_STAGE_VOLTAGE_LIMIT=7.0f;
static const long PWM_FREQUENCY_HZ=40000;
static const float STATIC_FRICTION_RAMP=0.060f;
static const float INTEGRAL_DECAY_V_PER_S=0.80f;
static const float DEAD_BAND=0.025f, SOFTEN_EXP=1.20f;
static const float VELOCITY_FILTER_TF=0.020f;
static const float TORQUE_SLEW_V_PER_S=6.0f;
static const float FOLDBACK_SLEW_V_PER_S=30.0f;
static const float SPEED_SOFT_LIMIT=4.0f, SPEED_HARD_LIMIT=7.0f;
static const float MAX_CONTROL_ERROR=3.50f;
static const float STRONG_ENDSTOP_START=0.90f;
static const float STRONG_ENDSTOP_FULL=3.50f;
static const float CURRENT_SOFT_LIMIT=0.40f;
static const float OVERCURRENT_LIMIT=0.65f;
static const float BOOST_CURRENT_SOFT_LIMIT=0.52f;
static const float BOOST_OVERCURRENT_LIMIT=0.70f;
static const float CURRENT_ENVELOPE_RELEASE_TF=0.080f;
static const float THERMAL_ESTIMATE_TF=45.0f;
static const float THERMAL_SOFT_CURRENT=0.45f;
static const float THERMAL_HARD_CURRENT=0.60f;
// Supervised handoff build: deterministic startup and latched runtime faults.
static const bool HANDOFF_MODE=true;
static const uint32_t HANDOFF_SESSION_MS=10UL*60UL*1000UL;
static const uint8_t CHANNEL=1;
static const uint32_t SEND_US=2000, TIMEOUT_MS=250, ARM_SYNC_MS=150;
static const uint32_t AUTO_START_DELAY_MS=5000, AUTO_RETRY_MS=1000;
static const uint32_t MAGIC=0x50524F42;
static const uint16_t PROTOCOL_VERSION=7;
static const uint8_t FOC_READY=1, ARMED=2, RUN_ENABLED=4;
enum ProfileId:uint8_t {
  PROFILE_GENTLE=0,PROFILE_NORMAL=1,PROFILE_STRONG=2,PROFILE_BOOST=3,PROFILE_COUNT=4
};
static const uint8_t HANDOFF_PROFILE=PROFILE_BOOST;
struct ControlProfile {
  const char*name;
  float voltageLimit,springK,frictionComp,positionI,integralLimit;
  float integralEnableError,viscousDamping,couplingDamping;
};
static const ControlProfile PROFILES[PROFILE_COUNT]={
  {"gentle",0.75f,0.80f,0.07f,0.10f,0.12f,0.10f,0.020f,0.025f},
  {"normal",1.20f,1.20f,0.10f,0.15f,0.18f,0.075f,0.025f,0.025f},
  {"strong",2.20f,1.30f,0.15f,0.24f,0.26f,0.060f,0.020f,0.040f},
  {"boost",2.50f,1.42f,0.16f,0.26f,0.28f,0.055f,0.020f,0.045f}
};
static const bool MASTER_FOLLOWER_TEST=false;
static const char MASTER_NODE='A';
static const bool USE_STORED_FOC_CALIBRATION=true;
static const float ZERO_ELECTRIC_A=0.630083f;
static const float ZERO_ELECTRIC_B=5.940915f;
static const uint8_t MAC_A[6]={0x30,0xC9,0x22,0x5F,0x5D,0x1C};
static const uint8_t MAC_B[6]={0x30,0xC9,0x22,0x5F,0x5D,0x24};

struct __attribute__((packed)) Packet {
  uint32_t magic, sequence;
  uint16_t version;
  float position, velocity;
  uint8_t flags, node, profile;
};

MagneticSensorSPI sensor(AS5048_SPI, CS_PIN, 1000000);
BLDCMotor motor(POLE_PAIRS);
BLDCDriver3PWM driver(PWM_A,PWM_B,PWM_C,EN);
InlineCurrentSense currentSense(0.01f,50.0f,CURRENT_A_PIN,CURRENT_B_PIN);
Preferences preferences;

uint8_t localMac[6]={}, peerMac[6]={};
char node='?';
uint8_t activeProfile=PROFILE_NORMAL;
float bootVin=0, zeroPosition=0, torqueVoltage=0;
bool radioReady=false, focReady=false, armed=false, autoStartEnabled=true;
bool currentSenseReady=false;
bool lowBattery=false;
bool currentFoldbackActive=false;
bool profileStorageReady=false;
bool manualStopLatched=false;
bool peerRunKnown=false, peerRunLast=false;
char lastDisarmReason[32]="boot";
volatile Packet remotePacket={};
volatile uint32_t lastRxMs=0, rxCount=0;
portMUX_TYPE packetMux=portMUX_INITIALIZER_UNLOCKED;
uint32_t sequence=0, txCount=0, txFailures=0, lastSendUs=0, lastPrintMs=0;
uint32_t currentLimitEvents=0;
float filteredVelocity=0, velocityPosition=0;
float positionIntegral=0;
float phaseCurrentA=0, phaseCurrentB=0, phaseCurrentC=0;
float filteredCurrentPeak=0, instantCurrentPeak=0, maxCurrentSinceArm=0;
float activeCurrentScale=1.0f, activeSpeedScale=1.0f;
float thermalCurrentSq=0, estimatedThermalCurrent=0, activeThermalScale=1.0f;
uint32_t velocityUpdateUs=0, controlUpdateUs=0, armedAtMs=0, nextAutoArmMs=0;
uint32_t currentMonitorUs=0;
uint32_t runIntentGraceUntilMs=0;

bool sameMac(const uint8_t*a,const uint8_t*b){return memcmp(a,b,6)==0;}
float readVin(){
  uint32_t millivolts=0;
  for(int i=0;i<64;i++){
    millivolts+=analogReadMilliVolts(VIN_PIN);
    delayMicroseconds(200);
  }
  return (millivolts/64.0f)*VIN_SCALE/1000.0f;
}
float batteryLevel(){
  return constrain((bootVin-LIPO_EMPTY_VOLTAGE)/
                   (LIPO_FULL_VOLTAGE-LIPO_EMPTY_VOLTAGE),0.0f,1.0f);
}
float coordinateSign(){return focReady?(float)motor.sensor_direction:1.0f;}
float position(){return coordinateSign()*(sensor.getAngle()-zeroPosition);}
float coordinateVelocity(){return coordinateSign()*sensor.getVelocity();}
bool progressiveProfile(){return activeProfile==PROFILE_STRONG||activeProfile==PROFILE_BOOST;}
float currentSoftLimit(){
  return activeProfile==PROFILE_BOOST?BOOST_CURRENT_SOFT_LIMIT:CURRENT_SOFT_LIMIT;
}
float currentHardLimit(){
  return activeProfile==PROFILE_BOOST?BOOST_OVERCURRENT_LIMIT:OVERCURRENT_LIMIT;
}

void printMac(const uint8_t*m){
  for(int i=0;i<6;i++){
    if(i)Serial.print(':');
    if(m[i]<16)Serial.print('0');
    Serial.print(m[i],HEX);
  }
}

void onReceive(const esp_now_recv_info_t*info,const uint8_t*data,int len){
  if(len!=sizeof(Packet)||!sameMac(info->src_addr,peerMac))return;
  Packet p; memcpy(&p,data,sizeof(p));
  if(p.magic!=MAGIC||p.version!=PROTOCOL_VERSION)return;
  portENTER_CRITICAL(&packetMux);
  memcpy((void*)&remotePacket,&p,sizeof(p));
  lastRxMs=millis(); rxCount++;
  portEXIT_CRITICAL(&packetMux);
}

void onSend(const esp_now_send_info_t*,esp_now_send_status_t status){
  if(status!=ESP_NOW_SEND_SUCCESS)txFailures++;
}

Packet remoteState(uint32_t&age){
  Packet p; uint32_t at;
  portENTER_CRITICAL(&packetMux);
  memcpy(&p,(const void*)&remotePacket,sizeof(p)); at=lastRxMs;
  portEXIT_CRITICAL(&packetMux);
  age=at?millis()-at:UINT32_MAX;
  return p;
}

bool initRadio(){
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  esp_wifi_set_channel(CHANNEL,WIFI_SECOND_CHAN_NONE);
  esp_wifi_get_mac(WIFI_IF_STA,localMac);
  if(sameMac(localMac,MAC_A)){node='A';memcpy(peerMac,MAC_B,6);}
  else if(sameMac(localMac,MAC_B)){node='B';memcpy(peerMac,MAC_A,6);}
  else return false;
  if(esp_now_init()!=ESP_OK)return false;
  esp_now_register_recv_cb(onReceive); esp_now_register_send_cb(onSend);
  esp_now_peer_info_t peer={};
  memcpy(peer.peer_addr,peerMac,6); peer.channel=CHANNEL;
  peer.ifidx=WIFI_IF_STA; peer.encrypt=false;
  return esp_now_add_peer(&peer)==ESP_OK;
}

void sendState(){
  uint8_t flags=(focReady?FOC_READY:0)|(armed?ARMED:0)|
                (autoStartEnabled?RUN_ENABLED:0);
  Packet p={MAGIC,sequence++,PROTOCOL_VERSION,position(),filteredVelocity,
            flags,(uint8_t)node,activeProfile};
  if(esp_now_send(peerMac,(uint8_t*)&p,sizeof(p))==ESP_OK)txCount++;
  else txFailures++;
}

void disarm(const char*reason){
  torqueVoltage=0; positionIntegral=0;
  motor.move(0); motor.disable(); armed=false;
  nextAutoArmMs=millis()+AUTO_RETRY_MS;
  strlcpy(lastDisarmReason,reason,sizeof(lastDisarmReason));
  Serial.print("DISARMED: "); Serial.println(reason);
}

void updateCurrentMonitor(){
  if(!currentSenseReady)return;
  uint32_t now=micros();
  float dt=currentMonitorUs?(now-currentMonitorUs)*1e-6f:0.0f;
  PhaseCurrent_s currents=currentSense.getPhaseCurrents();
  phaseCurrentA=currents.a; phaseCurrentB=currents.b;
  phaseCurrentC=-(phaseCurrentA+phaseCurrentB);
  instantCurrentPeak=max(fabsf(phaseCurrentA),max(fabsf(phaseCurrentB),fabsf(phaseCurrentC)));
  if(!currentMonitorUs||instantCurrentPeak>=filteredCurrentPeak){
    filteredCurrentPeak=instantCurrentPeak;
  }else{
    float alpha=dt/(CURRENT_ENVELOPE_RELEASE_TF+dt);
    filteredCurrentPeak+=alpha*(instantCurrentPeak-filteredCurrentPeak);
  }
  if(dt>0){
    float alpha=dt/(THERMAL_ESTIMATE_TF+dt);
    thermalCurrentSq+=alpha*(instantCurrentPeak*instantCurrentPeak-thermalCurrentSq);
    estimatedThermalCurrent=sqrtf(max(thermalCurrentSq,0.0f));
    activeThermalScale=constrain((THERMAL_HARD_CURRENT-estimatedThermalCurrent)/
                                 (THERMAL_HARD_CURRENT-THERMAL_SOFT_CURRENT),0.0f,1.0f);
  }
  currentMonitorUs=now;
  if(armed)maxCurrentSinceArm=max(maxCurrentSinceArm,instantCurrentPeak);
}

void updateFilteredVelocity(){
  uint32_t now=micros();
  float p=position();
  if(!velocityUpdateUs){velocityUpdateUs=now;velocityPosition=p;return;}
  float dt=(now-velocityUpdateUs)*1e-6f;
  if(dt<0.0005f)return;
  float raw=(p-velocityPosition)/dt;
  float alpha=dt/(VELOCITY_FILTER_TF+dt);
  filteredVelocity+=alpha*(raw-filteredVelocity);
  velocityPosition=p;velocityUpdateUs=now;
}

float springTorque(float error){
  const ControlProfile&profile=PROFILES[activeProfile];
  float m=fabsf(error);
  if(m<=DEAD_BAND)return 0;
  float travel=m-DEAD_BAND;
  float friction=profile.frictionComp*(1.0f-expf(-travel/STATIC_FRICTION_RAMP));
  float springTravel=progressiveProfile()?
                     min(travel,STRONG_ENDSTOP_START):travel;
  float magnitude=friction+profile.springK*powf(springTravel,SOFTEN_EXP);
  if(progressiveProfile()&&travel>STRONG_ENDSTOP_START){
    float endstop=constrain((travel-STRONG_ENDSTOP_START)/
                            (STRONG_ENDSTOP_FULL-STRONG_ENDSTOP_START),0.0f,1.0f);
    magnitude+=(profile.voltageLimit-magnitude)*endstop*endstop;
  }
  float spring=(error>0?-1.0f:1.0f)*magnitude;
  return constrain(spring,-profile.voltageLimit,profile.voltageLimit);
}

void selectProfile(uint8_t selected){
  if(HANDOFF_MODE){
    Serial.println("Profile selection locked in handoff mode");
    return;
  }
  if(selected>=PROFILE_COUNT)return;
  uint8_t previous=activeProfile;
  activeProfile=selected;
  if(profileStorageReady&&activeProfile!=previous&&activeProfile!=PROFILE_BOOST)
    preferences.putUChar("profile",activeProfile);
  positionIntegral=0;torqueVoltage=0;
  maxCurrentSinceArm=0;
  currentLimitEvents=0;
  currentFoldbackActive=false;
  activeCurrentScale=1;
  activeSpeedScale=1;
  if(armed)motor.move(0);
  Serial.print("Profile selected: ");Serial.println(PROFILES[activeProfile].name);
}

void calibrate(){
  if(armed)disarm("calibration");
  if(bootVin<MIN_VIN){Serial.println("Calibration refused: low boot VIN");return;}
  Serial.println("FOC calibration starting; brief movement expected...");
  focReady=false;
  motor.sensor_direction=Direction::CCW;
  motor.zero_electric_angle=NOT_SET;
  motor.voltage_sensor_align=CALIBRATION_ALIGN_VOLTAGE;
  motor.enable();
  focReady=motor.initFOC()!=0;
  motor.disable();
  motor.voltage_sensor_align=ALIGN_VOLTAGE;
  sensor.update(); zeroPosition=sensor.getAngle();
  Serial.print("FOC calibration: ");
  Serial.println(focReady?"OK, motor disabled":"FAILED, motor disabled");
  if(focReady){
    Serial.print("direction=");Serial.print((int)motor.sensor_direction);
    Serial.print(" zeroE=");Serial.println(motor.zero_electric_angle,6);
  }
}

void arm(){
  uint32_t age; Packet p=remoteState(age);
  bool peerReady=p.flags&FOC_READY;
  bool profileMatch=p.profile==activeProfile;
  if(!focReady||!radioReady||!peerReady||!profileMatch||age>TIMEOUT_MS){
    Serial.print("Arm refused localFOC=");Serial.print(focReady);
    Serial.print(" peerFOC=");Serial.print(peerReady);
    Serial.print(" profileMatch=");Serial.print(profileMatch);
    Serial.print(" ageMs=");Serial.println(age);return;
  }
  zeroPosition=sensor.getAngle();
  filteredVelocity=0; positionIntegral=0; maxCurrentSinceArm=0;
  currentLimitEvents=0;
  currentFoldbackActive=false;
  activeCurrentScale=1; activeSpeedScale=1;
  velocityUpdateUs=0; controlUpdateUs=micros();
  armedAtMs=millis();
  if(!MASTER_FOLLOWER_TEST||node!=MASTER_NODE)motor.enable();
  armed=true;
  strlcpy(lastDisarmReason,"none",sizeof(lastDisarmReason));
  if(MASTER_FOLLOWER_TEST&&node==MASTER_NODE)
    Serial.println("ARMED as sensor-only master; driver remains disabled.");
  else Serial.println("ARMED locally; torque waits for peer arm.");
}

void commands(){
  if(!Serial.available())return;
  char c=Serial.read(); while(Serial.available())Serial.read();
  if(c=='c'||c=='C'){
    if(HANDOFF_MODE)Serial.println("Calibration locked in handoff mode");
    else calibrate();
  }
  else if(c=='a'||c=='A'){
    if(lowBattery){Serial.println("Start refused: low boot VIN");return;}
    manualStopLatched=false;autoStartEnabled=true;
    runIntentGraceUntilMs=millis()+150;nextAutoArmMs=millis();
    Serial.println("Coordinated start requested");
  }
  else if(c=='x'||c=='X'){
    manualStopLatched=true;autoStartEnabled=false;runIntentGraceUntilMs=0;
    disarm("manual stop");
  }
  else if(c=='z'||c=='Z'){
    if(HANDOFF_MODE)Serial.println("Manual zero locked in handoff mode");
    else{zeroPosition=sensor.getAngle();Serial.println("Position zeroed");}
  }
  else if(c=='g'||c=='G')selectProfile(PROFILE_GENTLE);
  else if(c=='n'||c=='N')selectProfile(PROFILE_NORMAL);
  else if(c=='s'||c=='S')selectProfile(PROFILE_STRONG);
  else if(c=='b'||c=='B')selectProfile(PROFILE_BOOST);
  else if(c=='?'){
    if(HANDOFF_MODE)Serial.println("HANDOFF: a=arm x=disarm; tuning commands locked");
    else Serial.println("c=calibrate z=zero a=arm x=disarm g=gentle n=normal s=strong b=boost");
  }
}

void status(){
  uint32_t age; Packet p=remoteState(age);
  Serial.print("node=");Serial.print(node);
  Serial.print(" foc=");Serial.print(focReady);
  Serial.print(" armed=");Serial.print(armed);
  Serial.print(" auto=");Serial.print(autoStartEnabled);
  Serial.print(" stopLatch=");Serial.print(manualStopLatched);
  Serial.print(" lastStop=");Serial.print(lastDisarmReason);
  Serial.print(" bootVin=");Serial.print(bootVin,2);
  Serial.print(" battery=");Serial.print(batteryLevel()*100.0f,0);
  Serial.print(" profile=");Serial.print(PROFILES[activeProfile].name);
  Serial.print(" handoff=");Serial.print(HANDOFF_MODE);
  Serial.print(" protocol=");Serial.print(PROTOCOL_VERSION);
  Serial.print(" peerProtocol=");Serial.print(p.version);
  Serial.print(" peerProfile=");
  if(p.profile<PROFILE_COUNT)Serial.print(PROFILES[p.profile].name);else Serial.print("invalid");
  Serial.print(" peerFOC=");Serial.print((p.flags&FOC_READY)!=0);
  Serial.print(" peerArmed=");Serial.print((p.flags&ARMED)!=0);
  Serial.print(" local=");Serial.print(position(),3);
  Serial.print(" remote=");Serial.print(p.position,3);
  Serial.print(" torqueV=");Serial.print(torqueVoltage,3);
  Serial.print(" currentA=");Serial.print(phaseCurrentA,3);
  Serial.print(" currentB=");Serial.print(phaseCurrentB,3);
  Serial.print(" currentC=");Serial.print(phaseCurrentC,3);
  Serial.print(" currentPk=");Serial.print(filteredCurrentPeak,3);
  Serial.print(" currentMax=");Serial.print(maxCurrentSinceArm,3);
  Serial.print(" currentScale=");Serial.print(activeCurrentScale,2);
  Serial.print(" thermalA=");Serial.print(estimatedThermalCurrent,3);
  Serial.print(" thermalScale=");Serial.print(activeThermalScale,2);
  Serial.print(" speedScale=");Serial.print(activeSpeedScale,2);
  Serial.print(" limitEvents=");Serial.print(currentLimitEvents);
  Serial.print(" velocity=");Serial.print(filteredVelocity,2);
  Serial.print(" ageMs=");if(age==UINT32_MAX)Serial.print("never");else Serial.print(age);
  Serial.print(" txFail=");Serial.print(txFailures);
  Serial.print(" rx=");Serial.println(rxCount);
}

void setup(){
  Serial.begin(115200);delay(800);
  profileStorageReady=preferences.begin("parallel",false);
  if(HANDOFF_MODE){
    activeProfile=HANDOFF_PROFILE;
  }else if(profileStorageReady){
    uint8_t stored=preferences.getUChar("profile",PROFILE_NORMAL);
    activeProfile=stored<=PROFILE_STRONG?stored:PROFILE_NORMAL;
  }
  analogReadResolution(12);
  analogSetPinAttenuation(VIN_PIN,ADC_11db);
  bootVin=readVin();lowBattery=bootVin<MIN_VIN;
  if(lowBattery)autoStartEnabled=false;
  SPI.begin(SCK_PIN,MISO_PIN,MOSI_PIN,CS_PIN);
  sensor.init(&SPI);sensor.update();zeroPosition=sensor.getAngle();
  driver.voltage_power_supply=max(bootVin,9.0f);
  driver.voltage_limit=ALIGN_VOLTAGE;driver.pwm_frequency=PWM_FREQUENCY_HZ;
  if(!driver.init()){Serial.println("Driver init failed");while(true)delay(1000);}
  driver.disable();
  currentSenseReady=currentSense.init()!=0;
  motor.linkSensor(&sensor);motor.linkDriver(&driver);
  motor.controller=MotionControlType::torque;
  motor.torque_controller=TorqueControlType::voltage;
  motor.foc_modulation=FOCModulationType::SinePWM;
  motor.voltage_limit=PROFILES[PROFILE_BOOST].voltageLimit;
  motor.voltage_sensor_align=ALIGN_VOLTAGE;
  motor.velocity_limit=20;
  if(!motor.init()){Serial.println("Motor init failed");while(true)delay(1000);}
  motor.disable();radioReady=initRadio();
  if(USE_STORED_FOC_CALIBRATION&&radioReady){
    float zeroElectric=node=='A'?ZERO_ELECTRIC_A:ZERO_ELECTRIC_B;
    motor.zero_electric_angle=zeroElectric;
    motor.sensor_direction=Direction::CCW;
    focReady=motor.initFOC()!=0;
    motor.disable();sensor.update();zeroPosition=sensor.getAngle();
  }
  driver.voltage_limit=POWER_STAGE_VOLTAGE_LIMIT;
  nextAutoArmMs=millis()+AUTO_START_DELAY_MS;
  Serial.println("Parallel Robots MKS mirror - autonomous foldback build");
  Serial.print("Node ");Serial.print(node);Serial.print(" MAC ");printMac(localMac);
  Serial.print(" peer ");printMac(peerMac);Serial.println();
  Serial.print("bootVin=");Serial.print(bootVin,2);
  Serial.print(" battery=");Serial.print(batteryLevel()*100.0f,0);Serial.print("%");
  Serial.print(" radio=");Serial.println(radioReady?"OK":"ERROR");
  Serial.print("storedFOC=");Serial.println(focReady?"OK":"NOT READY");
  Serial.print("currentSense=");Serial.println(currentSenseReady?"OK":"ERROR");
  Serial.print("profile=");Serial.println(PROFILES[activeProfile].name);
  if(HANDOFF_MODE){
    Serial.print("HANDOFF MODE: fixed profile, session limit seconds=");
    Serial.println(HANDOFF_SESSION_MS/1000UL);
  }
  if(lowBattery)Serial.println("LOW BATTERY: automatic arm inhibited.");
  Serial.println("Motor disabled during startup; automatic peer-synchronized arm enabled.");
}

void loop(){
  commands();
  if(focReady)motor.loopFOC();else sensor.update();
  updateCurrentMonitor();
  updateFilteredVelocity();
  uint32_t age;Packet p=remoteState(age);
  if(age<=TIMEOUT_MS){
    bool peerRun=p.flags&RUN_ENABLED;
    bool peerRunRising=peerRunKnown&&!peerRunLast&&peerRun;
    peerRunKnown=true;
    peerRunLast=peerRun;
    bool localStarting=(int32_t)(runIntentGraceUntilMs-millis())>0;
    if(peerRunRising&&!lowBattery&&(!HANDOFF_MODE||!manualStopLatched)){
      manualStopLatched=false;autoStartEnabled=true;
      nextAutoArmMs=millis();
    }else if(!peerRun&&!localStarting){
      autoStartEnabled=false;
      if(HANDOFF_MODE)manualStopLatched=true;
      if(armed)disarm("peer stop");
    }
  }
  if(armed){
    if(HANDOFF_MODE&&millis()-armedAtMs>=HANDOFF_SESSION_MS){
      manualStopLatched=true;autoStartEnabled=false;runIntentGraceUntilMs=0;
      disarm("handoff session complete");
    }
    else if(age>TIMEOUT_MS){
      if(HANDOFF_MODE){
        manualStopLatched=true;autoStartEnabled=false;runIntentGraceUntilMs=0;
      }
      disarm("link timeout");
    }
    else if(MASTER_FOLLOWER_TEST&&node==MASTER_NODE){
      torqueVoltage=0;
    }
    else{
      const ControlProfile&profile=PROFILES[activeProfile];
      float error=position()-p.position;
      bool synchronized=(p.flags&ARMED)&&p.profile==activeProfile&&
                        (millis()-armedAtMs>=ARM_SYNC_MS);
      if(!synchronized){
        positionIntegral=0; torqueVoltage=0;
        activeCurrentScale=1; activeSpeedScale=1; motor.move(0);
      }
      else{
        uint32_t now=micros();
        float dt=(now-controlUpdateUs)*1e-6f; controlUpdateUs=now;
        dt=constrain(dt,0.0f,0.02f);

        float controlledError=constrain(error,-MAX_CONTROL_ERROR,MAX_CONTROL_ERROR);
        float speed=fabsf(filteredVelocity);
        activeSpeedScale=constrain((SPEED_HARD_LIMIT-speed)/
                                   (SPEED_HARD_LIMIT-SPEED_SOFT_LIMIT),0.0f,1.0f);

        float softCurrent=currentSoftLimit();
        float hardCurrent=currentHardLimit();
        activeCurrentScale=currentSenseReady?
          constrain((hardCurrent-filteredCurrentPeak)/
                    (hardCurrent-softCurrent),0.0f,1.0f):1.0f;
        if(currentSenseReady&&instantCurrentPeak>=hardCurrent)
          activeCurrentScale=0.0f;
        if(!currentFoldbackActive&&activeCurrentScale<0.95f){
          currentFoldbackActive=true;
          currentLimitEvents++;
        }else if(currentFoldbackActive&&activeCurrentScale>0.99f){
          currentFoldbackActive=false;
        }
        float errorMagnitude=fabsf(error);
        if(errorMagnitude<=DEAD_BAND)positionIntegral=0;
        else if(errorMagnitude>=profile.integralEnableError&&
                activeCurrentScale>0.95f&&activeSpeedScale>0.95f)
          positionIntegral=constrain(positionIntegral-profile.positionI*controlledError*dt,
                                     -profile.integralLimit,profile.integralLimit);
        else{
          float decayStep=INTEGRAL_DECAY_V_PER_S*dt;
          positionIntegral+=constrain(-positionIntegral,-decayStep,decayStep);
        }

        float remoteVelocity=constrain(p.velocity,-SPEED_HARD_LIMIT,SPEED_HARD_LIMIT);
        float relativeVelocity=filteredVelocity-remoteVelocity;
        float drive=(springTorque(controlledError)+positionIntegral)*activeSpeedScale;
        float damping=profile.viscousDamping*filteredVelocity+
                      profile.couplingDamping*relativeVelocity;
        float requested=(drive-damping)*activeCurrentScale*activeThermalScale;
        requested=constrain(requested,-profile.voltageLimit,profile.voltageLimit);
        float slew=activeCurrentScale<0.95f?FOLDBACK_SLEW_V_PER_S:TORQUE_SLEW_V_PER_S;
        float step=slew*dt;
        torqueVoltage+=constrain(requested-torqueVoltage,-step,step);
        motor.move(torqueVoltage);
      }
    }
  }
  uint32_t us=micros();
  if(radioReady&&us-lastSendUs>=SEND_US){sendState();lastSendUs=us;}
  uint32_t ms=millis();
  if(!armed&&autoStartEnabled&&ms>=nextAutoArmMs){
    bool peerReady=(p.flags&FOC_READY)&&p.profile==activeProfile&&age<=TIMEOUT_MS;
    if(peerReady)arm();
    nextAutoArmMs=ms+AUTO_RETRY_MS;
  }
  if(ms-lastPrintMs>=500){status();lastPrintMs=ms;}
}
