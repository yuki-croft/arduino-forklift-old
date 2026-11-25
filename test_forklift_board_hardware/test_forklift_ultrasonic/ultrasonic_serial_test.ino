/*
 * ultrasonic_serial_test.ino  -  후방 초음파를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기 함수는 test_forklift_ultrasonic.ino 에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

const long SERIAL_BAUD        = 9600;
const int  SAMPLE_COUNT       = 10;
const int  SAMPLE_INTERVAL_MS = 300;

void printMenu() {
  Serial.println(F("\n== Forklift sonar test =="));
  Serial.println(F("1 Sonar cm x10"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 초음파 (HC-SR04) ----------------
// 센서 앞(지게차에서는 후방)에 물체가 없을 때 어떤 값이 찍히는지도 이 테스트로 확인할 것.
void testUltrasonic() {
  Serial.println(F("Sonar cm (rear sensor; also check no-object value)"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    long distanceCm = readRearDistanceCm();
    Serial.println(distanceCm);
    delay(SAMPLE_INTERVAL_MS);
  }
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
    case '1': testUltrasonic(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
