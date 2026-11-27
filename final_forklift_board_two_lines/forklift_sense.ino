/*
 * forklift_sense.ino  -  센서 판별 함수
 *
 * 주의: senseJunction / senseCorner / senseMarker 는 forklift_line_pattern.h 의 "임시 규칙"으로 구현했다.
 *   사용자 실측(직선 중앙 / T자 교차점 / 모서리 / 시작 표식에서의 A2 A3 A4, PROBLEM_1PAGER.md 미결정 20번)이 없어서
 *   기준값은 제 가정(합성 시험값)이다. 실물에서 맞는지는 확인 전이다.
 *   - senseMarker  : 표식 위에 있는 동안 true (레벨)
 *   - senseCorner / senseJunction : 가지선/모서리를 지난 직후 한 번만 true (펄스, 판별이 끝나는 시점)
 *   - senseJunction 은 왼쪽 구간(segment 1, 상차)과 오른쪽 구간(segment 3, 하차) 모두 "왼쪽 가지"만 인정, 위/아래 구간에서는 항상 false
 *     (2026-11-28 사용자: "왼쪽 가지는 가지 실행 후 상차나 하차". 시계 방향이라 두 구간 모두 칸이 지게차 왼쪽. 이전 코드는 segment 3 에서 오른쪽 가지를 찾던 오류)
 *   - 오른쪽 가지 모양은 모서리로 본다 (사용자: "모서리는 오른쪽 가지 값으로")
 *   - 후진 중 본선 인식(reachedMainLine)에 이 규칙이 맞는지는 센서 위치를 몰라 확인하지 못했다.
 */

// ---------------- 라인 사건 (펄스) 보관 ----------------
const unsigned long LINE_SAMPLE_MS    = 5;     // ?? TODO ??: 라인 센서 샘플 주기 (loop 가 느리면 실제로는 더 김)
const unsigned long LINE_EVENT_TTL_MS = 300;   // ?? TODO ??: 알려진 사건을 아무도 가져가지 않으면 버리는 시간

LineDetector lineDet;
LineEvent lineEvent = EV_NONE;
unsigned long lineSampleMs = 0, lineEventMs = 0;

void lineDetectorPoll() {  // ◆ NEW: 샘플 주기마다 한 번만 읽고 판별한다. 감지 함수들이 부를 때마다 호출
  unsigned long now = millis();
  if (lineEvent != EV_NONE && now - lineEventMs > LINE_EVENT_TTL_MS) lineEvent = EV_NONE;
  if (now - lineSampleMs < LINE_SAMPLE_MS) return;
  lineSampleMs = now;
  LineSample s = { analogRead(PIN_LINE_LEFT), analogRead(PIN_LINE_CENTER), analogRead(PIN_LINE_RIGHT) };
  LineEvent ev = lineDetectorStep(lineDet, s);
  if (ev != EV_NONE) { lineEvent = ev; lineEventMs = now; }
}

bool takeLineEvent(LineEvent want) {  // ◆ NEW: 원하는 사건이면 한 번만 true 로 돌려주고 비움
  lineDetectorPoll();
  if (want == EV_NONE || lineEvent != want) return false;
  lineEvent = EV_NONE;
  return true;
}

bool senseJunction() {  // ◆ NEW
  LineEvent want = (segment == 1 || segment == 3) ? EV_JUNCTION_LEFT : EV_NONE;
  return takeLineEvent(want);
}

bool senseCorner() { return takeLineEvent(EV_CORNER); }  // ◆ NEW

bool senseMarker() {  // ◆ NEW
  lineDetectorPoll();
  return lineDet.markerOn;
}

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
