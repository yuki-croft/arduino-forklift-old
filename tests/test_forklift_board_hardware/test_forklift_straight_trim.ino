// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | wheels only: straight runs with a different left-wheel trim each run; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_straight_trim.ino  -  직진 보정값 찾기 (test_forklift_straight 를 바탕으로, 2026-11-28 사용자 요청)
 * 왼쪽 바퀴 보정값(%)을 한 번 달릴 때마다 바꿔 가며 직진 -> 가장 똑바로 간 회차를 보고 값을 고른다. 시리얼 없음, 부저로 회차 안내.
 * 진행 (전원을 켜면 3초 뒤 시작):
 *   회차 n (1 부터 RUNS 까지): 부저 n 번 -> 1초 뒤 앞으로 RUN_MS 직진 -> 1초 정지 -> 뒤로 RUN_MS (출발점 근처로) -> 정지
 *   -> 다음 회차까지 REPOSITION_MS (그동안 지게차를 출발선에 다시 똑바로 놓는다)
 *   왼쪽 보정값 = START_PCT - (n - 1) * STEP_PCT   (기본: 1회차 94, 2회차 92, 3회차 90 ... 8회차 80)
 *   모든 회차가 끝나면 긴 삐 2번 후 정지.
 * 고르는 법: 바닥에 출발선과 앞쪽 2m 지점 표시를 하고, 회차마다 휘는 방향을 적는다.
 *   오른쪽으로 휘던 것이 똑바로 -> 왼쪽으로 휘기 시작하는 사이의 회차가 답이다.
 *   그 회차의 값을 final_forklift_board.ino 의 WHEEL_LEFT_PCT 에 넣는다 (더 세밀하게 하려면 START_PCT/STEP_PCT 를 좁혀 다시).
 * 바닥에서도 오른쪽으로 간다 = 왼쪽 바퀴가 빠름 -> 왼쪽만 낮춘다. 반대로 왼쪽으로 가면 TRIM_LEFT_WHEEL = false (오른쪽 바퀴를 낮춤).
 */
#include <Servo.h>   // Arduino 표준 라이브러리

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_SERVO_LEFT  = 10;
const uint8_t PIN_SERVO_RIGHT = 11;
const uint8_t PIN_BUZZER      = 5;

// ---------------- 상수 ----------------
const int           SERVO_STOP_US   = 1500;     // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int           SPEED           = 60;       // final 의 LINE_BASE_SPEED 와 같음
const bool          TRIM_LEFT_WHEEL = true;     // true = 왼쪽 바퀴를 낮춰 가며 (오른쪽으로 휘는 경우)
const int           START_PCT       = 94;       // 사용자 지정: 1회차 94
const int           STEP_PCT        = 2;        // ?? TODO ?? 회차마다 낮추는 양
const uint8_t       RUNS            = 8;        // 94, 92, ... 80
const unsigned long RUN_MS          = 20000UL;  // 사용자 지정: 한 방향 20초
const unsigned long REPOSITION_MS   = 6000UL;   // 다음 회차 전에 다시 놓을 시간

Servo servoLeft, servoRight;

// ---------------- 하드웨어 함수 ----------------
void setWheelsPct(int sl, int sr, int leftPct, int rightPct) {  // ★ EXTERNAL(변형): 앞으로 = 왼쪽 1500+s, 오른쪽 1500-s + 보정
  servoLeft.writeMicroseconds(SERVO_STOP_US + (int)((long)sl * leftPct / 100));
  servoRight.writeMicroseconds(SERVO_STOP_US - (int)((long)sr * rightPct / 100));
}

void beep(unsigned long ms) { tone(PIN_BUZZER, 3000, ms); delay(ms); }   // ◆ NEW

// ====================================================================================================
// SERIAL TEST SECTION  (no serial in this sketch)
// ====================================================================================================
void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  setWheelsPct(0, 0, 100, 100);
  delay(3000);
  for (uint8_t n = 1; n <= RUNS; n++) {
    int pct = START_PCT - (n - 1) * STEP_PCT;
    int lp = TRIM_LEFT_WHEEL ? pct : 100;
    int rp = TRIM_LEFT_WHEEL ? 100 : pct;
    for (uint8_t b = 0; b < n; b++) { beep(150); delay(200); }   // 회차 번호 = 삐 횟수
    delay(1000);
    setWheelsPct(SPEED, SPEED, lp, rp);   delay(RUN_MS);
    setWheelsPct(0, 0, 100, 100);         delay(1000);
    setWheelsPct(-SPEED, -SPEED, lp, rp); delay(RUN_MS);
    setWheelsPct(0, 0, 100, 100);
    if (n < RUNS) delay(REPOSITION_MS);
  }
  beep(600); delay(200); beep(600);
}

void loop() {
}
