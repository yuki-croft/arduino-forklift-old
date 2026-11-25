"""cutplan_core.py - 60x90 판 4장 + 45x60 판 2장으로 맵(바닥/벽/칸 선반)을 만드는 재단 계획.

설계 규칙(C, B, E, P)은 map/layout.py 의 값과 같다 (단위 cm).
  - 사용자 확정: 지게차 40x12, 벽 두께 1cm / 높이 30cm 이상, 선 폭 19mm, 판 6장 = 27,000 cm2
  - 제 가정(사용자 미확인): 판 두께 1cm, 판은 결(방향) 없음(90도 회전 가능), 톱날 0.3cm,
    선반 뒷판 + 캔틸레버 구조, 칸 폭/깊이 15x20 (임시), 직선 구간 바닥 폭 20cm
"""
import random
from dataclasses import dataclass

EPS = 1e-9
KERF = 0.3
def make_boards(extra_a=0):
    """기본 판: 60x90 4장 + 45x60 2장. extra_a = 추가로 구한 60x90 판 수 (사용자 답변: 2장까지 가능)."""
    a = [("A%d" % (i + 1), 60.0, 90.0) for i in range(4 + extra_a)]
    return a + [("B1", 45.0, 60.0), ("B2", 45.0, 60.0)]


BOARDS = make_boards(0)
TOTAL_AREA = sum(w * h for _, w, h in BOARDS)          # 기본 27000


def set_boards(extra_a):
    """사용할 판 목록을 바꾼다 (BOARDS, TOTAL_AREA 모듈 전역을 갱신)."""
    global BOARDS, TOTAL_AREA
    BOARDS = make_boards(extra_a)
    TOTAL_AREA = sum(w * h for _, w, h in BOARDS)

FORK_L, FORK_W = 40.0, 12.0     # 지게차 길이 x 폭 (사용자 확정)
C, B, E, P = 25.0, 25.0, 25.0, 30.0  # 선->벽, 선->칸, 모서리->첫 교차점, 교차점 간격
CORNER_R = FORK_L / 2           # 모서리 제자리 회전 반경
STRAIGHT_HALF = 10.0            # 직선 구간 바닥 반폭 (몸체 폭 12 + 여유)
BOARD_T = 1.0                   # 판 두께 (가정)
WALL_H = 30.0                   # 벽 높이 (사용자: 30 이상)
RACK_D = 20.0                   # 칸 깊이 (임시)
SEAM_Y = 90.0                   # 좌/우 바닥 띠 이음선 높이 (가지선 사이 한가운데)
CENTER_SEAM_Y = 60.0             # 가운데 띠 이음선 높이 (fill_center 일 때)
SEAM_MIN = 8.0                  # 이음선과 선(교차점/모서리)의 최소 거리
LINE_W = 1.9                    # 선 폭 (사용자: 19mm)


@dataclass(frozen=True)
class Design:
    band_out: float   # 선에서 칸 쪽 바깥으로 바닥이 나가는 길이 (25 = 칸 앞까지, 20 = 몸체 반길이까지)
    back_h: float     # 선반 뒷판 높이
    wall_w: float     # 벽 가로(짧은 변) 길이
    rack_w: float     # 칸(선반) 폭
    fill_center: bool = False   # True = 가운데 구멍까지 판을 깔아 바닥을 한 장으로 이음 (벽도 바닥 위에 섬)

    @property
    def loop_w(self): return 2 * C + self.wall_w
    @property
    def loop_h(self): return 2 * E + 2 * P
    @property
    def xl(self): return self.band_out
    @property
    def xr(self): return self.xl + self.loop_w
    @property
    def yb(self): return CORNER_R
    @property
    def yt(self): return self.yb + self.loop_h
    @property
    def floor_w(self): return self.xr + self.band_out
    @property
    def floor_h(self): return self.yt + CORNER_R
    @property
    def band_w(self): return self.band_out + CORNER_R
    @property
    def junction_ys(self): return [self.yb + E + i * P for i in range(3)]


@dataclass(frozen=True)
class Part:
    pid: str
    name: str
    w: float
    h: float
    kind: str   # floor / wall / back / shelf


def floor_layout(d):
    """바닥 조각의 조립 위치 {pid: (x, y, w, h)}. 원점은 바닥 왼쪽 아래."""
    bw, fw, fh = d.band_w, d.floor_w, d.floor_h
    bridge_w = d.loop_w - 2 * CORNER_R
    bx = d.xl + CORNER_R
    out = {
        "F1": (0.0, 0.0, bw, SEAM_Y),
        "F2": (0.0, SEAM_Y, bw, fh - SEAM_Y),
        "F3": (fw - bw, 0.0, bw, SEAM_Y),
        "F4": (fw - bw, SEAM_Y, bw, fh - SEAM_Y),
    }
    if d.fill_center:       # 가운데 띠(폭 bridge_w, 높이 전체)를 이음선 CENTER_SEAM_Y 에서 둘로 (좌우 띠 이음선과 어긋나게)
        out["F5"] = (bx, 0.0, bridge_w, CENTER_SEAM_Y)
        out["F6"] = (bx, CENTER_SEAM_Y, bridge_w, fh - CENTER_SEAM_Y)
    else:
        out["F5"] = (bx, d.yb - STRAIGHT_HALF, bridge_w, 2 * STRAIGHT_HALF)
        out["F6"] = (bx, d.yt - STRAIGHT_HALF, bridge_w, 2 * STRAIGHT_HALF)
    return out


