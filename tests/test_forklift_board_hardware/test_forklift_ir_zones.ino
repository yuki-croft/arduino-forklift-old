// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | IR left/front/right at several frequencies + rear ultrasonic, once per second; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_ir_zones.ino  -  적외선 3개(왼/앞/오)의 "거리 단계" 사전값 측정 + 후방 초음파 (1초마다 자동, 시리얼 모니터 9600)
 *   (2026-11-28 사용자: 좌/우 적외선도 초음파와 비슷한 방식, 전방 적외선은 컨테이너 앞 도착 확인용 -> 사전값 측정)
 * 원리 (가정): 수신기는 38kHz 에서 가장 민감하고 주파수가 멀어질수록 둔해진다 -> 여러 주파수로 쏴서 몇 개에서 감지되는지 세면
 *   가까울수록 많은 주파수에서 감지된다 ("단계" = 감지된 주파수 개수). 이 가정과 주파수 목록은 실물에서 확인해야 한다.
 *   수업 PPT(5_ABOT적외선센서.pptx 슬라이드 29~32) 의 주파수 단계 값을 쓰려면 아래 FREQS 를 그 값으로 바꿀 것.
 * 쓰는 법: USB 업로드 -> 시리얼 모니터 9600. 바퀴/리프트는 움직이지 않는다. 블루투스(D2/D3)는 쓰지 않는다.
 *   1) 컨테이너(또는 같은 재질의 물체)를 지게차 앞 2, 4, 6, 8, 10cm ... 에 두고 각 거리의 줄을 적는다 (전방 도착 기준)
 *   2) 지게차를 컨테이너 앞 "제대로 도착한 위치" 에 두고 그 줄을 적는다 (이게 가장 중요)
 *   3) 왼쪽/오른쪽 옆에 벽이나 손을 여러 거리에 두고 같은 방법으로 (좌/우 단계)
 *   4) 아무것도 없는 곳에서 한 번 (바탕값)
 * 한 줄 형식: "초 | f1 LFR f2 LFR ... | zone L,F,R | rear cm"   (LFR 은 감지 = 1, 없음 = 0. zone = 감지된 주파수 개수)
 */
#include <Ultrasonic.h>  // ★ EXTERNAL LIBRARY: "Ultrasonic" by Erick Simoes (final 과 같음)

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_IR_TX       = 4;
const uint8_t PIN_US_TRIG     = 6;
const uint8_t PIN_US_ECHO     = 7;
const uint8_t PIN_IR_RX_LEFT  = 8;
const uint8_t PIN_IR_RX_FRONT = 9;
const uint8_t PIN_IR_RX_RIGHT = 13;

// ---------------- 상수 ----------------
const long          SERIAL_BAUD = 9600;
const unsigned long PRINT_MS    = 1000UL;
// ?? TODO ?? 시험할 주파수 (제 가정: 38kHz 부터 1kHz 간격). PPT 값이 있으면 그것으로 바꿀 것.
const long    FREQS[]  = {38000, 39000, 40000, 41000, 42000, 43000};
const uint8_t N_FREQS  = sizeof(FREQS) / sizeof(FREQS[0]);
const uint8_t REPEATS  = 5;   // 주파수마다 5번 쏴서 3번 이상 받으면 "감지" (순간 잡음 거르기)

Ultrasonic ultrasonic(PIN_US_TRIG, PIN_US_ECHO);

// ---------------- 하드웨어 함수 ----------------
// ★ EXTERNAL(변형): final 의 irScan 과 같은 방식 (5_ABOT적외선센서.pptx 슬라이드 9 의 irDetect). 수신기 0 = 감지
void irOnce(long f, uint8_t &l, uint8_t &fr, uint8_t &r) {
  tone(PIN_IR_TX, f, 8);
  delay(1);
  l  = digitalRead(PIN_IR_RX_LEFT)  == 0;
  fr = digitalRead(PIN_IR_RX_FRONT) == 0;
  r  = digitalRead(PIN_IR_RX_RIGHT) == 0;
  delay(9);
}

void irVote(long f, uint8_t &l, uint8_t &fr, uint8_t &r) {  // ◆ NEW: REPEATS 번 중 과반이면 감지
  uint8_t cl = 0, cf = 0, cr = 0;
  for (uint8_t i = 0; i < REPEATS; i++) { uint8_t a, b, c; irOnce(f, a, b, c); cl += a; cf += b; cr += c; }
  l = cl * 2 > REPEATS; fr = cf * 2 > REPEATS; r = cr * 2 > REPEATS;
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB serial monitor output only)
// ====================================================================================================
unsigned long lastPrintMs = 0;

void setup() {
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_IR_RX_LEFT, INPUT);
  pinMode(PIN_IR_RX_FRONT, INPUT);
  pinMode(PIN_IR_RX_RIGHT, INPUT);
  Serial.begin(SERIAL_BAUD);
  Serial.print(F("IR zones test. freqs:"));
  for (uint8_t i = 0; i < N_FREQS; i++) { Serial.print(' '); Serial.print(FREQS[i]); }
  Serial.println();
  Serial.println(F("sec | per freq: L F R (1 = detected) | zone L,F,R | rear cm"));
}

void loop() {
  if (millis() - lastPrintMs < PRINT_MS) return;
  lastPrintMs = millis();
  uint8_t zl = 0, zf = 0, zr = 0;
  Serial.print(lastPrintMs / 1000);
  Serial.print(F(" |"));
  for (uint8_t i = 0; i < N_FREQS; i++) {
    uint8_t l, f, r;
    irVote(FREQS[i], l, f, r);
    zl += l; zf += f; zr += r;
    Serial.print(' '); Serial.print(FREQS[i] / 100); Serial.print(':');
    Serial.print(l); Serial.print(f); Serial.print(r);
  }
  Serial.print(F(" | zone ")); Serial.print(zl); Serial.print(','); Serial.print(zf); Serial.print(','); Serial.print(zr);
  Serial.print(F(" | rear ")); Serial.println((long)ultrasonic.read());
}
