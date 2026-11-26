// ====================================================================================================
// TOOL (one-time setup, not final) | board: FORKLIFT board (or any UNO) + ONE HC-05 on D3/D4 | sets it as slave and shows its address
// ====================================================================================================
/*
 * tool_bluetooth_hc05_forklift_setup.ino  -  지게차 보드용 HC-05 설정 + 주소 표시 (tool_bluetooth_hc05_setup 을 바탕으로 줄임)
 *
 * 지게차 쪽 HC-05 를 슬레이브(ROLE=0)로 만들고, 센터의 모듈 B(마스터)가 BIND 할 주소를 읽어서 보여 준다.
 * 배선 (사용자 지정): 모듈 RXD -> D3, 모듈 TXD -> D4  (아두이노가 D4 로 받고 D3 으로 보낸다).
 *   D3(아두이노 송신) -> 모듈 RXD 는 3.3V 신호가 필요하므로 1K/2K 분압 권장 (martyncurrey: RX 는 분압, TX 는 직결).
 *   이 핀은 "설정할 때만" 쓴다. final_forklift_board 는 D4 를 IR 발신기로 쓰고 블루투스는 D0/D1 이므로, 설정이 끝나면 배선을 원래대로 돌려야 한다.
 * AT 모드 (EN/STATE 만 있는 ZS-040 형 보드): 보드의 작은 버튼을 누른 채 전원을 넣고 LED 가 약 2초에 한 번 깜빡이면 AT 모드.
 *   (EN 은 연결하지 않는다. 3/4 번 메뉴는 버튼을 계속 누르고 있어야 하는 경우가 있음: martyncurrey)
 * 사용법: 시리얼 모니터 9600 bps. 메뉴:
 *   1 지게차 모듈 설정: ROLE=0 (슬레이브) / 데이터 속도 9600 / 이름 / 비밀번호(PSWD) -> 끝나면 주소를 보여 줌
 *   2 주소 보기: AT+ADDR? -> 화면에 "BIND 용" 쉼표 형식도 함께 표시 (센터 tool_bluetooth_hc05_setup 의 3번 메뉴에 그대로 입력)
 *   3 현재 설정 보기   4 AT+RESET
 *   "AT" 로 시작하는 줄을 입력하면 그대로 모듈에 보낸다.
 * 중요: 비밀번호(PSWD)는 센터 모듈 B 가 페어링할 때 쓰는 값과 같아야 한다 (HC-05 기본값 1234).
 *
 * 명령 출처 (직접 읽어 확인한 자료): martyncurrey.com/?p=1348 (AT 진입, ROLE/UART/NAME), howtomechatronics.com (ROLE/ADDR?), core-electronics.com.au (PSWD/RESET).
 * 미확인: 모듈 펌웨어 버전별 차이, 주소 응답 형식이 모든 모듈에서 "+ADDR:xxxx:xx:xxxxxx" 인지 (martyncurrey 예: +ADDR:98d3:34:905d3f).
 *
 * [표기 규칙]  ★ EXTERNAL : 외부 자료/내장 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <SoftwareSerial.h>  // ★ EXTERNAL: Arduino 내장 라이브러리
#include <string.h>
#include <stdio.h>

// ---------------- 핀맵 (사용자 지정: 모듈 RXD = 3, 모듈 TXD = 4) ----------------
const uint8_t PIN_MODULE_TXD = 4;   // 모듈 TXD 연결 (아두이노가 수신)
const uint8_t PIN_MODULE_RXD = 3;   // 모듈 RXD 연결 (아두이노가 송신, 분압 권장)

// ---------------- 상수 ----------------
const long USB_BAUD        = 9600;    // 시리얼 모니터
const long MODULE_AT_BAUD  = 38400;   // ★ EXTERNAL: martyncurrey, HC-05 AT 모드 속도
const long DATA_BAUD       = 9600;    // final_forklift_board 의 BT_BAUD 와 같음
const char FORKLIFT_NAME[] = "FORKLIFT_HC05";   // ?? TODO ??: 임시 이름 (최대 32자)
const char FORKLIFT_PSWD[] = "1234";            // ?? TODO ??: 임시 비밀번호 (센터 모듈 B 와 같아야 함)
const unsigned long REPLY_TIMEOUT_MS = 2000UL;
const unsigned long INPUT_TIMEOUT_MS = 60000UL;
const uint8_t LINE_MAX = 40;
const uint8_t ADDR_MAX = 24;
const uint8_t REPLY_MAX = 48;

SoftwareSerial btModule(PIN_MODULE_TXD, PIN_MODULE_RXD);  // (rx, tx)
char lineBuf[LINE_MAX];
uint8_t lineLen = 0;
char replyBuf[REPLY_MAX];   // 마지막 명령의 응답 앞부분 (주소 읽기용)

// ---------------- 모듈 응답 ----------------
void drainModule(unsigned long ms) {  // ◆ NEW: 오류 번호 같은 뒷부분을 마저 보여 준다
  unsigned long start = millis();
  while (millis() - start < ms) {
    if (btModule.available() > 0) Serial.write(btModule.read());
  }
}

// 응답을 화면에 그대로 보여 주고 replyBuf 에 앞부분을 모은다. "OK\r\n" 이면 true, FAIL/ERROR/시간 초과면 false
bool waitReply(unsigned long timeoutMs) {  // ◆ NEW
  char tail[9];
  memset(tail, ' ', 8);
  tail[8] = '\0';
  uint8_t n = 0;
  replyBuf[0] = '\0';
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (btModule.available() <= 0) continue;
    char c = btModule.read();
    Serial.write(c);
    if (n < REPLY_MAX - 1) { replyBuf[n++] = c; replyBuf[n] = '\0'; }
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

bool sendUartCmd() {  // ◆ NEW: 데이터 속도 설정 (정지 비트 0, 패리티 0)
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

// ---------------- 주소 ----------------
// replyBuf 의 "+ADDR:98d3:34:905d3f" 에서 주소를 꺼내 colon 형식(addr)과 BIND 용 comma 형식(bindForm)으로 만든다. 실패하면 false
bool parseAddress(char *addr, char *bindForm, size_t outSize) {  // ◆ NEW
  const char *p = strstr(replyBuf, "+ADDR:");
  if (p == NULL) return false;
  p += 6;
  size_t n = 0;
  uint8_t seps = 0, segLen = 0;
  while (*p != '\0' && *p != '\r' && *p != '\n') {
    char c = *p++;
    if (n + 1 >= outSize) return false;
    if (c == ':') {
      if (segLen == 0) return false;
      seps++;
      segLen = 0;
      addr[n] = ':';
      bindForm[n] = ',';
    } else if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')) {
      segLen++;
      addr[n] = c;
      bindForm[n] = c;
    } else {
      return false;
    }
    n++;
  }
  addr[n] = '\0';
  bindForm[n] = '\0';
  return seps == 2 && segLen > 0;
}

void showAddress() {  // ◆ NEW
  char addr[ADDR_MAX], bindForm[ADDR_MAX];
  if (!sendAt("AT+ADDR?", REPLY_TIMEOUT_MS)) {
    Serial.println(F("No address (module not in AT mode? keep the on-board button pressed)"));
    return;
  }
  if (!parseAddress(addr, bindForm, ADDR_MAX)) {
    Serial.println(F("Could not read the address from the reply above - copy it by hand (3 groups)"));
    return;
  }
  Serial.print(F("Forklift module address: "));
  Serial.println(addr);
  Serial.print(F("For the center module B bind (menu 3): "));
  Serial.println(bindForm);
}

// ---------------- 설정 ----------------
bool setupForkliftModule() {  // ◆ NEW: 슬레이브, 9600, 이름, 비밀번호
  if (!sendAt("AT", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+ROLE=0", REPLY_TIMEOUT_MS)) return false;
  if (!sendUartCmd()) return false;
  if (!sendAtFmt("AT+NAME=%s", FORKLIFT_NAME)) return false;
  return sendAtFmt("AT+PSWD=%s", FORKLIFT_PSWD);
}

void printFailHint() {  // ◆ NEW
  Serial.println(F("Failed? Full AT mode needs pin 34 HIGH: keep the on-board button pressed (mini mode runs only some commands)"));
}

void runForkliftSetup() {  // ◆ NEW
  if (setupForkliftModule()) {
    Serial.println(F("Forklift module configured (slave, 9600, name, PSWD)"));
    showAddress();
  } else {
    Serial.println(F("Forklift module setup FAILED"));
    printFailHint();
  }
}

void showSettings() {  // ◆ NEW
  sendAt("AT", REPLY_TIMEOUT_MS);
  sendAt("AT+VERSION?", REPLY_TIMEOUT_MS);
  sendAt("AT+ADDR?", REPLY_TIMEOUT_MS);
  sendAt("AT+ROLE?", REPLY_TIMEOUT_MS);
  sendAt("AT+UART?", REPLY_TIMEOUT_MS);
  sendAt("AT+NAME?", REPLY_TIMEOUT_MS);
  sendAt("AT+PSWD?", REPLY_TIMEOUT_MS);
}

// ---------------- 메뉴 ----------------
void printMenu() {
  Serial.println(F("\n== Forklift HC-05 setup (module RXD=D3, TXD=D4) =="));
  Serial.println(F("1 Set as slave (9600, name, PSWD) and show address"));
  Serial.println(F("2 Show address (AT+ADDR?)"));
  Serial.println(F("3 Show settings   4 AT+RESET"));
  Serial.println(F("Or type a line starting with AT to send it as is"));
  Serial.println(F("Board with EN/STATE only: power up while holding the on-board button, keep holding"));
}

void handleLine(const char *line) {  // ◆ NEW
  if (strncmp(line, "AT", 2) == 0) {
    sendAt(line, REPLY_TIMEOUT_MS);
    return;
  }
  switch (line[0]) {
    case '1': runForkliftSetup(); break;
    case '2': showAddress(); break;
    case '3': showSettings(); break;
    case '4': sendAt("AT+RESET", REPLY_TIMEOUT_MS); break;
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
