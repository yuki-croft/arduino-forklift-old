// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | infrared check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_infrared.ino  -  IR 3방향(발신 1핀 + 수신 3핀) 단독 시험: 핀 / 상수 / 하드웨어 함수 (Serial 미사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [탭 구성]  같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   test_forklift_infrared.ino : 핀 / 상수 / 하드웨어 함수 (이 파일, 항상 먼저 이어 붙음)
 *   infrared_serial_test.ino   : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 규칙: 센서/모듈 세부사항은 사용자 확인 후 반영. 테스트는 "원시값 출력"만 하고 임계값은 판단하지 않는다.
 */

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_IR_TX       = 4;   // 2핀 IR 발신기 (좌/전/우 통합, 1개 핀)
const uint8_t PIN_IR_RX_LEFT  = 8;   // 3핀 IR 수신기 좌측
const uint8_t PIN_IR_RX_FRONT = 9;   // 3핀 IR 수신기 전방
const uint8_t PIN_IR_RX_RIGHT = 13;  // 3핀 IR 수신기 우측 (보드 내장 LED와 핀 공유)

// ---------------- 상수 ----------------
const long IR_FREQ_HZ = 38000;       // ★ EXTERNAL: 5_ABOT적외선센서.pptx 슬라이드 9

// ---------------- 하드웨어 함수 ----------------
void initHardware() {  // ◆ NEW
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_IR_RX_LEFT, INPUT);
  pinMode(PIN_IR_RX_FRONT, INPUT);
  pinMode(PIN_IR_RX_RIGHT, INPUT);
}

// ★ EXTERNAL(변형): 5_ABOT적외선센서.pptx 슬라이드 9의 irDetect()
//   원본은 발신/수신 핀 1:1 구조. 지게차는 발신 1핀(D4) + 수신 3핀이라
//   한 번 발신 후 3개 수신기를 연속 읽도록 변경함. 반환: 0=감지, 1=미감지
void irScan(long frequency, byte &left, byte &front, byte &right) {
  tone(PIN_IR_TX, frequency, 8);   // IR LED 38kHz 8ms
  delay(1);                        // 원본과 동일: 1ms 대기
  left  = digitalRead(PIN_IR_RX_LEFT);
  front = digitalRead(PIN_IR_RX_FRONT);
  right = digitalRead(PIN_IR_RX_RIGHT);
  delay(1);                        // 원본과 동일: 재확인 전 휴지
}
