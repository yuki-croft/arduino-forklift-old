// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | ultrasonic check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_ultrasonic.ino  -  후방 초음파(HC-SR04) 단독 시험: 핀 / 상수 / 객체 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT / 외부 라이브러리  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 규칙: 센서/모듈 세부사항은 사용자 확인 후 반영. 테스트는 "원시값 출력"만 하고 임계값은 판단하지 않는다.
 */
#include <Ultrasonic.h>  // ★ EXTERNAL LIBRARY: "Ultrasonic" by Erick Simões (MIT, 라이브러리 매니저에서 설치)
                         //   https://github.com/ErickSimoes/Ultrasonic

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_US_TRIG = 6;   // 초음파 Trig (HC-SR04)
const uint8_t PIN_US_ECHO = 7;   // 초음파 Echo (HC-SR04)

// ---------------- 공용 객체 ----------------
Ultrasonic ultrasonic(PIN_US_TRIG, PIN_US_ECHO);  // 타임아웃은 라이브러리 기본값(README: 20000UL us) 사용

// ---------------- 하드웨어 함수 ----------------
void initHardware() {  // ◆ NEW
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
}

// ★ EXTERNAL LIBRARY: Ultrasonic (Erick Simões). PPT에는 초음파 예제가 없어 라이브러리 사용.
//   read() 의 기본 단위는 cm (README 근거). 타임아웃 시 반환값은 README에 명시 없음 -> 테스트로 확인할 것.
long readRearDistanceCm() {  // ◆ NEW
  return ultrasonic.read();
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  후방 초음파를 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기 함수는 위쪽 하드웨어 함수 구역에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

const long SERIAL_BAUD        = 9600;
const int  SAMPLE_COUNT       = 10;
const int  SAMPLE_INTERVAL_MS = 300;

void printMenu() {
  Serial.println(F("\n== Forklift sonar test =="));
  Serial.println(F("1 Sonar cm x10"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 초음파 (HC-SR04) ----------------
// 센서 앞(지게차에서는 후방)에 물체가 없을 때 어떤 값이 찍히는지도 이 테스트로 확인할 것.
void testUltrasonic() {
  Serial.println(F("Sonar cm (rear sensor; also check no-object value)"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    long distanceCm = readRearDistanceCm();
    Serial.println(distanceCm);
    delay(SAMPLE_INTERVAL_MS);
  }
}

// ---------------- setup / loop (시리얼 메뉴) ----------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  initHardware();
  printMenu();
}

void loop() {
  if (Serial.available() <= 0) return;
  int cmd = Serial.read();
  switch (cmd) {
    case '1': testUltrasonic(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
