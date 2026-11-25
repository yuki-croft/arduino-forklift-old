"""test_base150.py - 150x150 풍차 베이스 재단/배치 시험 (map/map_final/ 의 계획을 검사). 실행: bash tests/test_pc/map_final/run_test_map_final.sh"""
import json
import os
import sys

MAP_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "map", "map_final")
sys.path.insert(0, MAP_DIR)
import cutplan_core as cc  # noqa: E402
import base150_plan as bp  # noqa: E402

data = json.load(open(os.path.join(MAP_DIR, "base150_result.json")))
cc.BOARDS = [tuple(b) for b in data["boards"]]
D = cc.Design(**data["design"])
G = bp.abs_geom(D)   # 베이스 좌표의 선 루프 (y 를 베이스 한가운데로 옮김)
PARTS = {p.pid: p for p in bp.make_parts(D)}
PLACED, FREE = data["placed"], data["free"]
TOL = 1e-6


def rect_overlap(a, b):
    return min(a[0] + a[2], b[0] + b[2]) - max(a[0], b[0]) > 1e-9 and min(a[1] + a[3], b[1] + b[3]) - max(a[1], b[1]) > 1e-9


def mirror(r):
    return (bp.BASE - r[0] - r[2], r[1], r[2], r[3])


def test_every_part_placed_once_with_right_size():
    assert sorted(p[0] for p in PLACED) == sorted(PARTS)
    for pid, si, x, y, w, h, rot in PLACED:
        p = PARTS[pid]
        want = (p.h, p.w) if rot else (p.w, p.h)
        assert abs(w - want[0]) < TOL and abs(h - want[1]) < TOL, pid
    assert not any(p.kind == "floor" for p in PARTS.values()), "바닥은 통판 4장이라 부품에 없어야 함"


def test_inside_board_and_kerf_gap():
    for si, (name, W, H) in enumerate(cc.BOARDS):
        items = [(pid, x, y, w, h) for pid, s, x, y, w, h, r in PLACED if s == si]
        for pid, x, y, w, h in items:
            assert x >= -TOL and y >= -TOL and x + w <= W + TOL and y + h <= H + TOL, (name, pid)
        for i in range(len(items)):
            for j in range(i + 1, len(items)):
                a, b = items[i], items[j]
                dx = max(a[1] - (b[1] + b[3]), b[1] - (a[1] + a[3]))
                dy = max(a[2] - (b[2] + b[4]), b[2] - (a[2] + a[4]))
                assert max(dx, dy) >= cc.KERF - TOL, ("겹침 또는 톱날 간격 부족", name, a[0], b[0])


def test_free_pieces_and_area_balance():
    for si, x, y, w, h in FREE:
        for pid, s, px, py, pw, ph, r in PLACED:
            if s == si:
                assert not rect_overlap((x, y, w, h), (px, py, pw, ph)), ("잔여 조각이 부품과 겹침", pid)
    cap = sum(w * h for _, w, h in cc.BOARDS)
    used = cc.parts_area(list(PARTS.values()))
    left = sum(w * h for _, _, _, w, h in FREE)
    assert used + left <= cap + TOL
    assert cap - used - left < 0.03 * cap, "톱날로 사라지는 면적이 3%를 넘음"


def test_extra_board_count_is_minimum():
    need = cc.parts_area(list(PARTS.values()))
    one = 5400 + 5400  # 추가 1장 + 45x60 두 장
    assert need > one * 0.985, "추가 1장으로 면적상 가능하면 2장은 낭비"
    assert data["extra_a"] == 2


