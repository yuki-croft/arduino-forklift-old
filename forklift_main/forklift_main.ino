/*
 * forklift_main.ino  -  지게차 통합 스케치 (설정 / 상태 변수 / setup() / loop())
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [탭 구성] 같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   forklift_main.ino : 설정, 상태 변수, setup/loop   (이 파일)
 *   fk_hw.ino         : 구동 함수 (바퀴, 리프트, 직선 보정, 광 추적, 회전)
 *   fk_sense.ino      : 센서 판별 함수 (교차점/모서리/표식/라인 이탈/짐/후방 장애물)
 *   fk_comm.ino       : 블루투스 명령 수신, 상태 보고
 *   fk_logic.ino      : 상태 머신 (입출력은 fk_hw / fk_sense / fk_comm 의 함수만 호출 -> PC 에서 시험 가능)
 * 시험용(시리얼 모니터) 코드는 forklift_test/ 폴더에 따로 있다. 이 스케치에는 USB 시리얼을 쓰지 않는다.
 *
 * [프로토콜] PROBLEM_1PAGER.md 의 "통신 프로토콜 초안" / "지게차 상태 머신 초안" (제안 단계)
 * [표기] ★ EXTERNAL : 수업 PPT / 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 실측/확인 전 임시값
 * 주의: 센서 판별(교차점, 모서리, 표식)은 실측 전이라 항상 "감지 안 됨"을 돌려준다 (fk_sense.ino 참고).
 */
#include <Servo.h>       // Arduino 표준 라이브러리 (서보)
#include <Ultrasonic.h>  // ★ EXTERNAL LIBRARY: "Ultrasonic" by Erick Simões (MIT, https://github.com/ErickSimoes/Ultrasonic)
#include <NeoSWSerial.h> // ★ EXTERNAL LIBRARY: "NeoSWSerial" by SlashDevin (https://github.com/SlashDevin/NeoSWSerial)

// ---------------- 핀맵 (사용자 제공 값 그대로) ----------------
const uint8_t PIN_IR_TX       = 4;   // IR 발신기 (좌/전/우 통합)
const uint8_t PIN_BUZZER      = 5;
const uint8_t PIN_US_TRIG     = 6;   // 후방 초음파 Trig
const uint8_t PIN_US_ECHO     = 7;   // 후방 초음파 Echo
const uint8_t PIN_IR_RX_LEFT  = 8;
const uint8_t PIN_IR_RX_FRONT = 9;
const uint8_t PIN_IR_RX_RIGHT = 13;
const uint8_t PIN_SERVO_LEFT  = 10;
const uint8_t PIN_SERVO_RIGHT = 11;
const uint8_t PIN_SERVO_LIFT  = 12;
const uint8_t PIN_BT_RX       = 2;   // 블루투스 모듈 TXD
const uint8_t PIN_BT_TX       = 3;   // 블루투스 모듈 RXD
const uint8_t PIN_PHOTO_LEFT  = A0;
const uint8_t PIN_PHOTO_RIGHT = A1;
const uint8_t PIN_LINE_LEFT   = A2;
const uint8_t PIN_LINE_CENTER = A3;
const uint8_t PIN_LINE_RIGHT  = A4;

// ---------------- 상수: 확정값 ----------------
const long IR_FREQ_HZ  = 38000;   // ★ EXTERNAL: 5_ABOT적외선센서.pptx 슬라이드 9
const int  SERVO_STOP_US = 1500;  // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const long BT_BAUD     = 9600;    // 지게차 HC-06 기본값 (PPT IoT12 슬라이드 4). 모듈 설정과 같아야 함
const int  LIFT_SPEED  = 200;     // 사용자 지정: 1700us 올림 / 1300us 내림
const int  LINE_LEVEL1 = 27;      // 사용자 지정: 직선 보정 1단계 기준
const int  LINE_LEVEL2 = 30;      // 사용자 지정: 직선 보정 2단계 기준
const int  LINE_WHITE_MAX = 29;   // 사용자 실측: 흰 바닥 약 24~29
const unsigned long LIFT_FULL_STROKE_MS = 8000UL;  // 최소~최대 높이 총 8초 (제작자 캘리브레이션, 속도는 미확인)

