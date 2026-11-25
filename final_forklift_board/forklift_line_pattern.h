/*
 * forklift_line_pattern.h  -  라인 센서 3개(A2 A3 A4) 값으로 교차점 / 모서리 / 시작 표식을 가려내는 순수 계산 (Arduino 함수 호출 없음)
 *
 * !! 임시 구현 !!  사용자 실측(직선 중앙 / T자 교차점 / 모서리 / 시작 표식에서의 A2 A3 A4)이 아직 없다.
 *   확정된 것 (사용자 실측): 검은 선 위 31 이상, 흰 바닥 약 24~29.
 *   제 가정 (합성 시험값, 실측 아님): 선 한가운데 센서는 약 46, 센서 간격이 선 폭과 같아서 직선 중앙에서 좌/우 센서는
 *   선 가장자리 값(약 31~36), 가지선/표식 위에 완전히 올라가면 약 46. 그래서 "강하게 선 위"의 기준을 40 으로 잡았다.
 *   실측하면 아래 LINE_* 상수만 바꾸면 된다 (규칙은 그대로 둘 수 있는지는 실측 뒤 확인).
 *
 * 규칙 (모두 가설):
 *   - 표식   : 세 센서가 모두 "강하게 선 위"인 상태가 LINE_CONFIRM_SAMPLES 번 연속 (상태가 유지되는 동안 true: "레벨")
 *   - 교차점 : 한쪽 센서만 "강하게 선 위"인 구간(버스트)이 LINE_CONFIRM_SAMPLES 번 이상 이어졌다 끝났고,
 *              그 순간 가운데 센서는 아직 선 위 (앞으로 선이 계속됨)  -> 가지선.  왼쪽 버스트 = 왼쪽 가지, 오른쪽 = 오른쪽 가지
 *   - 모서리 : 오른쪽 버스트가 끝난 순간 가운데 센서가 선을 잃음 (앞으로 선이 없음). 모서리는 모두 오른쪽으로 꺾임 (시계 방향)
 *   - 표식 근처 : 표식을 지난 직후 LINE_MARKER_GUARD 번은 가지/모서리 판정을 하지 않음 (표식을 떠날 때 한쪽만 먼저 떨어지는 경우)
 *   - 교차점/모서리는 버스트가 "끝난 뒤" 한 번만 알려 준다 (펄스). 표식은 레벨.
 */
#ifndef FORKLIFT_LINE_PATTERN_H
#define FORKLIFT_LINE_PATTERN_H
#include <stdint.h>

// ---------------- 상수 ----------------
const int     LINE_ON_MIN          = 31;   // 사용자 실측: 검은 선 위 31 이상 (흰 바닥 24~29)
const int     LINE_STRONG_MIN      = 40;   // ?? TODO ??: 제 가정. 센서가 선에 "완전히" 올라간 값의 하한
const uint8_t LINE_CONFIRM_SAMPLES = 3;    // ?? TODO ??: 한 번 튄 값(이음선 단차 등)을 거르기 위한 연속 횟수
const uint8_t LINE_MARKER_GUARD    = 6;    // ?? TODO ??: 표식 직후 무시 구간 (샘플 수)

// ---------------- 타입 ----------------
struct LineSample { int l, c, r; };        // A2(왼쪽), A3(가운데), A4(오른쪽) 원시값

enum LineEvent : uint8_t { EV_NONE = 0, EV_JUNCTION_LEFT, EV_JUNCTION_RIGHT, EV_CORNER };

struct LineDetector {
  uint8_t markerRun = 0;     // 세 센서가 모두 강하게 선 위인 연속 횟수
  uint8_t guard = 0;         // 표식 직후 무시 구간 남은 샘플 수
  int8_t  side = 0;          // 진행 중인 버스트: -1 왼쪽, +1 오른쪽, 0 없음
  uint8_t sideRun = 0;       // 그 버스트의 연속 횟수
  bool    markerOn = false;  // 표식 레벨 (markerRun 이 기준 이상)
};

// ---------------- 함수 ----------------
// 샘플 한 개를 넣고, 이번 샘플에서 끝난 가지/모서리 사건을 돌려준다 (없으면 EV_NONE). 표식은 d.markerOn 으로 본다.
inline LineEvent lineDetectorStep(LineDetector &d, LineSample s) {
  const bool lStrong = s.l >= LINE_STRONG_MIN;
  const bool rStrong = s.r >= LINE_STRONG_MIN;
  const bool cStrong = s.c >= LINE_STRONG_MIN;
  const bool cOn     = s.c >= LINE_ON_MIN;

  if (lStrong && rStrong && cStrong) {              // 표식 후보
    if (d.markerRun < 255) d.markerRun++;
  } else {
    d.markerRun = 0;
  }
  d.markerOn = d.markerRun >= LINE_CONFIRM_SAMPLES;

  if (lStrong && rStrong) {                         // 양쪽 모두: 표식 자리. 가지/모서리 버스트가 아님
    d.guard = LINE_MARKER_GUARD;
    d.side = 0;
    d.sideRun = 0;
    return EV_NONE;
  }
  if (d.guard > 0) {                                // 표식을 막 지난 직후
    d.guard--;
    d.side = 0;
    d.sideRun = 0;
    return EV_NONE;
  }

  const int8_t now = rStrong ? 1 : (lStrong ? -1 : 0);
  if (now != 0) {                                   // 버스트 진행
    if (d.side == now) {
      if (d.sideRun < 255) d.sideRun++;
    } else {
      d.side = now;
      d.sideRun = 1;
    }
    return EV_NONE;
  }

  LineEvent ev = EV_NONE;                           // 어느 쪽도 강하지 않음: 버스트가 막 끝났는지 본다
  if (d.side != 0 && d.sideRun >= LINE_CONFIRM_SAMPLES) {
    if (d.side < 0) {
      if (cOn) ev = EV_JUNCTION_LEFT;               // 왼쪽 모서리는 없음: 가운데가 선을 잃었으면 무시
    } else {
      ev = cOn ? EV_JUNCTION_RIGHT : EV_CORNER;
    }
  }
  d.side = 0;
  d.sideRun = 0;
  return ev;
}

#endif
