/*
 * forklift_hw.ino  -  구동 함수 (바퀴, 직선 보정, 광 추적, 회전, 리프트 저수준)
 * 설정/상태 변수/객체는 final_forklift_board.ino 에 있다. Serial 을 쓰지 않는다.
 */

void initHardware() {  // ◆ NEW
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  pinMode(PIN_IR_RX_LEFT, INPUT);
  pinMode(PIN_IR_RX_FRONT, INPUT);
  pinMode(PIN_IR_RX_RIGHT, INPUT);
}

// ★ EXTERNAL(변형): 5_ABOT적외선센서.pptx 슬라이드 9 의 irDetect(), 발신 1핀 + 수신 3핀으로 변경. 0 = 감지
void irScan(long frequency, byte &left, byte &front, byte &right) {
  tone(PIN_IR_TX, frequency, 8);
  delay(1);
  left  = digitalRead(PIN_IR_RX_LEFT);
  front = digitalRead(PIN_IR_RX_FRONT);
  right = digitalRead(PIN_IR_RX_RIGHT);
  delay(1);
}

// ---------------- 바퀴 ----------------
// ★ EXTERNAL(변형): 2_2_ABOT주행.pptx 슬라이드 19 의 maneuver() 에서 delay 를 뺀 버전 (오른쪽 속도는 부호 반전)
void setWheels(int speedLeft, int speedRight) {
  if (!wheelsAttached) {
    servoLeft.attach(PIN_SERVO_LEFT);
    servoRight.attach(PIN_SERVO_RIGHT);
    wheelsAttached = true;
  }
  servoLeft.writeMicroseconds(SERVO_STOP_US + speedLeft);
  servoRight.writeMicroseconds(SERVO_STOP_US - speedRight);
}

void stopWheels() {  // ◆ NEW: 정지 펄스를 계속 보냄 (detach 하지 않음)
  if (wheelsAttached) {
    servoLeft.writeMicroseconds(SERVO_STOP_US);
    servoRight.writeMicroseconds(SERVO_STOP_US);
  }
}

// ---------------- 라인 직진 보정 (사용자 규칙: 좌/우 27 초과 아주 미세, 30 초과 미세) ----------------
int lineLevel(int value) {  // ◆ NEW
  if (value > LINE_LEVEL2) return 2;
  if (value > LINE_LEVEL1) return 1;
  return 0;
}

int lineTrim(int level) {  // ◆ NEW
  if (level == 2) return LINE_TRIM_2;
  if (level == 1) return LINE_TRIM_1;
  return 0;
}

// ◆ NEW (방식 B): 두 선 사이를 달릴 때의 직선 보정. 좌/우 값 차이로 조향 (기준값/방향은 ?? TODO ?? 가정)
//   차이 = 왼쪽 - 오른쪽. 오른쪽으로 치우치면 왼쪽 센서가 선 안쪽 가장자리를 벗어나 값이 내려간다(가정) -> 왼쪽으로 돈다.
//   가운데가 검정이면 크게 치우친 것 -> 마지막으로 돌던 쪽으로 계속 크게 돈다.
int laneLastNet = 0;
void lineSteer(int lineLeft, int lineCenter, int lineRight, int &speedLeft, int &speedRight) {
  int net;   // 양수 = 오른쪽 바퀴를 늦춤 (오른쪽으로 돎), 음수 = 왼쪽 바퀴를 늦춤 (왼쪽으로 돎)
  if (lineCenter >= LINE_BLACK_MIN) {
    net = (laneLastNet >= 0) ? LINE_TRIM_2 : -LINE_TRIM_2;
  } else {
    int diff = lineLeft - lineRight;
    int mag = (abs(diff) > LANE_DIFF_2) ? LINE_TRIM_2 : ((abs(diff) > LANE_DIFF_1) ? LINE_TRIM_1 : 0);
    net = (diff < 0) ? -mag : mag;            // 왼쪽 값이 낮음 -> 음수 (왼쪽으로 돎)
    if (LANE_STEER_INVERT) net = -net;
    if (net != 0) laneLastNet = net;
  }
  speedLeft  = LINE_BASE_SPEED;
  speedRight = LINE_BASE_SPEED;
  if (net > 0) speedRight -= net;
  else         speedLeft  += net;
}

void driveLineStep() {  // ◆ NEW: 직선 보정 한 번 (delay 없음)
  int sl, sr;
  lineSteer(analogRead(PIN_LINE_LEFT), analogRead(PIN_LINE_CENTER), analogRead(PIN_LINE_RIGHT), sl, sr);
  setWheels(sl, sr);
}

