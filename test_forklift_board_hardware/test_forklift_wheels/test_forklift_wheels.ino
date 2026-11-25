// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | wheels check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_wheels.ino  -  구동 바퀴(연속회전 서보 2개) 단독 시험: 핀 / 상수 / 객체 / 하드웨어 함수 (Serial 미사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [탭 구성]  같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   test_forklift_wheels.ino : 핀 / 상수 / 객체 / 하드웨어 함수 (이 파일, 항상 먼저 이어 붙음)
 *   wheels_serial_test.ino   : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
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
