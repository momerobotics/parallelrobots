"""Check actual send/callback functions cannot queue multiple ESP-NOW packets."""
from pathlib import Path
import subprocess,tempfile
s=(Path(__file__).resolve().parents[2]/'firmware/MKS_Parallel_Mirror/MKS_Parallel_Mirror.ino').read_text()
send=s[s.index('void sendState()'):s.index('void disarm(')]
callback=s[s.index('void onSend('):s.index('Packet remoteState(')]
packet=s[s.index('struct __attribute__((packed)) Packet'):s.index('MagneticSensorSPI sensor(')]
code=r"""
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstring>
struct esp_now_send_info_t {};
using esp_now_send_status_t=int;
const int ESP_OK=0,ESP_NOW_SEND_SUCCESS=0;
const uint8_t FOC_READY=1,ARMED=2,RUN_ENABLED=4;
const uint32_t MAGIC=123;const uint16_t PROTOCOL_VERSION=7;
bool focReady=true,armed=false,autoStartEnabled=false;
volatile bool txInFlight=false;
uint32_t sequence=0,txCount=0,txFailures=0;
uint8_t peerMac[6]={},activeProfile=5;char node='A';float filteredVelocity=0;
float pos=0;float position(){return pos;}
int sendResult=ESP_OK,calls=0;
"""+packet+r"""
Packet last{};
int esp_now_send(const uint8_t*,const uint8_t*data,unsigned){
 ++calls;std::memcpy(&last,data,sizeof(last));return sendResult;
}
"""+callback+send+r"""
int main(){
 sendState();assert(txInFlight&&calls==1&&txCount==1);
 pos=2;sendState();sendState();assert(calls==1);
 onSend(nullptr,ESP_NOW_SEND_SUCCESS);sendState();assert(calls==2&&last.position==2);
 onSend(nullptr,1);assert(!txInFlight&&txFailures==1);
 sendResult=1;sendState();assert(!txInFlight&&txFailures==2);
 sendResult=ESP_OK;sendState();assert(txInFlight&&calls==4);
 puts("PASS: one packet in flight; newest state after completion; failure unlocks sender.");
}
"""
with tempfile.TemporaryDirectory(prefix='parallel-radio-') as t:
 cpp=Path(t)/'test.cpp';binary=Path(t)/'test';cpp.write_text(code)
 subprocess.run(['clang++','-std=c++17','-isystem','/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk/usr/include/c++/v1',str(cpp),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
