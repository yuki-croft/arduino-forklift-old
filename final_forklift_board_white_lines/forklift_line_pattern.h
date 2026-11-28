/*
 * (방식 C, final_forklift_board_white_lines: 이 파일은 방식 B 와 같다. 값은 lineRead() 로 흰/검정을 맞바꿔 들어오므로 아래 "검정" = 흰 선, "흰색" = 검은 바닥으로 읽는다)
 * forklift_line_pattern.h  (방식 B: 평행한 검은 선 두 줄)  -  라인 센서 3개 값으로 가지 / 모서리 / 표식을 가려내는 순수 계산
 *
 * 기준 (사용자 지정 2026-11-28): 30 이상 = 검정, 29 이하 = 흰색. 정상 주행 = 왼쪽 검정, 가운데 흰색, 오른쪽 검정.
 * !! 아래 규칙은 모두 제 가정이다 (두 줄 맵의 가지/모서리/표식 모양을 아직 모름). 맵을 만든 뒤 확인이 필요하다 !!
 *   - 표식   : 세 센서가 모두 검정 (두 선을 가로지르는 테이프) 이 LINE_CONFIRM_SAMPLES 번 연속 (레벨)
 *   - 왼쪽 가지 : 왼쪽 선이 끊김 (왼쪽 흰색, 가운데 흰색, 오른쪽 검정) 이 LINE_CONFIRM_SAMPLES 번 이상 이어졌다 끝남 (펄스)
 *   - 모서리 : 오른쪽 선이 끊김 (왼쪽 검정, 가운데 흰색, 오른쪽 흰색) 이 이어졌다 끝남 = "오른쪽 가지 값" (2026-11-28 사용자 지정) -> 오른쪽으로 꺾음
 *   - 선 사이 간격 약 19mm (2026-11-28 사용자) = 좌/우 센서 간격 20mm 와 거의 같음 -> 정상 주행에서 좌/우 센서는 각 선의 안쪽 가장자리에 걸림
 *   - 표식 근처 : 표식 직후 LINE_MARKER_GUARD 번은 가지/모서리 판정 안 함
 *   - 약하게 치우친 경우도 한쪽이 흰색이 되므로, 가지와 구분하는 것은 연속 횟수뿐이다 (방식 A 와 같은 한계).
 */
#ifndef FORKLIFT_LINE_PATTERN_H
#define FORKLIFT_LINE_PATTERN_H
#include <stdint.h>

// ---------------- 상수 ----------------
const int     LINE_ON_MIN          = 30;   // 사용자 지정: 검정 30~34
const int     LINE_ON_MAX          = 34;
const uint8_t LINE_CONFIRM_SAMPLES = 3;    // ?? TODO ??: 연속 횟수 (이음선 단차/잠깐 치우침 거르기)
const uint8_t LINE_MARKER_GUARD    = 6;    // ?? TODO ??: 표식 직후 무시 구간 (샘플 수)

// ---------------- 타입 ----------------
struct LineSample { int l, c, r; };        // A2(왼쪽), A3(가운데), A4(오른쪽) 원시값

enum LineEvent : uint8_t { EV_NONE = 0, EV_JUNCTION_LEFT, EV_JUNCTION_RIGHT, EV_CORNER };

struct LineDetector {
  uint8_t markerRun = 0;     // 세 센서가 모두 검정인 연속 횟수
  uint8_t guard = 0;         // 표식 직후 무시 구간 남은 샘플 수
  int8_t  side = 0;          // 끊긴 쪽: -1 왼쪽 선 끊김, +1 오른쪽 선 끊김, 0 없음
  uint8_t sideRun = 0;       // 그 상태의 연속 횟수
  bool    markerOn = false;  // 표식 레벨
};

// ---------------- 함수 ----------------
inline LineEvent lineDetectorStep(LineDetector &d, LineSample s) {
  const bool lB = s.l >= LINE_ON_MIN;
  const bool cB = s.c >= LINE_ON_MIN;   // 표식 판정에만 사용
  const bool rB = s.r >= LINE_ON_MIN;

  if (lB && cB && rB) {                             // 표식 후보
    if (d.markerRun < 255) d.markerRun++;
    d.markerOn = d.markerRun >= LINE_CONFIRM_SAMPLES;
    d.guard = LINE_MARKER_GUARD;
    d.side = 0;
    d.sideRun = 0;
    return EV_NONE;
  }
  d.markerRun = 0;
  d.markerOn = false;
  if (d.guard > 0) {
    d.guard--;
    d.side = 0;
    d.sideRun = 0;
    return EV_NONE;
  }

  const int8_t now = (!lB && !cB && rB) ? -1 : ((lB && !cB && !rB) ? 1 : 0);
  if (now != 0) {                                   // 한쪽 선이 끊긴 상태가 이어짐
    if (d.side == now) {
      if (d.sideRun < 255) d.sideRun++;
    } else {
      d.side = now;
      d.sideRun = 1;
    }
    return EV_NONE;
  }

  LineEvent ev = EV_NONE;                           // 끊긴 상태가 막 끝남
  if (d.side != 0 && d.sideRun >= LINE_CONFIRM_SAMPLES) {
    ev = (d.side < 0) ? EV_JUNCTION_LEFT : EV_CORNER;   // 2026-11-28 사용자: 모서리는 오른쪽 가지 값으로
  }
  d.side = 0;
  d.sideRun = 0;
  return ev;
}

#endif
