/*
 * photo_serial_test.ino  -  광원 추적 센서를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기 함수는 test_forklift_photo.ino 에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

const long SERIAL_BAUD        = 9600;
const int  SAMPLE_COUNT       = 10;
const int  SAMPLE_INTERVAL_MS = 300;

void printMenu() {
  Serial.println(F("\n== Forklift photo test =="));
  Serial.println(F("1 Photo (A0,A1) x10"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 광원 추적 ----------------
void testPhoto() {
  Serial.println(F("Photo A0 raw/V   A1 raw/V"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int l, r;
    readPhotoRaw(l, r);
    Serial.print(l); Serial.print(" / ");
    Serial.print(volts(PIN_PHOTO_LEFT));  Serial.print("V    ");
    Serial.print(r); Serial.print(" / ");
    Serial.print(volts(PIN_PHOTO_RIGHT)); Serial.println("V");
    delay(SAMPLE_INTERVAL_MS);
  }
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
    case '1': testPhoto(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
