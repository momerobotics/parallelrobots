"""Exercise actual peer-run handling and disarm code with deterministic time."""
from pathlib import Path
import subprocess,tempfile
s=(Path(__file__).resolve().parents[2]/'firmware/MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino').read_text()
start=s.index('  if(age<=TIMEOUT_MS){',s.index('void loop()'))
peer=s[start:s.index('  if(armed){',start)]
disarm=s[s.index('void disarm('):s.index('void updateCurrentMonitor(')]
constants='\n'.join(l for l in s.splitlines() if l.startswith('static const ') and any(x in l for x in ['SEND_US=', 'AUTO_START_DELAY_MS=', 'FOC_READY=', 'HANDOFF_MODE=']))
code=r"""
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
uint32_t nowMs=1000;
uint32_t millis(){return nowMs;}
struct SerialStub {template<class T> void print(T){} template<class T> void println(T){}} Serial;
struct MotorStub {void move(float){} void disable(){}} motor;
float torqueVoltage=0,positionIntegral=0;
bool armed=false,autoStartEnabled=false,manualStopLatched=false,lowBattery=false;
bool peerRunKnown=false,peerRunLast=false;
uint32_t nextAutoArmMs=0,peerRestartBlockedUntilMs=0,runIntentGraceUntilMs=0;
char lastDisarmReason[64]={};
void strlcpy(char*d,const char*s,unsigned n){std::snprintf(d,n,"%s",s);}
"""+constants+'\n'+disarm+r"""
void receive(bool run,uint32_t age=0){
 struct {uint8_t flags;}p={uint8_t(run?RUN_ENABLED:0)};
"""+peer+r"""
}
int main(){
 receive(false);
 receive(true);
 assert(autoStartEnabled && runIntentGraceUntilMs==nowMs+500);
 armed=true;
 nowMs+=20;receive(false); // Older stop during startup must not cancel peer startup.
 assert(armed && autoStartEnabled);
 nowMs+=20;receive(true);
 nowMs+=501;receive(false); // A real stop after grace must still win.
 assert(!armed && !autoStartEnabled);
 nowMs+=10;receive(true); // In-flight run echo must not immediately rearm.
 assert(!armed && !autoStartEnabled);
 nowMs+=TIMEOUT_MS+1;receive(false);receive(true);
 assert(autoStartEnabled); // A later fresh explicit start can be accepted.
 armed=true;receive(false,TIMEOUT_MS+1);
 assert(armed); // Stale packets are ignored here; outer loop handles link timeout.
 puts("PASS: symmetric startup grace; peer stop; post-stop run-echo suppression; later restart.");
}
"""
with tempfile.TemporaryDirectory(prefix='parallel-peer-start-') as t:
 cpp=Path(t)/'test.cpp';binary=Path(t)/'test';cpp.write_text(code)
 subprocess.run(['clang++','-std=c++17','-Wall','-Wextra','-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1',str(cpp),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
