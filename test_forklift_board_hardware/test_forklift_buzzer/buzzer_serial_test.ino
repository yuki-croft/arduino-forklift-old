/*
 * buzzer_serial_test.ino  -  부저를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 동작 함수는 test_forklift_buzzer.ino 에 있다.
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
