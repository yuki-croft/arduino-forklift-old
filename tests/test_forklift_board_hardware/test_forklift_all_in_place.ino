// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | in-place check of everything except the line sensors and wheels; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_all_in_place.ino  -  지게차 제자리 시험: 라인 트레이서와 바퀴를 뺀 나머지 전부 (2026-11-28 사용자 요청)
 * 보드: FRIBEE EDU (UNO R3 호환). 블루투스 = NeoSWSerial D2(모듈 TXD) / D3(모듈 RXD), 9600 (사용자: 모듈은 D2/D3, D0 에는 빨간 LED).
 * 시작: 폰 앱에서 상차 칸과 하차 칸(예: 1, A)을 고르고 Start. 센터는 Start 때 두 글자를 보낸다 (센터 코드 startJob 확인).
 *   어떤 상차 숫자 + 하차 알파벳이든 두 글자가 모이면 시작한다. 바퀴 서보는 붙이지 않아 움직이지 않는다.
 * 순서 (각 단계 시작 때 폰에 문구가 뜬다: 센터가 지게차 글자를 문구로 바꿈)
 *   1) 시작        "ST TO LOAD"   : 부저 짧게 2번 + 빨간 LED(D0) 3번 깜박
 *   2) 리프트 올림 "ST LIFT UP"   : LIFT_TEST_MS 동안 올림 (1300us, 사용자 최종 확인 2026-11-28)
 *   3) 리프트 내림 "ST UNLOAD"    : 같은 시간 내림 (1700us) -> 시작 높이로 돌아옴
 *   4) 적외선      "ST APPROACH"  : STAGE_MS 동안, 왼/앞/오 수신기 중 하나라도 반사를 받으면 짧게 삐 (손을 앞에 대 보기)
 *   5) 초음파      "ST REVERSE"   : STAGE_MS 동안, 후방 거리가 US_NEAR_CM 이하면 짧게 삐 (손을 뒤에 대 보기)
 *   6) 포토        "ST RETURN"    : STAGE_MS 동안, 왼/오 밝기가 시작 때보다 PHOTO_DELTA 이상 바뀌면 짧게 삐 (손으로 가려 보기)
 *   7) 끝          "ST DONE"      : 부저 길게 1번. 다시 대기 (다음 시험은 다시 칸 선택 + Start)
 * 주의: 센터는 "ST DONE" 을 받으면 상차 칸을 비움/하차 칸을 점유로 기록한다. 같은 칸으로 다시 하려면 센터 전원을 껐다 켜거나 다른 칸을 고른다.
 * 리프트 시험 시간 동안 리프트 위아래에 걸리는 것이 없게 할 것.
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT / 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <Servo.h>        // Arduino 표준 라이브러리
#include <Ultrasonic.h>   // ★ EXTERNAL LIBRARY: "Ultrasonic" by Erick Simoes (final 과 같음)
#include <NeoSWSerial.h>  // ★ EXTERNAL LIBRARY: "NeoSWSerial" by SlashDevin (final 과 같음)

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_RED_LED     = 0;    // 사용자: D0 빨간 LED
const uint8_t PIN_BT_RX       = 2;    // 블루투스 모듈 TXD
const uint8_t PIN_BT_TX       = 3;    // 블루투스 모듈 RXD
const uint8_t PIN_IR_TX       = 4;
const uint8_t PIN_BUZZER      = 5;
const uint8_t PIN_US_TRIG     = 6;
const uint8_t PIN_US_ECHO     = 7;
const uint8_t PIN_IR_RX_LEFT  = 8;
const uint8_t PIN_IR_RX_FRONT = 9;
const uint8_t PIN_IR_RX_RIGHT = 13;
const uint8_t PIN_SERVO_LIFT  = 12;
const uint8_t PIN_PHOTO_LEFT  = A0;
const uint8_t PIN_PHOTO_RIGHT = A1;

// ---------------- 상수 ----------------
const long          BT_BAUD       = 9600;
const long          IR_FREQ_HZ    = 38000;    // ★ EXTERNAL: 5_ABOT적외선센서.pptx 슬라이드 9
const int           SERVO_STOP_US = 1500;     // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int           LIFT_UP_US    = 1300;     // 사용자 최종 확인 (2026-11-28): 1300 = 올림 (반대로 보였던 것은 톱니 어긋남)
const int           LIFT_DOWN_US  = 1700;     //                                1700 = 내림
const unsigned long LIFT_TEST_MS  = 2000UL;   // ?? TODO ?? 시험용: 2초 = 약 2cm (8초에 8cm 기준)
const unsigned long STAGE_MS      = 5000UL;   // ?? TODO ?? 센서 단계마다 5초
const long          US_NEAR_CM    = 10;       // ?? TODO ?? 시험용 기준 거리
const int           PHOTO_DELTA   = 100;      // ?? TODO ?? 시험용 밝기 변화 기준
const unsigned int  BEEP_HZ       = 3000;

