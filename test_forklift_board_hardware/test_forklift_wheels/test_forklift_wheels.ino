// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | wheels check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_wheels.ino  -  구동 바퀴(연속회전 서보 2개) 단독 시험: 핀 / 상수 / 객체 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 안전: 시험 전에 바퀴를 바닥에서 띄운다 (시험 시작 때 y 를 입력해야 움직인다).
 */
#include <Servo.h>       // Arduino 표준 라이브러리 (서보)

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_SERVO_LEFT  = 10;  // 구동 서보 왼쪽
const uint8_t PIN_SERVO_RIGHT = 11;  // 구동 서보 오른쪽

// ---------------- 상수 ----------------
const int SERVO_STOP_US    = 1500;   // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int WHEEL_TEST_SPEED = 100;    // ◆ NEW: 안전한 저속 테스트값 (PPT 최대는 200)
const int WHEEL_TEST_MS    = 1000;

// ---------------- 공용 객체 ----------------
Servo servoLeft;
Servo servoRight;

// ---------------- 하드웨어 함수 ----------------
void attachWheels() {  // ◆ NEW
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
}

// ★ EXTERNAL(변형): 2_2_ABOT주행.pptx 슬라이드 19의 maneuver()
//   (-1 이면 detach 하던 분기는 stopWheels()로 분리). 오른쪽 속도는 부호를 반전해 보낸다.
void maneuver(int speedLeft, int speedRight, int msTime) {
  servoLeft.writeMicroseconds(SERVO_STOP_US + speedLeft);
  servoRight.writeMicroseconds(SERVO_STOP_US - speedRight);
  delay(msTime);
}

void stopWheels() {  // ◆ NEW
  servoLeft.writeMicroseconds(SERVO_STOP_US);
  servoRight.writeMicroseconds(SERVO_STOP_US);
  delay(200);
  servoLeft.detach();
  servoRight.detach();
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  구동 바퀴를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 구동 함수는 위쪽 하드웨어 함수 구역에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

const long          SERIAL_BAUD      = 9600;
const unsigned long INPUT_TIMEOUT_MS = 30000UL;

// ---------------- 입력 보조 (◆ NEW) ----------------
int waitForChar(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (Serial.available() > 0) {
      int c = Serial.read();
      if (c != '\n' && c != '\r') return c;
    }
  }
  return -1;
}

void printMenu() {
  Serial.println(F("\n== Forklift wheels test =="));
  Serial.println(F("1 Wheels (6 moves, 1s each)"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 구동 바퀴 ----------------
void testWheels() {
  Serial.println(F("Lift wheels off ground! y=start"));
  if (waitForChar(INPUT_TIMEOUT_MS) != 'y') {
    Serial.println(F("Cancelled"));
    return;
  }
  attachWheels();
  const int s = WHEEL_TEST_SPEED;
  Serial.println(F("Left only"));   maneuver(s, 0, WHEEL_TEST_MS);
  Serial.println(F("Right only"));  maneuver(0, s, WHEEL_TEST_MS);
  Serial.println(F("Forward"));     maneuver(s, s, WHEEL_TEST_MS);
  Serial.println(F("Backward"));    maneuver(-s, -s, WHEEL_TEST_MS);
  Serial.println(F("Spin left"));   maneuver(-s, s, WHEEL_TEST_MS);
  Serial.println(F("Spin right"));  maneuver(s, -s, WHEEL_TEST_MS);
  stopWheels();
  Serial.println(F("Done. Report any reversed wheel"));
}

// ---------------- setup / loop (시리얼 메뉴) ----------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  printMenu();
}

void loop() {
  if (Serial.available() <= 0) return;
  int cmd = Serial.read();
  switch (cmd) {
    case '1': testWheels(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
