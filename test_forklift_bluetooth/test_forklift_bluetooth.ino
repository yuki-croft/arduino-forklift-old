// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | Bluetooth check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_bluetooth.ino  -  블루투스(HC-06, 슬레이브) 단독 시험: 핀 / 상수 / 객체 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT / 외부 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <NeoSWSerial.h> // ★ EXTERNAL LIBRARY: "NeoSWSerial" by SlashDevin (라이브러리 매니저에서 설치)
                         //   https://github.com/SlashDevin/NeoSWSerial  (HC-05/06 블루투스용 소프트웨어 시리얼)

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_BT_RX = 2;   // 블루투스 TXD 연결 (아두이노가 수신) - 장착 예정
const uint8_t PIN_BT_TX = 3;   // 블루투스 RXD 연결 (아두이노가 송신) - 장착 예정

// ---------------- 상수 ----------------
const long BT_BAUD = 9600;     // ?? TODO ??: 모듈 보드레이트 미확인 (NeoSWSerial은 9600/19200/38400 등 제한)

// ---------------- 공용 객체 ----------------
// 인자 순서(rx, tx): README가 "SoftwareSerial의 drop-in 대체"라고 밝힌 점에 근거한 추정 (헤더 미확인).
// 블루투스 수신이 전혀 안 되면 이 순서부터 의심할 것.
NeoSWSerial bt(PIN_BT_RX, PIN_BT_TX);

// ---------------- 하드웨어 함수 ----------------
void initHardware() {  // ◆ NEW
  pinMode(PIN_BT_RX, INPUT);
  pinMode(PIN_BT_TX, OUTPUT);
}

void btStart() {  // ◆ NEW
  bt.begin(BT_BAUD);
}

// 수신한 바이트 1개를 돌려준다. 없으면 -1.
int btReadByte() {  // ◆ NEW
  return (bt.available() > 0) ? bt.read() : -1;
}

// ★ EXTERNAL LIBRARY: NeoSWSerial. 송신(write) 중에는 인터럽트가 꺼져 수신/서보에 영향
//   -> 송신은 최소화할 것.
void btSendText(const char *text) {
  while (*text) bt.write(*text++);
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  블루투스 수신을 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 송수신 함수는 위쪽 하드웨어 함수 구역에 있다.
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
