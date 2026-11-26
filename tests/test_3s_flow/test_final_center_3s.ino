// ====================================================================================================
// TEST CODE (not final) | board: CENTER (generic UNO R3) | single-file copy of final_center_board (detach the HC-05 from D0/D1 before upload)
// ====================================================================================================
/*
 * test_final_center_3s.ino  -  3초 전진 시험용 센터 보드 (단일 파일)
 *
 * 이 파일은 final_center_board 의 세 탭(final_center_board.ino + center_hw.ino + center_logic.ino)을 그대로 이어 붙인 것이다.
 * 로직은 final 과 같다: 시험용 지게차(test_final_forklift_3s.ino)가 final 과 같은 1바이트 프로토콜을 쓰기 때문에
 * 센터는 바꿀 필요가 없다 (폰 명령 -> 지게차 전송, 지게차 보고 -> 폰 메시지, 구간 '1' -> 좌하 LED, 완료 'c' -> LED 끄기, 점유 기록).
 * 시험 중 폰 앱에서 볼 메시지 순서: JOB x>y, ST TO LOAD, (LINE LOST), ST APPROACH, ST LIFT UP, ST TO UNLOAD, ST UNLOAD, ST DONE, ST WAIT.
 * 주의: final 코드를 고치면 이 파일도 다시 만들어야 한다 (tests/test_pc 의 시험이 두 코드가 같은지 확인한다).
 */

#include <SoftwareSerial.h>  // ★ EXTERNAL: Arduino 내장 라이브러리 (IoT12 PPT 슬라이드 10 과 같은 사용법)

// ---------------- 핀맵 (D4~D7 은 제안값, 사용자 미확인) ----------------
const uint8_t PIN_PHONE_RX = 2;    // 폰 모듈 TXD 연결 (아두이노가 수신)
const uint8_t PIN_PHONE_TX = 3;    // 폰 모듈 RXD 연결 (아두이노가 송신)
// 시계 방향 순서: 0 = 좌하(아래 구간), 1 = 좌상(왼쪽 구간), 2 = 우상(위쪽 구간), 3 = 우하(오른쪽 구간)
const uint8_t LED_PINS_CW[4] = {7, 4, 5, 6};
const uint8_t LED_ON_LEVEL   = HIGH;   // ?? TODO ??: LED 배선 극성 미확인

// ---------------- 상수 ----------------
const long LINK_BAUD  = 9600;      // 지게차 링크 (HC-05 설정 보레이트와 같아야 함, 사전 설정 때 맞출 것)
const long PHONE_BAUD = 9600;      // 폰 링크: 폰용 HC-05 를 tool_bluetooth_hc05_setup 으로 9600 에 맞춤
const char START_CHAR = 'S';       // ?? TODO ??: 앱의 시작 버튼에 지정할 글자 (임시)
const uint8_t CELL_COUNT = 6;
const uint8_t QUEUE_SIZE = 8;

// 폰으로 보낼 메시지 번호 (문구는 center_hw.ino 의 msgText)
const uint8_t MSG_NONE = 0, MSG_SEL_LOAD = 1, MSG_SEL_UNLOAD = 2, MSG_EMPTY = 3, MSG_BUSY = 4,
              MSG_JOB = 5, MSG_ST_WAIT = 6, MSG_ST_TO_LOAD = 7, MSG_ST_APPROACH = 8,
              MSG_ST_LIFT_UP = 9, MSG_ST_REVERSE = 10, MSG_ST_TO_UNLOAD = 11,
              MSG_ST_UNLOAD = 12, MSG_ST_RETURN = 13, MSG_ST_DONE = 14, MSG_EMERGENCY = 15,
              MSG_LINE_LOST = 16, MSG_JOB_RUNNING = 17;   // JOB_RUNNING 은 제가 추가한 문구 (사용자 미확인)

// ---------------- 상태 ----------------
bool loadFull[CELL_COUNT]   = {true, true, true, true, true, true};       // 상차 6칸: 처음에는 모두 짐 있음 (사용자 확인)
bool unloadFull[CELL_COUNT] = {false, false, false, false, false, false}; // 하차 6칸: 처음에는 모두 비어 있음 (사용자 확인)
int8_t selLoad   = -1;     // 선택한 상차 칸 (0~5), -1 = 없음
int8_t selUnload = -1;     // 선택한 하차 칸 (0~5), -1 = 없음
int8_t lastLedIdx = -1;    // 가장 최근에 켠 LED 번호 (LED_PINS_CW 순서). 작업이 끝나도 기억 (사용자 확인), -1 = 아직 켠 적 없음
bool picked      = false;  // 이번 작업에서 이미 짐을 실었는지 (지게차가 'b','m','d' 를 보고하면 true)
bool jobActive   = false;
bool emergency   = false;

