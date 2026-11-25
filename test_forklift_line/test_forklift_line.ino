// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | line sensor check through the serial monitor; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_line.ino  -  라인 트레이서(A2~A4) 단독 시험 + 직선 보정 값 확인: 핀 / 상수 / 하드웨어 함수 (USB Serial 은 아래쪽 시험 영역에서만 사용)
 * 보드: FRIBEE EDU (UNO R3 호환)
 *
 * [파일 구성]  .ino 파일 1개 (별도 .h 없음)
 *   위쪽 : 핀 / 상수 / 하드웨어 함수 (Serial 사용 없음)
 *   아래쪽 ("SERIAL TEST SECTION" 이후) : 시리얼 모니터 메뉴 + setup()/loop() (진단 전용)
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 * 규칙: 센서 세부사항은 사용자 확인 후 반영. 바퀴는 움직이지 않고 값만 출력한다.
 */

// ---------------- 핀맵 (사용자 제공 값 그대로, 임의 변경 금지) ----------------
const uint8_t PIN_LINE_LEFT   = A2;  // 라인 트레이서 왼쪽
const uint8_t PIN_LINE_CENTER = A3;  // 라인 트레이서 중앙
const uint8_t PIN_LINE_RIGHT  = A4;  // 라인 트레이서 오른쪽

// ---------------- 상수 ----------------
// 라인 센서 실측(사용자): 검은 선(폭 19mm) 위 31 이상, 흰 바닥 약 24~29 (analogRead 원시값, 선이 클수록 큼)
const int  LINE_LEVEL1       = 27;    // 사용자 지정: 좌/우 값이 이 값 초과 -> 아주 미세한 직선 보정
const int  LINE_LEVEL2       = 30;    // 사용자 지정: 좌/우 값이 이 값 초과 -> 미세한 직선 보정
const bool LINE_STEER_INVERT = true;  // 사용자 지시("회전은 반전으로"): 처음 가정과 반대 방향으로 조향. 실주행에서 반대면 false
const int  LINE_BASE_SPEED   = 100;   // ?? TODO ??: 직진 기본 속도 (임시값)
const int  LINE_TRIM_1       = 5;     // ?? TODO ??: "아주 미세" 보정의 바퀴 감속량 (임시값, 서보 펄스 us)
const int  LINE_TRIM_2       = 15;    // ?? TODO ??: "미세" 보정의 바퀴 감속량 (임시값)

// ---------------- 하드웨어 함수 ----------------
void readLineRaw(int &left, int &center, int &right) {  // ◆ NEW: analogRead 원시값만 읽는다 (센서 종류/극성 판단 없음)
  left   = analogRead(PIN_LINE_LEFT);
  center = analogRead(PIN_LINE_CENTER);
  right  = analogRead(PIN_LINE_RIGHT);
}

// ---------------- 라인 직진 보정 (좌/우 라인 센서 단계별 조향) ----------------
// ◆ NEW: 사용자 규칙 -- 좌/우 값이 각각 27 초과면 아주 미세, 30 초과면 미세하게 방향 조정.
//   (PPT 의 ndShade 비율 방식은 쓰지 않음: 실측값이 24~31 로 좁아 비율이 최대 약 0.06 밖에 변하지 않음)
int lineLevel(int value) {
  if (value > LINE_LEVEL2) return 2;
  if (value > LINE_LEVEL1) return 1;
  return 0;
}

int lineTrim(int level) {
  if (level == 2) return LINE_TRIM_2;
  if (level == 1) return LINE_TRIM_1;
  return 0;
}

// 기본 방향(제 가정): 한쪽 값이 커지면 그쪽으로 조향 (그쪽 바퀴 감속).
// 사용자가 "회전은 반전으로"라고 지시해 LINE_STEER_INVERT = true 로 반대 방향을 쓴다.
void lineSteer(int lineLeft, int lineRight, int &speedLeft, int &speedRight) {
  int net = lineTrim(lineLevel(lineRight)) - lineTrim(lineLevel(lineLeft));
  if (LINE_STEER_INVERT) net = -net;
  speedLeft  = LINE_BASE_SPEED;
  speedRight = LINE_BASE_SPEED;
  if (net > 0) speedRight -= net;   // net > 0: 오른쪽 바퀴 감속 (오른쪽으로 회전)
  else         speedLeft  += net;   // net <= 0: 왼쪽 바퀴 감속 (왼쪽으로 회전)
}

// ====================================================================================================
// SERIAL TEST SECTION  (USB Serial is used only below this line)
// ====================================================================================================
/*
 * 시리얼 테스트 구역  -  라인 센서와 직선 보정 값을 시리얼 모니터로 확인하는 테스트 코드 (진단 전용)
 *
 * 이 아래의 모든 함수는 Serial(USB) 을 사용한다. 실제 읽기/계산 함수는 위쪽 하드웨어 함수 구역에 있다.
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