// ---------------- 광 추적 (라인 이탈 / 비상 복귀) ----------------
// ★ EXTERNAL(변형): 4_ABOT빛으로로봇조종.pptx 슬라이드 28~30 의 ndShade 비례 조향.
//   원본은 어둠에서 멀어지는 방향, 여기서는 밝은 쪽(켜진 LED)으로 가도록 부호를 반대로 함.
//   포토트랜지스터 극성/값 범위 미확인 -> LIGHT_ACTIVE_HIGH, LIGHT_GAIN 은 TODO.
void driveLightStep() {  // ◆ NEW
  float l = float(analogRead(PIN_PHOTO_LEFT));
  float r = float(analogRead(PIN_PHOTO_RIGHT));
  if (!LIGHT_ACTIVE_HIGH) { l = 1023.0 - l; r = 1023.0 - r; }
  float total = l + r;
  float nd = (total > 0.0) ? r / total - 0.5 : 0.0;   // -0.5 ~ +0.5, 양수 = 오른쪽이 더 밝음
  int sl = LIGHT_BASE_SPEED, sr = LIGHT_BASE_SPEED;
  if (nd > 0.0) sr = constrain(int(LIGHT_BASE_SPEED - nd * LIGHT_GAIN), -200, 200);
  else          sl = constrain(int(LIGHT_BASE_SPEED + nd * LIGHT_GAIN), -200, 200);
  setWheels(sl, sr);
}

// ---------------- 제자리 회전 (새 선을 찾을 때까지, 시간 기반 방식은 쓰지 않음: 가정 A1) ----------------
// dir: -1 왼쪽, +1 오른쪽. 막힘 방지를 위해 TURN_TIMEOUT_MS 에서 포기. 회전 중에는 다른 처리가 멈춘다.
bool laneFound() {  // ◆ NEW (방식 B): 왼쪽 검정 + 가운데 흰색 + 오른쪽 검정 (?? TODO ?? 회전 중 잠깐 맞는 경우는 거르지 않음)
  return analogRead(PIN_LINE_LEFT) >= LINE_BLACK_MIN && analogRead(PIN_LINE_CENTER) <= LINE_WHITE_MAX &&
         analogRead(PIN_LINE_RIGHT) >= LINE_BLACK_MIN;
}

void spinUntilLine(int dir) {  // ◆ NEW
  setWheels(dir * TURN_SPEED, -dir * TURN_SPEED);
  unsigned long start = millis();
  while (millis() - start < TURN_MIN_MS) { }                 // 기존 선에서 벗어날 시간
  while (millis() - start < TURN_TIMEOUT_MS && !laneFound()) { }   // 방식 B: 새 두 줄 사이에 들어오면 멈춤
  stopWheels();
}

// ◆ NEW (2026-11-28 사용자): 가지로 들어갈 때는 "축 회전" (한쪽 바퀴는 멈추고 다른 쪽만 움직임),
//   가지에서 후진해 나올 때는 들어간 길을 거꾸로 되짚는 뒤로 축 회전.
//   들어갈 때 (왼쪽): 왼쪽 바퀴 정지 + 오른쪽 바퀴 앞으로 -> 왼쪽 바퀴를 축으로 머리가 왼쪽으로 감
//   나올 때 (오른쪽): 왼쪽 바퀴 정지 + 오른쪽 바퀴 뒤로   -> 같은 축으로 거꾸로 돌아 원래 진행 방향
//   멈추는 조건은 제자리 회전과 같음. 회전 반경/멈추는 위치가 실물에 맞는지는 미확인 (?? TODO ??)
void pivotUntilLine(int speedLeft, int speedRight) {
  setWheels(speedLeft, speedRight);
  unsigned long start = millis();
  while (millis() - start < TURN_MIN_MS) { }
  while (millis() - start < TURN_TIMEOUT_MS && !laneFound()) { }
  stopWheels();
}

void pivotTurnLeft()    { pivotUntilLine(0, TURN_SPEED); }    // ◆ NEW: 가지 진입 (왼쪽 바퀴 축)
void reverseTurnRight() { pivotUntilLine(0, -TURN_SPEED); }   // ◆ NEW: 가지에서 나옴 (같은 축, 뒤로)

void turnLeft()  { spinUntilLine(-1); }   // ◆ NEW
void turnRight() { spinUntilLine(+1); }   // ◆ NEW

// ---------------- 리프트 저수준 (LIFT_INVERT=true 이므로 올림 1300us / 내림 1700us, 실물 시험 결과) ----------------
void liftRun(int dir) {  // ◆ NEW
  servoLift.attach(PIN_SERVO_LIFT);
  servoLift.writeMicroseconds(SERVO_STOP_US + (LIFT_INVERT ? -dir : dir) * LIFT_SPEED);
}

void liftHalt() {  // ◆ NEW
  servoLift.writeMicroseconds(SERVO_STOP_US);
  servoLift.detach();
}
