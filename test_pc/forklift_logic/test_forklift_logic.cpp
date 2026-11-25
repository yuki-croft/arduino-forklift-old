#include <cstdio>
#include <string>
#include <vector>
#include "Arduino.h"
// ---- fake time / pins (PC test only) ----
static unsigned long fakeNow = 0;
unsigned long millis(){ return fakeNow; }
void pinMode(uint8_t,uint8_t){} int digitalRead(uint8_t){return 1;} int analogRead(uint8_t){return 0;}
void delay(unsigned long){} void tone(uint8_t,unsigned int,unsigned long){}
static std::vector<std::string> ev;           // actions requested by the state machine
static bool gCorner=false,gJunction=false,gMarker=false,gCargo=false,gLineLost=false,gRear=false;
// prototypes the Arduino IDE would generate
#include "protos.inc"
#include SRC_MAIN
#include SRC_COMM
#include SRC_LOGIC
// ---- fake hardware / sensing layer (replaces forklift_hw.ino and forklift_sense.ino) ----
void initHardware(){}
void setWheels(int l,int r){ char b[40]; snprintf(b,sizeof b,"wheels %d %d",l,r); ev.push_back(b); }
void stopWheels(){ ev.push_back("stop"); }
void driveLineStep(){ ev.push_back("line"); }
void driveLightStep(){ ev.push_back("light"); }
void turnLeft(){ ev.push_back("turnLeft"); }
void turnRight(){ ev.push_back("turnRight"); }
void liftRun(int d){ ev.push_back(d>0?"liftUp":"liftDown"); }
void liftHalt(){ ev.push_back("liftHalt"); }
bool senseJunction(){ return gJunction; }
bool senseCorner(){ return gCorner; }
bool senseMarker(){ return gMarker; }
bool senseLineLost(){ return gLineLost; }
bool senseCargoAhead(){ return gCargo; }
bool rearObstaclePersistent(){ return gRear; }
// ---- test helpers ----
static int fails=0;
static void expect(const char*n,bool ok){ printf("%s  %s\n",ok?"PASS":"FAIL",n); if(!ok) fails++; }
static void step(int n=1){ for(int i=0;i<n;i++){ fakeNow+=10; loop(); } }
static void pulse(bool&f,int hold=1){ f=true; step(hold); f=false; step(1); }
static bool had(const char*s){ for(auto&e:ev) if(e==s) return true; return false; }
int main(){
  setup(); expect("boot reports 'w'", bt.out=="w");
  // cell mapping
  expect("load junction counts '1'..'6' = 3,2,1,3,2,1",
    loadJunctionCount(0)==3&&loadJunctionCount(1)==2&&loadJunctionCount(2)==1&&loadJunctionCount(3)==3&&loadJunctionCount(4)==2&&loadJunctionCount(5)==1);
  expect("unload junction counts A..F = 1,2,3,1,2,3",
    unloadJunctionCount(0)==1&&unloadJunctionCount(1)==2&&unloadJunctionCount(2)==3&&unloadJunctionCount(3)==1&&unloadJunctionCount(4)==2&&unloadJunctionCount(5)==3);
  expect("upper level = cells 0..2 only", cellIsUpper(0)&&cellIsUpper(2)&&!cellIsUpper(3)&&!cellIsUpper(5));
  // only one cell -> no start
  bt.in="4"; step(3); expect("load only: stays in WAIT", state==S_WAIT && bt.out=="w");
  bt.in="B"; step(2); expect("both cells received -> job starts ('1','l')", bt.out=="w1l" && state==S_TO_LOAD);
  bt.in="5"; step(2); expect("cell letters ignored during a job", loadCell==3);
  // reset and run the full job with load '3' (idx2), unload 'B' (idx1)
  finishJob(false); ev.clear(); bt.out.clear();          // abort the previous test job, back to WAIT
  bt.in="3B"; step(3);
  expect("job 3>B starts: reports '1' then 'l'", state==S_TO_LOAD && targetCount==1 && bt.out=="1l");
  bt.out.clear(); ev.clear();
  step(3); expect("tracing drives the line (no sensing -> line step)", had("line"));
  pulse(gCorner); expect("corner: turn right, report '2'", had("turnRight") && bt.out=="2" && segment==1);
  pulse(gJunction,3); expect("target junction reached: turn left, state APPROACH, report 'a'", had("turnLeft") && state==S_APPROACH && bt.out=="2a");
  gCargo=true; step(1); expect("cargo ahead: LIFT_PICK 'u'", bt.out=="2au" && state==S_LIFT_PICK);
  step(1); expect("pick done: REVERSE 'b', carrying", bt.out=="2aub" && carrying && state==S_REVERSE);
  gCargo=false; ev.clear(); step(2); expect("reverses straight", had("wheels -100 -100"));
  pulse(gJunction,3); expect("main line reached: turn right, TO_UNLOAD 'm', target = 2", had("turnRight") && state==S_TO_UNLOAD && targetCount==2 && bt.out=="2aubm");
  pulse(gCorner); pulse(gCorner); expect("two corners -> segments 2,3 reported '3','4'", bt.out=="2aubm34" && segment==3);
  pulse(gJunction,2); expect("1st junction on right side does not finish", state==S_TO_UNLOAD && junctionCount==1);
  pulse(gJunction,2); expect("2nd junction -> APPROACH, report 'd' (carrying)", state==S_APPROACH && bt.out=="2aubm34d");
  gCargo=true; step(1); expect("shelf ahead: DROP", state==S_DROP);
  step(1); expect("drop done: REVERSE 'b', no longer carrying", state==S_REVERSE && !carrying && dropDone && bt.out=="2aubm34db");
  gCargo=false; pulse(gJunction,3); expect("main line again: RETURN 'r'", state==S_RETURN && bt.out=="2aubm34dbr");
  pulse(gCorner); expect("bottom corner: segment 0 report '1'", segment==0 && bt.out=="2aubm34dbr1");
  step(2); gMarker=true; step(1); expect("marker reached: 'c' then 'w'", state==S_WAIT && bt.out=="2aubm34dbr1cw");
  expect("job cleared", loadCell==-1 && unloadCell==-1 && !carrying);
  gMarker=false;
  // line lost
  bt.out.clear(); bt.in="1A"; step(3); bt.out.clear(); ev.clear();
  gLineLost=true; step(3); expect("line lost: 'n' once, light tracking used", bt.out=="n" && had("light") && !had("line"));
  gLineLost=false; ev.clear(); step(2); expect("line back: line steering again", had("line") && !had("light"));
  gLineLost=true; bt.out.clear(); step(2); gLineLost=false; step(1); gLineLost=true; step(2); gLineLost=false;
  expect("second loss reports 'n' again", bt.out=="nn");
  // emergency
  bt.out.clear(); ev.clear(); gRear=true; step(1); gRear=false;
  expect("rear obstacle: stop, state EM_WAIT, report 'e'", state==S_EM_WAIT && bt.out=="e" && had("stop"));
  ev.clear(); step(3); expect("EM_WAIT holds still (no driving)", !had("line") && !had("light") && state==S_EM_WAIT);
  bt.in="X"; step(2); expect("'X' -> light guidance, no line tracing", state==S_EM_GUIDE && had("light") && !had("line"));
  bt.out.clear(); gMarker=true; step(1); expect("marker reached: report 'w' only (no 'c')", state==S_WAIT && bt.out=="w");
  gMarker=false;
  // lift timing
  ev.clear(); setLiftTarget(500); step(1); expect("lift goes up toward target", had("liftUp") && liftDir==1);
  step(60); expect("lift stops at target, position tracked", liftDone() && liftPosMs==500 && had("liftHalt"));
  ev.clear(); setLiftTarget(0); step(60); expect("lift comes back down to 0", had("liftDown") && liftPosMs==0 && liftDone());
  setLiftTarget(99999); expect("target clamped to full stroke", liftTargetMs==8000);
  printf("fails=%d\n",fails); return fails;
}
