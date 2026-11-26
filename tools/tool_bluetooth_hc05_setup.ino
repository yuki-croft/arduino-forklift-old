// ====================================================================================================
// TOOL (one-time setup, not final) | board: any UNO + ONE HC-05 at a time | configures the 2 center-board HC-05 modules
// ====================================================================================================
/*
 * tool_bluetooth_hc05_setup.ino  -  센터 보드용 HC-05 2개 사전 설정 (한 번에 모듈 1개씩 연결)
 *
 * 역할: 모듈 A (폰용) = 슬레이브 ROLE=0, 최종 배선 D2/D3 / 모듈 B (지게차 링크) = 마스터 ROLE=1 + 지게차 모듈 주소에 BIND, 최종 배선 D0/D1. 데이터 속도는 둘 다 9600.
 * (2026-11-26 사용자: 폰용은 HC-06 으로 바꿈, 지게차 모듈은 HC-05. 모듈 A 메뉴는 HC-05 일 때만 해당.)
 * 배선 (설정할 때): 모듈 TXD -> D2, 모듈 RXD -> D3 (분압). wiring_hc05_setup.png 참고. 시리얼 모니터 9600, 줄 끝 설정 상관없음 (CR+LF 를 붙여 보냄).
 * AT 모드: 사용자 보드는 버튼/KEY 없음 (EN, STATE 만). 포럼 글 한 건의 방법: EN 을 3.3V (5V 아님) 에 연결한 채 전원을 넣는다 (이 모듈에서 응답 확인됨). 설정 뒤 EN 을 떼고 다시 켠다.
 * 메뉴: 1 모듈 A 설정 / 2 모듈 B 주변 검색 / 3 모듈 B 를 입력한 주소에 고정 (98:DA:60:01:B1:60 형식도 됨) / 4 현재 설정 보기 / 5 AT+RESET
 *   6 모듈 B 를 지게차 모듈 Yuki (98:DA:60:01:B1:60, PIN 1234) 에 고정: BIND=98DA,60,01B160 + PSWD=1234
 *   7 (역할 바꾸기 시험) 모듈 B 를 슬레이브로: ROLE=0, PSWD=1234, UART 9600. 되돌리려면 6.
 *   "AT" 로 시작하는 줄은 그대로 모듈에 보낸다 (예: AT+PAIR=..., AT+LINK=...).
 * 명령 출처: martyncurrey.com/?p=1348, ?p=2098 (ROLE/UART/NAME, INQ/PAIR/BIND/LINK, 주소는 쉼표), howtomechatronics.com, core-electronics.com.au.
 * 미확인: CMODE 값의 뜻 (자료마다 반대), 펌웨어 5.0-20220104 의 동작 차이 (AT+INIT 이 ERROR:(0)), 검색 소요 시간.
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
// 사용자가 알려 준 지게차 모듈 값 (2026-11-26): 이름 Yuki, 주소 98:DA:60:01:B1:60, 비밀번호/PIN 1234. 이미 설정돼 있어 바꿀 수 없다고 함.
const char FORKLIFT_MODULE_NAME[] = "Yuki";
const char FORKLIFT_MODULE_ADDR[] = "98:DA:60:01:B1:60";   // 6바이트 형식 -> BIND 형식 NAP,UAP,LAP = 98DA,60,01B160 (아래 normalizeAddress)
const char FORKLIFT_MODULE_PSWD[] = "1234";                // 모듈 B(마스터) 의 PSWD 도 같게 설정
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

// 주소를 BIND 용 쉼표 형식 NAP,UAP,LAP 로 바꾼다. 받는 형식 두 가지:
//   (1) 모듈 형식 3묶음 "98d3:34:905d3f" 또는 "98d3,34,905d3f" (그대로 쉼표로)
//   (2) 일반 6바이트 형식 "98:DA:60:01:B1:60" (각 묶음 정확히 16진 2글자) -> 앞 2바이트 + "," + 3번째 바이트 + "," + 나머지 3바이트 = "98DA,60,01B160"
// 그 밖에는 false
bool normalizeAddress(const char *in, char *out, size_t outSize) {  // ◆ NEW
  uint8_t groups = 0, segLen = 0, segLens[6];
  size_t n = strlen(in);
  if (n == 0 || n >= outSize) return false;
  for (size_t i = 0; i < n; i++) {
    char c = in[i];
    if (c == ':' || c == ',') {
      if (segLen == 0 || groups >= 6) return false;
      segLens[groups++] = segLen;
      segLen = 0;
    } else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
      segLen++;
    } else {
      return false;
    }
  }
  if (segLen == 0 || groups >= 6) return false;
  segLens[groups++] = segLen;
  if (groups == 3) {
    for (size_t i = 0; i < n; i++) out[i] = (in[i] == ':') ? ',' : in[i];
    out[n] = '\0';
    return true;
  }
  if (groups != 6) return false;
  for (uint8_t g = 0; g < 6; g++) if (segLens[g] != 2) return false;
  if (outSize < 15) return false;
  const char *p = in;
  size_t o = 0;
  for (uint8_t g = 0; g < 6; g++) {
    out[o++] = p[0];
    out[o++] = p[1];
    p += 3;                                  // 두 글자 + 구분 글자
    if (g == 1 || g == 2) out[o++] = ',';    // 98DA , 60 , 01B160
  }
  out[o] = '\0';
  return true;
}

// ---------------- 설정 순서 ----------------
bool setupPhoneModule() {  // ◆ NEW: 모듈 A = 슬레이브
  if (!sendAt("AT", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+ROLE=0", REPLY_TIMEOUT_MS)) return false;
  if (!sendUartCmd()) return false;
  return sendAtFmt("AT+NAME=%s", PHONE_MODULE_NAME);
}

// ★ EXTERNAL(변형): martyncurrey.com/?p=1348 댓글의 검색 순서 중 RMAAD, ROLE=1, INIT, INQ. INIT 은 이미 초기화돼 있으면 오류일 수 있어 무시
bool scanForForklift() {  // ◆ NEW: 모듈 B 로 주변 블루투스를 찾는다 (지게차 모듈 전원을 켜 둘 것)
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
  if (!sendAtFmt("AT+PSWD=%s", FORKLIFT_MODULE_PSWD)) return false;   // 지게차 모듈 PIN 과 같게
  if (!sendUartCmd()) return false;
  return sendAtFmt("AT+NAME=%s", LINK_MODULE_NAME);
}

void report(bool ok, const __FlashStringHelper *okMsg, const __FlashStringHelper *failMsg) {  // ◆ NEW: 결과 한 줄 + 실패 힌트
  Serial.println(ok ? okMsg : failMsg);
  if (!ok) printFailHint();
}

void runPhoneSetup() { report(setupPhoneModule(), F("Module A configured"), F("Module A setup FAILED")); }  // ◆ NEW
void bindAndReport(const char *addr) { report(bindToForklift(addr), F("Module B configured"), F("Module B setup FAILED")); }  // ◆ NEW

void askAddressAndBind() {  // ◆ NEW
  char addr[ADDR_MAX];
  Serial.println(F("Type the forklift module address (e.g. 98:DA:60:01:B1:60 or 98d3:34:905d3f), then Enter:"));
  if (!readLineWait(INPUT_TIMEOUT_MS)) {
    Serial.println(F("Cancelled (no input)"));
    return;
  }
  if (!normalizeAddress(lineBuf, addr, sizeof(addr))) {
    Serial.println(F("Bad address. Expected 6 bytes like 98:DA:60:01:B1:60 or 3 groups like 98d3:34:905d3f"));
    return;
  }
  bindAndReport(addr);
}

void bindToYuki() {  // ◆ NEW: 사용자가 알려 준 지게차 모듈 값으로 바로 연결 (입력 없음)
  char addr[ADDR_MAX];
  if (!normalizeAddress(FORKLIFT_MODULE_ADDR, addr, sizeof(addr))) {
    Serial.println(F("Preset address is invalid"));
    return;
  }
  Serial.print(F("Binding to "));
  Serial.print(FORKLIFT_MODULE_NAME);
  Serial.print(F(" "));
  Serial.print(FORKLIFT_MODULE_ADDR);
  Serial.print(F(" -> "));
  Serial.println(addr);
  bindAndReport(addr);
}

void showSettings() {  // ◆ NEW
  sendAt("AT", REPLY_TIMEOUT_MS);
  sendAt("AT+VERSION?", REPLY_TIMEOUT_MS);
  sendAt("AT+ADDR?", REPLY_TIMEOUT_MS);
  sendAt("AT+ROLE?", REPLY_TIMEOUT_MS);
  sendAt("AT+UART?", REPLY_TIMEOUT_MS);
  sendAt("AT+NAME?", REPLY_TIMEOUT_MS);
  sendAt("AT+CMODE?", REPLY_TIMEOUT_MS);   // ◆ 2026-11-26 추가: 링크 점검용 (이 모듈에서 응답하는지 미확인)
  sendAt("AT+BIND?", REPLY_TIMEOUT_MS);
  sendAt("AT+PSWD?", REPLY_TIMEOUT_MS);
  sendAt("AT+STATE?", REPLY_TIMEOUT_MS);
}

// ---------------- 메뉴 ----------------
bool setupLinkSlave() {  // ◆ NEW (2026-11-26): 역할 바꾸기 시험용
  if (!sendAt("AT", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+ROLE=0", REPLY_TIMEOUT_MS)) return false;
  if (!sendAtFmt("AT+PSWD=%s", FORKLIFT_MODULE_PSWD)) return false;
  if (!sendUartCmd()) return false;
  return sendAtFmt("AT+NAME=%s", LINK_MODULE_NAME);
}

void runLinkSlave() {  // ◆ NEW
  report(setupLinkSlave(), F("Module B configured as SLAVE (for the swapped-role test; menu 6 restores master)"), F("Module B slave setup FAILED"));
}

void printFailHint() {  // ◆ NEW
  Serial.println(F("Failed? AT mode: tie EN to 3.3V (NOT 5V) before power-up (forum report); module answers at 38400"));
}

void printMenu() {
  Serial.println(F("\n== HC-05 setup (one module at a time) =="));
  Serial.println(F("1 Module A (phone side): slave, 9600, name"));
  Serial.println(F("2 Module B (link): scan for forklift HC-06"));
  Serial.println(F("3 Module B (link): master + bind to address"));
  Serial.println(F("4 Show settings (VERSION ADDR ROLE UART NAME CMODE BIND PSWD STATE)   5 AT+RESET"));
  Serial.println(F("6 Module B (link): master + bind to the preset forklift module (Yuki 98:DA:60:01:B1:60, PSWD 1234)"));
  Serial.println(F("7 Module B (link): SLAVE for the swapped-role test (ROLE=0, PSWD 1234, 9600)"));
  Serial.println(F("Or type a line starting with AT to send it as is"));
  Serial.println(F("No button on the board: tie EN to 3.3V (NOT 5V) before power-up, then use 1-6 (forum report, unverified)"));
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
    case '6': bindToYuki(); break;
    case '7': runLinkSlave(); break;
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
