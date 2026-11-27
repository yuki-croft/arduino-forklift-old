// ====================================================================================================
// TOOL (one-time, not final) | board: forklift or center UNO + ONE HC-05 on D2/D3 | factory reset without a serial monitor
// ====================================================================================================
/*
 * tool_bluetooth_hc05_factory_reset.ino  -  HC-05 공장 초기화 (시리얼 모니터 없이, 업로드하면 바로 실행, 결과는 부저)
 *
 * 하는 일: AT (최대 3번) -> AT+ORGL (공장 기본값으로 되돌림) -> AT+UART=9600,0,0 (SET_UART_9600_AFTER = true 일 때)
 *   AT+ORGL 의 기본값 (DFRobot TEL0023 위키): 슬레이브, 페어링 코드 1234, 이름 HHW-SPP-1800-2, 시리얼 38400.
 *   같은 위키의 UART 항목은 기본값을 9600,0,0 이라고 적어 서로 다르다. 그래서 초기화 뒤 9600 을 직접 다시 넣는다 (이 프로젝트 코드는 모두 9600).
 *   CMODE, BIND 기본값은 그 위키에 없음. 모듈마다 기본값이 다를 수 있음 (미확인).
 * 배선: 모듈 TXD -> D2, 모듈 RXD -> D3 (분압), VCC, GND. 지게차 모듈이 D0/D1 에 있으면 이 시험 동안만 D2/D3 로 옮긴다.
 * AT 모드: EN 을 3.3V (5V 아님) 에 연결한 채로 전원을 넣는다 (사용자 보드에서 이 방법으로 AT 응답 확인됨). 끝나면 EN 을 떼고 다시 켠다.
 * 부저: FORKLIFT_BOARD = true 면 D5 (지게차 부저), false 면 D13 (센터 부저, 사용자 답 "13번").
 * 결과 소리 (반복):
 *   성공 = 높은 소리 1초 (5초마다 반복)
 *   실패 = 낮은 짧은 소리 N번 (3초마다 반복): 1번 = AT 무응답 (AT 모드 아님/배선) / 2번 = AT+ORGL 거부 / 3번 = AT+UART 거부
 * 명령 형식: CR+LF 를 붙임 (martyncurrey: HC-05 는 줄 끝이 없으면 응답하지 않음), AT 모드 속도 38400 (martyncurrey).
 *
 * [표기 규칙]  ★ EXTERNAL : 외부 자료/내장 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <SoftwareSerial.h>  // ★ EXTERNAL: Arduino 내장 라이브러리
#include <string.h>

// ---------------- 설정 ----------------
const bool    FORKLIFT_BOARD      = true;    // true = 지게차 보드 (부저 D5), false = 센터 보드 (부저 D13)
const bool    SET_UART_9600_AFTER = true;    // 초기화 뒤 데이터 속도를 9600 으로 (이 프로젝트 코드와 맞춤)
const uint8_t PIN_BUZZER          = FORKLIFT_BOARD ? 5 : 13;
const uint8_t PIN_MODULE_TXD      = 2;       // 모듈 TXD 연결 (아두이노 RX)
const uint8_t PIN_MODULE_RXD      = 3;       // 모듈 RXD 연결 (아두이노 TX, 분압)

// ---------------- 상수 ----------------
const long          MODULE_AT_BAUD   = 38400;    // ★ EXTERNAL: martyncurrey, HC-05 AT 모드 속도
const unsigned long BOOT_WAIT_MS     = 1500UL;   // ?? TODO ?? 모듈이 켜질 때까지 기다림 (제 임시값)
const unsigned long REPLY_TIMEOUT_MS = 2000UL;
const unsigned long ORGL_WAIT_MS     = 1000UL;   // ?? TODO ?? 초기화 뒤 기다림 (제 임시값)
const uint8_t       AT_TRIES         = 3;        // 첫 AT 에 ERROR:(0) 이 나올 수 있다는 포럼 글 때문에 여러 번
const unsigned int  OK_HZ            = 3000;
const unsigned int  FAIL_HZ          = 800;

enum ResetResult : uint8_t { RESET_OK = 0, FAIL_NO_AT = 1, FAIL_ORGL = 2, FAIL_UART = 3 };

SoftwareSerial btModule(PIN_MODULE_TXD, PIN_MODULE_RXD);  // (rx, tx)
uint8_t result = RESET_OK;

// ---------------- 모듈 ----------------
bool waitOk(unsigned long timeoutMs) {  // ◆ NEW: "OK\r\n" 이면 true, ERROR/FAIL/시간 초과면 false
  char tail[9];
  memset(tail, ' ', 8);
  tail[8] = '\0';
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (btModule.available() <= 0) continue;
    memmove(tail, tail + 1, 7);
    tail[7] = (char)btModule.read();
    if (strcmp(tail + 4, "OK\r\n") == 0) return true;
    if (strstr(tail, "FAIL") != NULL || strstr(tail, "ERROR") != NULL) return false;
  }
  return false;
}

bool sendAt(const char *cmd) {  // ◆ NEW
  while (btModule.available() > 0) btModule.read();   // 오래된 바이트 버림
  btModule.print(cmd);
  btModule.print("\r\n");
  return waitOk(REPLY_TIMEOUT_MS);
}

bool wakeAt() {  // ◆ NEW
  for (uint8_t i = 0; i < AT_TRIES; i++) {
    if (sendAt("AT")) return true;
    delay(300);
  }
  return false;
}

uint8_t runFactoryReset() {  // ◆ NEW
  if (!wakeAt()) return FAIL_NO_AT;
  if (!sendAt("AT+ORGL")) return FAIL_ORGL;
  delay(ORGL_WAIT_MS);
  if (!SET_UART_9600_AFTER) return RESET_OK;
  wakeAt();
  if (!sendAt("AT+UART=9600,0,0")) return FAIL_UART;
  return RESET_OK;
}

// ---------------- 부저 ----------------
void beep(unsigned int hz, unsigned long ms) {  // ◆ NEW: 소리가 끝날 때까지 기다림
  tone(PIN_BUZZER, hz, ms);
  delay(ms);
}

void playResult(uint8_t code) {  // ◆ NEW
  if (code == RESET_OK) {
    beep(OK_HZ, 1000);
    delay(4000);
    return;
  }
  for (uint8_t i = 0; i < code; i++) {
    beep(FAIL_HZ, 200);
    delay(250);
  }
  delay(3000);
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  btModule.begin(MODULE_AT_BAUD);
  delay(BOOT_WAIT_MS);
  result = runFactoryReset();
}

void loop() {
  playResult(result);
}
