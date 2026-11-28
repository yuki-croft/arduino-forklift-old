// ====================================================================================================
// TOOL (not final) | board: FORKLIFT | line sensor calibration with buzzer cues, results on the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * tool_forklift_line_calibrate.ino  -  라인 트레이서 3개 다시 캘리브레이션 (2026-11-28 사용자 요청)
 * 보드: FRIBEE EDU (UNO R3 호환). 출력: USB 시리얼 모니터 9600 (보기만 함: 입력은 받지 않는다. D0 에 빨간 LED 가 있어 PC -> 보드 입력이 불안정할 수 있음)
 * 진행은 시간과 부저로 안내한다. 바퀴/리프트는 움직이지 않는다 (손으로 지게차를 옮긴다).
 *   0) 켜면 긴 삐 -> 5초 준비
 *   1) 삐 1번  : 세 센서 모두 흰 바닥 위.  자리 잡을 시간 PLACE_MS 뒤 SAMPLE_MS 동안 측정 (그동안 흰 바닥 위에서 천천히 움직여도 됨)
 *   2) 삐 2번  : 왼쪽 센서만 검은 선 한가운데 위.  자리 잡을 시간 뒤 측정 (가만히)
 *   3) 삐 3번  : 가운데 센서를 검은 선 한가운데 위 (평소 주행 자세).  측정 (가만히)
 *   4) 삐 4번  : 오른쪽 센서만 검은 선 한가운데 위.  측정 (가만히)
 *   5) 긴 삐 2번: 끝. 결과와 지게차 코드에 붙여 넣을 줄을 10초마다 다시 보여 준다
 *   측정 중에는 0.5초마다 짧은 틱 소리. 단계가 바뀔 때 시리얼 모니터에도 안내 문장이 나온다.
 * 기준 계산 (지금 지게차 코드와 같은 방식):
 *   흰색 상한 = 흰 바닥 최댓값 + 1   (넘으면 "선에 걸치기 시작" = 아주 미세 보정)
 *   검정 하한 = (흰 바닥 최댓값 + 검은 선 최솟값) / 2 를 올림   (넘으면 "검정")
 *   검은 선 최솟값이 흰 바닥 최댓값보다 MIN_GAP 이하로 크면 "WARNING" (구분이 어려움 -> 센서 높이/위치 확인)
 */

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_BUZZER      = 5;
const uint8_t PIN_LINE_LEFT   = A2;
const uint8_t PIN_LINE_CENTER = A3;
const uint8_t PIN_LINE_RIGHT  = A4;
const uint8_t PINS[3]         = {PIN_LINE_LEFT, PIN_LINE_CENTER, PIN_LINE_RIGHT};

// ---------------- 상수 ----------------
const long          SERIAL_BAUD = 9600;
const unsigned long READY_MS    = 5000UL;   // 처음 준비 시간
const unsigned long PLACE_MS    = 6000UL;   // ?? TODO ?? 단계마다 자리 잡을 시간
const unsigned long SAMPLE_MS   = 6000UL;   // ?? TODO ?? 단계마다 측정 시간
const unsigned long READ_MS     = 5UL;      // 읽기 간격 (6초에 약 1200번)
const int           MIN_GAP     = 4;        // ?? TODO ?? 흰/검 차이가 이 값 이하이면 경고
const unsigned int  BEEP_HZ     = 3000;

int whiteMin[3], whiteMax[3]; long whiteSum[3]; unsigned long whiteN = 0;
int blackMin[3], blackMax[3]; long blackSum[3]; unsigned long blackN[3] = {0, 0, 0};
bool done = false;
unsigned long lastReportMs = 0;

// ---------------- 하드웨어 함수 ----------------
void beep(unsigned long ms) { tone(PIN_BUZZER, BEEP_HZ, ms); delay(ms); }   // ◆ NEW
void beeps(uint8_t n) { for (uint8_t i = 0; i < n; i++) { beep(150); delay(150); } }   // ◆ NEW

