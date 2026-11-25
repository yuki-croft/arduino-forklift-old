/*
 * bluetooth_serial_test.ino  -  블루투스 수신을 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 송수신 함수는 test_forklift_bluetooth.ino 에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 1 입력 -> 15초 동안 마스터(다른 UNO)가 보낸 글자를 출력
 */

const long          SERIAL_BAUD = 9600;
const unsigned long BT_TEST_MS  = 15000UL;   // ◆ NEW: 블루투스 수신 테스트 시간

void printMenu() {
  Serial.println(F("\n== Forklift BT test =="));
  Serial.println(F("1 BT receive 15s"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 블루투스 (HC-05/06, 슬레이브) ----------------
// ★ EXTERNAL LIBRARY: NeoSWSerial. 마스터(다른 UNO)가 보낸 바이트를 USB 시리얼로 출력.
void testBluetooth() {
  Serial.println(F("BT rx 15s: send from master"));
  btStart();
  btSendText("BT OK\n");
  unsigned long start = millis();
  while (millis() - start < BT_TEST_MS) {
    int c = btReadByte();
    if (c >= 0) {
      Serial.print(F("rx "));
      Serial.print((char)c);
      Serial.print(' ');
      Serial.println(c);
    }
  }
  Serial.println(F("BT test end"));
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
    case '1': testBluetooth(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
