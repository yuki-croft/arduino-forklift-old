// PC test: the single-file center copy (test_final_center_3s.ino) fed with the report letters of the 3 s forklift test.
#include <cstdio>
#include <string>
#include "Arduino.h"
PortStub Serial;
static int ledPin[20];
void pinMode(uint8_t,uint8_t){}
void digitalWrite(uint8_t p,uint8_t v){ ledPin[p]=v; }
void ledsOff(); void setLedIndex(int8_t); void initMasterHw(); void sendToForklift(char);
const __FlashStringHelper *msgText(uint8_t); void queuePhone(uint8_t,char,char);
void phoneSendEntry(uint8_t,char,char); void flushPhoneQueue();
void finishJob(); void selectLoad(char); void selectUnload(char); void startJob(); void handlePhoneByte(char);
uint8_t stateMsgId(char); void onSegment(int8_t); void onEmergency(); void settleOccupancy(); void onJobDone(); void handleForkliftByte(char);
void setup(); void loop();
#include SRC_CENTER
static int fails=0;
static void expect(const char*name, bool ok){ printf("%s  %s\n", ok?"PASS":"FAIL", name); if(!ok) fails++; }
static void run(const std::string&phoneIn, const std::string&fwdIn){ phone.in+=phoneIn; Serial.in+=fwdIn; for(int i=0;i<30;i++) loop(); }
int main(){
  setup(); phone.out.clear(); Serial.out.clear();
  run("1A",""); phone.out.clear();
  run("S",""); expect("start sends '1' then 'A' to the forklift", Serial.out=="1A");
  phone.out.clear(); run("","1laumdcw");   // exactly what test_final_forklift_3s.ino reports
  expect("LEDs are off after 'c'", ledPin[7]!=HIGH && ledPin[4]!=HIGH && ledPin[5]!=HIGH && ledPin[6]!=HIGH);
  expect("phone sees the whole flow in order",
         phone.out=="ST TO LOAD\nST APPROACH\nST LIFT UP\nST TO UNLOAD\nST UNLOAD\nST DONE\nST WAIT\n");
  expect("occupancy updated: load 1 empty, unload A full", !loadFull[0] && unloadFull[0]);
  phone.out.clear(); run("","n"); expect("'n' alone is reported as LINE LOST", phone.out=="LINE LOST\n");
  printf("%s\n", fails ? "FAILED" : "all passed"); return fails?1:0;
}