// ---------------- 공용 객체 ----------------
SoftwareSerial phone(PIN_PHONE_RX, PIN_PHONE_TX);  // (rx, tx), IoT12 PPT 슬라이드 10 과 같은 핀 구성

// 폰 송신 큐 (지게차 링크 수신이 비어 있을 때만 송신: 소프트웨어 시리얼 송신 중 인터럽트가 꺼지므로)
uint8_t queueId[QUEUE_SIZE];
char    queueArg1[QUEUE_SIZE];
char    queueArg2[QUEUE_SIZE];
uint8_t queueHead = 0, queueCount = 0;

void setup() {
  initMasterHw();
}

void loop() {
  while (Serial.available() > 0) handleForkliftByte((char)Serial.read());
  while (phone.available() > 0)  handlePhoneByte((char)phone.read());
  flushPhoneQueue();
}

// ======== center_hw.ino ========
void ledsOff() {  // ◆ NEW
  const uint8_t offLevel = (LED_ON_LEVEL == HIGH) ? LOW : HIGH;
  for (uint8_t i = 0; i < 4; i++) digitalWrite(LED_PINS_CW[i], offLevel);
}

// idx: LED_PINS_CW 순서 번호. 한 번에 1개만 켠다 (사용자 확인).
void setLedIndex(int8_t idx) {  // ◆ NEW
  ledsOff();
  if (idx >= 0 && idx < 4) digitalWrite(LED_PINS_CW[idx], LED_ON_LEVEL);
}

void initMasterHw() {  // ◆ NEW
  for (uint8_t i = 0; i < 4; i++) pinMode(LED_PINS_CW[i], OUTPUT);
  ledsOff();
  Serial.begin(LINK_BAUD);   // 지게차 링크 (하드웨어 시리얼)
  phone.begin(PHONE_BAUD);   // 폰 링크 (소프트웨어 시리얼)
}

void sendToForklift(char c) {  // ◆ NEW: 1바이트, 응답 없음
  Serial.write(c);
}

// ---------------- 폰 메시지 ----------------
// 문구는 모두 16자 이내 영문 (16x2 LCD 차선책에도 그대로 사용 가능, 플래시 절약)
const __FlashStringHelper *msgText(uint8_t id) {  // ◆ NEW
  switch (id) {
    case MSG_SEL_LOAD:    return F("SEL LOAD");
    case MSG_SEL_UNLOAD:  return F("SEL UNLOAD");
    case MSG_EMPTY:       return F("EMPTY");
    case MSG_BUSY:        return F("BUSY");
    case MSG_JOB:         return F("JOB");
    case MSG_ST_WAIT:     return F("ST WAIT");
    case MSG_ST_TO_LOAD:  return F("ST TO LOAD");
    case MSG_ST_APPROACH: return F("ST APPROACH");
    case MSG_ST_LIFT_UP:  return F("ST LIFT UP");
    case MSG_ST_REVERSE:  return F("ST REVERSE");
    case MSG_ST_TO_UNLOAD: return F("ST TO UNLOAD");
    case MSG_ST_UNLOAD:   return F("ST UNLOAD");
    case MSG_ST_RETURN:   return F("ST RETURN");
    case MSG_ST_DONE:     return F("ST DONE");
    case MSG_EMERGENCY:   return F("EMERGENCY");
    case MSG_LINE_LOST:   return F("LINE LOST");
    case MSG_JOB_RUNNING: return F("JOB RUNNING");
    default:              return F("");
  }
}

// arg1/arg2: 문구 뒤에 붙일 글자 (0 이면 생략). JOB 은 "JOB 3>B" 형식.
void queuePhone(uint8_t id, char arg1, char arg2) {  // ◆ NEW
  if (queueCount >= QUEUE_SIZE) return;   // 가득 차면 새 메시지를 버림
  uint8_t tail = (queueHead + queueCount) % QUEUE_SIZE;
  queueId[tail] = id;
  queueArg1[tail] = arg1;
  queueArg2[tail] = arg2;
  queueCount++;
}

void phoneSendEntry(uint8_t id, char arg1, char arg2) {  // ◆ NEW
  phone.print(msgText(id));
  if (arg1 != 0) { phone.write(' '); phone.write(arg1); }
  if (arg2 != 0) { phone.write('>'); phone.write(arg2); }
  phone.write('\n');
}

// 지게차 링크로 받을 바이트가 없을 때만 한 줄 송신 (수신 손실 방지, 제안)
void flushPhoneQueue() {  // ◆ NEW
  if (queueCount == 0 || Serial.available() > 0) return;
  phoneSendEntry(queueId[queueHead], queueArg1[queueHead], queueArg2[queueHead]);
  queueHead = (queueHead + 1) % QUEUE_SIZE;
  queueCount--;
}

// ======== center_logic.ino ========
// ---------------- 작업 종료 ----------------
void finishJob() {  // ◆ NEW
  jobActive = false;
  emergency = false;
  picked = false;
  selLoad = -1;
  selUnload = -1;
  ledsOff();            // lastLedIdx 는 일부러 지우지 않음 (다음 비상의 기준)
}

