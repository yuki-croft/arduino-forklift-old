// ====================================================================================================
// TOOL (not final) | board: FORKLIFT | wheel balance check on the floor (straight forward/back with trim); do not use as the final program
// ====================================================================================================
/*
 * tool_forklift_wheel_trim.ino  -  바퀴 속도 보정값 찾기 (2026-11-28 사용자: 선이 없는 바닥에서도 오른쪽으로 감)
 * 보드: FRIBEE EDU (UNO R3 호환). 시리얼/블루투스 없음. 전원을 켜면 3초 뒤: 앞으로 DRIVE_MS -> 1초 정지 -> 뒤로 DRIVE_MS -> 정지, 반복.
 * 쓰는 법: 바닥에 테이프로 출발선을 긋고 지게차를 똑바로 놓은 뒤 켠다. 2m 정도 앞으로 가며 휘는지 본다.
 *   오른쪽으로 휘면 WHEEL_LEFT_PCT 를 낮춘다 (예 95 -> 92), 왼쪽으로 휘면 올린다. 다시 올려서 확인.
 *   똑바로 가는 값을 찾으면 final_forklift_board.ino 의 WHEEL_LEFT_PCT / WHEEL_RIGHT_PCT 에 같은 값을 넣는다.
 *   속도는 지게차 코드의 직진 속도와 같은 60 (1500 +- 60).
 */
#include <Servo.h>   // Arduino 표준 라이브러리

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_SERVO_LEFT  = 10;
const uint8_t PIN_SERVO_RIGHT = 11;
const uint8_t PIN_BUZZER      = 5;

// ---------------- 상수 ----------------
const int           SERVO_STOP_US   = 1500;    // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int           SPEED           = 60;      // 지게차 코드 LINE_BASE_SPEED 와 같음
const int           WHEEL_LEFT_PCT  = 95;      // ?? TODO ?? 이 값을 바꿔 가며 맞춘다
const int           WHEEL_RIGHT_PCT = 100;
const unsigned long DRIVE_MS        = 4000UL;  // ?? TODO ?? 한 방향으로 가는 시간

Servo servoLeft, servoRight;

// ---------------- 하드웨어 함수 ----------------
void setWheels(int sl, int sr) {  // ★ EXTERNAL(변형): final 의 setWheels 와 같은 규칙 (앞으로 = 왼쪽 1500+s, 오른쪽 1500-s) + 보정
  servoLeft.writeMicroseconds(SERVO_STOP_US + (int)((long)sl * WHEEL_LEFT_PCT / 100));
  servoRight.writeMicroseconds(SERVO_STOP_US - (int)((long)sr * WHEEL_RIGHT_PCT / 100));
}

// ====================================================================================================
// SERIAL TEST SECTION  (no serial in this tool)
// ====================================================================================================
void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  setWheels(0, 0);
  tone(PIN_BUZZER, 3000, 200);
  delay(3000);
}

void loop() {
  setWheels(SPEED, SPEED);   delay(DRIVE_MS);
  setWheels(0, 0);           delay(1000);
  setWheels(-SPEED, -SPEED); delay(DRIVE_MS);
  setWheels(0, 0);           delay(3000);
}
