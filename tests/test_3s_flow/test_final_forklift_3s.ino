// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT (FRIBEE EDU, HC-06 on D0/D1) | 3 s forward, then load and unload; line sensors only RECOGNIZE
// ====================================================================================================
/*
 * test_final_forklift_3s.ino  -  지게차 시험용: final_forklift_board 와 같은 블루투스 프로토콜로 한 작업을 흉내 낸다 (단일 파일)
 * 보드: FRIBEE EDU (UNO R3 호환). HC-06 = 하드웨어 시리얼 D0/D1 (final 과 같음). USB Serial 디버그 출력 없음.
 *
 * [동작] 센터가 상차 글자('1'~'6')와 하차 글자('A'~'F')를 모두 보내면 한 번 실행한다 (칸 번호는 시험에서 쓰지 않음):
 *   1) 출발 보고 '1','l' -> 3초 전진 (라인을 따라가지 않고 직진만. 라인 센서는 값을 읽어 "인식"만 함)
 *   2) 정지 -> 접근 'a' -> 상차: 포크 올림 (보고 'u') LIFT_TEST_MS
 *   3) 하차 이동 'm' -> 하차: 포크 내림 (보고 'd') LIFT_TEST_MS
 *   4) 완료 'c', 대기 'w'
 * [라인 인식] 3초 전진 동안 라인 센서 A2/A3/A4 를 20ms 마다 읽는다. 사용자 실측값만 사용: 검은 선 31 이상, 흰 바닥 29 이하.
 *   - 셋 중 하나라도 31 이상이 3번 연속이면 "라인을 봄". 전진이 끝난 뒤 부저: 라인을 봤으면 긴 소리 1번, 못 봤으면 짧은 소리 2번.
 *   - 셋 다 29 이하가 되면 final 과 같이 'n' (라인 이탈) 을 한 번 보고 (조향/광 추적은 하지 않음).
 *   교차점/모서리/표식 판별(forklift_line_pattern.h)과 직선 보정은 쓰지 않는다 (실측 전 임시 규칙이므로).
 * [안 하는 것] 후방 초음파 비상, 'X'(비상 선언), 시작 표식 복귀, 회전, 라인 보정 주행. 센터가 'X' 를 보내도 무시한다.
 * [안전] 시험 전에 앞쪽으로 3초 가는 거리 (속도 100 기준 약 수십 cm, 바닥에 따라 다름) 를 비우고, 포크 위/아래 여유를 확인한다.
 *   리프트는 켤 때 최소 높이라고 가정하고 올림/내림 시간을 같게 해서 원래 높이로 돌아온다.
 *
 * [표기] ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <Servo.h>       // Arduino 표준 라이브러리 (서보)

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_BUZZER      = 5;
const uint8_t PIN_SERVO_LEFT  = 10;
const uint8_t PIN_SERVO_RIGHT = 11;
const uint8_t PIN_SERVO_LIFT  = 12;
const uint8_t PIN_LINE_LEFT   = A2;
const uint8_t PIN_LINE_CENTER = A3;
const uint8_t PIN_LINE_RIGHT  = A4;

// ---------------- 상수: final 과 같은 값 ----------------
const int  SERVO_STOP_US  = 1500;   // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const long BT_BAUD        = 9600;   // 지게차 HC-06 (final 과 같음)
const int  DRIVE_SPEED    = 100;    // final 의 LINE_BASE_SPEED (직진 기본 속도)
const int  LIFT_SPEED     = 200;    // final 의 LIFT_SPEED
const bool LIFT_INVERT    = true;   // final 과 같음 (실물 시험 결과: 올림 1300us / 내림 1700us)
const int  LINE_ON_MIN    = 31;     // 사용자 실측: 검은 선 위 31 이상
const int  LINE_WHITE_MAX = 29;     // 사용자 실측: 흰 바닥 약 24~29

// ---------------- 상수: 시험용 임시값 (?? TODO ??) ----------------
const unsigned long FORWARD_MS     = 3000;  // 사용자 지정: 3초 전진
const unsigned long LIFT_TEST_MS   = 2000;  // 올림/내림 시간. 선반 높이를 모르므로 임시값 (최소~최대 총 8초)
const unsigned long LINE_POLL_MS   = 20;    // 라인 읽는 주기
const uint8_t       LINE_CONFIRM   = 3;     // "라인을 봄" 으로 인정하는 연속 횟수
const unsigned int  BEEP_HZ        = 3000;  // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 10 의 예제 주파수

// ---------------- 상태 / 객체 ----------------
int8_t loadChar = -1, unloadChar = -1;   // 받은 글자 (0~5), -1 = 미수신
Servo servoLeft, servoRight, servoLift;
HardwareSerial &bt = Serial;             // HC-06 (D0/D1)

// ---------------- 하드웨어 함수 ----------------
void report(char code) { bt.write(code); }   // ◆ NEW: 상태 글자 1바이트 (final 과 같은 글자)

void setWheels(int speedLeft, int speedRight) {  // ★ EXTERNAL(변형): maneuver() 에서 delay 를 뺌 (오른쪽 속도는 부호 반전)
  servoLeft.writeMicroseconds(SERVO_STOP_US + speedLeft);
  servoRight.writeMicroseconds(SERVO_STOP_US - speedRight);
}

void stopWheels() { setWheels(0, 0); }   // ◆ NEW: 정지 펄스를 계속 보냄

void liftMove(int dir, unsigned long ms) {  // ◆ NEW: dir +1 올림 / -1 내림, ms 동안 (final 의 liftRun/liftHalt 와 같은 펄스)
  servoLift.attach(PIN_SERVO_LIFT);
  servoLift.writeMicroseconds(SERVO_STOP_US + (LIFT_INVERT ? -dir : dir) * LIFT_SPEED);
  delay(ms);
  servoLift.writeMicroseconds(SERVO_STOP_US);
  servoLift.detach();
}

void buzzerBeep(unsigned int hz, unsigned long ms) {  // ◆ NEW: 소리가 끝날 때까지 기다림
  tone(PIN_BUZZER, hz, ms);
  delay(ms);
}

// ---------------- 라인 "인식만" 하며 3초 전진 ----------------
void forwardRecognizeOnly() {  // ◆ NEW
  bool lineSeen = false, lostReported = false;
  uint8_t run = 0;
  setWheels(DRIVE_SPEED, DRIVE_SPEED);
  unsigned long start = millis(), lastPoll = start;
  while (millis() - start < FORWARD_MS) {
    if (millis() - lastPoll < LINE_POLL_MS) continue;
    lastPoll += LINE_POLL_MS;
    int l = analogRead(PIN_LINE_LEFT), c = analogRead(PIN_LINE_CENTER), r = analogRead(PIN_LINE_RIGHT);
    if (l >= LINE_ON_MIN || c >= LINE_ON_MIN || r >= LINE_ON_MIN) {
      if (run < 255) run++;
      if (run >= LINE_CONFIRM) lineSeen = true;
    } else {
      run = 0;
    }
    bool allWhite = (l <= LINE_WHITE_MAX && c <= LINE_WHITE_MAX && r <= LINE_WHITE_MAX);
    if (allWhite && !lostReported) { report('n'); lostReported = true; }   // final 과 같이 이탈 시작 때 한 번만
    if (!allWhite) lostReported = false;
  }
  stopWheels();
  if (lineSeen) { buzzerBeep(BEEP_HZ, 600); }                    // 라인을 봄: 긴 소리 1번
  else { buzzerBeep(BEEP_HZ, 150); delay(150); buzzerBeep(BEEP_HZ, 150); }   // 못 봄: 짧은 소리 2번
}

// ---------------- 한 작업 ----------------
void runJob() {  // ◆ NEW
  report('1');                       // 아래 구간 진입 (센터가 좌하 LED 점등, final 의 beginJob 과 같음)
  report('l');                       // 상차 칸으로 이동
  forwardRecognizeOnly();            // 3초 전진 (라인 인식만)
  report('a');                       // 접근
  report('u');                       // 상차: 포크 올림
  liftMove(+1, LIFT_TEST_MS);
  report('m');                       // 하차 칸으로 이동 (시험에서는 움직이지 않음)
  report('d');                       // 하차: 포크 내림
  liftMove(-1, LIFT_TEST_MS);
  report('c');                       // 정상 완료
  loadChar = -1;
  unloadChar = -1;
  while (bt.available() > 0) bt.read();   // 작업 중 들어온 글자는 버림 (final 도 작업 중에는 무시)
  report('w');                       // 대기
}

// 칸 글자는 대기 중에만 받는다. 'X' 등은 무시.
void commPoll() {  // ◆ NEW
  while (bt.available() > 0) {
    char c = (char)bt.read();
    if (c >= '1' && c <= '6') loadChar = c - '1';
    else if (c >= 'A' && c <= 'F') unloadChar = c - 'A';
  }
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  bt.begin(BT_BAUD);
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  stopWheels();
  report('w');
}

void loop() {
  commPoll();
  if (loadChar >= 0 && unloadChar >= 0) runJob();
}