// 측정: sensor = -1 이면 세 센서 모두 흰 바닥으로, 0/1/2 이면 그 센서를 검은 선으로 기록
void sampleFor(int8_t sensor) {  // ◆ NEW
  unsigned long t0 = millis(), lastTick = 0;
  while (millis() - t0 < SAMPLE_MS) {
    for (uint8_t i = 0; i < 3; i++) {
      int v = analogRead(PINS[i]);
      if (sensor < 0) {
        if (v < whiteMin[i]) whiteMin[i] = v;
        if (v > whiteMax[i]) whiteMax[i] = v;
        whiteSum[i] += v;
      } else if (i == (uint8_t)sensor) {
        if (v < blackMin[i]) blackMin[i] = v;
        if (v > blackMax[i]) blackMax[i] = v;
        blackSum[i] += v;
        blackN[i]++;
      }
    }
    if (sensor < 0) whiteN++;
    if (millis() - lastTick >= 500) { lastTick = millis(); tone(PIN_BUZZER, 1500, 20); }
    delay(READ_MS);
  }
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB serial monitor output only, no input)
// ====================================================================================================
const char *const NAMES[3] = {"L(A2)", "C(A3)", "R(A4)"};
const char *const SUFFIX[3] = {"L", "C", "R"};

void step(uint8_t n, const char *what, int8_t sensor) {
  Serial.print(F("STEP ")); Serial.print(n); Serial.print(F(": ")); Serial.println(what);
  beeps(n);
  Serial.println(F("  place it now..."));
  delay(PLACE_MS);
  Serial.println(F("  measuring, hold still..."));
  sampleFor(sensor);
  Serial.println(F("  ok"));
}

void report() {
  Serial.println(F("---- result (white min/avg/max | black min/avg/max) ----"));
  for (uint8_t i = 0; i < 3; i++) {
    Serial.print(NAMES[i]); Serial.print(F("  white "));
    Serial.print(whiteMin[i]); Serial.print('/'); Serial.print(whiteN ? (int)(whiteSum[i] / (long)whiteN) : 0); Serial.print('/'); Serial.print(whiteMax[i]);
    Serial.print(F(" | black "));
    Serial.print(blackMin[i]); Serial.print('/'); Serial.print(blackN[i] ? (int)(blackSum[i] / (long)blackN[i]) : 0); Serial.print('/'); Serial.print(blackMax[i]);
    if (blackMin[i] - whiteMax[i] <= MIN_GAP) Serial.print(F("   WARNING: black and white too close"));
    Serial.println();
  }
  Serial.println(F("---- paste into final_forklift_board.ino (replace the 3 lines) ----"));
  for (uint8_t i = 0; i < 3; i++) {
    int wmax = whiteMax[i] + 1;
    int bmin = (whiteMax[i] + blackMin[i] + 1) / 2;
    if (bmin <= wmax) bmin = wmax + 1;
    Serial.print(F("const int  LINE_WHITE_MAX_")); Serial.print(SUFFIX[i]); Serial.print(F(" = ")); Serial.print(wmax);
    Serial.print(F(", LINE_BLACK_MIN_")); Serial.print(SUFFIX[i]); Serial.print(F(" = ")); Serial.print(bmin);
    Serial.print(F(";   // calibrated: white max ")); Serial.print(whiteMax[i]); Serial.print(F(", black min ")); Serial.println(blackMin[i]);
  }
}

void setup() {
  pinMode(PIN_BUZZER, OUTPUT);
  Serial.begin(SERIAL_BAUD);
  for (uint8_t i = 0; i < 3; i++) { whiteMin[i] = 1023; whiteMax[i] = 0; whiteSum[i] = 0; blackMin[i] = 1023; blackMax[i] = 0; blackSum[i] = 0; }
  Serial.println(F("line sensor calibration: follow the beeps (1 = all on white, 2 = LEFT on line, 3 = CENTER on line, 4 = RIGHT on line)"));
  beep(600);
  delay(READY_MS);
  step(1, "all three sensors on WHITE floor (you may slide it slowly on white)", -1);
  step(2, "LEFT sensor only on the middle of the BLACK line", 0);
  step(3, "CENTER sensor on the middle of the BLACK line (normal driving pose)", 1);
  step(4, "RIGHT sensor only on the middle of the BLACK line", 2);
  beep(600); delay(200); beep(600);
  done = true;
  report();
  lastReportMs = millis();
}

void loop() {
  if (done && millis() - lastReportMs >= 10000UL) { lastReportMs = millis(); report(); }
}
