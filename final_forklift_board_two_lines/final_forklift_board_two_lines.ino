// ====================================================================================================
// FINAL CODE  | board: FORKLIFT (FRIBEE EDU, UNO R3 compatible) | upload the whole folder as one sketch
// ====================================================================================================
/*
 * final_forklift_board_two_lines.ino  -  지게차 통합 스케치, 라인 방식 B: "평행한 검은 선 두 줄 사이를 달림"
 *   정상 주행: 왼쪽 센서 = 왼쪽 선 위(검정), 가운데 = 두 선 사이 흰 바닥, 오른쪽 센서 = 오른쪽 선 위(검정)
 *   (2026-11-28 사용자: 맵에 평행한 선 두 줄, 두 선 사이 간격 약 19mm -> 좌/우 센서(간격 20mm)가 각 선의 안쪽 가장자리에 걸림)
 *   사용자 기준값: 흰 바닥 25~29, 검은 선 30~34 -> 30 이상 = 검정, 29 이하 = 흰색
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [탭 구성] 같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 는 forklift_line_pattern.h 하나)
 *   final_forklift_board_two_lines.ino : 설정, 상태 변수, setup/loop   (이 파일)
 *   forklift_hw.ino         : 구동 함수 (바퀴, 리프트, 직선 보정, 광 추적, 회전)
 *   forklift_sense.ino      : 센서 판별 함수 (교차점/모서리/표식/라인 이탈/짐/후방 장애물)
 *   forklift_line_pattern.h : 라인 3개 값 -> 교차점/모서리/표식 판별 (순수 계산, 임시 기준값, PC 에서 시험)
 *   forklift_comm.ino       : 블루투스 명령 수신, 상태 보고
 *   forklift_logic.ino      : 상태 머신 (입출력은 forklift_hw / forklift_sense / forklift_comm 의 함수만 호출 -> PC 에서 시험 가능)
 * 시험용(시리얼 모니터) 코드는 tests/test_forklift_board_hardware/ 에 따로 있다. 이 스케치에는 USB 시리얼을 쓰지 않는다.
 *
 * [프로토콜] PROBLEM_1PAGER.md 의 "통신 프로토콜 초안" / "지게차 상태 머신 초안" (제안 단계)
 * [표기] ★ EXTERNAL : 수업 PPT / 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 실측/확인 전 임시값
 * 주의: 교차점/모서리/표식 판별은 실측 없이 제 가정값으로 만든 임시 규칙이다 (forklift_line_pattern.h, forklift_sense.ino 참고).
 */
#include <Servo.h>       // Arduino 표준 라이브러리 (서보)
#include <Ultrasonic.h>  // ★ EXTERNAL LIBRARY: "Ultrasonic" by Erick Simões (MIT, https://github.com/ErickSimoes/Ultrasonic)
#include "forklift_line_pattern.h"   // ◆ NEW: 라인 패턴 판별 (순수 계산, 임시 기준값)

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
// 블루투스 HC-06 = 하드웨어 시리얼 D0(RX)/D1(TX) (사용자 확인 2026-11-26: 납땜됨). D0 에는 빨간 LED 도 납땜돼 있음. 별도 핀 상수/pinMode 없음.
// D0/D1 은 USB 와 같은 포트 -> 이 프로그램은 Serial 에 디버그 출력을 하지 않는다 (모듈로 나가 폰에 보임).
const uint8_t PIN_PHOTO_LEFT  = A0;
const uint8_t PIN_PHOTO_RIGHT = A1;
const uint8_t PIN_LINE_LEFT   = A2;
const uint8_t PIN_LINE_CENTER = A3;
const uint8_t PIN_LINE_RIGHT  = A4;

