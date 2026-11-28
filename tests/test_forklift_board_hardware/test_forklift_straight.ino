// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | wheels only: drive straight forward forever; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_straight.ino  -  바퀴만, 계속 직진 (2026-11-28 사용자 요청)
 * 전원을 켜면 2초 뒤부터 멈추지 않고 앞으로 간다. 멈추려면 전원을 끈다.
 * 속도와 보정값은 final_forklift_board.ino 와 같게 (속도 60, 왼쪽 95% / 오른쪽 100%). 휘면 WHEEL_LEFT_PCT 를 조절.
 */
#include <Servo.h>   // Arduino 표준 라이브러리

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_SERVO_LEFT  = 10;
const uint8_t PIN_SERVO_RIGHT = 11;

// ---------------- 상수 ----------------
const int SERVO_STOP_US   = 1500;   // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int SPEED           = 60;     // final 의 LINE_BASE_SPEED 와 같음
const int WHEEL_LEFT_PCT  = 95;     // ?? TODO ?? 오른쪽으로 휘면 낮추고, 왼쪽으로 휘면 올림
const int WHEEL_RIGHT_PCT = 100;

Servo servoLeft, servoRight;

// ---------------- 하드웨어 함수 ----------------
void setWheels(int sl, int sr) {  // ★ EXTERNAL(변형): 앞으로 = 왼쪽 1500+s, 오른쪽 1500-s (final 과 같은 규칙) + 보정
  servoLeft.writeMicroseconds(SERVO_STOP_US + (int)((long)sl * WHEEL_LEFT_PCT / 100));
  servoRight.writeMicroseconds(SERVO_STOP_US - (int)((long)sr * WHEEL_RIGHT_PCT / 100));
}

// ====================================================================================================
// SERIAL TEST SECTION  (no serial in this sketch)
// ====================================================================================================
void setup() {
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  setWheels(0, 0);
  delay(2000);
  setWheels(SPEED, SPEED);
}

void loop() {
}