// ---------------- 상수: 임시값 (?? TODO ??, 실측/확인 후 교체) ----------------
const bool LINE_STEER_INVERT = true;  // 사용자 지시 "회전은 반전으로". 주행 시험 후 확인
const int  LINE_BASE_SPEED = 100;     // 직진 기본 속도
const int  LINE_TRIM_1 = 5;           // "아주 미세" 보정 감속량 (사용자 승인 임시값)
const int  LINE_TRIM_2 = 15;          // "미세" 보정 감속량
const int  TURN_SPEED = 100;          // 제자리 회전 속도
const unsigned long TURN_MIN_MS = 300;       // 회전 중 기존 선에서 벗어나는 최소 시간 (임시값)
const unsigned long TURN_TIMEOUT_MS = 2500;  // 새 선을 못 찾을 때 회전 포기 시간 (임시값)
const unsigned long JUNCTION_ADVANCE_MS = 0; // 교차점 감지 후 회전 전 전진 시간 (0 = 바로 회전)
const bool  LIGHT_ACTIVE_HIGH = true;        // 광이 강할수록 analogRead 값이 큰지 (미확인)
const int   LIGHT_BASE_SPEED = 100;          // 광 추적 기본 속도
const float LIGHT_GAIN = 400.0;              // 광 추적 조향 민감도
const long REAR_STOP_CM = 5;                 // 후방 비상 거리 (사용자: 매우 짧게. 값은 임시)
const unsigned long REAR_CHECK_MS = 100;     // 후방 초음파 확인 주기 (읽기에 최대 약 20ms)
const unsigned long REAR_HOLD_MS = 1000;     // 기준 이하가 이 시간 지속되면 비상
// 리프트 높이: 최소 높이에서 해당 높이까지 올리는 시간 (선반 높이 미확인 -> 0 = 움직이지 않음)
const unsigned long LIFT_UPPER_MS = 0;       // 2층 선반
const unsigned long LIFT_LOWER_MS = 0;       // 1층 선반
const unsigned long LIFT_PICK_MS  = 0;       // 짐을 들어 올리는 추가 높이
const unsigned long LIFT_CARRY_MS = 0;       // 주행 중 포크 높이 (0 = 최소 높이)

// ---------------- 상태 번호 ----------------
const uint8_t S_WAIT = 0, S_TO_LOAD = 1, S_APPROACH = 2, S_LIFT_PICK = 3, S_REVERSE = 4,
              S_TO_UNLOAD = 5, S_DROP = 6, S_RETURN = 7, S_EM_WAIT = 8, S_EM_GUIDE = 9;

// ---------------- 상태 변수 ----------------
uint8_t state = S_WAIT;
int8_t  loadCell = -1, unloadCell = -1;  // 0~5 (상차 '1'~'6', 하차 'A'~'F'), -1 = 미수신
uint8_t segment = 0;                     // 0 아래, 1 왼쪽, 2 위쪽, 3 오른쪽 (보고 글자 '1'~'4')
uint8_t junctionCount = 0, targetCount = 0;
bool carrying = false, dropDone = false;
bool markerArmed = false, junctionLatched = false, lineLostReported = false;
bool emergencyDeclared = false, wheelsAttached = false, rearClose = false;
long  liftPosMs = 0;                     // 가정: 전원을 켤 때 포크는 최소 높이
long  liftTargetMs = 0;
int   liftDir = 0;
unsigned long liftLastMs = 0, rearCheckMs = 0, rearSinceMs = 0;

// ---------------- 공용 객체 ----------------
Servo servoLeft, servoRight, servoLift;
Ultrasonic ultrasonic(PIN_US_TRIG, PIN_US_ECHO);
// 인자 순서(rx, tx)는 SoftwareSerial 과 같다고 보는 추정 (라이브러리 문서로 확인 못함). 수신이 안 되면 이 순서부터 의심.
NeoSWSerial bt(PIN_BT_RX, PIN_BT_TX);

void setup() {
  initHardware();
  bt.begin(BT_BAUD);
  enterState(S_WAIT);
}

void loop() {
  commPoll();
  liftService();
  runState();
}