def test_pinwheel_base_is_150_with_30_hole():
    boards = list(bp.PINWHEEL.values())
    for b in boards:
        assert sorted(b[2:]) == [60.0, 90.0]
    for i in range(4):
        for j in range(i + 1, 4):
            assert not rect_overlap(boards[i], boards[j])
    assert min(b[0] for b in boards) == 0 and max(b[0] + b[2] for b in boards) == 150
    assert min(b[1] for b in boards) == 0 and max(b[1] + b[3] for b in boards) == 150
    assert abs(sum(b[2] * b[3] for b in boards) - (150 * 150 - 30 * 30)) < TOL
    assert not any(rect_overlap(bp.HOLE, b) for b in boards), "구멍이 판과 겹침"


def test_loop_is_100_square_centered_and_corner_circles_on_base():
    assert abs(G.loop_w - 100) < TOL and abs(G.loop_h - 100) < TOL, "사용자 요청: 선 루프 100x100"
    assert abs((G.xr - G.xl) - 100) < TOL and abs((G.yt - G.yb) - 100) < TOL
    assert abs(G.xl - (150 - G.xr)) < TOL and abs(G.yb - (150 - G.yt)) < TOL, "베이스 한가운데"
    assert G.xl - cc.CORNER_R >= 0 and G.xr + cc.CORNER_R <= 150, "모서리 회전 원이 베이스 밖"
    assert G.yb - cc.CORNER_R >= 0 and G.yt + cc.CORNER_R <= 150, "모서리 회전 원이 베이스 밖"
    assert G.junction_ys == [45.0, 75.0, 105.0]


def strips(d):
    """벽 4면의 위에서 본 두께 1cm 자리 (x, y, w, h)."""
    wx, wy, ww, wl = d.xl + cc.C, d.yb + cc.C, d.wall_w, d.loop_h - 2 * cc.C
    t = cc.BOARD_T
    return [(wx, wy, t, wl), (wx + ww - t, wy, t, wl), (wx + t, wy, ww - 2 * t, t), (wx + t, wy + wl - t, ww - 2 * t, t)]


def on_base(x, y):
    for b in bp.PINWHEEL.values():
        if b[0] - 1e-9 <= x <= b[0] + b[2] + 1e-9 and b[1] - 1e-9 <= y <= b[1] + b[3] + 1e-9:
            return not (bp.HOLE[0] + 1e-9 < x < bp.HOLE[0] + bp.HOLE[2] - 1e-9 and bp.HOLE[1] + 1e-9 < y < bp.HOLE[1] + bp.HOLE[3] - 1e-9)
    return False


def test_walls_stand_on_boards_not_over_hole():
    for s in strips(G):
        assert not rect_overlap(s, bp.HOLE), ("벽이 구멍 위에 걸침", s)
        xs = [s[0] + s[2] * i / 10 for i in range(11)]
        ys = [s[1] + s[3] * i / 10 for i in range(11)]
        for x in xs:
            for y in ys:
                assert on_base(x, y), ("벽 자리 일부가 판 밖", s, x, y)
    # 구멍은 벽 안쪽 공간에 완전히 들어감 (벽 바깥 치수 안)
    wx, wy, ww, wl = G.xl + cc.C, G.yb + cc.C, D.wall_w, D.loop_h - 2 * cc.C
    assert wx < bp.HOLE[0] and wx + ww > bp.HOLE[0] + bp.HOLE[2] and wy < bp.HOLE[1] and wy + wl > bp.HOLE[1] + bp.HOLE[3]


def line_segments(d):
    segs = [((d.xl, d.yb), (d.xl, d.yt)), ((d.xr, d.yb), (d.xr, d.yt)), ((d.xl, d.yb), (d.xr, d.yb)), ((d.xl, d.yt), (d.xr, d.yt))]
    for yj in d.junction_ys:
        segs += [((d.xl - cc.B, yj), (d.xl, yj)), ((d.xr, yj), (d.xr + cc.B, yj))]
    return segs


