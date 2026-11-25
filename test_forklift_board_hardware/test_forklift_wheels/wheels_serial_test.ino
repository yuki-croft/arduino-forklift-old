/*
 * wheels_serial_test.ino  -  구동 바퀴를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 구동 함수는 test_forklift_wheels.ino 에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
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
  Serial.println(F("\n== Forklift wheels test =="));
  Serial.println(F("1 Wheels (6 moves, 1s each)"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 구동 바퀴 ----------------
void testWheels() {
  Serial.println(F("Lift wheels off ground! y=start"));
  if (waitForChar(INPUT_TIMEOUT_MS) != 'y') {
    Serial.println(F("Cancelled"));
    return;
  }
  attachWheels();
  const int s = WHEEL_TEST_SPEED;
  Serial.println(F("Left only"));   maneuver(s, 0, WHEEL_TEST_MS);
  Serial.println(F("Right only"));  maneuver(0, s, WHEEL_TEST_MS);
  Serial.println(F("Forward"));     maneuver(s, s, WHEEL_TEST_MS);
  Serial.println(F("Backward"));    maneuver(-s, -s, WHEEL_TEST_MS);
  Serial.println(F("Spin left"));   maneuver(-s, s, WHEEL_TEST_MS);
  Serial.println(F("Spin right"));  maneuver(s, -s, WHEEL_TEST_MS);
  stopWheels();
  Serial.println(F("Done. Report any reversed wheel"));
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
    case '1': testWheels(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
