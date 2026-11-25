// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | hardware checks through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_board_hardware.ino  -  소형 지게차 공용 설정 (핀맵 / 상수 / 객체)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  같은 폴더의 .ino 파일은 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   test_forklift_board_hardware.ino : 공용 설정 (이 파일, 항상 먼저 이어 붙음)
 *   test_hw_drivers.ino    : 하드웨어 구동/읽기 함수 (Serial 미사용, 통합 주행 때 재사용)
 *   test_serial_menus.ino  : 시리얼 모니터로 확인하는 테스트 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]
 *   ★ EXTERNAL ★ : 수업 PPT(Parallax.com 자료)에서 가져온 코드 (출처 파일/슬라이드 명시)
 *   ◆ NEW ◆      : 이번에 새로 작성한 코드 (PPT에 없음)
 *   ?? TODO ??   : 사용자 확인 전까지 임시값. 확정되면 교체할 것
 *
 * 규칙: 센서/모듈 세부사항은 사용자 확인 후 반영. 테스트는 "원시값 출력"과
 *       "동작 확인"만 하고, 임계값/극성 등은 판단하지 않는다.
 */
#include <Servo.h>       // Arduino 표준 라이브러리 (서보)
#include <Ultrasonic.h>  // ★ EXTERNAL LIBRARY: "Ultrasonic" by Erick Simões (MIT, 라이브러리 매니저에서 설치)
                         //   https://github.com/ErickSimoes/Ultrasonic
#include <NeoSWSerial.h> // ★ EXTERNAL LIBRARY: "NeoSWSerial" by SlashDevin (라이브러리 매니저에서 설치)
                         //   https://github.com/SlashDevin/NeoSWSerial  (HC-05/06 블루투스용 소프트웨어 시리얼)
// tone()/pulse 등은 Arduino 코어 내장 함수만 사용 (별도 헤더 파일 없음)

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
// 디지털
const uint8_t PIN_IR_TX       = 4;   // 2핀 IR 발신기 (좌/전/우 통합, 1개 핀)
const uint8_t PIN_BUZZER      = 5;   // 수동 부저
const uint8_t PIN_US_TRIG     = 6;   // 초음파 Trig (HC-SR04)
const uint8_t PIN_US_ECHO     = 7;   // 초음파 Echo (HC-SR04)
const uint8_t PIN_IR_RX_LEFT  = 8;   // 3핀 IR 수신기 좌측
const uint8_t PIN_IR_RX_FRONT = 9;   // 3핀 IR 수신기 전방
const uint8_t PIN_IR_RX_RIGHT = 13;  // 3핀 IR 수신기 우측 (보드 내장 LED와 핀 공유)
const uint8_t PIN_SERVO_LEFT  = 10;  // 구동 서보 왼쪽
const uint8_t PIN_SERVO_RIGHT = 11;  // 구동 서보 오른쪽
const uint8_t PIN_SERVO_LIFT  = 12;  // 수압 펌프식 리프트 서보
const uint8_t PIN_BT_RX       = 2;   // 블루투스 TXD 연결 (아두이노가 수신) - 장착 예정
const uint8_t PIN_BT_TX       = 3;   // 블루투스 RXD 연결 (아두이노가 송신) - 장착 예정
// 빨간 LED: 사용자 안: D0 또는 D1 (미확정, USB 시리얼과 핀 공유) -> 핀 확정 전까지 코드 미반영
// 아날로그
const uint8_t PIN_PHOTO_LEFT  = A0;  // 광원 추적(포토 트랜지스터) 왼쪽
const uint8_t PIN_PHOTO_RIGHT = A1;  // 광원 추적(포토 트랜지스터) 오른쪽
const uint8_t PIN_LINE_LEFT   = A2;  // 라인 트레이서 왼쪽
const uint8_t PIN_LINE_CENTER = A3;  // 라인 트레이서 중앙
const uint8_t PIN_LINE_RIGHT  = A4;  // 라인 트레이서 오른쪽

// ---------------- 상수 ----------------
const long          SERIAL_BAUD         = 9600;
const long          IR_FREQ_HZ          = 38000;    // ★ EXTERNAL: 5_ABOT적외선센서.pptx 슬라이드 9
const int           SERVO_STOP_US       = 1500;     // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int           WHEEL_TEST_SPEED    = 100;      // ◆ NEW: 안전한 저속 테스트값 (PPT 최대는 200)
const int           WHEEL_TEST_MS       = 1000;
const int           SAMPLE_COUNT        = 10;
const int           SAMPLE_INTERVAL_MS  = 300;
const unsigned long INPUT_TIMEOUT_MS    = 30000UL;
const int           LIFT_SPEED          = 200;      // 사용자 지정: 펄스 1700us(=1500+200) = 올림, 1300us(=1500-200) = 내림
const int           LIFT_TEST_MS        = 300;      // ?? TODO ??: 리프트 1스텝 시간 미확정
// 라인 센서 실측(사용자): 검은 선(폭 19mm) 위 31 이상, 흰 바닥 약 24~29 (analogRead 원시값, 선이 클수록 큼)
const int           LINE_LEVEL1         = 27;       // 사용자 지정: 좌/우 값이 이 값 초과 -> 아주 미세한 직선 보정
const int           LINE_LEVEL2         = 30;       // 사용자 지정: 좌/우 값이 이 값 초과 -> 미세한 직선 보정
const bool          LINE_STEER_INVERT   = true;     // 사용자 지시("회전은 반전으로"): 처음 가정과 반대 방향으로 조향. 실주행에서 반대면 false
const int           LINE_BASE_SPEED     = 100;      // ?? TODO ??: 직진 기본 속도 (임시값 = WHEEL_TEST_SPEED)
const int           LINE_TRIM_1         = 5;        // ?? TODO ??: "아주 미세" 보정의 바퀴 감속량 (임시값, 서보 펄스 us)
const int           LINE_TRIM_2         = 15;       // ?? TODO ??: "미세" 보정의 바퀴 감속량 (임시값)
const unsigned long LIFT_FULL_STROKE_MS = 8000UL;   // 최소~최대 높이 총 8초: 하드웨어 제작자의 수동 캘리브레이션 값 (사용자 전달). 1700/1300us 속도에서의 값인지는 미확인
const long          BT_BAUD             = 9600;     // ?? TODO ??: 모듈 보드레이트 미확인 (NeoSWSerial은 9600/19200/38400 등 제한)
const unsigned long BT_TEST_MS          = 15000UL;  // ◆ NEW: 블루투스 수신 테스트 시간

// ---------------- 공용 객체 ----------------
Servo servoLeft;
Servo servoRight;
Servo servoLift;
Ultrasonic ultrasonic(PIN_US_TRIG, PIN_US_ECHO);  // 타임아웃은 라이브러리 기본값(README: 20000UL us) 사용
// 인자 순서(rx, tx): README가 "SoftwareSerial의 drop-in 대체"라고 밝힌 점에 근거한 추정 (헤더 미확인).
// 블루투스 수신이 전혀 안 되면 이 순서부터 의심할 것.
NeoSWSerial bt(PIN_BT_RX, PIN_BT_TX);
