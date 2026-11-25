/*
 * test_serial_menus.ino  -  시리얼 모니터로 결과를 확인하는 테스트 코드 (진단 전용)
 *
 * 이 파일의 모든 함수는 Serial(USB) 을 사용한다. 통합 주행 스케치에서는 이 파일을 빼면 된다.
 * 실제 하드웨어 동작 함수는 test_hw_drivers.ino, 핀맵/상수/객체는 test_forklift_board_hardware.ino 에 있다.
 * 사용법: 시리얼 모니터 9600 bps -> 메뉴 번호 입력
 */

// ---------------- 입력 보조 (◆ NEW) ----------------
int waitForChar(unsigned long timeoutMs) {
  unsigned long start = millis();
  while (millis() - start < timeoutMs) {
    if (Serial.available() > 0) {
      int c = Serial.read();
      if (c != '\n' && c != '\r') return c;
    }
  }
  return -1;
}

void printMenu() {
  Serial.println(F("\n== Forklift test =="));
  Serial.println(F("1 Buzzer 2 Sonar 3 IR(L/F/R) 4 Photo(A0,A1)"));
  Serial.println(F("5 Line(A2-A4) 6 Wheels 7 Lift 8 BT 9 LineSteer"));
  Serial.println(F("Select:"));
}

// ---------------- 1. 부저 ----------------
void testBuzzer() {
  // ★ EXTERNAL: 2_2_ABOT주행.pptx 슬라이드 10 (핀 4 -> PIN_BUZZER 로만 변경)
  tone(PIN_BUZZER, 3000, 1000);
  delay(1000);
  Serial.println(F("Buzzer 3kHz 1s done"));
}

