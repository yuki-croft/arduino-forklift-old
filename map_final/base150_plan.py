"""base150_plan.py - 사용자 베이스 (60x90 판 4장을 자르지 않고 풍차 모양으로 놓은 150x150, 가운데 30x30 구멍 유지)
위에 맵을 올리고, 벽/선반 뒷판/선반만 남는 판(추가 60x90 + 45x60 2장)에서 자르는 재단 계획.

사용자 답변: "구멍 유지", 벽은 베이스 위에 세움. 사진으로 확인한 것: 흰 판 4장이 풍차 모양, 가운데 작은 정사각형 구멍.
2026-11-26 사용자: 검은 주행 선 루프를 가로 100cm x 세로 100cm 정사각형으로 (선 중심선 기준, 제 해석).
  - 가로 100 = 선->벽 25 x2 + 벽 가로 50. 세로 100 = 2E + 2P 이므로 기존 E=25, P=30 (합 110)을 지킬 수 없다.
    제 선택(사용자 미확인): P=30 (칸 폭 15 + 지게차 12 + 여유 3) 은 유지하고 E (모서리->첫 교차점)를 25 -> 20 으로 줄임.
    E=20 은 모서리 회전 반경(20)과 같아 여유 5cm 가 사라지는 점을 사용자에게 알림.
  - 제 가정(미확인): 판 4장은 60x90, 구멍 30x30 (60x90 풍차의 계산값), 맵 루프를 베이스 한가운데에 둠.
  python3 base150_plan.py   -> base150_result.json (이 폴더에 씀)
"""
import json
from pathlib import Path
import cutplan_core as cc

BASE = 150.0
HOLE = (60.0, 60.0, 30.0, 30.0)         # x, y, w, h  (60x90 풍차의 가운데)
WALL_W = 50.0                            # 루프 가로 100 = 2*C(25) + 50
cc.E = 20.0                              # 세로 100 = 2*E + 2*P(30) -> E = 20 (기존 25, 줄임 - 사용자 확인 필요)
# 풍차 베이스 (원점 = 왼쪽 아래). 'cw' 배치. 거울 배치('ccw')는 x 를 좌우 반전.
PINWHEEL = {"A1": (0.0, 0.0, 60.0, 90.0), "A2": (0.0, 90.0, 90.0, 60.0),
            "A3": (90.0, 60.0, 60.0, 90.0), "A4": (60.0, 0.0, 90.0, 60.0)}


def design(back_h, rack_w=15.0):
    """맵 루프를 150x150 한가운데에 놓는다: band_out = (150 - loop_w) / 2."""
    loop_w = 2 * cc.C + WALL_W
    return cc.Design(band_out=(BASE - loop_w) / 2, back_h=back_h, wall_w=WALL_W, rack_w=rack_w, fill_center=True)


def abs_geom(d):
    """루프의 베이스 좌표 (원점 = 왼쪽 아래). core 의 yb 는 코너 반경 기준이라 베이스 한가운데로 y 를 옮긴다."""
    from types import SimpleNamespace
    y0 = (BASE - d.floor_h) / 2
    return SimpleNamespace(xl=d.xl, xr=d.xr, yb=d.yb + y0, yt=d.yt + y0, loop_w=d.loop_w, loop_h=d.loop_h,
                           wall_w=d.wall_w, rack_w=d.rack_w, back_h=d.back_h,
                           junction_ys=[y + y0 for y in d.junction_ys])


def make_parts(d):
    """바닥은 통판 4장이라 제외. 벽은 베이스 위에 서므로 높이 WALL_H (보정 없음)."""
    return [p for p in cc.make_parts(d) if p.kind != "floor"]


def boards(extra_a):
    return [("A%d" % (5 + i), 60.0, 90.0) for i in range(extra_a)] + [("B1", 45.0, 60.0), ("B2", 45.0, 60.0)]


if __name__ == "__main__":
    done = False
    for back_h in (16.0, 14.0, 12.0):
        d = design(back_h)
        parts = make_parts(d)
        need = cc.parts_area(parts)
        print("뒷판 높이 %g: 부품 %d개, 면적 %.0f cm2" % (back_h, len(parts), need))
        for extra in (0, 1, 2):
            cc.BOARDS = boards(extra)
            cap = sum(w * h for _, w, h in cc.BOARDS)
            if need > cap * 0.985:
                print("  추가 판 %d장 (합계 %.0f): 면적 부족" % (extra, cap))
                continue
            res = cc.search(parts, 20000, 1)
            print("  추가 판 %d장 (합계 %.0f): %s" % (extra, cap, "가능" if res else "못 넣음"))
            if res is None:
                continue
            res2 = cc.search(parts, 60000, 2)
            if res2 and res2[0] > res[0]:
                res = res2
            _, placed, free = res
            print("  잔여 조각 합계 %.0f cm2" % sum(w * h for *_, w, h in free))
            json.dump({"extra_a": extra, "boards": cc.BOARDS, "design": d.__dict__, "placed": placed, "free": free},
                      open(Path(__file__).resolve().parent / "base150_result.json", "w"), ensure_ascii=False, indent=1)
            done = True
            break
        if done:
            break
    if not done:
        raise SystemExit("추가 판 2장, 뒷판 12까지 줄여도 못 넣음")