// ---------------- 상수: 확정값 ----------------
const long IR_FREQ_HZ  = 38000;   // ★ EXTERNAL: 5_ABOT적외선센서.pptx 슬라이드 9
const int  SERVO_STOP_US = 1500;  // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const long BT_BAUD     = 9600;    // 지게차 HC-06 기본값 (PPT IoT12 슬라이드 4). 모듈 설정과 같아야 함
const int  LIFT_SPEED  = 200;     // 사용자 지정 속도 크기 (1500 +- 200)
const bool LIFT_INVERT = true;    // 사용자 시험 결과(2026-11-26): 1700us=올림 으로 하면 실물이 반대 -> 반전 (test_forklift_lift 와 같은 값)
const int  LINE_LEVEL1 = 27;      // 사용자 지정: 직선 보정 1단계 기준
const int  LINE_LEVEL2 = 30;      // 사용자 지정: 직선 보정 2단계 기준
const int  LINE_WHITE_MAX = 29;   // 사용자 지정 (2026-11-28): 흰 바닥 25~29
const int  LINE_BLACK_MIN = 30;   // 사용자 지정 (2026-11-28): 검은 선 30~34
constexpr unsigned long LIFT_FULL_STROKE_MS = 8000UL;  // 최소~최대 높이 총 8초 (제작자 캘리브레이션, 속도는 미확인)

