/*
 * lift_serial_test.ino  -  리프트를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 구동 함수는 test_forklift_lift.ino 에 있다.
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
