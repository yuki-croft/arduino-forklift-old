// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | drives forward/backward in place by itself, NO serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_shuffle.ino  -  배치 전 시험: 제자리에서 앞 -> 뒤 -> 앞 -> 뒤 ... 왕복 (구동 바퀴 서보 2개만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [동작] 전원/업로드 후 시작 전 대기(부저 카운트다운) -> 전진 -> 정지 -> 후진 -> 정지 를 SHUFFLE_CYCLES 번 반복 -> 정지하고 끝 (다시 하려면 리셋).
 *        USB Serial 을 쓰지 않는다 (지게차의 HC-06 이 D0/D1 에 납땜돼 있어 시리얼 모니터와 겹치기 때문). 상태는 부저로만 알린다.
 *        - 시작 전: 짧은 삐 소리 START_BEEPS 번 (1초 간격) 뒤 긴 삐 1번 = 이제 움직인다
 *        - 끝: 낮은 소리 길게 2번 = 정상 종료
 * [방향] 앞/뒤 방향은 test_forklift_wheels 의 maneuver(s, s) = 전진 규칙과 같다 (사용자 실물 시험에서 정상이었음).
 *        실제 앞뒤가 반대로 움직이면 SHUFFLE_SPEED 의 부호를 바꾸지 말고 알려 주세요 (상수 SHUFFLE_REVERSE 로 한 번에 뒤집을 수 있음).
 * [안전] 같은 속도 / 같은 시간으로 앞뒤를 번갈아 하므로 원래 자리 근처로 돌아오지만 바닥 상태에 따라 조금씩 어긋날 수 있다.
 *        앞뒤로 SHUFFLE_MS 동안 움직일 여유 공간 (약 10~20cm 이상)을 비워 두고, 손으로 잡을 수 있는 곳에서 시험한다.
 *        바퀴를 띄우고 시험하려면 시작 전 대기 중에 들어 올린다.
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <Servo.h>       // Arduino 표준 라이브러리 (서보)

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_BUZZER      = 5;   // 수동 부저
const uint8_t PIN_SERVO_LEFT  = 10;  // 구동 서보 왼쪽
const uint8_t PIN_SERVO_RIGHT = 11;  // 구동 서보 오른쪽

// ---------------- 상수 ----------------
const int SERVO_STOP_US = 1500;      // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19

// ?? TODO ?? 시험용 임시값: 바꿔도 된다
const int           SHUFFLE_SPEED  = 100;      // test_forklift_wheels 와 같은 저속 (PPT 최대는 200)
const bool          SHUFFLE_REVERSE = false;   // true 면 앞/뒤를 서로 바꿈
const unsigned long SHUFFLE_MS     = 700;      // 한 방향으로 가는 시간
const unsigned long SHUFFLE_PAUSE_MS = 500;    // 방향을 바꾸기 전 정지 시간
const uint8_t       SHUFFLE_CYCLES = 5;        // 왕복 횟수 (앞+뒤 1회 = 1)
const uint8_t       START_BEEPS    = 5;        // 시작 전 카운트다운 소리 수 (초)
const unsigned int  BEEP_HZ        = 3000;     // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 10 의 예제 주파수

// ---------------- 공용 객체 ----------------
Servo servoLeft;
Servo servoRight;

// ---------------- 하드웨어 함수 ----------------
void buzzerBeep(unsigned int frequency, unsigned long ms) {  // ◆ NEW: 소리가 끝날 때까지 기다림 (tone 은 시간이 지나면 스스로 멈춤)
  tone(PIN_BUZZER, frequency, ms);
  delay(ms);
}

// ★ EXTERNAL(변형): 2_2_ABOT주행.pptx 슬라이드 19의 maneuver(). 오른쪽 속도는 부호를 반전해 보낸다 (test_forklift_wheels 와 같음).
void maneuver(int speedLeft, int speedRight, unsigned long msTime) {
  servoLeft.writeMicroseconds(SERVO_STOP_US + speedLeft);
  servoRight.writeMicroseconds(SERVO_STOP_US - speedRight);
  delay(msTime);
}

void stopWheels(unsigned long msTime) {  // ◆ NEW: 정지 펄스를 보내며 기다림 (detach 하지 않음)
  servoLeft.writeMicroseconds(SERVO_STOP_US);
  servoRight.writeMicroseconds(SERVO_STOP_US);
  delay(msTime);
}

// ◆ NEW: 앞뒤 한 번씩
void shuffleOnce() {
  const int s = SHUFFLE_REVERSE ? -SHUFFLE_SPEED : SHUFFLE_SPEED;
  maneuver(s, s, SHUFFLE_MS);     // 전진
  stopWheels(SHUFFLE_PAUSE_MS);
  maneuver(-s, -s, SHUFFLE_MS);   // 후진
  stopWheels(SHUFFLE_PAUSE_MS);
}

// ====================================================================================================
// SERIAL TEST SECTION  (this sketch does not use USB Serial at all)
// ====================================================================================================
/*
 * 이 시험은 시리얼 모니터를 쓰지 않는다. 아래 setup() 이 위의 함수들을 순서대로 한 번 실행하고 loop() 는 비어 있다.
 */

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  stopWheels(100);                                   // 먼저 정지 신호
  for (uint8_t i = 0; i < START_BEEPS; i++) {        // 시작 전 대기: 이 동안 위치를 잡거나 손을 뗀다
    buzzerBeep(BEEP_HZ, 100);
    delay(900);
  }
  buzzerBeep(BEEP_HZ, 600);                          // 긴 삐 = 이제 움직인다
  for (uint8_t i = 0; i < SHUFFLE_CYCLES; i++) shuffleOnce();
  stopWheels(200);
  buzzerBeep(BEEP_HZ / 3, 600);                      // 끝: 낮은 소리 길게 2번
  delay(200);
  buzzerBeep(BEEP_HZ / 3, 600);
}

void loop() {
  stopWheels(100);   // 끝난 뒤에도 정지 신호 유지 (리셋하면 다시 시작)
}
