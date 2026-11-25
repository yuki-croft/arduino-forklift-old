// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | ultrasonic check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_ultrasonic.ino  -  후방 초음파(HC-SR04) 단독 시험: 핀 / 상수 / 객체 / 하드웨어 함수 (Serial 미사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [탭 구성]  같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   test_forklift_ultrasonic.ino : 핀 / 상수 / 객체 / 하드웨어 함수 (이 파일, 항상 먼저 이어 붙음)
 *   ultrasonic_serial_test.ino   : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
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
