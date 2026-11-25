"""test_cutplan.py - 재단 계획 시험. 실행: PYTHONPATH=. python3 test_cutplan.py"""
import os
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "map_final"))   # cutplan_core lives in map_final/
os.chdir(HERE)
import json
import os
import cutplan_core as cc

data = json.load(open(os.environ.get("CUTPLAN_JSON", "cutplan_result.json")))
cc.set_boards(data["extra_a"])
D = cc.Design(**data["design"])
PARTS = {p.pid: p for p in cc.make_parts(D)}
PLACED = data["placed"]
FREE = data["free"]
TOL = 1e-6


def test_every_part_placed_once_with_right_size():
    ids = sorted(p[0] for p in PLACED)
    assert ids == sorted(PARTS), "부품 누락/중복"
    for pid, si, x, y, w, h, rot in PLACED:
        p = PARTS[pid]
        want = (p.h, p.w) if rot else (p.w, p.h)
        assert abs(w - want[0]) < TOL and abs(h - want[1]) < TOL, pid


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
                assert max(dx, dy) >= cc.KERF - TOL, ("겹침 또는 톱날 간격 부족", name, a[0], b[0], dx, dy)


def test_free_pieces_do_not_overlap_parts_and_area_balance():
    for si, x, y, w, h in FREE:
        for pid, s, px, py, pw, ph, r in PLACED:
            if s != si:
                continue
            dx = max(x - (px + pw), px - (x + w))
            dy = max(y - (py + ph), py - (y + h))
            assert max(dx, dy) >= -TOL, ("잔여 조각이 부품과 겹침", pid)
    used = cc.parts_area(list(PARTS.values()))
    left = sum(w * h for _, _, _, w, h in FREE)
    assert used + left <= cc.TOTAL_AREA + TOL
    assert cc.TOTAL_AREA - used - left < 0.03 * cc.TOTAL_AREA, "톱날로 사라지는 면적이 3%를 넘음"


def test_floor_pieces_match_layout_and_seams():
    fl = cc.floor_layout(D)
    for pid, (x, y, w, h) in fl.items():
        p = PARTS[pid]
        assert (p.w, p.h) == (w, h)
        assert max(w, h) <= 90.0 and min(w, h) <= 60.0, "판 안에 안 들어가는 크기"
    line_ys = [D.yb, D.yt] + D.junction_ys
    for y in line_ys:
        assert abs(cc.SEAM_Y - y) >= cc.SEAM_MIN, ("이음선이 선에 너무 가까움", y)
    if D.fill_center:
        # 가운데 가로 이음선(y=CENTER_SEAM_Y)은 위/아래 가로선에서, 세로 이음선(x=띠 경계)은 모서리/시작 표식에서 떨어져야 함
        for y in (D.yb, D.yt):
            assert abs(cc.CENTER_SEAM_Y - y) >= cc.SEAM_MIN, ("가운데 이음선이 가로선에 가까움", y)
        cx = (D.xl + D.xr) / 2
        for sx in (D.band_w, D.floor_w - D.band_w):
            for fx in (D.xl, D.xr, cx - 4, cx + 4):    # 모서리 두 곳 + 시작 표식(폭 8) 양 끝
                assert abs(sx - fx) >= cc.SEAM_MIN, ("세로 이음선이 모서리/표식에 가까움", sx, fx)
        assert D.band_w != D.xl, "세로 이음선이 세로선 위에 있음"


def covered(pt):
    for x, y, w, h in cc.floor_layout(D).values():
        if x - 1e-9 <= pt[0] <= x + w + 1e-9 and y - 1e-9 <= pt[1] <= y + h + 1e-9:
            return True
    return False


