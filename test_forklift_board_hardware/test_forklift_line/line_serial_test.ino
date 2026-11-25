/*
 * line_serial_test.ino  -  라인 센서와 직선 보정 값을 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기/계산 함수는 test_forklift_line.ino 에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

const long SERIAL_BAUD        = 9600;
const int  SAMPLE_COUNT       = 10;
const int  SAMPLE_INTERVAL_MS = 300;

void printMenu() {
  Serial.println(F("\n== Forklift line test =="));
  Serial.println(F("1 Line raw (A2-A4) 2 LineSteer values"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 라인 트레이서 ----------------
// ◆ NEW: 센서 종류/극성/임계값 미확인 -> 원시 analogRead 값만 출력
void testLine() {
  Serial.println(F("Line raw  A2(L) A3(C) A4(R)"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int l, c, r;
    readLineRaw(l, c, r);
    Serial.print(l); Serial.print('\t');
    Serial.print(c); Serial.print('\t');
    Serial.println(r);
    delay(SAMPLE_INTERVAL_MS);
  }
}

// ---------------- 2. 라인 직진 보정 값 확인 ----------------
// ★ EXTERNAL(변형): 4_ABOT빛으로로봇조종.pptx 슬라이드 34 실습과제 1
//   ("ndShade 에 따른 speedLeft 와 speedRight 값을 출력")과 같은 방식. 바퀴는 돌리지 않고 값만 출력.
void testLineSteer() {
  Serial.println(F("Line steer: L C R -> speedL speedR (wheels NOT moved)"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int l, c, r;
    readLineRaw(l, c, r);
    int sl, sr;
    lineSteer(l, r, sl, sr);
    Serial.print(l); Serial.print(' ');
    Serial.print(c); Serial.print(' ');
    Serial.print(r); Serial.print(F(" -> "));
    Serial.print(sl);   Serial.print(' ');
    Serial.println(sr);
    delay(SAMPLE_INTERVAL_MS);
  }
}

// ---------------- setup / loop (시리얼 메뉴) ----------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  printMenu();
}

void loop() {
  if (Serial.available() <= 0) return;
  int cmd = Serial.read();
  switch (cmd) {
    case '1': testLine();      break;
    case '2': testLineSteer(); break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