// ---------------- 폰 -> 센터 ----------------
void selectLoad(char c) {  // ◆ NEW
  if (jobActive) { queuePhone(MSG_JOB_RUNNING, 0, 0); return; }
  int8_t idx = c - '1';
  if (!loadFull[idx]) { queuePhone(MSG_EMPTY, c, 0); return; }   // 짐 없는 상차 칸은 거부
  selLoad = idx;
  queuePhone(MSG_SEL_LOAD, c, 0);
}

void selectUnload(char c) {  // ◆ NEW
  if (jobActive) { queuePhone(MSG_JOB_RUNNING, 0, 0); return; }
  int8_t idx = c - 'A';
  if (unloadFull[idx]) { queuePhone(MSG_BUSY, c, 0); return; }   // 점유된 하차 칸은 거부
  selUnload = idx;
  queuePhone(MSG_SEL_UNLOAD, c, 0);
}

// 상차와 하차를 모두 고른 뒤에만 출발 (사용자 확인). 아니면 무시.
void startJob() {  // ◆ NEW
  if (jobActive || selLoad < 0 || selUnload < 0) return;
  jobActive = true;
  sendToForklift('1' + selLoad);    // 상차 글자 (숫자)
  sendToForklift('A' + selUnload);  // 하차 글자 (알파벳)
  queuePhone(MSG_JOB, '1' + selLoad, 'A' + selUnload);
}

void handlePhoneByte(char c) {  // ◆ NEW
  if (c == START_CHAR) startJob();
  else if (c >= '1' && c <= '6') selectLoad(c);
  else if (c >= 'A' && c <= 'F') selectUnload(c);
  // 그 밖의 글자(줄바꿈 등)는 무시
}

// ---------------- 지게차 -> 센터 ----------------
// 상태 글자 -> 폰 메시지 번호 (구간 진입 '1'~'4' 는 LED 만 쓰므로 여기 없음)
uint8_t stateMsgId(char c) {  // ◆ NEW
  switch (c) {
    case 'w': return MSG_ST_WAIT;
    case 'l': return MSG_ST_TO_LOAD;
    case 'a': return MSG_ST_APPROACH;
    case 'u': return MSG_ST_LIFT_UP;
    case 'b': return MSG_ST_REVERSE;
    case 'm': return MSG_ST_TO_UNLOAD;
    case 'd': return MSG_ST_UNLOAD;
    case 'r': return MSG_ST_RETURN;
    case 'c': return MSG_ST_DONE;
    case 'e': return MSG_EMERGENCY;
    case 'n': return MSG_LINE_LOST;
    default:  return MSG_NONE;
  }
}

// 구간 진입 보고: 진행 방향 앞쪽 모서리 LED 를 켠다 (사용자 확인). 비상 중에는 LED 를 바꾸지 않는다.
void onSegment(int8_t idx) {  // ◆ NEW
  if (emergency) return;
  lastLedIdx = idx;
  setLedIndex(idx);
}

// 비상: 'X' 를 보내고, "가장 최근에 켠 LED"의 시계 방향 다음 LED 를 계속 켠다 (사용자 확인).
// LED 를 한 번도 켠 적이 없으면 좌하(0)를 기준으로 삼음 (제안).
void onEmergency() {  // ◆ NEW
  if (!jobActive || emergency) return;
  emergency = true;
  sendToForklift('X');
  int8_t base = (lastLedIdx < 0) ? 0 : lastLedIdx;
  lastLedIdx = (base + 1) % 4;
  setLedIndex(lastLedIdx);
}

// 점유 기록 반영. 정상 완료: 상차 칸 비움 + 하차 칸 점유.
// 비상 복귀: 이미 짐을 실은 상태면 상차만 반영 (사용자 확인), 아니면 기록 그대로.
void settleOccupancy() {  // ◆ NEW
  if (!emergency) {
    loadFull[selLoad] = false;
    unloadFull[selUnload] = true;
  } else if (picked) {
    loadFull[selLoad] = false;
  }
}

// 정상 완료('c') 또는 비상 복귀 도착('w')
void onJobDone() {  // ◆ NEW
  if (!jobActive) return;
  settleOccupancy();
  finishJob();
}

void handleForkliftByte(char c) {  // ◆ NEW
  if (c >= '1' && c <= '4') { onSegment(c - '1'); return; }
  uint8_t id = stateMsgId(c);
  if (id == MSG_NONE) return;
  queuePhone(id, 0, 0);
  if (c == 'b' || c == 'm' || c == 'd') picked = true;   // 후진/하차 이동/하차 = 이미 짐을 실은 뒤
  if (c == 'e') onEmergency();
  else if (c == 'c') onJobDone();
  else if (c == 'w' && emergency) onJobDone();           // 비상 복귀 도착
}
