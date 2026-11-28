// ====================================================================================================
// TOOL (not final) | board: FORKLIFT | prints the 3 line sensors once per second on the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * tool_forklift_line_values.ino  -  지게차 라인 트레이서 값 측정 (A2 왼쪽 / A3 가운데 / A4 오른쪽, 1초마다 자동)
 * 보드: FRIBEE EDU (UNO R3 호환). 출력: USB 시리얼 모니터 9600.
 *   (2026-11-28 사용자 요청: "1초마다 값 자동으로 측정되는 거 3개 다")
 * 쓰는 법: USB 를 꽂고 업로드 -> 시리얼 모니터 9600 -> 지게차를 흰 바닥 / 검은 선 / 가지 / 모서리 / 표식 위에 차례로 올려 값을 본다.
 *   블루투스 모듈은 D2/D3 이라 USB 시리얼(D0/D1)과 겹치지 않는다. 바퀴/리프트는 움직이지 않는다.
 *   한 줄 형식: "초,왼쪽A2,가운데A3,오른쪽A4,판정" (판정: 30 이상 = B 검정, 29 이하 = W 흰색, 사용자 기준 2026-11-28)
 *   매 출력 사이 1초 동안 센서마다 여러 번 읽어 평균을 낸다 (순간 튀는 값 완화). 최소/최대는 10초마다 한 줄.
 */

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_LINE_LEFT   = A2;
const uint8_t PIN_LINE_CENTER = A3;
const uint8_t PIN_LINE_RIGHT  = A4;

// ---------------- 상수 ----------------
const long          SERIAL_BAUD    = 9600;
const unsigned long PRINT_MS       = 1000UL;   // 사용자: 1초마다
const unsigned long SAMPLE_MS      = 10UL;     // 평균용 읽기 간격 (1초에 약 100번)
const unsigned long MINMAX_EVERY   = 10;       // 출력 10번(10초)마다 최소/최대 한 줄
const int           LINE_BLACK_MIN = 30;       // 사용자 기준: 검은 선 30~34
const uint8_t       PINS[3]        = {PIN_LINE_LEFT, PIN_LINE_CENTER, PIN_LINE_RIGHT};

long sum[3] = {0, 0, 0};
unsigned int count = 0;
int minV[3] = {1023, 1023, 1023};
int maxV[3] = {0, 0, 0};
unsigned long lastSampleMs = 0, lastPrintMs = 0, printCount = 0;

void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println(F("line values: every 1 s, average of the last second"));
  Serial.println(F("sec,L(A2),C(A3),R(A4),LCR (B = 30 or more, W = 29 or less)"));
}

void loop() {
  unsigned long now = millis();
  if (now - lastSampleMs >= SAMPLE_MS) {
    lastSampleMs = now;
    for (uint8_t i = 0; i < 3; i++) {
      int v = analogRead(PINS[i]);
      sum[i] += v;
      if (v < minV[i]) minV[i] = v;
      if (v > maxV[i]) maxV[i] = v;
    }
    count++;
  }
  if (now - lastPrintMs >= PRINT_MS && count > 0) {
    lastPrintMs = now;
    int avg[3];
    for (uint8_t i = 0; i < 3; i++) { avg[i] = (int)((sum[i] + count / 2) / count); sum[i] = 0; }
    count = 0;
    Serial.print(now / 1000); Serial.print(',');
    Serial.print(avg[0]); Serial.print(','); Serial.print(avg[1]); Serial.print(','); Serial.print(avg[2]); Serial.print(',');
    for (uint8_t i = 0; i < 3; i++) Serial.print(avg[i] >= LINE_BLACK_MIN ? 'B' : 'W');
    Serial.println();
    if (++printCount % MINMAX_EVERY == 0) {
      Serial.print(F("min L,C,R = ")); Serial.print(minV[0]); Serial.print(','); Serial.print(minV[1]); Serial.print(','); Serial.print(minV[2]);
      Serial.print(F("   max L,C,R = ")); Serial.print(maxV[0]); Serial.print(','); Serial.print(maxV[1]); Serial.print(','); Serial.println(maxV[2]);
      for (uint8_t i = 0; i < 3; i++) { minV[i] = 1023; maxV[i] = 0; }
    }
  }
}
