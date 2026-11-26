// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | checks the Bluetooth link both ways with the buzzer only; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_link.ino  -  지게차 쪽 블루투스 링크 시험 (양방향). 바퀴/리프트는 움직이지 않는다.
 * 보드: FRIBEE EDU (UNO R3 호환), 지게차 모듈은 하드웨어 시리얼 D0/D1 (9600). USB 시리얼 모니터는 쓸 수 없고, 상태는 부저로만 알린다.
 *
 * [쓰는 법] 센터 보드에는 test_final_center_3s.ino (또는 final_center_board) 를 올려 두고, 폰 앱을 연결한 상태에서 지게차 전원을 켠다.
 *   1) 전원을 켜면 긴 삐 1번 = 이 스케치가 돌고 있다 (안 들리면 업로드/전원/부저 문제)
 *   2) 지게차 -> 센터: 이 스케치는 SEND_PERIOD_MS 마다 'w' 를 보낸다. 폰 앱에 "ST WAIT" 가 주기적으로 뜨면 이 방향이 된다.
 *   3) 센터 -> 지게차: 폰 앱에서 상차 칸, 하차 칸을 고르고 Start 를 누르면 센터가 글자 2개 (상차 숫자, 하차 알파벳) 를 보낸다.
 *      글자 하나를 받을 때마다 짧은 삐 1번 (Start 한 번에 2번). 안 울리면 이 방향이 안 된다.
 *   (센터 코드는 지게차 'w' 를 받으면 작업 중이 아니어도 폰에 "ST WAIT" 를 보낸다: test_final_center_3s.ino 의 handleForkliftByte 확인함. 실제 보드에서는 확인하지 못함)
 * [주의] 코드를 업로드할 때는 지게차 모듈 선(D0/D1)을 빼 둔다 (하드웨어 시리얼이 겹침).
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_BUZZER = 5;   // 수동 부저

// ---------------- 상수 ----------------
const unsigned int  BEEP_HZ       = 3000;      // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 10 의 예제 주파수
const long          BT_BAUD       = 9600;      // 지게차 모듈 (final 과 같음)
const unsigned long SEND_PERIOD_MS = 2000UL;   // ?? TODO ?? 시험용 임시값: 'w' 를 보내는 간격
const char          REPORT_CHAR   = 'w';       // final 의 "대기" 보고 글자와 같음

// ---------------- 하드웨어 함수 ----------------
void buzzerBeep(unsigned int frequency, unsigned long ms) {  // ◆ NEW: 소리가 끝날 때까지 기다림
  tone(PIN_BUZZER, frequency, ms);
  delay(ms);
}

// ====================================================================================================
// SERIAL TEST SECTION  (the link module is on the hardware Serial D0/D1; there is no USB monitor)
// ====================================================================================================
unsigned long lastSendMs = 0;

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.begin(BT_BAUD);
  buzzerBeep(BEEP_HZ, 600);          // 긴 삐 = 스케치가 돌고 있다
}

void loop() {
  while (Serial.available() > 0) {   // 센터 -> 지게차: 받은 글자 하나당 짧은 삐 1번
    Serial.read();
    buzzerBeep(BEEP_HZ, 100);
    delay(100);
  }
  if (millis() - lastSendMs >= SEND_PERIOD_MS) {   // 지게차 -> 센터
    lastSendMs = millis();
    Serial.write(REPORT_CHAR);
  }
}
