// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | lift check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_lift.ino  -  리프트 서보(연속회전, 수압 펌프식) 단독 시험: 핀 / 상수 / 객체 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 주의: 한계 스위치 정보가 없다. 짧은 스텝으로만 올리고 내리며, 최소~최대 높이 총 이동 시간(아래 상수)을 넘기지 말 것.
 */
#include <Servo.h>       // Arduino 표준 라이브러리 (서보)

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_SERVO_LIFT = 12;   // 수압 펌프식 리프트 서보

// ---------------- 상수 ----------------
const int           SERVO_STOP_US       = 1500;     // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int           LIFT_SPEED          = 200;      // 사용자 지정: 펄스 1700us(=1500+200) = 올림, 1300us(=1500-200) = 내림
const int           LIFT_TEST_MS        = 300;      // ?? TODO ??: 리프트 1스텝 시간 미확정
const unsigned long LIFT_FULL_STROKE_MS = 8000UL;   // 최소~최대 높이 총 8초: 하드웨어 제작자의 수동 캘리브레이션 값 (사용자 전달). 1700/1300us 속도에서의 값인지는 미확인 (이 시험에서는 사용하지 않음, 참고용)

// ---------------- 공용 객체 ----------------
Servo servoLift;

// ---------------- 하드웨어 함수 ----------------
void attachLift() {  // ◆ NEW: 서보를 연결하고 정지 펄스를 보낸다
  servoLift.attach(PIN_SERVO_LIFT);
  servoLift.writeMicroseconds(SERVO_STOP_US);
}

void detachLift() {  // ◆ NEW
  servoLift.detach();
}

// ◆ NEW: speed > 0 = 올림(1700us), speed < 0 = 내림(1300us) (사용자 지정). 한계 스위치 정보 없음 -> 짧은 스텝만 수동 조작
void liftStep(int speed) {
  servoLift.writeMicroseconds(SERVO_STOP_US + speed);
  delay(LIFT_TEST_MS);
  servoLift.writeMicroseconds(SERVO_STOP_US);
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  리프트를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 구동 함수는 위쪽 하드웨어 함수 구역에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 1 입력 -> u(올림) / d(내림) 한 스텝씩, q 로 종료
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
  Serial.println(F("\n== Forklift lift test =="));
  Serial.println(F("1 Lift (u/d step, q quit)"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 리프트 서보 ----------------
void testLift() {
  Serial.println(F("Lift: u=up(1700us) d=down(1300us), one step, q=quit"));
  attachLift();
  while (true) {
    int c = waitForChar(INPUT_TIMEOUT_MS);
    if (c == 'q' || c == -1) break;
    if (c == 'u') liftStep(LIFT_SPEED);
    else if (c == 'd') liftStep(-LIFT_SPEED);
  }
  detachLift();
  Serial.println(F("Lift test end"));
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
    case '1': testLift(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
