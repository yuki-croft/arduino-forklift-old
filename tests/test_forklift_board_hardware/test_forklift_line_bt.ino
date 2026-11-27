// ====================================================================================================
// TEST CODE (not final) | board: FORKLIFT | streams the 3 line sensors to a phone Bluetooth terminal; do not use as the final program
// ====================================================================================================
/*
 * test_forklift_line_bt.ino  -  라인 센서 A2/A3/A4 값을 블루투스로 폰 터미널 앱에 계속 보낸다 (USB 시리얼 모니터 불필요)
 * 보드: FRIBEE EDU (UNO R3 호환). 지게차 블루투스 모듈은 하드웨어 시리얼 D0/D1, 9600 (폰 터미널과 양방향 동작은 사용자 확인 완료).
 * 목적: "라인이 읽히지 않는 것 같다"는 증상을 실제 값으로 확인하고, PROBLEM_1PAGER 미결정 20번 (직선/T자/모서리/표식의 시간 순 기록) 을 얻는다.
 *
 * [쓰는 법] 업로드 때는 모듈 선(D0/D1)을 뺀다. 센터 보드 전원은 끄고 (모듈이 센터와 붙지 않게), 폰 터미널 앱으로 지게차 모듈에 연결한다.
 *   폰에서 글자 하나를 보내 모드를 고른다 (줄 끝 설정 상관없음):
 *     p = 값만 보내기 (바퀴 정지). 지게차를 손으로 천천히 밀면서 직선 / T자 가지 / 모서리 / 표식 위를 지나가 본다
 *     d = 최종 코드와 같은 직선 보정으로 천천히 주행하면서 값 보내기 (선 위에 올려 두고 시작)
 *     s = 정지 (값 보내기도 멈춤)
 *     m = 지금까지의 센서별 최솟값/최댓값 한 줄
 *     r = 최솟값/최댓값 초기화
 *   보내는 줄: "시간ms,왼쪽A2,가운데A3,오른쪽A4" (예: 12345,26,33,27). 이 줄들을 복사해서 보내 주면 판정 규칙을 정할 수 있다.
 *   전원을 켜면 짧은 삐 1번 (스케치 동작 확인).
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성  /  ?? TODO ?? : 사용자 확인 전 임시값
 */
#include <Servo.h>   // Arduino 표준 라이브러리 (바퀴 서보)

// ---------------- 핀맵 (final_forklift_board 와 같음) ----------------
const uint8_t PIN_LINE_LEFT   = A2;
const uint8_t PIN_LINE_CENTER = A3;
const uint8_t PIN_LINE_RIGHT  = A4;
const uint8_t PIN_SERVO_LEFT  = 10;
const uint8_t PIN_SERVO_RIGHT = 11;
const uint8_t PIN_BUZZER      = 5;

// ---------------- 상수 (주행 보정은 final_forklift_board 의 값과 같음) ----------------
const long          BT_BAUD           = 9600;
const unsigned long SEND_PERIOD_MS    = 50UL;   // ?? TODO ?? 시험용 임시값: 초당 20줄 (9600 bps 로 충분)
const int           SERVO_STOP_US     = 1500;   // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 19
const int           LINE_LEVEL1       = 27;     // 사용자 지정
const int           LINE_LEVEL2       = 30;     // 사용자 지정
const bool          LINE_STEER_INVERT = true;   // 사용자 지시 "회전은 반전으로"
const int           DRIVE_SPEED       = 40;     // ?? TODO ?? 시험용 임시값: final(100)보다 느리게 (값을 촘촘히 보려고)
const int           LINE_TRIM_1       = 5;
const int           LINE_TRIM_2       = 15;

Servo servoLeft, servoRight;
bool wheelsAttached = false;

