/*
 * forklift_logic.ino  -  지게차 상태 머신 (PROBLEM_1PAGER.md "지게차 상태 머신 초안")
 *
 * 입출력은 forklift_hw / forklift_sense / forklift_comm 의 함수만 호출한다 (PC 시험용으로 가짜 함수를 끼울 수 있음).
 * 주행 중에는 긴 delay 를 쓰지 않는다 (회전 spinUntilLine 은 예외, 약 0.3~2.5초 동안 다른 처리가 멈춤).
 * 모든 칸의 가지선은 지게차의 "왼쪽"이다 (시계 방향: 왼쪽 구간은 북쪽으로, 오른쪽 구간은 남쪽으로 지나감).
 */

// ---------------- 칸 번호 -> 교차점 / 층 (확정된 번호 체계에서 계산) ----------------
uint8_t cellPosition(int8_t cell) { return (cell % 3) + 1; }          // 층 안에서 위에서 아래로 1~3
uint8_t loadJunctionCount(int8_t cell)   { return 4 - cellPosition(cell); }  // 상차: 올라가며 지나므로 아래가 1번째
uint8_t unloadJunctionCount(int8_t cell) { return cellPosition(cell); }      // 하차: 내려오며 지나므로 위가 1번째
bool cellIsUpper(int8_t cell) { return cell < 3; }                    // '1'~'3', 'A'~'C' = 2층
long shelfLiftMs(int8_t cell) { return cellIsUpper(cell) ? (long)LIFT_UPPER_MS : (long)LIFT_LOWER_MS; }   // 선반보다 살짝 위
long shelfDropMs(int8_t cell)  { return cellIsUpper(cell) ? (long)LIFT_UPPER_DROP_MS : (long)LIFT_LOWER_DROP_MS; }   // ◆ NEW: 선반보다 살짝 아래
uint8_t targetSegment() { return carrying ? 3 : 1; }                  // 짐을 싣고 있으면 오른쪽(하차), 아니면 왼쪽(상차)

// ---------------- 리프트 (시간으로 높이를 추정, 비차단) ----------------
void setLiftTarget(long ms) {  // ◆ NEW
  liftTargetMs = constrain(ms, 0L, (long)LIFT_FULL_STROKE_MS);
}

bool liftDone() { return liftDir == 0 && liftPosMs == liftTargetMs; }  // ◆ NEW

void liftService() {  // ◆ NEW: 매 loop 호출. 움직인 시간만큼 위치(ms)를 갱신하고 목표에서 멈춤
  unsigned long now = millis();
  if (liftDir != 0) {
    liftPosMs += liftDir * (long)(now - liftLastMs);
    liftLastMs = now;
    bool reached = (liftDir > 0) ? (liftPosMs >= liftTargetMs) : (liftPosMs <= liftTargetMs);
    if (reached) { liftPosMs = liftTargetMs; liftDir = 0; liftHalt(); }
  } else if (liftPosMs != liftTargetMs) {
    liftDir = (liftTargetMs > liftPosMs) ? 1 : -1;
    liftLastMs = now;
    liftRun(liftDir);
  }
}

// ---------------- 상태 전환 / 보고 ----------------
char stateReportCode(uint8_t s) {  // ◆ NEW: 0 = 보고 없음
  switch (s) {
    case S_WAIT:      return 'w';
    case S_TO_LOAD:   return 'l';
    case S_APPROACH:  return carrying ? 'd' : 'a';   // 짐을 싣고 접근하면 하차
    case S_LIFT_PICK: return 'u';
    case S_REVERSE:   return 'b';
    case S_TO_UNLOAD: return 'm';
    case S_RETURN:    return 'r';
    case S_EM_WAIT:   return 'e';
    default:          return 0;
  }
}

void enterState(uint8_t s) {  // ◆ NEW
  state = s;
  char code = stateReportCode(s);
  if (code != 0) report(code);
}

void beginJob() {  // ◆ NEW: 상차+하차 칸을 모두 받으면 (마스터가 시작 버튼 뒤에 보냄) 출발
  segment = 0;
  junctionCount = 0;
  targetCount = loadJunctionCount(loadCell);
  carrying = false;
  dropDone = false;
  markerArmed = false;          // 시작 표식 위에서 출발하므로, 표식을 벗어난 뒤에야 도착 판별을 켠다
  junctionLatched = false;
  lineLostReported = false;
  emergencyDeclared = false;
  setLiftTarget(shelfLiftMs(loadCell));
  report('1');                  // 아래 구간 진입 (센터가 좌하 LED 점등)
  enterState(S_TO_LOAD);
}

void finishJob(bool normal) {  // ◆ NEW: 정상 완료면 'c' 보고, 대기 진입 때 'w' 보고
  stopWheels();
  if (normal) report('c');
  loadCell = -1;
  unloadCell = -1;
  carrying = false;
  dropDone = false;
  emergencyDeclared = false;
  setLiftTarget(0);
  enterState(S_WAIT);
}

// ---------------- 주행 공용 ----------------
bool rearGuard() {  // ◆ NEW: 후방 비상 (거리 기준 이하가 일정 시간 지속). 걸리면 정지 후 마스터 지시 대기
  if (!rearObstaclePersistent()) return false;
  stopWheels();
  enterState(S_EM_WAIT);
  return true;
}

bool handleCorner() {  // ◆ NEW: 모서리는 모두 오른쪽으로 꺾음. 구간 번호를 올리고 보고
  if (!senseCorner()) return false;
  stopWheels();
  turnRight();
  segment = (segment + 1) % 4;
  junctionCount = 0;
  junctionLatched = false;
  report('1' + segment);
  return true;
}