// ---------------- 공용 객체 ----------------
Servo servoLift;
Ultrasonic ultrasonic(PIN_US_TRIG, PIN_US_ECHO);
NeoSWSerial bt(PIN_BT_RX, PIN_BT_TX);   // (RX, TX): martyncurrey.com/?p=9971 예제 "NeoSWSerial( RX pin, TX pin )"

// ---------------- 하드웨어 함수 ----------------
void beep(unsigned long ms) {  // ◆ NEW
  tone(PIN_BUZZER, BEEP_HZ, ms);
  delay(ms);
}

void redBlink(uint8_t times) {  // ◆ NEW
  for (uint8_t i = 0; i < times; i++) {
    digitalWrite(PIN_RED_LED, HIGH); delay(200);
    digitalWrite(PIN_RED_LED, LOW);  delay(200);
  }
}

void liftFor(int pulseUs, unsigned long ms) {  // ◆ NEW
  servoLift.attach(PIN_SERVO_LIFT);
  servoLift.writeMicroseconds(pulseUs);
  delay(ms);
  servoLift.writeMicroseconds(SERVO_STOP_US);
  servoLift.detach();
}

bool irAnyDetected() {  // ★ EXTERNAL(변형): final 의 irScan 과 같은 방식. 0 = 감지
  tone(PIN_IR_TX, IR_FREQ_HZ, 8);
  delay(1);
  bool hit = digitalRead(PIN_IR_RX_LEFT) == 0 || digitalRead(PIN_IR_RX_FRONT) == 0 || digitalRead(PIN_IR_RX_RIGHT) == 0;
  delay(10);   // 38kHz tone 이 끝난 뒤 부저 tone 을 쓰도록
  return hit;
}

bool usNear() {  // ◆ NEW: 0 이하(측정 실패 추정)는 무시
  long cm = (long)ultrasonic.read();
  return cm > 0 && cm <= US_NEAR_CM;
}

void report(char c) { bt.write(c); }   // 센터가 폰 문구로 바꿈

// ====================================================================================================
// SERIAL TEST SECTION  (no USB Serial in this sketch: D0 is the red LED, Bluetooth is on D2/D3)
// ====================================================================================================
int8_t loadCell = -1, unloadCell = -1;

void runStage(char code, uint8_t kind) {  // kind: 0 IR, 1 ultrasonic, 2 photo
  report(code);
  int baseL = analogRead(PIN_PHOTO_LEFT), baseR = analogRead(PIN_PHOTO_RIGHT);
  unsigned long t0 = millis();
  while (millis() - t0 < STAGE_MS) {
    bool hit = false;
    if (kind == 0) hit = irAnyDetected();
    else if (kind == 1) { hit = usNear(); delay(60); }
    else {
      hit = abs(analogRead(PIN_PHOTO_LEFT) - baseL) >= PHOTO_DELTA || abs(analogRead(PIN_PHOTO_RIGHT) - baseR) >= PHOTO_DELTA;
      delay(20);
    }
    if (hit) { beep(60); delay(60); }
  }
}

void runAllTest() {
  report('l');                              // 1) 시작
  beep(100); delay(100); beep(100);
  redBlink(3);
  report('u');                              // 2) 리프트 올림
  liftFor(LIFT_UP_US, LIFT_TEST_MS);
  delay(300);
  report('d');                              // 3) 리프트 내림
  liftFor(LIFT_DOWN_US, LIFT_TEST_MS);
  runStage('a', 0);                         // 4) 적외선
  runStage('b', 1);                         // 5) 초음파
  runStage('r', 2);                         // 6) 포토
  report('c');                              // 7) 끝
  beep(800);
  while (bt.available() > 0) bt.read();     // 시험 중 들어온 글자는 버림
}

void setup() {
  pinMode(PIN_RED_LED, OUTPUT);
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_IR_RX_LEFT, INPUT);
  pinMode(PIN_IR_RX_FRONT, INPUT);
  pinMode(PIN_IR_RX_RIGHT, INPUT);
  digitalWrite(PIN_RED_LED, LOW);
  bt.begin(BT_BAUD);
  beep(150);                                // 켜짐 확인
}

void loop() {
  while (bt.available() > 0) {
    char c = (char)bt.read();
    if (c >= '1' && c <= '6') loadCell = c - '1';
    else if (c >= 'A' && c <= 'F') unloadCell = c - 'A';
  }
  if (loadCell >= 0 && unloadCell >= 0) {
    runAllTest();
    loadCell = -1;
    unloadCell = -1;
  }
}