def test_lines_do_not_touch_hole_and_seams_keep_away_from_key_points():
    key = [(G.xl, G.yb), (G.xr, G.yb), (G.xl, G.yt), (G.xr, G.yt)]
    key += [(x, yj) for yj in G.junction_ys for x in (G.xl, G.xr)]
    cx = (G.xl + G.xr) / 2
    key += [(cx - 4, G.yb), (cx + 4, G.yb)]            # 시작 표식 (폭 8)
    segs = line_segments(G)
    for (a, b) in segs:                                 # 선은 구멍을 지나지 않음
        r = (min(a[0], b[0]), min(a[1], b[1]), abs(a[0] - b[0]), abs(a[1] - b[1]))
        assert not rect_overlap((r[0] - 1, r[1] - 1, r[2] + 2, r[3] + 2), bp.HOLE), ("선이 구멍에 닿음", a, b)
    seams_cw = [((0, 90), (60, 90)), ((90, 90), (90, 150)), ((90, 60), (150, 60)), ((60, 0), (60, 60))]
    for mir in (False, True):                           # 풍차 두 방향 모두
        seams = [tuple((bp.BASE - x if mir else x, y) for x, y in s) for s in seams_cw]
        for (p1, p2) in seams:
            vert = abs(p1[0] - p2[0]) < 1e-9
            for (a, b) in segs:
                svert = abs(a[0] - b[0]) < 1e-9
                if vert != svert:                        # 수직으로 만남: 교차점 계산
                    v, h = ((p1, p2), (a, b)) if vert else ((a, b), (p1, p2))
                    x, y = v[0][0], h[0][1]
                    if min(h[0][0], h[1][0]) - 1e-9 <= x <= max(h[0][0], h[1][0]) + 1e-9 and min(v[0][1], v[1][1]) - 1e-9 <= y <= max(v[0][1], v[1][1]) + 1e-9:
                        for k in key:
                            assert ((x - k[0]) ** 2 + (y - k[1]) ** 2) ** 0.5 >= cc.SEAM_MIN, ("이음선 교차점이 교차점/모서리에 가까움", mir, (x, y), k)
                else:                                    # 평행: 범위가 겹치면 간격 확인
                    if vert:
                        lo, hi = max(min(p1[1], p2[1]), min(a[1], b[1])), min(max(p1[1], p2[1]), max(a[1], b[1]))
                        off = abs(p1[0] - a[0])
                    else:
                        lo, hi = max(min(p1[0], p2[0]), min(a[0], b[0])), min(max(p1[0], p2[0]), max(a[0], b[0]))
                        off = abs(p1[1] - a[1])
                    if hi > lo:
                        assert off >= cc.SEAM_MIN, ("이음선이 선과 나란히 가까움", mir, p1, a)


def test_rule_checks_and_documented_relaxation():
    assert cc.C >= cc.FORK_L / 2 + 5 and cc.B >= cc.FORK_L / 2 + 5
    assert cc.P >= D.rack_w + 15
    # 세로 100 = 2E + 2P: P=30 을 지키면 E=20. 기존 규칙 E >= 반길이+5 = 25 를 일부러 어김 (사용자에게 알림)
    assert cc.E == 20.0 and 2 * cc.E + 2 * cc.P == 100
    assert cc.E < cc.FORK_L / 2 + 5 and cc.E >= cc.FORK_L / 2, "E 는 지게차 반길이(20) 이상은 유지"
    assert G.xl - cc.B >= 0, "칸 앞면이 베이스 안쪽이어야 칸이 베이스 바깥에 선다"
    assert abs(G.xl - cc.B) < TOL, "이 도안: 칸 앞면 = 베이스 가장자리"
    assert max(p.h for p in PARTS.values() if p.kind == "wall") == cc.WALL_H, "벽은 베이스 위라 높이 보정 없음"
    assert D.wall_w == 2 * (G.xr - G.xl) / 2 - 2 * cc.C, "벽 가로 = 루프 가로 - 2*C"


if __name__ == "__main__":
    n = 0
    for k, v in sorted(globals().items()):
        if k.startswith("test_") and callable(v):
            v()
            n += 1
            print("PASS", k)
    print("%d tests passed" % n)
