/*
 * infrared_serial_test.ino  -  IR 3방향을 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기 함수는 test_forklift_infrared.ino 에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

const long SERIAL_BAUD        = 9600;
const int  SAMPLE_COUNT       = 10;
const int  SAMPLE_INTERVAL_MS = 300;

void printMenu() {
  Serial.println(F("\n== Forklift IR test =="));
  Serial.println(F("1 IR (L/F/R) x10"));
  Serial.println(F("Select:"));
}

// ---------------- 1. IR 3방향 ----------------
void testIr() {
  Serial.println(F("IR 0=detect 1=none  L F R"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    byte l, f, r;
    irScan(IR_FREQ_HZ, l, f, r);
    Serial.print(l); Serial.print(' ');
    Serial.print(f); Serial.print(' ');
    Serial.println(r);
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
    case '1': testIr(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
