/*
 * test_hw_drivers.ino  -  하드웨어 구동/읽기 함수 모음 (시리얼 모니터 사용 없음)
 *
 * 이 파일은 Serial 을 전혀 쓰지 않는다. 나중에 통합 주행 스케치에서 그대로 재사용할 수 있다.
 * 핀맵/상수/객체(servoLeft, bt 등)는 test_forklift_board_hardware.ino 에 정의되어 있다.
 * Arduino IDE 는 같은 폴더의 .ino 파일들을 하나로 이어 붙여 컴파일한다 (별도 .h 불필요).
 *
 * [표기 규칙]  ★ EXTERNAL : 수업 PPT 코드  /  ◆ NEW : 새로 작성
 */

// ---------------- 초기화 ----------------
void initHardware() {  // ◆ NEW
  pinMode(PIN_IR_TX, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_US_TRIG, OUTPUT);
  pinMode(PIN_US_ECHO, INPUT);
  pinMode(PIN_IR_RX_LEFT, INPUT);
  pinMode(PIN_IR_RX_FRONT, INPUT);
  pinMode(PIN_IR_RX_RIGHT, INPUT);
  pinMode(PIN_BT_RX, INPUT);
  pinMode(PIN_BT_TX, OUTPUT);
}

// ---------------- IR 3방향 (발신 1핀 공유) ----------------
// ★ EXTERNAL(변형): 5_ABOT적외선센서.pptx 슬라이드 9의 irDetect()
//   원본은 발신/수신 핀 1:1 구조. 지게차는 발신 1핀(D4) + 수신 3핀이라
//   한 번 발신 후 3개 수신기를 연속 읽도록 변경함. 반환: 0=감지, 1=미감지
void irScan(long frequency, byte &left, byte &front, byte &right) {
  tone(PIN_IR_TX, frequency, 8);   // IR LED 38kHz 8ms
  delay(1);                        // 원본과 동일: 1ms 대기
  left  = digitalRead(PIN_IR_RX_LEFT);
  front = digitalRead(PIN_IR_RX_FRONT);
  right = digitalRead(PIN_IR_RX_RIGHT);
  delay(1);                        // 원본과 동일: 재확인 전 휴지
}

// ---------------- 광원 추적 (포토트랜지스터) ----------------
// ★ EXTERNAL: 4_ABOT빛으로로봇조종.pptx 슬라이드 11 (volts 함수 그대로)
float volts(int adPin) {
  return float(analogRead(adPin)) * 5.0 / 1023.0;
}

// ---------------- 구동 바퀴 ----------------
// ★ EXTERNAL(변형): 2_2_ABOT주행.pptx 슬라이드 19의 maneuver()
//   (-1 이면 detach 하던 분기는 stopWheels()로 분리)
void maneuver(int speedLeft, int speedRight, int msTime) {
  servoLeft.writeMicroseconds(SERVO_STOP_US + speedLeft);
  servoRight.writeMicroseconds(SERVO_STOP_US - speedRight);
  delay(msTime);
}

void stopWheels() {  // ◆ NEW
  servoLeft.writeMicroseconds(SERVO_STOP_US);
  servoRight.writeMicroseconds(SERVO_STOP_US);
  delay(200);
  servoLeft.detach();
  servoRight.detach();
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

// ---------------- 리프트 서보 (연속회전) ----------------
// ◆ NEW: speed > 0 = 올림(1700us), speed < 0 = 내림(1300us) (사용자 지정). 한계 스위치 정보 없음 -> 짧은 스텝만 수동 조작
void liftStep(int speed) {
  servoLift.writeMicroseconds(SERVO_STOP_US + speed);
  delay(LIFT_TEST_MS);
  servoLift.writeMicroseconds(SERVO_STOP_US);
}

// ---------------- 블루투스 송신 ----------------
// ★ EXTERNAL LIBRARY: NeoSWSerial. 송신(write) 중에는 인터럽트가 꺼져 수신/서보에 영향
//   -> 송신은 최소화할 것.
void btSendText(const char *text) {
  while (*text) bt.write(*text++);
}