def test_full_floor_is_gapless_tiling():
    if not D.fill_center:
        return
    fl = list(cc.floor_layout(D).values())
    assert abs(sum(w * h for _, _, w, h in fl) - D.floor_w * D.floor_h) < TOL, "바닥 조각 합이 직사각형 면적과 다름(빈틈/겹침)"
    for i in range(len(fl)):
        for j in range(i + 1, len(fl)):
            a, b = fl[i], fl[j]
            dx = max(a[0] - (b[0] + b[2]), b[0] - (a[0] + a[2]))
            dy = max(a[1] - (b[1] + b[3]), b[1] - (a[1] + a[3]))
            assert max(dx, dy) >= -TOL, "바닥 조각이 겹침"


def test_floor_covers_driving_area():
    pts = []
    n = 0.5
    def frange(a, b):
        k = int(round((b - a) / n))
        return [a + i * n for i in range(k + 1)]
    for xc in (D.xl, D.xr):                                   # 세로 직선 +-20
        pts += [(x, y) for x in frange(xc - 20, xc + 20) for y in frange(0, D.floor_h)]
    for yc in (D.yb, D.yt):                                   # 가로 직선 +-10
        pts += [(x, y) for x in frange(D.xl, D.xr) for y in frange(yc - cc.STRAIGHT_HALF, yc + cc.STRAIGHT_HALF)]
    for yj in D.junction_ys:                                  # 가지선 복도: 선에서 몸체 반길이(20)까지, 칸 앞(25)까지는 band_out 이 25일 때
        reach = min(cc.B, D.band_out)
        pts += [(x, y) for x in frange(D.xl - reach, D.xl) for y in frange(yj - 10, yj + 10)]
        pts += [(x, y) for x in frange(D.xr, D.xr + reach) for y in frange(yj - 10, yj + 10)]
    bad = [p for p in pts if not covered(p)]
    assert not bad, "바닥이 덮지 못한 점 %d개, 예: %s" % (len(bad), bad[:3])


def test_wall_not_on_floor_and_hole_clearance():
    wx0, wx1 = D.xl + cc.C, D.xl + cc.C + D.wall_w
    wy0, wy1 = D.yb + cc.C, D.yt - cc.C
    if D.fill_center:     # 벽이 바닥 위에 서므로 벽 자리가 전부 바닥 위여야 함
        for i in range(int(wx0 * 2), int(wx1 * 2) + 1):
            for j in range(int(wy0 * 2), int(wy1 * 2) + 1):
                assert covered((i / 2, j / 2)), "벽 자리 일부가 바닥 밖"
    else:                 # 벽이 탁자 위(구멍)에 서므로 바닥 조각과 겹치면 안 됨
        for pid, (x, y, w, h) in cc.floor_layout(D).items():
            dx = max(wx0 - (x + w), x - wx1)
            dy = max(wy0 - (y + h), y - wy1)
            assert max(dx, dy) >= 0, "벽 자리가 바닥 조각과 겹침 " + pid
    assert wy1 - wy0 >= 60 - TOL


def test_rule_checks():
    assert cc.C >= cc.FORK_L / 2 + 5 and cc.B >= cc.FORK_L / 2 + 5 and cc.E >= cc.FORK_L / 2 + 5
    assert cc.P >= D.rack_w + cc.FORK_W + 3 - TOL
    assert D.yb >= cc.CORNER_R and D.xl >= 0
    wall = [p for p in PARTS.values() if p.kind == "wall"]
    need = cc.WALL_H + (0.0 if D.fill_center else cc.BOARD_T)   # 탁자 위에 서면 바닥 판 두께만큼 더
    assert all(p.h >= need - TOL for p in wall), "벽 높이 30 미만"
    assert cc.parts_area(list(PARTS.values())) <= cc.TOTAL_AREA


if __name__ == "__main__":
    fails = 0
    tests = [(k, v) for k, v in sorted(globals().items()) if k.startswith("test_")]
    for name, fn in tests:
        try:
            fn()
            print("통과", name)
        except AssertionError as e:
            fails += 1
            print("실패", name, e)
    print("%d개 중 %d개 실패" % (len(tests), fails))
    raise SystemExit(1 if fails else 0)
