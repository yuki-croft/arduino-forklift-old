// ====================================================================================================
// TEST CODE (not final) | board: CENTER | checks the forklift link (module B on D0/D1) both ways with a buzzer on D13; do not use as the final program
// ====================================================================================================
/*
 * test_center_link.ino  -  센터 쪽 지게차 링크 시험 (양방향). 폰 모듈과 SoftwareSerial 은 쓰지 않는다.
 * 보드: UNO (센터). 모듈 B (지게차 링크용 HC-05 마스터) = 하드웨어 시리얼 D0 (모듈 TXD) / D1 (모듈 RXD), 9600.
 * 알림: 센터 부저 D13 (사용자가 "13번"이라고 답함, 2026-11-26). 수동/능동 부저인지는 미확인: tone() 을 쓴다.
 * 짝: 지게차에는 tests/test_forklift_board_hardware/test_forklift_link.ino 를 올린다 (2초마다 'w' 를 보내고, 받은 글자마다 짧은 삐).
 *
 * [쓰는 법] 업로드 때는 모듈 B 선(D0/D1)을 빼고, 업로드 뒤 다시 꽂는다. 가능하면 USB 대신 어댑터/배터리 전원으로 켠다 (D0/D1 이 USB 통신과 같은 선).
 *   1) 센터 전원을 켜면 센터 부저 긴 삐 1번 = 스케치가 돌고 있다
 *   2) 센터 -> 지게차: SEND_PERIOD_MS 마다 '1' 을 보낸다 (센터는 소리 없음). 지게차가 2초마다 짧게 삐 하면 이 방향이 된다.
 *   3) 지게차 -> 센터: 글자를 하나 받을 때마다 센터 부저가 짧게 삐 (높은 소리). 지게차는 2초마다 'w' 를 보내므로 센터가 2초마다 울리면 이 방향이 된다.
 *
 * [표기 규칙]  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */

// ---------------- 핀맵 ----------------
const uint8_t PIN_BUZZER = 13;   // 센터 부저 (사용자 답 "13번")

// ---------------- 상수 ----------------
const long          LINK_BAUD      = 9600;     // final 의 LINK_BAUD 와 같음
const unsigned long SEND_PERIOD_MS = 2000UL;   // ?? TODO ?? 시험용 임시값
const unsigned int  BOOT_HZ        = 1000;     // ?? TODO ?? 시험용 임시값
const unsigned int  RX_HZ          = 3000;     // ?? TODO ?? 시험용 임시값 (지게차 부저와 구분되게 높은 소리)
const char          SEND_CHAR      = '1';      // 지게차 시험 스케치는 어떤 글자든 삐 한다

// ---------------- 하드웨어 함수 ----------------
void buzzerBeep(unsigned int frequency, unsigned long ms) {  // ◆ NEW: 소리가 끝날 때까지 기다림
  tone(PIN_BUZZER, frequency, ms);
  delay(ms);
}

// ====================================================================================================
// SERIAL TEST SECTION  (module B is on the hardware Serial D0/D1; there is no USB monitor)
// ====================================================================================================
unsigned long lastSendMs = 0;

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.begin(LINK_BAUD);
  buzzerBeep(BOOT_HZ, 600);          // 긴 삐 = 스케치가 돌고 있다
}

void loop() {
  while (Serial.available() > 0) {   // 지게차 -> 센터: 받은 글자 하나당 짧은 삐
    Serial.read();
    buzzerBeep(RX_HZ, 80);
    delay(80);
  }
  if (millis() - lastSendMs >= SEND_PERIOD_MS) {   // 센터 -> 지게차 (소리 없음)
    lastSendMs = millis();
    Serial.write(SEND_CHAR);
  }
}
