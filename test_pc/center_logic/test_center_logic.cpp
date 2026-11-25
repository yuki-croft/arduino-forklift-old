#include <cstdio>
#include <cstring>
#include <string>
#include "Arduino.h"
PortStub Serial;
static int ledPin[20];
void pinMode(uint8_t,uint8_t){}
void digitalWrite(uint8_t p,uint8_t v){ ledPin[p]=v; }
// prototypes the Arduino IDE would generate automatically
void ledsOff(); void setLedIndex(int8_t); void initMasterHw(); void sendToForklift(char);
const __FlashStringHelper *msgText(uint8_t); void queuePhone(uint8_t,char,char);
void phoneSendEntry(uint8_t,char,char); void flushPhoneQueue();
void finishJob(); void selectLoad(char); void selectUnload(char); void startJob(); void handlePhoneByte(char);
uint8_t stateMsgId(char); void onSegment(int8_t); void onEmergency(); void onJobDone(); void handleForkliftByte(char);
void setup(); void loop();
#include SRC_MAIN
#include SRC_HW
#include SRC_LOGIC
static int fails=0;
static void expect(const char*name, bool ok){ printf("%s  %s\n", ok?"PASS":"FAIL", name); if(!ok) fails++; }
static void run(const std::string&phoneIn, const std::string&fwdIn){ phone.in+=phoneIn; Serial.in+=fwdIn; for(int i=0;i<20;i++) loop(); }
static std::string litLeds(){ std::string s; const int pins[4]={7,4,5,6}; for(int i=0;i<4;i++) if(ledPin[pins[i]]==LED_ON_LEVEL) s+=char('0'+i); return s; }
int main(){
  setup(); phone.out.clear(); Serial.out.clear();
  // A: normal job
  run("3B",""); expect("select 3 and B accepted", phone.out=="SEL LOAD 3\nSEL UNLOAD B\n");
  phone.out.clear(); run("S",""); expect("start sends '3' then 'B' to forklift", Serial.out=="3B");
  expect("JOB message", phone.out=="JOB 3>B\n");
  phone.out.clear(); run("","1"); expect("segment '1' lights LED idx0 only", litLeds()=="0");
  run("","2"); expect("segment '2' lights LED idx1 only", litLeds()=="1");
  run("","3"); expect("segment '3' lights LED idx2 only", litLeds()=="2");
  phone.out.clear(); run("1",""); expect("selection during job -> JOB RUNNING", phone.out=="JOB RUNNING\n");
  phone.out.clear(); run("","c"); expect("done: ST DONE sent, LEDs off", phone.out=="ST DONE\n" && litLeds()=="");
  phone.out.clear(); run("3",""); expect("load 3 now empty -> EMPTY 3", phone.out=="EMPTY 3\n");
  phone.out.clear(); run("B",""); expect("unload B now occupied -> BUSY B", phone.out=="BUSY B\n");
  // C: start without selection is ignored
  Serial.out.clear(); run("S",""); expect("start without selection ignored", Serial.out=="");
  // B: emergency job
  run("1A",""); phone.out.clear(); Serial.out.clear(); run("S",""); expect("job 1>A sent", Serial.out=="1A");
  run("","3"); expect("top segment LED idx2", litLeds()=="2");
  Serial.out.clear(); phone.out.clear(); run("","e");
  expect("emergency sends 'X'", Serial.out=="X");
  expect("emergency lights next LED (idx3) only, steady", litLeds()=="3");
  expect("EMERGENCY message", phone.out=="EMERGENCY\n");
  run("","4"); expect("segment report ignored during emergency", litLeds()=="3");
  run("","w"); expect("arrival 'w' ends emergency, LEDs off", litLeds()=="");
  phone.out.clear(); run("1A",""); expect("occupancy unchanged after emergency (1 and A selectable)", phone.out=="SEL LOAD 1\nSEL UNLOAD A\n");
  // F: emergency AFTER pickup -> only the loading side is kept; last lit LED is remembered across jobs
  Serial.out.clear(); phone.out.clear(); run("S",""); expect("job 1>A started again", Serial.out=="1A");
  run("","b"); expect("'b' marks cargo as picked (no LED change)", litLeds()=="");
  Serial.out.clear(); run("","e");
  expect("emergency sends 'X' again", Serial.out=="X");
  expect("next LED after the most recently lit one (was idx3 -> idx0)", litLeds()=="0");
  run("","w"); expect("arrival ends emergency, LEDs off", litLeds()=="");
  phone.out.clear(); run("1",""); expect("load 1 stays emptied after pickup+emergency", phone.out=="EMPTY 1\n");
  phone.out.clear(); run("A",""); expect("unload A stays free (only loading kept)", phone.out=="SEL UNLOAD A\n");
  // D: other forklift state codes
  phone.out.clear(); run("","l"); expect("'l' -> ST TO LOAD", phone.out=="ST TO LOAD\n");
  phone.out.clear(); run("","n"); expect("'n' -> LINE LOST", phone.out=="LINE LOST\n");
  phone.out.clear(); run("","?"); expect("unknown byte ignored", phone.out=="");
  // E: queue not flushed while forklift bytes pending
  phone.out.clear(); Serial.in="zzz"; queuePhone(MSG_ST_WAIT,0,0); flushPhoneQueue();
  expect("phone text held back while forklift RX pending", phone.out=="");
  printf("fails=%d\n", fails); return fails;
}
