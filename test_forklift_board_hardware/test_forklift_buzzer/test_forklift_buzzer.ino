// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | buzzer check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_buzzer.ino  -  부저 단독 시험: 핀 / 상수 / 하드웨어 함수 (이 파일은 Serial 을 쓰지 않는다)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [탭 구성]  같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   test_forklift_buzzer.ino : 핀 / 상수 / 하드웨어 함수 (이 파일, 항상 먼저 이어 붙음)
 *   buzzer_serial_test.ino   : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 규칙: 센서/모듈 세부사항은 사용자 확인 후 반영. 테스트는 "동작 확인"만 한다.
 */

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_BUZZER = 5;   // 수동 부저

// ---------------- 상수 ----------------
const unsigned int  BUZZER_TEST_HZ = 3000;   // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 10 의 예제 주파수
const unsigned long BUZZER_TEST_MS = 1000;

// ---------------- 하드웨어 함수 ----------------
void initHardware() {  // ◆ NEW
  pinMode(PIN_BUZZER, OUTPUT);
}

// ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 10 (핀 4 -> PIN_BUZZER 로만 변경). 소리가 끝날 때까지 기다린다
void buzzerBeep(unsigned int frequency, unsigned long ms) {
  tone(PIN_BUZZER, frequency, ms);
  delay(ms);
}
