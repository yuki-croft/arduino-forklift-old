// ====================================================================================================
// FINAL CODE  | board: CENTER (generic UNO R3) | upload the whole folder as one sketch (detach the HC-05 from D0/D1 first)
// ====================================================================================================
/*
 * final_center_board.ino  -  센터(마스터) 보드: 폰 명령 <-> 지게차 링크, 유도 LED 4개
 * 보드: 일반 UNO R3
 *
 * [탭 구성] 같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   final_center_board.ino : 설정 / 상태 변수 / setup() / loop()   (이 파일)
 *   center_hw.ino     : 입출력 (LED, 지게차 송신, 폰 송신 큐)
 *   center_logic.ino  : 판단 로직 (입출력은 center_hw.ino 의 함수만 호출 -> PC 에서 단독 시험 가능)
 *
 * [연결] 지게차 링크: 하드웨어 시리얼 D0/D1 = HC-05 마스터 (업로드할 때는 모듈을 분리할 것)
 *        폰 링크    : 소프트웨어 시리얼 D2(모듈 TXD) / D3(모듈 RXD) = 폰용 HC-05 (슬레이브로 설정, 사용자 확인)
 *        LED 4개    : D4~D7 (시계 방향 순서 표 참고)
 * [규칙] 프로토콜은 PROBLEM_1PAGER.md 의 "통신 프로토콜 초안"을 따른다 (제안 단계, 사용자 확인 필요 항목 있음).
 *        USB 시리얼은 지게차 링크가 쓰므로 이 보드에는 시리얼 모니터 디버깅 코드를 넣지 않는다.
 * [표기] ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
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
const long PHONE_BAUD = 9600;      // 폰 링크: 폰용 HC-05 를 tool_hc05_center_setup 으로 9600 에 맞춤
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
