// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | buzzer check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_buzzer.ino  -  부저 단독 시험: 핀 / 상수 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
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

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  부저를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 동작 함수는 위쪽 하드웨어 함수 구역에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

const long SERIAL_BAUD = 9600;

void printMenu() {
  Serial.println(F("\n== Forklift buzzer test =="));
  Serial.println(F("1 Buzzer 3kHz 1s"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 부저 ----------------
void testBuzzer() {
  buzzerBeep(BUZZER_TEST_HZ, BUZZER_TEST_MS);
  Serial.println(F("Buzzer 3kHz 1s done"));
}

// ---------------- setup / loop (시리얼 메뉴) ----------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  initHardware();
  printMenu();
}

void loop() {
  if (Serial.available() <= 0) return;
  int cmd = Serial.read();
  switch (cmd) {
    case '1': testBuzzer(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
