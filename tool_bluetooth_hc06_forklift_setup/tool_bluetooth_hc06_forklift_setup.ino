// ====================================================================================================
// TOOL (one-time setup, not final) | board: FORKLIFT (FRIBEE EDU / UNO R3) | sets the soldered HC-06 by itself after upload
// ====================================================================================================
/*
 * tool_bluetooth_hc06_forklift_setup.ino  -  지게차 HC-06 자동 설정 (업로드만 하면 끝, 시리얼 모니터/입력 필요 없음)
 *
 * 상황 (사용자): HC-06 과 D0 의 빨간 LED 가 납땜돼 있어서 USB 시리얼 모니터를 쓸 수 없다. 그래서 이 스케치는
 *   Serial.print 로 화면에 아무것도 출력하지 않고, 결과를 부저(D5)로 알려 준다.
 * 가정 (사용자 미확인): HC-06 이 D0/D1 (하드웨어 시리얼) 에 납땜돼 있다. D2/D3 이면 아래 HC06_ON_D0_D1 을 0 으로 바꾼다.
 *
 * 하는 일 (전원/리셋 직후 한 번):
 *   1. 모듈의 현재 속도를 찾는다: 속도 목록을 차례로 바꿔 가며 "AT" 를 보내 "OK" 가 오는 속도를 찾음.
 *   2. AT+NAME<이름>  3. AT+PIN<4자리>  4. 속도가 9600 이 아니면 AT+BAUD4 (9600) 후 "AT" 로 다시 확인.
 *   명령은 대문자, 줄 끝 문자 없이 보낸다 (martyncurrey 의 HC-06 설명).
 * 결과 소리 (3초마다 반복):
 *   성공       : 높은 소리(3000Hz) 길게 1번 (1초)
 *   실패       : 낮은 소리(800Hz) 짧게 N번. N=1 AT 에 응답 없음(어느 속도에서도) / 2 이름 설정 실패 / 3 PIN 설정 실패 / 4 속도 변경 실패
 *   시작 직후  : 짧은 소리 1번 (스케치가 시작됐다는 표시)
 *
 * 사용법: 폰이 모듈에 연결돼 있지 않아야 한다 (연결 중에는 AT 명령이 안 먹음: martyncurrey). 업로드 -> 소리 확인.
 *   이름이 방송에 반영되려면 모듈 전원을 한 번 껐다 켜야 할 수 있음 (martyncurrey).
 *   센터 모듈 B 의 PSWD 는 여기서 정한 PIN 과 같아야 한다.
 * 한계 (martyncurrey): 바꾼 속도와 PIN 은 읽어서 확인할 수 없고, 속도를 잊어도 하드웨어 리셋 방법이 없다.
 *
 * 명령 출처 (직접 읽어 확인): martyncurrey.com/?p=12932 (AT, AT+NAME, AT+PINxxxx, AT+BAUDx 코드표, 이름 최대 20자, PIN 숫자 4자리, 응답 "OK..."),
 *   martyncurrey.com/?p=3424 (hc01.comV2.0 기본값 9600 / 이름 HC-06 / PIN 1234, 응답에 줄 끝 문자 없음).
 * 미확인: 사용자 모듈의 펌웨어 버전, HC-06 의 실제 연결 핀, 업로드 중 D0 에 연결된 HC-06 TX 가 업로드를 방해하는지, 명령 사이 대기 시간(여기서는 1초로 넉넉히 잡음).
 *
 * [표기 규칙]  ★ EXTERNAL : 외부 자료  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#define HC06_ON_D0_D1 1   // ?? TODO ??: 1 = 하드웨어 시리얼(D0/D1), 0 = D2/D3 소프트웨어 시리얼. 사용자 확인 필요

#if HC06_ON_D0_D1
  #define BT_PORT Serial
#else
  #include <SoftwareSerial.h>   // ★ EXTERNAL: Arduino 내장 라이브러리
  SoftwareSerial btSoft(2, 3);  // (rx, tx) = 모듈 TXD -> D2, 모듈 RXD <- D3
  #define BT_PORT btSoft
#endif

// ---------------- 핀맵 (사용자 제공 값) ----------------
const uint8_t PIN_BUZZER = 5;   // 수동 부저

// ---------------- 상수 ----------------
const char HC06_NAME[] = "FORKLIFT";   // ?? TODO ??: 임시 이름 (최대 20자)
const char HC06_PIN[]  = "1234";       // ?? TODO ??: 임시 PIN (숫자 4자리). 센터 모듈 B 의 PSWD 와 같아야 함
const long BAUD_TRY[]  = {9600, 38400, 115200, 19200, 57600, 4800, 2400, 1200};  // ◆ NEW: 기본값 9600 을 먼저
const uint8_t BAUD_TRY_COUNT = sizeof(BAUD_TRY) / sizeof(BAUD_TRY[0]);
const long TARGET_BAUD = 9600;                 // final_forklift_board 의 BT_BAUD 와 같음
const char TARGET_BAUD_CMD[] = "AT+BAUD4";     // ★ EXTERNAL: 4 = 9600 (martyncurrey 코드표)
const unsigned long REPLY_FIRST_MS = 1000UL;   // 첫 글자를 기다리는 시간
const unsigned long REPLY_GAP_MS   = 150UL;    // 글자 사이가 이만큼 비면 응답 끝 (응답에 줄 끝 문자가 없으므로)
const unsigned long STEP_PAUSE_MS  = 1000UL;   // ?? TODO ??: 명령 사이 대기
const unsigned long REPEAT_MS      = 3000UL;
const uint8_t FAIL_NO_AT = 1, FAIL_NAME = 2, FAIL_PIN = 3, FAIL_BAUD = 4;

uint8_t resultCode = 0;   // 0 = 성공, 그 외 = 실패 번호

// ---------------- 부저 ----------------
void beep(unsigned int hz, unsigned long ms) {  // ◆ NEW (test_forklift_buzzer 의 buzzerBeep 과 같음)
  tone(PIN_BUZZER, hz, ms);
  delay(ms);
}

void reportResult() {  // ◆ NEW
  if (resultCode == 0) {
    beep(3000, 1000);
    return;
  }
  for (uint8_t i = 0; i < resultCode; i++) {
    beep(800, 150);
    delay(250);
  }
}

// ---------------- HC-06 통신 ----------------
// 응답을 모은다: 첫 글자는 REPLY_FIRST_MS 까지, 이후 REPLY_GAP_MS 동안 글자가 없으면 끝.
size_t readReply(char *buf, size_t size) {  // ◆ NEW
  size_t n = 0;
  unsigned long last = millis();
  unsigned long limit = REPLY_FIRST_MS;
  while (millis() - last < limit) {
    if (BT_PORT.available() > 0) {
      char c = BT_PORT.read();
      if (n < size - 1) buf[n++] = c;
      last = millis();
      limit = REPLY_GAP_MS;
    }
  }
  buf[n] = '\0';
  return n;
}

bool replyIsOk(const char *buf) {  // ◆ NEW: "OK", "OKsetname", "OK9600" ... 모두 "OK" 로 시작
  return buf[0] == 'O' && buf[1] == 'K';
}

// 대문자 명령을 줄 끝 문자 없이 보내고 "OK..." 가 오면 true
bool sendCmd(const char *cmd) {  // ◆ NEW
  char reply[24];
  while (BT_PORT.available() > 0) BT_PORT.read();   // 오래된 바이트 버림
  BT_PORT.print(cmd);
  BT_PORT.flush();
  readReply(reply, sizeof(reply));
  return replyIsOk(reply);
}

bool findModuleBaud(long &foundBaud) {  // ◆ NEW
  for (uint8_t i = 0; i < BAUD_TRY_COUNT; i++) {
    BT_PORT.begin(BAUD_TRY[i]);
    delay(100);
    if (sendCmd("AT")) {
      foundBaud = BAUD_TRY[i];
      return true;
    }
  }
  return false;
}

bool sendNameCmd() {  // ◆ NEW: AT+NAME<이름>
  char cmd[32];
  snprintf(cmd, sizeof(cmd), "AT+NAME%s", HC06_NAME);
  return sendCmd(cmd);
}

bool sendPinCmd() {  // ◆ NEW: AT+PIN<4자리>
  char cmd[16];
  snprintf(cmd, sizeof(cmd), "AT+PIN%s", HC06_PIN);
  return sendCmd(cmd);
}

uint8_t configureHc06() {  // ◆ NEW: 0 = 성공, 아니면 실패 번호
  long baud = 0;
  if (!findModuleBaud(baud)) return FAIL_NO_AT;
  delay(STEP_PAUSE_MS);
  if (!sendNameCmd()) return FAIL_NAME;
  delay(STEP_PAUSE_MS);
  if (!sendPinCmd()) return FAIL_PIN;
  if (baud != TARGET_BAUD) {
    delay(STEP_PAUSE_MS);
    if (!sendCmd(TARGET_BAUD_CMD)) return FAIL_BAUD;
    delay(STEP_PAUSE_MS);
    BT_PORT.begin(TARGET_BAUD);                       // 모듈은 새 속도로 바뀜
    delay(100);
    if (!sendCmd("AT")) return FAIL_BAUD;
  }
  return 0;
}

// ---------------- setup / loop ----------------
void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  beep(2000, 100);              // 시작 표시
  delay(500);
  resultCode = configureHc06();
}

void loop() {
  reportResult();
  delay(REPEAT_MS);
}
