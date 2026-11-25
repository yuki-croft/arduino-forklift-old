/*
 * forklift_sense.ino  -  센서 판별 함수
 *
 * 주의: senseJunction / senseCorner / senseMarker 는 아직 구현하지 못했다.
 *   이유: 좌-우 라인 센서 간격이 선 폭(19mm)과 같아서, 직선 위에서도 3개가 모두 선 위로 읽힐 수 있다.
 *   직선 중앙 / T자 교차점 / 시작 표식 / 모서리에서의 A2 A3 A4 실측값(PROBLEM_1PAGER.md 미결정 20번)이 필요하다.
 *   그 전까지 항상 false 를 돌려주므로 지게차는 교차점/모서리/표식을 감지하지 못한다 (의도된 안전한 기본값).
 */

bool senseJunction() { return false; }  // ?? TODO ?? 교차점(T자) / 가지선에서 본선 복귀 판별
bool senseCorner()   { return false; }  // ?? TODO ?? 모서리(오른쪽으로 꺾이는 곳) 판별
bool senseMarker()   { return false; }  // ?? TODO ?? 시작 표식(굵은 막대) 판별

// 사용자 실측: 흰 바닥 약 24~29 -> 세 센서가 모두 그 이하면 라인 이탈 (가운데 센서 포함)
bool senseLineLost() {  // ◆ NEW
  return analogRead(PIN_LINE_LEFT)   <= LINE_WHITE_MAX &&
         analogRead(PIN_LINE_CENTER) <= LINE_WHITE_MAX &&
         analogRead(PIN_LINE_RIGHT)  <= LINE_WHITE_MAX;
}

// ?? TODO ?? 38kHz 한 번 쏴서 전방 수신기가 받으면(0) 짐이 있다고 봄. 정지 거리 기준은 PPT 의
//   주파수 단계 방식(5_ABOT적외선센서.pptx 슬라이드 29~32)으로 실제 정지 거리에 맞춰 보정해야 함.
bool senseCargoAhead() {  // ◆ NEW
  byte l, f, r;
  irScan(IR_FREQ_HZ, l, f, r);
  return f == 0;
}

long readRearCm() {  // ◆ NEW
  return (long)ultrasonic.read();   // 단위 cm (라이브러리 README 근거). 타임아웃 때 반환값은 문서에 없음
}

// 사용자 규칙: 거리가 기준 이하인 상태가 일정 시간 지속될 때만 물체로 판단.
// 0 이하의 값은 "측정 실패"로 보고 무시한다 (가정, 타임아웃 반환값을 모르기 때문).
bool rearObstaclePersistent() {  // ◆ NEW
  unsigned long now = millis();
  if (now - rearCheckMs < REAR_CHECK_MS) return false;
  rearCheckMs = now;
  long cm = readRearCm();
  if (cm <= 0 || cm > REAR_STOP_CM) { rearClose = false; return false; }
  if (!rearClose) { rearClose = true; rearSinceMs = now; }
  return (now - rearSinceMs) >= REAR_HOLD_MS;
}