void traceLine() {  // ◆ NEW: 직선 보정, 라인 이탈 시 광 추적 병용 ('n' 은 이탈 시작 때 한 번만 보고)
  if (!senseMarker()) markerArmed = true;
  if (senseLineLost()) {
    if (!lineLostReported) { report('n'); lineLostReported = true; }
    driveLightStep();
  } else {
    lineLostReported = false;
    driveLineStep();
  }
}

void countJunction() {  // ◆ NEW: 교차점은 "감지 시작" 순간만 센다. 목표 구간 안에서만 센다
  bool sensed = senseJunction();
  if (sensed && !junctionLatched && segment == targetSegment()) junctionCount++;
  junctionLatched = sensed;
}

// ---------------- 상태별 동작 ----------------
void runWait() {  // ◆ NEW
  if (loadCell >= 0 && unloadCell >= 0) beginJob();
}

void arriveAtCell() {  // ◆ NEW: 목표 교차점 도착 -> 왼쪽 가지선으로 회전, 포크를 선반 높이로
  stopWheels();
  if (JUNCTION_ADVANCE_MS > 0) {
    setWheels(LINE_BASE_SPEED, LINE_BASE_SPEED);
    delay(JUNCTION_ADVANCE_MS);
    stopWheels();
  }
  turnLeft();
  int8_t cell = carrying ? unloadCell : loadCell;
  setLiftTarget(shelfLiftMs(cell) + (carrying ? (long)LIFT_PICK_MS : 0L));   // 하차는 선반 위로 띄워서 진입
  junctionCount = 0;
  junctionLatched = false;
  enterState(S_APPROACH);
}

void runToCell() {  // ◆ NEW: S_TO_LOAD / S_TO_UNLOAD
  if (rearGuard() || handleCorner()) return;
  traceLine();
  countJunction();
  if (junctionCount >= targetCount) arriveAtCell();
}

void runApproach() {  // ◆ NEW: 포크가 높이에 도달한 뒤 가지선을 따라 들어가 전방 IR 이 짐/선반을 보면 정지
  if (rearGuard()) return;
  if (!liftDone()) { stopWheels(); return; }
  if (!senseCargoAhead()) { driveLineStep(); return; }
  stopWheels();
  if (carrying) {
    setLiftTarget(shelfDropMs(unloadCell));                       // 내려놓기: 선반 윗면보다 조금 아래까지 (짐이 선반에 얹힘)
    state = S_DROP;                                               // 'd' 는 접근 때 이미 보고함
  } else {
    setLiftTarget(shelfLiftMs(loadCell) + (long)LIFT_PICK_MS);    // 들어 올리기
    enterState(S_LIFT_PICK);
  }
}

void runLiftPick() {  // ◆ NEW
  if (!liftDone()) return;
  carrying = true;
  enterState(S_REVERSE);
}

void runDrop() {  // ◆ NEW
  if (!liftDone()) return;
  carrying = false;
  dropDone = true;
  enterState(S_REVERSE);
}

bool reachedMainLine() {  // ◆ NEW: 후진 중 본선을 가로지르는 순간 (2026-11-28 변경)
  // 가지선에서 똑바로 후진하면 세 센서가 본선을 동시에 지나므로 "세 센서 모두 검정"(senseMarker) 이 된다.
  // 이전의 senseJunction(한쪽만 검정) 으로는 이 순간을 잡지 못한다. 센서와 바퀴 위치 차이는 미확인 (?? TODO ??)
  bool sensed = senseMarker();
  bool edge = sensed && !junctionLatched;
  junctionLatched = sensed;
  return edge;
}

void runReverse() {  // ◆ NEW: 가지선에서 직진 후진 (라인 보정 없음) -> 본선에서 오른쪽 90도 회전해 진행 방향 복귀
  if (rearGuard()) return;
  setWheels(-LINE_BASE_SPEED, -LINE_BASE_SPEED);
  if (!reachedMainLine()) return;
  stopWheels();
  turnRight();
  junctionCount = 0;
  junctionLatched = false;
  markerArmed = false;   // 본선을 가로지른 "세 센서 검정" 을 시작 표식으로 착각하지 않게, 흰 바닥을 본 뒤에 다시 켠다 (2026-11-28)
  if (dropDone) {
    setLiftTarget(0);
    enterState(S_RETURN);
  } else {
    setLiftTarget(LIFT_CARRY_MS);
    targetCount = unloadJunctionCount(unloadCell);
    enterState(S_TO_UNLOAD);
  }
}

void runReturn() {  // ◆ NEW: 시작 표식에 도착하면 정상 완료
  if (rearGuard() || handleCorner()) return;
  traceLine();
  if (markerArmed && senseMarker()) finishJob(true);
}

void runEmWait() {  // ◆ NEW: 정지한 채 센터의 'X'(비상 선언) 대기
  stopWheels();
  if (emergencyDeclared) enterState(S_EM_GUIDE);
}

void runEmGuide() {  // ◆ NEW: 라인트레이싱 중지, 켜진 LED 를 광 추적으로 따라가 시작 표식에서 정지
  if (!senseMarker()) markerArmed = true;
  driveLightStep();
  if (markerArmed && senseMarker()) finishJob(false);
}

void runState() {  // ◆ NEW
  switch (state) {
    case S_WAIT:      runWait();      break;
    case S_TO_LOAD:
    case S_TO_UNLOAD: runToCell();    break;
    case S_APPROACH:  runApproach();  break;
    case S_LIFT_PICK: runLiftPick();  break;
    case S_DROP:      runDrop();      break;
    case S_REVERSE:   runReverse();   break;
    case S_RETURN:    runReturn();    break;
    case S_EM_WAIT:   runEmWait();    break;
    case S_EM_GUIDE:  runEmGuide();   break;
    default:          break;
  }
}
