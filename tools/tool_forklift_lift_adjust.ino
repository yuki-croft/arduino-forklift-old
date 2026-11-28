// ====================================================================================================
// TOOL (not final) | board: FORKLIFT | move the lift by hand from a phone Bluetooth terminal; do not use as the final program
// ====================================================================================================
/*
 * tool_forklift_lift_adjust.ino  -  리프트 조절 (폰 블루투스 터미널 앱으로 명령, USB 시리얼 모니터 불필요)
 * 보드: FRIBEE EDU (UNO R3 호환). 지게차 블루투스 모듈 = 하드웨어 시리얼 D0/D1, 9600. 리프트 서보 D12.
 * 리프트 값은 final_forklift_board_* 와 같음: 올림 1300us / 내림 1700us (LIFT_INVERT), 최하단 4.5cm ~ 최상단 12.5cm 를 8초.
 * 높이는 시간으로 추정한다 (센서 없음). 전원을 켤 때 리프트가 최하단이라고 본다 (사용자: 시작할 때 최하단).
 *
 * [쓰는 법] 업로드 때는 모듈 선(D0/D1)을 뺀다. 센터 보드 전원은 끄고, 폰 터미널 앱으로 지게차 모듈에 연결한다.
 *   글자 하나씩 보낸다 (줄 끝 설정 상관없음). 매 명령 뒤 지금 추정 높이를 보낸다.
 *     u / d : 0.1cm 올림 / 내림 (0.1초)        U / D : 1cm 올림 / 내림 (1초)
 *     1 : 하단 칸 진입 높이 5.2cm   2 : 상단 칸 진입 높이 12.2cm   3 : 하단 칸 들기 6.2cm   4 : 최상단 12.5cm
 *     b : 최하단으로 (끝까지 내린 뒤 0 으로 맞춤)   z : 지금 높이를 최하단(4.5cm)으로 다시 맞춤   p : 높이 보기
 *   추정 범위 밖(4.5cm 아래, 12.5cm 위)으로는 움직이지 않는다. 실제 높이와 다르면 b 나 z 로 다시 맞춘다.
 *   전원을 켜면 짧은 삐 1번.
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <Servo.h>   // Arduino 표준 라이브러리

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_SERVO_LIFT = 12;
const uint8_t PIN_BUZZER     = 5;

// ---------------- 상수 (final_forklift_board_* 와 같은 값) ----------------
const long          BT_BAUD          = 9600;
const int           SERVO_STOP_US    = 1500;     // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int           LIFT_SPEED       = 200;      // 사용자 지정 (1500 +- 200)
const bool          LIFT_INVERT      = true;     // 사용자 시험 결과: 올림 1300us / 내림 1700us
const unsigned long LIFT_FULL_MS     = 8000UL;   // 최하단 ~ 최상단 8초
const float         LIFT_BASE_CM     = 4.5;      // 사용자: 최하단 4.5cm
const float         LIFT_STROKE_CM   = 8.0;      // 사용자: 4.5 ~ 12.5cm
const unsigned long HOME_EXTRA_MS    = 1000UL;   // ?? TODO ?? b: 끝까지 확실히 내리려고 8초 + 1초

Servo servoLift;
long liftPosMs = 0;   // 추정 위치 (0 = 최하단, LIFT_FULL_MS = 최상단)

// ---------------- 하드웨어 함수 ----------------
void liftDrive(int dir) {  // ◆ NEW: +1 올림, -1 내림, 0 정지 (final 의 liftDrive 와 같은 규칙)
  if (dir == 0) { servoLift.detach(); return; }
  servoLift.attach(PIN_SERVO_LIFT);
  servoLift.writeMicroseconds(SERVO_STOP_US + (LIFT_INVERT ? -dir : dir) * LIFT_SPEED);
}

long cmToMs(float cm) {  // ◆ NEW (반올림)
  if (cm <= LIFT_BASE_CM) return 0;
  return (long)((cm - LIFT_BASE_CM) * LIFT_FULL_MS / LIFT_STROKE_CM + 0.5f);
}

void liftMoveTo(long targetMs) {  // ◆ NEW: 추정 위치에서 목표까지 시간만큼 움직임 (범위 밖은 자름)
  if (targetMs < 0) targetMs = 0;
  if (targetMs > (long)LIFT_FULL_MS) targetMs = LIFT_FULL_MS;
  long delta = targetMs - liftPosMs;
  if (delta == 0) return;
  liftDrive(delta > 0 ? 1 : -1);
  delay((unsigned long)(delta > 0 ? delta : -delta));
  liftDrive(0);
  liftPosMs = targetMs;
}

void liftHome() {  // ◆ NEW: 끝까지 내린 뒤 0 으로
  liftDrive(-1);
  delay(LIFT_FULL_MS + HOME_EXTRA_MS);
  liftDrive(0);
  liftPosMs = 0;
}

// ====================================================================================================
// SERIAL TEST SECTION  (the Bluetooth module is on the hardware Serial D0/D1; output goes to the phone terminal)
// ====================================================================================================
void reportHeight() {
  Serial.print(F("lift "));
  Serial.print(LIFT_BASE_CM + LIFT_STROKE_CM * liftPosMs / LIFT_FULL_MS, 1);
  Serial.print(F(" cm ("));
  Serial.print(liftPosMs);
  Serial.println(F(" ms)"));
}

void handleCommand(char c) {
  switch (c) {
    case 'u': liftMoveTo(liftPosMs + 100); break;
    case 'd': liftMoveTo(liftPosMs - 100); break;
    case 'U': liftMoveTo(liftPosMs + 1000); break;
    case 'D': liftMoveTo(liftPosMs - 1000); break;
    case '1': liftMoveTo(cmToMs(5.2)); break;
    case '2': liftMoveTo(cmToMs(12.2)); break;
    case '3': liftMoveTo(cmToMs(6.2)); break;
    case '4': liftMoveTo(LIFT_FULL_MS); break;
    case 'b': Serial.println(F("going to the bottom (9 s)")); liftHome(); break;
    case 'z': liftPosMs = 0; Serial.println(F("set: now = bottom")); break;
    case 'p': break;
    default: return;   // 줄 끝 글자 등은 무시
  }
  reportHeight();
}

void setup() {
  Serial.begin(BT_BAUD);
  pinMode(PIN_BUZZER, OUTPUT);
  tone(PIN_BUZZER, 3000, 150);
  Serial.println(F("lift adjust: u/d 0.1cm  U/D 1cm  1=5.2 2=12.2 3=6.2 4=top  b=bottom z=zero p=show"));
  reportHeight();
}

void loop() {
  while (Serial.available() > 0) handleCommand((char)Serial.read());
}
