// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | photo-transistor check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_photo.ino  -  광원 추적(포토 트랜지스터 2개) 단독 시험: 핀 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 규칙: 센서 세부사항은 사용자 확인 후 반영. 테스트는 "원시값 출력"만 하고 임계값/극성은 판단하지 않는다.
 */

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_PHOTO_LEFT  = A0;  // 광원 추적(포토 트랜지스터) 왼쪽
const uint8_t PIN_PHOTO_RIGHT = A1;  // 광원 추적(포토 트랜지스터) 오른쪽

// ---------------- 하드웨어 함수 ----------------
// ★ EXTERNAL: 4_ABOT빛으로로봇조종.pptx 슬라이드 11 (volts 함수 그대로)
float volts(int adPin) {
  return float(analogRead(adPin)) * 5.0 / 1023.0;
}

void readPhotoRaw(int &left, int &right) {  // ◆ NEW: analogRead 원시값 (0~1023)
  left  = analogRead(PIN_PHOTO_LEFT);
  right = analogRead(PIN_PHOTO_RIGHT);
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  광원 추적 센서를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기 함수는 위쪽 하드웨어 함수 구역에 있다.
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
