// ====================================================================================================
// TEST CODE (not final) | board: CENTER | checks the forklift link (module B on D0/D1) both ways with LEDs only; do not use as the final program
// ====================================================================================================
/*
 * test_center_link.ino  -  센터 쪽 지게차 링크 시험 (양방향). 폰 모듈과 SoftwareSerial 은 쓰지 않는다.
 * 보드: UNO (센터). 모듈 B (지게차 링크용 HC-05 마스터) = 하드웨어 시리얼 D0 (모듈 TXD) / D1 (모듈 RXD), 9600.
 * 짝: 지게차에는 tests/test_forklift_board_hardware/test_forklift_link.ino 를 올린다 (2초마다 'w' 를 보내고, 받은 글자마다 짧은 삐).
 *
 * [쓰는 법] 업로드 때는 모듈 B 선(D0/D1)을 빼고, 업로드 뒤 다시 꽂는다. 가능하면 USB 대신 어댑터/배터리 전원으로 켠다 (D0/D1 이 USB 통신과 같은 선).
 *   1) 전원을 켜면 LED 4개가 1초 동안 모두 켜졌다가 꺼진다 = 스케치가 돌고 있다 (안 켜지면 LED 극성/배선 확인: LED_ON_LEVEL)
 *   2) 센터 -> 지게차: SEND_PERIOD_MS 마다 '1' 을 보낸다. 보낼 때 좌상 LED(D4) 가 잠깐 켜진다. 지게차가 그때마다 짧게 삐 하면 이 방향이 된다.
 *   3) 지게차 -> 센터: 글자를 하나 받을 때마다 좌하 LED(D7) 가 RX_LED_MS 동안 켜진다. 지게차는 2초마다 'w' 를 보내므로 2초마다 깜박이면 이 방향이 된다.
 *
 * [표기 규칙]  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */

// ---------------- 핀맵 (test_final_center_3s / final_center_board 와 같음) ----------------
const uint8_t LED_PINS_CW[4] = {7, 4, 5, 6};   // 0 = 좌하, 1 = 좌상, 2 = 우상, 3 = 우하
const uint8_t LED_ON_LEVEL   = HIGH;            // ?? TODO ??: LED 배선 극성 미확인 (final 과 같은 값)
const uint8_t LED_RX = 0;                       // 받으면 켤 LED 번호 (좌하, D7)
const uint8_t LED_TX = 1;                       // 보낼 때 켤 LED 번호 (좌상, D4)

// ---------------- 상수 ----------------
const long          LINK_BAUD      = 9600;     // final 의 LINK_BAUD 와 같음
const unsigned long SEND_PERIOD_MS = 2000UL;   // ?? TODO ?? 시험용 임시값
const unsigned long TX_LED_MS      = 150UL;
const unsigned long RX_LED_MS      = 300UL;
const char          SEND_CHAR      = '1';      // 지게차 시험 스케치는 어떤 글자든 삐 한다

// ---------------- 하드웨어 함수 ----------------
void ledSet(uint8_t idx, bool on) {  // ◆ NEW
  digitalWrite(LED_PINS_CW[idx], on ? LED_ON_LEVEL : (LED_ON_LEVEL == HIGH ? LOW : HIGH));
}

void ledsAll(bool on) {  // ◆ NEW
  for (uint8_t i = 0; i < 4; i++) ledSet(i, on);
}

// ====================================================================================================
// SERIAL TEST SECTION  (module B is on the hardware Serial D0/D1; there is no USB monitor)
// ====================================================================================================
unsigned long lastSendMs = 0;
unsigned long txLedOffMs = 0;
unsigned long rxLedOffMs = 0;
bool txLedOn = false;
bool rxLedOn = false;

void setup() {
  for (uint8_t i = 0; i < 4; i++) pinMode(LED_PINS_CW[i], OUTPUT);
  ledsAll(true);                     // 1초 동안 모두 켬 = 스케치가 돌고 있다
  delay(1000);
  ledsAll(false);
  Serial.begin(LINK_BAUD);
}

void loop() {
  unsigned long now = millis();
  while (Serial.available() > 0) {   // 지게차 -> 센터
    Serial.read();
    ledSet(LED_RX, true);
    rxLedOn = true;
    rxLedOffMs = now + RX_LED_MS;
  }
  if (now - lastSendMs >= SEND_PERIOD_MS) {   // 센터 -> 지게차
    lastSendMs = now;
    Serial.write(SEND_CHAR);
    ledSet(LED_TX, true);
    txLedOn = true;
    txLedOffMs = now + TX_LED_MS;
  }
  if (rxLedOn && (long)(now - rxLedOffMs) >= 0) { ledSet(LED_RX, false); rxLedOn = false; }
  if (txLedOn && (long)(now - txLedOffMs) >= 0) { ledSet(LED_TX, false); txLedOn = false; }
}
