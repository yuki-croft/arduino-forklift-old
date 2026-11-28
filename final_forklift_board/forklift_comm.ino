/*
 * forklift_comm.ino  -  블루투스 명령 수신 / 상태 보고 (HC-06 = 하드웨어 시리얼 D0/D1, 1바이트 프로토콜)
 *
 * 수신: '1'~'6' 상차 칸, 'A'~'F' 하차 칸, 'X' 비상 선언
 * 송신: 상태 글자 w l a u b m d r c e n, 구간 진입 '1'~'4' (PROBLEM_1PAGER.md 표)
 * 상태가 바뀔 때만 1바이트씩 보낸다 (이전 NeoSWSerial 때의 습관 유지: 송신 최소화).
 */

void report(char code) {  // ◆ NEW
  Serial.write(code);
}

// 칸 글자는 대기 상태에서만 받는다 (작업 중 들어온 글자는 무시). 'X' 는 언제든 받는다.
void commHandle(char c) {  // ◆ NEW
  if (c == 'X') { emergencyDeclared = true; return; }
  if (state != S_WAIT) return;
  if (c >= '1' && c <= '6') loadCell = c - '1';
  else if (c >= 'A' && c <= 'F') unloadCell = c - 'A';
}

void commPoll() {  // ◆ NEW
  while (Serial.available() > 0) commHandle((char)Serial.read());
}
