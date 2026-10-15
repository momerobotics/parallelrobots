"""Compile actual firmware spring/profile definitions and check their behavior."""
from pathlib import Path
import re
import subprocess
import tempfile

source = (Path(__file__).resolve().parents[2] / 'firmware/MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino').read_text()
constants = '\n'.join(line for line in source.splitlines() if line.startswith('static const float ') and any(x in line for x in ['STATIC_FRICTION_RAMP=', 'DEAD_BAND=', 'STRONG_ENDSTOP_START=', 'STRONG_ENDSTOP_FULL=', 'POWER_STAGE_VOLTAGE_LIMIT=', 'BOOST_CURRENT_SOFT_LIMIT=', 'BOOST_OVERCURRENT_LIMIT=', 'CURRENT_SOFT_LIMIT=', 'OVERCURRENT_LIMIT=', 'FORCE_ATTACK_V_PER_S=', 'THERMAL_SOFT_CURRENT=', 'THERMAL_HARD_CURRENT=']))
profiles = source[source.index('enum ProfileId:'):source.index('static const bool MASTER_FOLLOWER_TEST')]
function = source[source.index('float springTorque('):source.index('void selectProfile(')]
current_limits = source[source.index('float thermalSoftLimit()'):source.index('void printMac(')]
progressive = re.search(r'bool progressiveProfile\(\)\{[^\n]+', source).group(0)
runtime_limits = source[source.index('void configureRuntimeVoltageLimits()'):source.index('bool sameMac(')]
setup = source[source.index('void setup()'):source.index('void loop()')]
assert setup.index('configureRuntimeVoltageLimits();') > setup.index('motor.init()')
hard_cut = source[source.index('        if(currentSenseReady&&instantCurrentPeak>=hardCurrent)torqueVoltage=0;'):source.index('        motor.move(torqueVoltage);', source.index('        if(currentSenseReady&&instantCurrentPeak>=hardCurrent)torqueVoltage=0;'))]
code = r'''
#include <cmath>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <cstdint>
using std::min; using std::max;
template<class T> T constrain(T x,T lo,T hi){return max(lo,min(hi,x));}
''' + constants + '\n' + profiles + '\nuint8_t activeProfile=0;\n' + progressive + '\n' + current_limits + '\n' + function + r'''
struct { float voltage_limit=1, voltage_power_supply=11.8f; } driver;
struct FakeMotor {
 float voltage_limit=1, controller_limit=1;
 void updateVoltageLimit(float v){voltage_limit=controller_limit=v;}
} motor;
''' + runtime_limits + r'''
float hardCut(float torqueVoltage, float instantCurrentPeak, float requested=0){
 bool currentSenseReady=true; float hardCurrent=currentHardLimit(), step=.05f, dt=.001f;
''' + hard_cut + r'''
 return torqueVoltage;
}
''' + r'''
int main(){
 assert(motor.voltage_limit==1);
 configureRuntimeVoltageLimits();
 assert(driver.voltage_limit==7 && motor.voltage_limit==3.5f && motor.controller_limit==3.5f);
 assert(BOOST_CURRENT_SOFT_LIMIT==1.10f && BOOST_OVERCURRENT_LIMIT==1.35f);
 assert(hardCut(2.f,BOOST_OVERCURRENT_LIMIT+.01f)==0);
 assert(fabsf(hardCut(2.f,.3f)-1.95f)<1e-6f);
 assert(HANDOFF_PROFILE==PROFILE_BOOST);
 for(activeProfile=0;activeProfile<PROFILE_COUNT;++activeProfile){
  const auto &p=PROFILES[activeProfile];
  assert(springTorque(0)==0 && springTorque(p.deadBand)==0);
  float previous=0;
  for(int i=0;i<=4000;++i){
   float angle=i*.001f, output=-springTorque(angle);
   assert(std::isfinite(output) && output>=0 && output<=p.voltageLimit+1e-6f);
   assert(fabsf(springTorque(-angle)-output)<1e-5f);
   assert(output+1e-5f>=previous); previous=output;
  }
 }
 activeProfile=PROFILE_BOOST;
 for(float degrees:{5.f,10.f,20.f}){
  float angle=degrees*3.14159265359f/180, oldTravel=angle-.025f;
  float old=.16f*(1-expf(-oldTravel/.060f))+1.42f*powf(oldTravel,1.2f);
  assert(-springTorque(angle)>3*old);
  std::cout<<degrees<<" deg: "<<old<<" V -> "<<-springTorque(angle)<<" V\n";
 }
 activeProfile=PROFILE_LEGACY;
 assert(springTorque(.009f)==0);
 for(float angle:{.02f,.1f,.5f,1.f,2.f})
  assert(fabsf(springTorque(angle)+std::min(2.f*angle,3.5f))<1e-6f);
 activeProfile=PROFILE_FORCE;
 assert(hardCut(3.f,.3f,0.f)==0.f); // Release has no stale torque tail.
 assert(hardCut(3.f,.3f,-2.f)<0.f); // Reversal never keeps the previous sign.
 assert(softenForceRequest(3.f,0.f,.001f)==0.f);
 assert(softenForceRequest(3.f,1.f,.001f)==1.f);
 assert(fabsf(softenForceRequest(0.f,5.5f,.001f)-.08f)<1e-6f);
 assert(fabsf(softenForceRequest(3.f,-2.f,.001f)+.08f)<1e-6f);
 float attack=0;
 for(int i=0;i<69;++i)attack=softenForceRequest(attack,5.5f,.001f);
 assert(fabsf(attack-5.5f)<1e-5f);
 for(float previous:{-5.5f,-1.f,0.f,1.f,5.5f})
  for(float requested:{-5.5f,-1.f,0.f,1.f,5.5f}){
   float v=softenForceRequest(previous,requested,.001f);
   assert(v*requested>=0 && fabsf(v)<=fabsf(requested)+1e-6f);
  }
 assert(thermalFoldback(.84f)==1.f);
 assert(fabsf(thermalFoldback(1.00f)-.5f)<1e-5f);
 assert(thermalFoldback(1.10f)==0.f);
 assert(fabsf(springTorque(3.14159265359f/3)+5.5f)<1e-5f);
 const float rad=3.14159265359f/180.f;
 assert(fabsf(springTorque(10.f*rad)+3.74f)<1e-5f);
 assert(fabsf(springTorque(30.f*rad)+4.444f)<1e-5f);
 assert(fabsf(springTorque(20.f*rad)+4.092f)<1e-5f);
 assert(-springTorque(10.f*rad-.000001f)<-springTorque(10.f*rad+.000001f));
 assert(fabsf(springTorque(10.f*rad-.000001f)-springTorque(10.f*rad+.000001f))<.0001f);
 assert(fabsf(springTorque(PROFILES[activeProfile].deadBand+.000001f))<.0001f);
 assert(currentSoftLimit()==1.75f && currentHardLimit()==2.00f);
 assert(hardCut(3.f,currentHardLimit()+.01f,-2.f)==0.f);
 configureRuntimeVoltageLimits();
 assert(motor.voltage_limit==5.5f && driver.voltage_limit==11.8f);
 assert(springTorque(2.f)==-5.5f);
 driver.voltage_power_supply=10.f;configureRuntimeVoltageLimits();
 assert(motor.voltage_limit==5.f);
 activeProfile=PROFILE_LEGACY;configureRuntimeVoltageLimits();
 assert(thermalFoldback(.6f)==0.f && thermalFoldback(.45f)==1.f);
 assert(motor.voltage_limit==3.5f && driver.voltage_limit==7.f);
 assert(PROFILES[PROFILE_GENTLE].voltageLimit==.75f);
 assert(PROFILES[PROFILE_BOOST].voltageLimit==3.5f);
 std::cout<<"PASS: runtime motor limit restored after init; immediate hard-current cut; bounded symmetric monotonic profiles; 10-degree knee; no error filter; current and thermal limits.\n";
}
'''
with tempfile.TemporaryDirectory(prefix='parallel-profile-') as tmp:
    cpp = Path(tmp)/'test.cpp'
    cpp.write_text(code)
    binary = Path(tmp)/'test'
    subprocess.run(['clang++','-std=c++17','-Wall','-Wextra','-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1',str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
