// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | photo-transistor check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_photo.ino  -  광원 추적(포토 트랜지스터 2개) 단독 시험: 핀 / 하드웨어 함수 (Serial 미사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [탭 구성]  같은 폴더의 .ino 는 Arduino IDE 가 하나로 이어 붙여 컴파일한다 (.h 없음)
 *   test_forklift_photo.ino : 핀 / 하드웨어 함수 (이 파일, 항상 먼저 이어 붙음)
 *   photo_serial_test.ino   : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 규칙: 센서 세부사항은 사용자 확인 후 반영. 테스트는 "원시값 출력"만 하고 임계값/극성은 판단하지 않는다.
 */

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_PHOTO_LEFT  = A0;  // 광원 추적(포토 트랜지스터) 왼쪽
const uint8_t PIN_PHOTO_RIGHT = A1;  // 광원 추적(포토 트랜지스터) 오른쪽

// ---------------- 하드웨어 함수 ----------------
// ★ EXTERNAL: 4_ABOT빛으로로봇조종.pptx 슬라이드 11 (volts 함수 그대로)
float volts(int adPin) {
  return float(analogRead(adPin)) * 5.0 / 1023.0;
}

void readPhotoRaw(int &left, int &right) {  // ◆ NEW: analogRead 원시값 (0~1023)
  left  = analogRead(PIN_PHOTO_LEFT);
  right = analogRead(PIN_PHOTO_RIGHT);
}
