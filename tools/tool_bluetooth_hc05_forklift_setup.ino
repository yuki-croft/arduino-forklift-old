// ====================================================================================================
// TOOL (one-time setup, not final) | board: FORKLIFT board (or any UNO) + ONE HC-05 on D2/D3 | sets it as slave and shows its address
// ====================================================================================================
/*
 * tool_bluetooth_hc05_forklift_setup.ino  -  지게차 보드용 HC-05 설정 + 주소 표시 (tool_bluetooth_hc05_setup 을 바탕으로 줄임)
 *
 * 지게차 쪽 HC-05 를 슬레이브(ROLE=0)로 만들고, 센터의 모듈 B(마스터)가 BIND 할 주소를 읽어서 보여 준다.
 * 배선 (사용자 지정 2026-11-26): 아두이노 RX = D2, TX = D3  ->  모듈 TXD -> D2, 모듈 RXD -> D3 (SoftwareSerial(2, 3), 이 저장소의 test_forklift_bluetooth 와 같은 표기).
 *   D3(아두이노 송신) -> 모듈 RXD 는 3.3V 신호가 필요하므로 1K/2K 분압 권장 (martyncurrey: RX 는 분압, TX 는 직결).
 *   반대로 연결했다면 아래 PIN_MODULE_TXD 와 PIN_MODULE_RXD 숫자를 서로 바꾼다.
 *   이 핀은 "설정할 때만" 쓴다. final_forklift_board 는 블루투스를 D0/D1 로 쓰므로, 설정이 끝나면 배선을 원래대로 돌려야 한다.
 * AT 모드 (사용자 보드: 버튼 없음, KEY 핀 없음, EN/STATE 만 있음):
 *   Arduino 포럼 글(forum.arduino.cc/t/setting-hc05-zs-040-to-at-mode-code-example/1059616)의 방법: EN 핀을 3.3V 에 연결(5V 는 안 됨)한 채로
 *   전원을 넣으면 AT 모드 (그 글: LED 가 약 3초 켜지고 3초 꺼짐, 38400 baud, 첫 "AT" 에 ERROR:(0) 이 나올 수 있고 다시 보내면 OK).
 *   !! 한 사용자의 글이며 이 모듈에서 확인된 것이 아니다. martyncurrey.com/?p=1895 는 "EN 을 LOW 로 하면 모듈이 꺼진다"고만 적고 이 방법은 다루지 않는다.
 *   안 되면 그 글과 달리 모듈 칩 34번 핀에 3.3V 를 직접 연결해야 하는 보드일 수 있다 (martyncurrey: 버튼이 34번 핀에 3.3V 를 연결함).
 * 사용법: 시리얼 모니터 9600 bps. 메뉴:
 *   1 지게차 모듈 설정: ROLE=0 (슬레이브) / 데이터 속도 9600 / 이름 / 비밀번호(PSWD) -> 끝나면 주소를 보여 줌
 *   2 주소 보기: AT+ADDR? -> 화면에 "BIND 용" 쉼표 형식도 함께 표시 (센터 tool_bluetooth_hc05_setup 의 3번 메뉴에 그대로 입력)
 *   3 현재 설정 보기   4 AT+RESET
 *   5 (역할 바꾸기 시험, 2026-11-26) 지게차 모듈을 마스터로: RMAAD, ROLE=1, CMODE=0, BIND=센터 모듈 B 주소, PSWD, UART 9600
 *   6 (역할 바꾸기 시험) 센터 모듈 B 에 PAIR + LINK: INIT(오류 무시), PAIR=주소,9, LINK=주소 (martyncurrey.com/?p=2098 순서)
 *     센터 모듈 B 는 먼저 슬레이브로 바꿔 둔다 (tool_bluetooth_hc05_setup 메뉴 7). 이 시험은 "모듈 B 가 마스터일 때만 데이터가 안 넘어간다"는 의심을 가르려는 것.
 *     CMODE 값의 뜻은 자료마다 반대 (martyncurrey: 0 = 아무 장치, HC-05 설명서 EPFL 사본: 0 = 마지막 페어링 장치). 지금까지 CMODE=0 + BIND 로 연결(LED)은 됐으므로 0 을 쓴다.
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

// ---------------- 핀맵 (사용자 지정: 아두이노 RX = 2, TX = 3) ----------------
const uint8_t PIN_MODULE_TXD = 2;   // 모듈 TXD 연결 (아두이노가 수신 = RX)
const uint8_t PIN_MODULE_RXD = 3;   // 모듈 RXD 연결 (아두이노가 송신 = TX, 분압 권장)

// ---------------- 상수 ----------------
const long USB_BAUD        = 9600;    // 시리얼 모니터
const long MODULE_AT_BAUD  = 38400;   // ★ EXTERNAL: martyncurrey, HC-05 AT 모드 속도
const long DATA_BAUD       = 9600;    // final_forklift_board 의 BT_BAUD 와 같음
const char FORKLIFT_NAME[] = "FORKLIFT_HC05";   // ?? TODO ??: 임시 이름 (최대 32자)
const char FORKLIFT_PSWD[] = "1234";            // ?? TODO ??: 임시 비밀번호 (센터 모듈 B 와 같아야 함)
const char CENTER_B_ADDR[] = "2025,05,0086A4";  // 센터 모듈 B 주소 (사용자가 보낸 AT+ADDR? 응답 "+ADDR:2025:05:0086A4", 2026-11-26) 를 쉼표 형식으로
const char LINK_MASTER_NAME[] = "FORKLIFT_MASTER";   // ?? TODO ??: 임시 이름 (마스터로 바꿀 때)
const unsigned long PAIR_TIMEOUT_MS = 12000UL;        // PAIR=주소,9 (9초) 보다 조금 길게
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
    Serial.println(F("No address (module not in AT mode? see the hint below)"));
    printFailHint();
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
// 첫 "AT" 에 ERROR:(0) 이 나올 수 있다는 포럼 글 때문에, 실패하면 한 번 더 보낸다
bool wakeAt() {  // ◆ NEW
  if (sendAt("AT", REPLY_TIMEOUT_MS)) return true;
  return sendAt("AT", REPLY_TIMEOUT_MS);
}

bool setupForkliftModule() {  // ◆ NEW: 슬레이브, 9600, 이름, 비밀번호
  if (!wakeAt()) return false;
  if (!sendAt("AT+ROLE=0", REPLY_TIMEOUT_MS)) return false;
  if (!sendUartCmd()) return false;
  if (!sendAtFmt("AT+NAME=%s", FORKLIFT_NAME)) return false;
  return sendAtFmt("AT+PSWD=%s", FORKLIFT_PSWD);
}

// ◆ NEW (2026-11-26): 역할 바꾸기 시험 - 지게차 모듈 = 마스터, 센터 모듈 B 에 BIND
bool setupForkliftMaster() {
  if (!wakeAt()) return false;
  if (!sendAt("AT+RMAAD", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+ROLE=1", REPLY_TIMEOUT_MS)) return false;
  if (!sendAt("AT+CMODE=0", REPLY_TIMEOUT_MS)) return false;
  if (!sendAtFmt("AT+BIND=%s", CENTER_B_ADDR)) return false;
  if (!sendAtFmt("AT+PSWD=%s", FORKLIFT_PSWD)) return false;
  if (!sendUartCmd()) return false;
  return sendAtFmt("AT+NAME=%s", LINK_MASTER_NAME);
}

void runForkliftMaster() {  // ◆ NEW
  if (setupForkliftMaster()) {
    Serial.println(F("Forklift module configured as MASTER bound to center module B"));
    Serial.println(F("Remove EN from 3.3V and power-cycle both modules (center module B must be slave: center tool menu 7)"));
  } else {
    Serial.println(F("Forklift master setup FAILED"));
    printFailHint();
  }
}

// ◆ NEW (2026-11-26): martyncurrey.com/?p=2098 의 INIT -> PAIR -> LINK. INIT 오류(17 등, 이 모듈 펌웨어에서는 0 이 보고됨)는 무시하고 계속
void pairAndLinkCenter() {
  char cmd[LINE_MAX + 16];
  if (!wakeAt()) { printFailHint(); return; }
  sendAt("AT+INIT", REPLY_TIMEOUT_MS);
  snprintf(cmd, sizeof(cmd), "AT+PAIR=%s,9", CENTER_B_ADDR);
  if (!sendAt(cmd, PAIR_TIMEOUT_MS)) { Serial.println(F("PAIR FAILED (center module B on, slave, not connected to anything?)")); return; }
  if (!sendAtFmt("AT+LINK=%s", CENTER_B_ADDR)) { Serial.println(F("LINK FAILED")); return; }
  Serial.println(F("PAIR + LINK OK"));
}

void printFailHint() {  // ◆ NEW
  Serial.println(F("Failed? AT mode: EN to 3.3V (NOT 5V) BEFORE power-up, 38400 baud; send AT again if the 1st reply is ERROR (forum report, unverified)"));
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
  wakeAt();
  sendAt("AT+VERSION?", REPLY_TIMEOUT_MS);
  sendAt("AT+ADDR?", REPLY_TIMEOUT_MS);
  sendAt("AT+ROLE?", REPLY_TIMEOUT_MS);
  sendAt("AT+UART?", REPLY_TIMEOUT_MS);
  sendAt("AT+NAME?", REPLY_TIMEOUT_MS);
  sendAt("AT+PSWD?", REPLY_TIMEOUT_MS);
  sendAt("AT+CMODE?", REPLY_TIMEOUT_MS);   // ◆ 2026-11-26 추가
  sendAt("AT+BIND?", REPLY_TIMEOUT_MS);
}

// ---------------- 메뉴 ----------------
void printMenu() {
  Serial.println(F("\n== Forklift HC-05 setup (module TXD=D2, RXD=D3) =="));
  Serial.println(F("1 Set as slave (9600, name, PSWD) and show address"));
  Serial.println(F("2 Show address (AT+ADDR?)"));
  Serial.println(F("3 Show settings   4 AT+RESET"));
  Serial.println(F("5 Set as MASTER bound to center module B (2025,05,0086A4)"));
  Serial.println(F("6 PAIR + LINK to center module B"));
  Serial.println(F("Or type a line starting with AT to send it as is"));
  Serial.println(F("No button/KEY: tie EN to 3.3V (NOT 5V) before powering up, then use 1/2/3"));
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
    case '5': runForkliftMaster(); break;
    case '6': pairAndLinkCenter(); break;
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