// ---------------- 2. 초음파 (HC-SR04) ----------------
// ★ EXTERNAL LIBRARY: Ultrasonic (Erick Simões). PPT에는 초음파 예제가 없어 라이브러리 사용.
//   read() 의 기본 단위는 cm (README 근거). 타임아웃 시 반환값은 README에 명시 없음
//   -> 센서 앞(지게차에서는 후방)에 물체가 없을 때 어떤 값이 찍히는지 이 테스트로 확인할 것.
void testUltrasonic() {
  Serial.println(F("Sonar cm (rear sensor; also check no-object value)"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    long distanceCm = ultrasonic.read();
    Serial.println(distanceCm);
    delay(SAMPLE_INTERVAL_MS);
  }
}

// ---------------- 3. IR 3방향 ----------------
void testIr() {
  Serial.println(F("IR 0=detect 1=none  L F R"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    byte l, f, r;
    irScan(IR_FREQ_HZ, l, f, r);
    Serial.print(l); Serial.print(' ');
    Serial.print(f); Serial.print(' ');
    Serial.println(r);
    delay(SAMPLE_INTERVAL_MS);
  }
}

// ---------------- 4. 광원 추적 ----------------
void testPhoto() {
  Serial.println(F("Photo A0 raw/V   A1 raw/V"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    Serial.print(analogRead(PIN_PHOTO_LEFT));  Serial.print(" / ");
    Serial.print(volts(PIN_PHOTO_LEFT));       Serial.print("V    ");
    Serial.print(analogRead(PIN_PHOTO_RIGHT)); Serial.print(" / ");
    Serial.print(volts(PIN_PHOTO_RIGHT));      Serial.println("V");
    delay(SAMPLE_INTERVAL_MS);
  }
}

// ---------------- 5. 라인 트레이서 ----------------
// ◆ NEW: 센서 종류/극성/임계값 미확인 -> 원시 analogRead 값만 출력
void testLine() {
  Serial.println(F("Line raw  A2(L) A3(C) A4(R)"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    Serial.print(analogRead(PIN_LINE_LEFT));   Serial.print('\t');
    Serial.print(analogRead(PIN_LINE_CENTER)); Serial.print('\t');
    Serial.println(analogRead(PIN_LINE_RIGHT));
    delay(SAMPLE_INTERVAL_MS);
  }
}

// ---------------- 5b. 라인 직진 보정 값 확인 ----------------
// ★ EXTERNAL(변형): 4_ABOT빛으로로봇조종.pptx 슬라이드 34 실습과제 1
//   ("ndShade 에 따른 speedLeft 와 speedRight 값을 출력")과 같은 방식. 바퀴는 돌리지 않고 값만 출력.
void testLineSteer() {
  Serial.println(F("Line steer: L C R -> speedL speedR (wheels NOT moved)"));
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    int l = analogRead(PIN_LINE_LEFT);
    int c = analogRead(PIN_LINE_CENTER);
    int r = analogRead(PIN_LINE_RIGHT);
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

// ---------------- 6. 구동 바퀴 ----------------
void testWheels() {
  Serial.println(F("Lift wheels off ground! y=start"));
  if (waitForChar(INPUT_TIMEOUT_MS) != 'y') {
    Serial.println(F("Cancelled"));
    return;
  }
  servoLeft.attach(PIN_SERVO_LEFT);
  servoRight.attach(PIN_SERVO_RIGHT);
  const int s = WHEEL_TEST_SPEED;
  Serial.println(F("Left only"));   maneuver(s, 0, WHEEL_TEST_MS);
  Serial.println(F("Right only"));  maneuver(0, s, WHEEL_TEST_MS);
  Serial.println(F("Forward"));     maneuver(s, s, WHEEL_TEST_MS);
  Serial.println(F("Backward"));    maneuver(-s, -s, WHEEL_TEST_MS);
  Serial.println(F("Spin left"));   maneuver(-s, s, WHEEL_TEST_MS);
  Serial.println(F("Spin right"));  maneuver(s, -s, WHEEL_TEST_MS);
  stopWheels();
  Serial.println(F("Done. Report any reversed wheel"));
}

// ---------------- 7. 리프트 서보 ----------------
void testLift() {
  Serial.println(F("Lift: u=up(1700us) d=down(1300us), one step, q=quit"));
  servoLift.attach(PIN_SERVO_LIFT);
  servoLift.writeMicroseconds(SERVO_STOP_US);
  while (true) {
    int c = waitForChar(INPUT_TIMEOUT_MS);
    if (c == 'q' || c == -1) break;
    if (c == 'u') liftStep(LIFT_SPEED);
    else if (c == 'd') liftStep(-LIFT_SPEED);
  }
  servoLift.detach();
  Serial.println(F("Lift test end"));
}

// ---------------- 8. 블루투스 (HC-05/06, 슬레이브) ----------------
// ★ EXTERNAL LIBRARY: NeoSWSerial. 마스터(다른 UNO)가 보낸 바이트를 USB 시리얼로 출력.
void testBluetooth() {
  Serial.println(F("BT rx 15s: send from master"));
  bt.begin(BT_BAUD);
  btSendText("BT OK\n");
  unsigned long start = millis();
  while (millis() - start < BT_TEST_MS) {
    if (bt.available() > 0) {
      int c = bt.read();
      Serial.print(F("rx "));
      Serial.print((char)c);
      Serial.print(' ');
      Serial.println(c);
    }
  }
  Serial.println(F("BT test end"));
}

// ---------------- setup / loop (시리얼 메뉴) ----------------
void setup() {
  Serial.begin(SERIAL_BAUD);
  initHardware();
  printMenu();
}

void loop() {
  if (Serial.available() <= 0) return;
  int cmd = Serial.read();
  switch (cmd) {
    case '1': testBuzzer();     break;
    case '2': testUltrasonic(); break;
    case '3': testIr();         break;
    case '4': testPhoto();      break;
    case '5': testLine();       break;
    case '6': testWheels();     break;
    case '7': testLift();       break;
    case '8': testBluetooth();  break;
    case '9': testLineSteer();  break;
    case '\n':
    case '\r': return;
    default:  Serial.println(F("Unknown cmd"));
  }
  printMenu();
}
