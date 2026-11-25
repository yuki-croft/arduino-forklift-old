// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | infrared check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_infrared.ino  -  IR 3방향(발신 1핀 + 수신 3핀) 단독 시험: 핀 / 상수 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 규칙: 센서/모듈 세부사항은 사용자 확인 후 반영. 테스트는 "원시값 출력"만 하고 임계값은 판단하지 않는다.
 */

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_IR_TX       = 4;   // 2핀 IR 발신기 (좌/전/우 통합, 1개 핀)
const uint8_t PIN_IR_RX_LEFT  = 8;   // 3핀 IR 수신기 좌측
const uint8_t PIN_IR_RX_FRONT = 9;   // 3핀 IR 수신기 전방
const uint8_t PIN_IR_RX_RIGHT = 13;  // 3핀 IR 수신기 우측 (보드 내장 LED와 핀 공유)

// ---------------- 상수 ----------------
const long IR_FREQ_HZ = 38000;       // ★ EXTERNAL: 5_ABOT적외선센서.pptx 슬라이드 9

// ---------------- 하드웨어 함수 ----------------
void initHardware() {  // ◆ NEW
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_IR_RX_LEFT, INPUT);
  pinMode(PIN_IR_RX_FRONT, INPUT);
  pinMode(PIN_IR_RX_RIGHT, INPUT);
}

// ★ EXTERNAL(변형): 5_ABOT적외선센서.pptx 슬라이드 9의 irDetect()
//   원본은 발신/수신 핀 1:1 구조. 지게차는 발신 1핀(D4) + 수신 3핀이라
//   한 번 발신 후 3개 수신기를 연속 읽도록 변경함. 반환: 0=감지, 1=미감지
void irScan(long frequency, byte &left, byte &front, byte &right) {
  tone(PIN_IR_TX, frequency, 8);   // IR LED 38kHz 8ms
  delay(1);                        // 원본과 동일: 1ms 대기
  left  = digitalRead(PIN_IR_RX_LEFT);
  front = digitalRead(PIN_IR_RX_FRONT);
  right = digitalRead(PIN_IR_RX_RIGHT);
  delay(1);                        // 원본과 동일: 재확인 전 휴지
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  IR 3방향을 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기 함수는 위쪽 하드웨어 함수 구역에 있다.
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
