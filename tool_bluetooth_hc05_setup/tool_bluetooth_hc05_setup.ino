// ====================================================================================================
// TOOL (one-time setup, not final) | board: any UNO + ONE HC-05 at a time | configures the 2 center-board HC-05 modules
// ====================================================================================================
/*
 * tool_bluetooth_hc05_setup.ino  -  센터 보드용 HC-05 2개 사전 설정 (한 번에 모듈 1개씩 연결)
 *
 * 센터 보드의 블루투스 모듈은 둘 다 HC-05 (사용자 확인). 역할:
 *   모듈 A (폰용)        : 슬레이브 (ROLE=0). 폰 앱이 접속한다.      -> 최종 배선 D2/D3
 *   모듈 B (지게차 링크) : 마스터 (ROLE=1) + 지게차 HC-06 주소에 고정 (CMODE=0, BIND). -> 최종 배선 D0/D1
 * 최종 코드(final_center_board)가 두 링크 모두 9600 baud 로 쓰므로 두 모듈의 데이터 속도를 9600 으로 맞춘다.
 *
 * 사용법: 배선은 wiring_hc05_setup.png 참고. 사용자 모듈은 KEY 핀 없이 EN, STATE 핀만 있는 보드 (ZS-040 형).
 *   AT 모드: 보드의 작은 버튼(3.3V 를 칩 34번 핀에 연결, martyncurrey)을 누른 채 전원을 넣고 LED 가 약 2초에 한 번 깜빡이면 AT 모드.
 *   버튼을 잠깐 누르고 떼면 일부 명령만 되는 "mini" 모드: 검색(INQ)/이름 조회(NAME?) 등은 버튼을 계속 누르고 있어야 함 (전체 AT 모드).
 *   EN 은 연결하지 않는다 (martyncurrey: EN 을 LOW 로 하면 모듈이 꺼짐). STATE 는 연결 표시용이라 설정에 필요 없음.
 *   시리얼 모니터 9600 bps. 줄 끝 설정은 상관없음 (이 스케치가 CR+LF 를 붙여 보낸다). 메뉴:
 *   1 모듈 A 설정 / 2 모듈 B: 주변 장치 검색 / 3 모듈 B: 지게차 주소에 고정 / 4 현재 설정 보기 / 5 AT+RESET
 *   "AT" 로 시작하는 줄을 입력하면 그대로 모듈에 보낸다 (예: AT+PSWD=..., AT+PAIR=..., AT+LINK=...).
 *
 * 명령 출처 (직접 읽어 확인한 자료): martyncurrey.com/?p=1348 (AT 진입, ROLE/UART/NAME, INQ/PAIR/BIND/LINK 순서, 주소는 쉼표),
 *   howtomechatronics.com (ROLE/CMODE/BIND/ADDR?), core-electronics.com.au (PSWD/RESET).
 * 미확인: 사용자 보드에 위 버튼이 있는지, EN 이 칩 34번 핀에 연결됐는지 (martyncurrey 댓글: 일부 ZS-040 은 연결 안 됨), 모듈 펌웨어 버전별 차이 (martyncurrey: 3.0 펌웨어와 FC-114 는 다르게 동작할 수 있음), 검색 소요 시간, 지게차 HC-06 PIN.
 *
 * [표기 규칙]  ★ EXTERNAL : 외부 자료/내장 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <SoftwareSerial.h>  // ★ EXTERNAL: Arduino 내장 라이브러리 (IoT12 PPT 슬라이드 10 과 같은 사용법)
#include <string.h>
#include <stdio.h>

// ---------------- 핀맵 (기존 tool_bluetooth_at_setup 과 같음) ----------------
const uint8_t PIN_MODULE_TXD = 2;   // 모듈 TXD 연결 (아두이노가 수신)
const uint8_t PIN_MODULE_RXD = 3;   // 모듈 RXD 연결 (아두이노가 송신, 분압 필요)

// ---------------- 상수 ----------------
const long USB_BAUD        = 9600;    // 시리얼 모니터
const long MODULE_AT_BAUD  = 38400;   // ★ EXTERNAL: martyncurrey, HC-05 AT 모드 속도 (바꿀 수 없다고 적혀 있음)
const long DATA_BAUD       = 9600;    // final_center_board 의 LINK_BAUD / PHONE_BAUD 와 같아야 함
const char PHONE_MODULE_NAME[] = "FORKLIFT_PHONE";   // ?? TODO ??: 임시 이름
const char LINK_MODULE_NAME[]  = "FORKLIFT_LINK";    // ?? TODO ??: 임시 이름
const unsigned long REPLY_TIMEOUT_MS = 2000UL;
const unsigned long SCAN_TIMEOUT_MS  = 60000UL;      // ?? TODO ??: AT+INQ 소요 시간 미확인
const unsigned long INPUT_TIMEOUT_MS = 60000UL;
const uint8_t LINE_MAX = 40;
const uint8_t ADDR_MAX = 24;

SoftwareSerial btModule(PIN_MODULE_TXD, PIN_MODULE_RXD);  // (rx, tx)
char lineBuf[LINE_MAX];
uint8_t lineLen = 0;

// ---------------- 모듈 응답 ----------------
// 마지막 8글자만 보고 "OK\r\n" / "FAIL" / "ERROR" 를 찾는다 (INQ 목록처럼 긴 응답도 화면에 그대로 흘려 보냄)
bool waitReply(unsigned long timeoutMs) {  // ◆ NEW
  char tail[9];
  memset(tail, ' ', 8);
  tail[8] = '\0';
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (btModule.available() <= 0) continue;
    char c = btModule.read();
    Serial.write(c);
    memmove(tail, tail + 1, 7);
    tail[7] = c;
    if (strcmp(tail + 4, "OK\r\n") == 0) return true;
    if (strstr(tail, "FAIL") != NULL || strstr(tail, "ERROR") != NULL) {
      drainModule(150);
      return false;
    }
  }
  Serial.println(F("\n(timeout: no OK from module)"));
  return false;
}

void drainModule(unsigned long ms) {  // ◆ NEW: 오류 번호 같은 뒷부분을 마저 보여 준다
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (btModule.available() > 0) Serial.write(btModule.read());
  }
}

// 명령 하나를 CR+LF 와 함께 보낸다 (HC-05 는 \r\n 이 없으면 응답하지 않음: martyncurrey)
bool sendAt(const char *cmd, unsigned long timeoutMs) {  // ◆ NEW
  while (btModule.available() > 0) btModule.read();   // 오래된 바이트 버림
  Serial.print(F("> "));
  Serial.println(cmd);
  btModule.print(cmd);
  btModule.print(F("\r\n"));
  return waitReply(timeoutMs);
}

bool sendAtFmt(const char *fmt, const char *arg) {  // ◆ NEW: 문자열 하나를 끼워 만든 명령
  char cmd[LINE_MAX + 16];
  snprintf(cmd, sizeof(cmd), fmt, arg);
  return sendAt(cmd, REPLY_TIMEOUT_MS);
}

bool sendUartCmd() {  // ◆ NEW: 데이터 속도 설정 (정지 비트 0, 패리티 0 은 martyncurrey 예제와 같은 형식)
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+UART=%ld,0,0", DATA_BAUD);
  return sendAt(cmd, REPLY_TIMEOUT_MS);
}

// ---------------- 입력 ----------------
bool pollUsbLine() {  // ◆ NEW: 한 줄이 완성되면 true (lineBuf 에 들어 있음)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (lineLen == 0) continue;
      lineBuf[lineLen] = '\0';
      lineLen = 0;
      return true;
    }
    if (lineLen < LINE_MAX - 1) lineBuf[lineLen++] = c;
  }
  return false;
}

bool readLineWait(unsigned long timeoutMs) {  // ◆ NEW
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (pollUsbLine()) return true;
  }
  return false;
}

// 주소 "98d3:34:905d3f" 또는 "98d3,34,905d3f" -> BIND 용 쉼표 형식. 16진 글자 3묶음이 아니면 false
bool normalizeAddress(const char *in, char *out, size_t outSize) {  // ◆ NEW
  uint8_t seps = 0, segLen = 0;
  size_t n = strlen(in);
  if (n == 0 || n >= outSize) return false;
  for (size_t i = 0; i < n; i++) {
    char c = in[i];
    if (c == ':' || c == ',') {
      if (segLen == 0) return false;
      seps++;
      segLen = 0;
      out[i] = ',';
    } else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
      segLen++;
      out[i] = c;
    } else {
      return false;
    }
  }
  out[n] = '\0';
  return seps == 2 && segLen > 0;
}

// ---------------- 설정 순서 ----------------
bool setupPhoneModule() {  // ◆ NEW: 모듈 A = 슬레이브
  if (!sendAt("AT", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+ROLE=0", REPLY_TIMEOUT_MS)) return false;
  if (!sendUartCmd()) return false;
  return sendAtFmt("AT+NAME=%s", PHONE_MODULE_NAME);
}

// ★ EXTERNAL(변형): martyncurrey.com/?p=1348 댓글의 검색 순서 중 RMAAD, ROLE=1, INIT, INQ. INIT 은 이미 초기화돼 있으면 오류일 수 있어 무시
bool scanForForklift() {  // ◆ NEW: 모듈 B 로 주변 블루투스를 찾는다 (지게차 HC-06 전원을 켜 둘 것)
  if (!sendAt("AT", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+RMAAD", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+ROLE=1", REPLY_TIMEOUT_MS)) return false;
  sendAt("AT+INIT", REPLY_TIMEOUT_MS);
  Serial.println(F("Scanning... look for the forklift HC-06 address (+INQ:...)"));
  return sendAt("AT+INQ", SCAN_TIMEOUT_MS);
}

bool bindToForklift(const char *addr) {  // ◆ NEW: 모듈 B = 마스터, 주소 고정
  if (!sendAt("AT", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+ROLE=1", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+CMODE=0", REPLY_TIMEOUT_MS)) return false;
  if (!sendAtFmt("AT+BIND=%s", addr)) return false;
  if (!sendUartCmd()) return false;
  return sendAtFmt("AT+NAME=%s", LINK_MODULE_NAME);
}

void runPhoneSetup() {  // ◆ NEW
  if (setupPhoneModule()) {
    Serial.println(F("Module A configured"));
  } else {
    Serial.println(F("Module A setup FAILED"));
    printFailHint();
  }
}

void askAddressAndBind() {  // ◆ NEW
  char addr[ADDR_MAX];
  Serial.println(F("Type the forklift module address (colons or commas), then Enter:"));
  if (!readLineWait(INPUT_TIMEOUT_MS)) {
    Serial.println(F("Cancelled (no input)"));
    return;
  }
  if (!normalizeAddress(lineBuf, addr, sizeof(addr))) {
    Serial.println(F("Bad address. Expected 3 hex groups like 98d3:34:905d3f"));
    return;
  }
  if (bindToForklift(addr)) {
    Serial.println(F("Module B configured"));
  } else {
    Serial.println(F("Module B setup FAILED"));
    printFailHint();
  }
}

void showSettings() {  // ◆ NEW: 일부 조회는 34번 핀이 HIGH(버튼 누름) 일 때만 응답 (martyncurrey)
  sendAt("AT", REPLY_TIMEOUT_MS);
  sendAt("AT+VERSION?", REPLY_TIMEOUT_MS);
  sendAt("AT+ADDR?", REPLY_TIMEOUT_MS);
  sendAt("AT+ROLE?", REPLY_TIMEOUT_MS);
  sendAt("AT+UART?", REPLY_TIMEOUT_MS);
  sendAt("AT+NAME?", REPLY_TIMEOUT_MS);
}

// ---------------- 메뉴 ----------------
void printFailHint() {  // ◆ NEW: 버튼만 쓰는 보드에서 흔한 실패 원인
  Serial.println(F("Failed? Full AT mode needs pin 34 HIGH: keep the on-board button pressed (mini mode runs only some commands)"));
}

void printMenu() {
  Serial.println(F("\n== HC-05 setup (one module at a time) =="));
  Serial.println(F("1 Module A (phone side): slave, 9600, name"));
  Serial.println(F("2 Module B (link): scan for forklift HC-06"));
  Serial.println(F("3 Module B (link): master + bind to address"));
  Serial.println(F("4 Show settings   5 AT+RESET"));
  Serial.println(F("Or type a line starting with AT to send it as is"));
  Serial.println(F("Board with EN/STATE only: power up while holding the on-board button, keep holding for 2/3/4"));
}

void handleLine(const char *line) {  // ◆ NEW
  if (strncmp(line, "AT", 2) == 0) {
    sendAt(line, REPLY_TIMEOUT_MS);
    return;
  }
  switch (line[0]) {
    case '1': runPhoneSetup(); break;
    case '2': if (!scanForForklift()) printFailHint(); break;
    case '3': askAddressAndBind(); break;
    case '4': showSettings(); break;
    case '5': sendAt("AT+RESET", REPLY_TIMEOUT_MS); break;
    default:  Serial.println(F("Unknown cmd"));
  }
}

void setup() {
  Serial.begin(USB_BAUD);
  btModule.begin(MODULE_AT_BAUD);
  printMenu();
}

void loop() {
  if (!pollUsbLine()) return;
  handleLine(lineBuf);
  printMenu();
}