// ---------------- 하드웨어 함수 ----------------
void setWheels(int speedLeft, int speedRight) {  // ★ EXTERNAL(변형): final 의 setWheels 와 같음
  if (!wheelsAttached) {
    servoLeft.attach(PIN_SERVO_LEFT);
    servoRight.attach(PIN_SERVO_RIGHT);
    wheelsAttached = true;
  }
  servoLeft.writeMicroseconds(SERVO_STOP_US + speedLeft);
  servoRight.writeMicroseconds(SERVO_STOP_US - speedRight);
}

void stopWheels() {  // ◆ NEW
  if (wheelsAttached) setWheels(0, 0);
}

int lineTrim(int value) {  // ◆ NEW: final 의 lineLevel + lineTrim 을 합친 것
  if (value > LINE_LEVEL2) return LINE_TRIM_2;
  if (value > LINE_LEVEL1) return LINE_TRIM_1;
  return 0;
}

void driveLineStep(int left, int right) {  // ◆ NEW: final 의 lineSteer 와 같은 규칙, 속도만 DRIVE_SPEED
  int net = lineTrim(right) - lineTrim(left);
  if (LINE_STEER_INVERT) net = -net;
  int sl = DRIVE_SPEED, sr = DRIVE_SPEED;
  if (net > 0) sr -= net;
  else         sl += net;
  setWheels(sl, sr);
}

// ====================================================================================================
// SERIAL TEST SECTION  (the Bluetooth module is on the hardware Serial D0/D1; output goes to the phone terminal)
// ====================================================================================================
enum Mode : uint8_t { MODE_STOP, MODE_PRINT, MODE_DRIVE };
Mode mode = MODE_STOP;
unsigned long lastSendMs = 0;
int minV[3] = {1023, 1023, 1023};
int maxV[3] = {0, 0, 0};

void sendMinMax() {
  Serial.print(F("min L,C,R = "));
  Serial.print(minV[0]); Serial.print(','); Serial.print(minV[1]); Serial.print(','); Serial.println(minV[2]);
  Serial.print(F("max L,C,R = "));
  Serial.print(maxV[0]); Serial.print(','); Serial.print(maxV[1]); Serial.print(','); Serial.println(maxV[2]);
}

void handleCommand(char c) {
  if (c == 'p') { mode = MODE_PRINT; stopWheels(); Serial.println(F("PRINT (push by hand)")); }
  else if (c == 'd') { mode = MODE_DRIVE; Serial.println(F("DRIVE slow + print")); }
  else if (c == 's') { mode = MODE_STOP; stopWheels(); Serial.println(F("STOP")); }
  else if (c == 'm') { sendMinMax(); }
  else if (c == 'r') { for (uint8_t i = 0; i < 3; i++) { minV[i] = 1023; maxV[i] = 0; } Serial.println(F("min/max reset")); }
}

void setup() {
  Serial.begin(BT_BAUD);
  pinMode(PIN_BUZZER, OUTPUT);
  tone(PIN_BUZZER, 3000, 150);
  Serial.println(F("line_bt ready: p=print d=drive s=stop m=min/max r=reset"));
  Serial.println(F("ms,L(A2),C(A3),R(A4)"));
}

void loop() {
  while (Serial.available() > 0) handleCommand((char)Serial.read());
  int v[3] = { analogRead(PIN_LINE_LEFT), analogRead(PIN_LINE_CENTER), analogRead(PIN_LINE_RIGHT) };
  for (uint8_t i = 0; i < 3; i++) {
    if (v[i] < minV[i]) minV[i] = v[i];
    if (v[i] > maxV[i]) maxV[i] = v[i];
  }
  if (mode == MODE_DRIVE) driveLineStep(v[0], v[2]);
  if (mode != MODE_STOP && millis() - lastSendMs >= SEND_PERIOD_MS) {
    lastSendMs = millis();
    Serial.print(lastSendMs); Serial.print(',');
    Serial.print(v[0]); Serial.print(','); Serial.print(v[1]); Serial.print(','); Serial.println(v[2]);
  }
}
