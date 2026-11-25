/*
 * center_logic.ino  -  센터 보드 판단 로직 (점유 기록, 폰 명령, 지게차 보고, 비상 LED)
 *
 * 이 파일은 입출력을 직접 하지 않고 center_hw.ino 의 함수만 부른다:
 *   sendToForklift(), queuePhone(), setLedIndex(), ledsOff()
 * 그래서 Arduino 없이 PC 에서 같은 코드를 컴파일해 시험할 수 있다.
 * 프로토콜 표는 PROBLEM_1PAGER.md 의 "통신 프로토콜 초안" 참고 (제안 단계).
 */

// ---------------- 작업 종료 ----------------
void finishJob() {  // ◆ NEW
  jobActive = false;
  emergency = false;
  picked = false;
  selLoad = -1;
  selUnload = -1;
  ledsOff();            // lastLedIdx 는 일부러 지우지 않음 (다음 비상의 기준)
}

// ---------------- 폰 -> 센터 ----------------
void selectLoad(char c) {  // ◆ NEW
  if (jobActive) { queuePhone(MSG_JOB_RUNNING, 0, 0); return; }
  int8_t idx = c - '1';
  if (!loadFull[idx]) { queuePhone(MSG_EMPTY, c, 0); return; }   // 짐 없는 상차 칸은 거부
  selLoad = idx;
  queuePhone(MSG_SEL_LOAD, c, 0);
}

void selectUnload(char c) {  // ◆ NEW
  if (jobActive) { queuePhone(MSG_JOB_RUNNING, 0, 0); return; }
  int8_t idx = c - 'A';
  if (unloadFull[idx]) { queuePhone(MSG_BUSY, c, 0); return; }   // 점유된 하차 칸은 거부
  selUnload = idx;
  queuePhone(MSG_SEL_UNLOAD, c, 0);
}

// 상차와 하차를 모두 고른 뒤에만 출발 (사용자 확인). 아니면 무시.
void startJob() {  // ◆ NEW
  if (jobActive || selLoad < 0 || selUnload < 0) return;
  jobActive = true;
  sendToForklift('1' + selLoad);    // 상차 글자 (숫자)
  sendToForklift('A' + selUnload);  // 하차 글자 (알파벳)
  queuePhone(MSG_JOB, '1' + selLoad, 'A' + selUnload);
}

void handlePhoneByte(char c) {  // ◆ NEW
  if (c == START_CHAR) startJob();
  else if (c >= '1' && c <= '6') selectLoad(c);
  else if (c >= 'A' && c <= 'F') selectUnload(c);
  // 그 밖의 글자(줄바꿈 등)는 무시
}

// ---------------- 지게차 -> 센터 ----------------
// 상태 글자 -> 폰 메시지 번호 (구간 진입 '1'~'4' 는 LED 만 쓰므로 여기 없음)
uint8_t stateMsgId(char c) {  // ◆ NEW
  switch (c) {
    case 'w': return MSG_ST_WAIT;
    case 'l': return MSG_ST_TO_LOAD;
    case 'a': return MSG_ST_APPROACH;
    case 'u': return MSG_ST_LIFT_UP;
    case 'b': return MSG_ST_REVERSE;
    case 'm': return MSG_ST_TO_UNLOAD;
    case 'd': return MSG_ST_UNLOAD;
    case 'r': return MSG_ST_RETURN;
    case 'c': return MSG_ST_DONE;
    case 'e': return MSG_EMERGENCY;
    case 'n': return MSG_LINE_LOST;
    default:  return MSG_NONE;
  }
}

// 구간 진입 보고: 진행 방향 앞쪽 모서리 LED 를 켠다 (사용자 확인). 비상 중에는 LED 를 바꾸지 않는다.
void onSegment(int8_t idx) {  // ◆ NEW
  if (emergency) return;
  lastLedIdx = idx;
  setLedIndex(idx);
}

// 비상: 'X' 를 보내고, "가장 최근에 켠 LED"의 시계 방향 다음 LED 를 계속 켠다 (사용자 확인).
// LED 를 한 번도 켠 적이 없으면 좌하(0)를 기준으로 삼음 (제안).
void onEmergency() {  // ◆ NEW
  if (!jobActive || emergency) return;
  emergency = true;
  sendToForklift('X');
  int8_t base = (lastLedIdx < 0) ? 0 : lastLedIdx;
  lastLedIdx = (base + 1) % 4;
  setLedIndex(lastLedIdx);
}

// 점유 기록 반영. 정상 완료: 상차 칸 비움 + 하차 칸 점유.
// 비상 복귀: 이미 짐을 실은 상태면 상차만 반영 (사용자 확인), 아니면 기록 그대로.
void settleOccupancy() {  // ◆ NEW
  if (!emergency) {
    loadFull[selLoad] = false;
    unloadFull[selUnload] = true;
  } else if (picked) {
    loadFull[selLoad] = false;
  }
}

// 정상 완료('c') 또는 비상 복귀 도착('w')
void onJobDone() {  // ◆ NEW
  if (!jobActive) return;
  settleOccupancy();
  finishJob();
}

void handleForkliftByte(char c) {  // ◆ NEW
  if (c >= '1' && c <= '4') { onSegment(c - '1'); return; }
  uint8_t id = stateMsgId(c);
  if (id == MSG_NONE) return;
  queuePhone(id, 0, 0);
  if (c == 'b' || c == 'm' || c == 'd') picked = true;   // 후진/하차 이동/하차 = 이미 짐을 실은 뒤
  if (c == 'e') onEmergency();
  else if (c == 'c') onJobDone();
  else if (c == 'w' && emergency) onJobDone();           // 비상 복귀 도착
}