// ---------------- 상수: 임시값 (?? TODO ??, 실측/확인 후 교체) ----------------
const bool LINE_STEER_INVERT = true;  // 사용자 지시 "회전은 반전으로". 주행 시험 후 확인
const int  LINE_BASE_SPEED = 60;      // 직진/후진 기본 속도. 사용자 (2026-11-28): "최대로 주지 말고 천천히" -> 100 에서 60 으로 (최대 200)
const int  LINE_TRIM_1 = 5;           // "아주 미세" 보정 감속량 (사용자 승인 임시값)
const int  LINE_TRIM_2 = 15;          // "미세" 보정 감속량
// 방식 B 직선 보정 (?? TODO ?? 모두 제 가정): 좌/우 값의 차이로 조향. 차이가 LANE_DIFF_1 초과면 아주 미세, LANE_DIFF_2 초과면 미세
const int  LANE_DIFF_1 = 1;
const int  LANE_DIFF_2 = 3;
// 선 사이 간격 약 19mm (사용자) 이므로 좌/우 센서는 각 선의 안쪽 가장자리 -> 오른쪽으로 치우치면 왼쪽 센서가 흰 틈으로 나가 값이 내려감
//   -> 값이 내려간 쪽으로 돈다. 실제 주행에서 반대로 돌면 true (바퀴 방향 반전이 필요했던 방식 A 의 경험 참고)
const bool LANE_STEER_INVERT = false;
const int  TURN_SPEED = 60;           // 제자리/축 회전 속도 (천천히, 100 에서 60)
const unsigned long TURN_MIN_MS = 500;       // 회전 중 기존 선에서 벗어나는 최소 시간 (임시값, 느린 회전에 맞춰 300 -> 500)
const unsigned long TURN_TIMEOUT_MS = 4200;  // 새 선을 못 찾을 때 회전 포기 시간 (임시값, 속도 100->60 비율로 2500 -> 4200)
const unsigned long JUNCTION_ADVANCE_MS = 0; // 교차점 감지 후 회전 전 전진 시간 (0 = 바로 회전)
const bool  LIGHT_ACTIVE_HIGH = true;        // 광이 강할수록 analogRead 값이 큰지 (미확인)
const int   LIGHT_BASE_SPEED = 60;           // 광 추적 기본 속도 (천천히, 100 에서 60)
const float LIGHT_GAIN = 400.0;              // 광 추적 조향 민감도
const long REAR_STOP_CM = 5;                 // 후방 비상 거리 (사용자: 매우 짧게. 값은 임시)
const unsigned long REAR_CHECK_MS = 100;     // 후방 초음파 확인 주기 (읽기에 최대 약 20ms)
const unsigned long REAR_HOLD_MS = 1000;     // 기준 이하가 이 시간 지속되면 비상
// 리프트 높이 (2026-11-28 사용자): 랙 하단 5cm, 상단 12cm. 포크를 선반보다 살짝 위로 맞춰 들어가 1cm 더 올리고, 후진한 뒤 내린다.
//   시작 지점(첫 표식)에서는 리프트를 최하단에 두고 시작한다 (사용자) -> 전원을 켤 때 위치 = 0 으로 본다.
//   리프트는 시간으로 높이를 추정한다: 최소~최대 LIFT_FULL_STROKE_MS (8초). 그 8초 동안 올라가는 높이(cm)는 아직 모름.
constexpr float LIFT_STROKE_CM     = 8.0;   // 사용자 (2026-11-28): 최하단 4.5cm ~ 최상단 12.5cm = 8cm (LIFT_FULL_STROKE_MS 8초 동안)
constexpr float LIFT_BASE_CM       = 4.5;   // 사용자 (2026-11-28): 최하단 포크 높이 4.5cm
//   상단 칸: 12cm + 0.2cm = 12.2cm 로 들어가고, 1cm 를 올리려 해도 최상단 12.5cm 에서 멈추므로 실제로는 0.3cm 만 든다 (사용자 승인 2026-11-28).
constexpr float RACK_LOWER_CM      = 5.0;   // 사용자: 하단 랙 높이
constexpr float RACK_UPPER_CM      = 12.0;  // 사용자: 상단 랙 높이
constexpr float LIFT_ABOVE_CM      = 0.2;   // 사용자 (2026-11-28): "살짝 위" = 0.2cm. 하차 때 "살짝 아래" 에도 같은 값 사용
constexpr float LIFT_PICK_CM       = 1.0;   // 사용자: 들어 올리는 높이 1cm
constexpr unsigned long liftCmToMs(float cm) {   // ◆ NEW: 포크 높이(cm) -> 최하단에서 올리는 시간(ms). 모르는 값이면 0
  return (LIFT_STROKE_CM <= 0.0 || cm <= LIFT_BASE_CM) ? 0UL
       : (unsigned long)((cm - LIFT_BASE_CM) * LIFT_FULL_STROKE_MS / LIFT_STROKE_CM + 0.5f);   // 반올림 (float 오차로 7699 가 되지 않게)
}
const unsigned long LIFT_UPPER_MS      = liftCmToMs(RACK_UPPER_CM + LIFT_ABOVE_CM);   // 2층: 들어갈 때 / 상차 전
const unsigned long LIFT_LOWER_MS      = liftCmToMs(RACK_LOWER_CM + LIFT_ABOVE_CM);   // 1층
const unsigned long LIFT_UPPER_DROP_MS = liftCmToMs(RACK_UPPER_CM - LIFT_ABOVE_CM);   // 2층에 내려놓을 때
const unsigned long LIFT_LOWER_DROP_MS = liftCmToMs(RACK_LOWER_CM - LIFT_ABOVE_CM);   // 1층에 내려놓을 때
const unsigned long LIFT_PICK_MS = (LIFT_STROKE_CM > 0.0) ? (unsigned long)(LIFT_PICK_CM * LIFT_FULL_STROKE_MS / LIFT_STROKE_CM + 0.5f) : 0UL;  // 1cm (상단 칸은 최상단에서 잘려 0.3cm)
const unsigned long LIFT_CARRY_MS = 0;       // 후진해서 본선에 오면 최하단까지 내림 (사용자: "후진해서 내리면 돼")

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
// 블루투스 모듈 = 하드웨어 Serial (D0/D1). 예전에는 전역 참조 "bt" 를 썼으나 IDE 에서 "'bt' was not declared" 오류가 보고되어 (2026-11-28) Serial 을 직접 쓴다

void setup() {
  initHardware();
  Serial.begin(BT_BAUD);
  enterState(S_WAIT);
}

void loop() {
  commPoll();
  liftService();
  runState();
}