FLOOR_NAMES = {"F1": "바닥 왼쪽 띠 아래", "F2": "바닥 왼쪽 띠 위", "F3": "바닥 오른쪽 띠 아래",
               "F4": "바닥 오른쪽 띠 위", "F5": "바닥 아래 다리", "F6": "바닥 위 다리"}
FLOOR_NAMES_FULL = dict(FLOOR_NAMES, F5="바닥 가운데 아래", F6="바닥 가운데 위")


def make_parts(d):
    names = FLOOR_NAMES_FULL if d.fill_center else FLOOR_NAMES
    parts = [Part(pid, names[pid], w, h, "floor") for pid, (_, _, w, h) in floor_layout(d).items()]
    wh = WALL_H + (0.0 if d.fill_center else BOARD_T)   # 벽이 탁자 위에 서면 바닥 판 두께만큼 더 높여야 함
    long_len = d.loop_h - 2 * C
    short_len = d.wall_w - 2 * BOARD_T
    parts += [Part("W1", "벽 긴 면", long_len, wh, "wall"), Part("W2", "벽 긴 면", long_len, wh, "wall"),
              Part("W3", "벽 짧은 면", short_len, wh, "wall"), Part("W4", "벽 짧은 면", short_len, wh, "wall")]
    back_len = 2 * P + d.rack_w
    parts += [Part("R1", "선반 뒷판(상차)", back_len, d.back_h, "back"),
              Part("R2", "선반 뒷판(하차)", back_len, d.back_h, "back")]
    parts += [Part("S%d" % (i + 1), "선반", d.rack_w, RACK_D, "shelf") for i in range(12)]
    return parts


def parts_area(parts):
    return sum(p.w * p.h for p in parts)


# ---------------------------------------------------------------- 직선(기요틴) 재단 배치 탐색
def pack_once(parts, rng, kerf=KERF):
    """무작위 순서 + 기요틴 분할. 부품은 (w+kerf, h+kerf) 로 부풀려 놓아 톱날 두께를 반영한다."""
    sheets = [[[0.0, 0.0, W + kerf, H + kerf]] for _, W, H in BOARDS]
    placed = []
    noise = rng.choice((0.0, 0.15, 0.5))
    mode = rng.choice(("baf", "bssf"))                 # 가장 작은 자리 / 짧은 변 맞춤
    order = sorted(parts, key=lambda p: -max(p.w, p.h) * rng.uniform(1 - noise, 1 + noise) - 0.001 * p.w * p.h)
    for p in order:
        best = None
        for si, free in enumerate(sheets):
            for fi, f in enumerate(free):
                for rot in (0, 1):
                    pw, ph = (p.h, p.w) if rot else (p.w, p.h)
                    pw += kerf
                    ph += kerf
                    if pw <= f[2] + EPS and ph <= f[3] + EPS:
                        if mode == "baf":
                            sc = f[2] * f[3] - pw * ph
                        else:
                            sc = min(f[2] - pw, f[3] - ph) * 100 + (f[2] * f[3] - pw * ph) * 0.01
                        sc *= 1 + noise * rng.random()
                        if best is None or sc < best[0]:
                            best = (sc, si, fi, rot, pw, ph)
        if best is None:
            return None
        _, si, fi, rot, pw, ph = best
        f = sheets[si].pop(fi)
        placed.append((p.pid, si, f[0], f[1], pw - kerf, ph - kerf, rot))
        if (f[2] - pw) < (f[3] - ph):   # 남는 변이 짧은 쪽으로 먼저 자른다 (SAS), 가끔 반대로
            vertical = rng.random() < 0.15
        else:
            vertical = rng.random() < 0.85
        if vertical:
            new = [[f[0] + pw, f[1], f[2] - pw, f[3]], [f[0], f[1] + ph, pw, f[3] - ph]]
        else:
            new = [[f[0] + pw, f[1], f[2] - pw, ph], [f[0], f[1] + ph, f[2], f[3] - ph]]
        sheets[si] += [r for r in new if r[2] > kerf + EPS and r[3] > kerf + EPS]
    free_true = [(si, r[0], r[1], r[2] - kerf, r[3] - kerf) for si, fr in enumerate(sheets) for r in fr]
    return placed, free_true


def search(parts, iters, seed):
    """iters 번 시도해 가장 '잔여가 한 덩어리로 모이는' 배치를 고른다. 없으면 None."""
    rng = random.Random(seed)
    best = None
    for _ in range(iters):
        res = pack_once(parts, rng)
        if res is None:
            continue
        score = sum((w * h) ** 2 for _, _, _, w, h in res[1])
        if best is None or score > best[0]:
            best = (score, res[0], res[1])
    return best


def candidates():
    out = [Design(bo, bh, ww, rw) for bo in (25.0, 20.0) for bh in (18.0, 16.0, 14.0, 12.0)
           for ww in (30.0, 26.0, 22.0) for rw in (15.0, 14.5)]
    out.sort(key=lambda d: -parts_area(make_parts(d)))   # 재료를 많이 쓰는(여유로운) 안부터
    return out


def choose(quick=300, deep=3000, seed=1, log=None):
    """판 6장에 모두 들어가는 첫(가장 여유로운) 설계를 찾는다."""
    tried = 0
    for d in candidates():
        parts = make_parts(d)
        if parts_area(parts) > TOTAL_AREA * 0.985:
            continue
        tried += 1
        res = search(parts, quick, seed)
        if log:
            log("시도 %2d: %s 면적 %.0f -> %s" % (tried, d, parts_area(parts), "가능" if res else "불가"))
        if res:
            res2 = search(parts, deep, seed + 1)
            if res2 and res2[0] > res[0]:
                res = res2
            return d, parts, res[1], res[2], tried
    return None
