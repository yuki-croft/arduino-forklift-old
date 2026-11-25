#!/usr/bin/env python3
"""Draws the forklift map: track layout (map_layout.png) and board layout (board_layout.png).
Usage: python3 make_map.py [output_dir]      Geometry and proposals live in layout.py."""
import os
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, FancyArrowPatch, Rectangle

import layout as L

plt.rcParams["font.family"] = ["Noto Sans CJK KR", "DejaVu Sans"]
BOARD_COLOR, LINE_COLOR, WALL_COLOR = "#efe9dc", "#111111", "#8a3b2a"
LOAD_COLOR, UNLOAD_COLOR, ARROW_COLOR = "#cfe3f5", "#f9dcc0", "#1f6fb5"


# ---------------- small drawing helpers ----------------
def rect(ax, x, y, w, h, **kw):
    ax.add_patch(Rectangle((x, y), w, h, **kw))


def hline(ax, x0, x1, y, width=L.LINE_W, **kw):
    rect(ax, x0, y - width / 2, x1 - x0, width, **kw)


def vline(ax, x, y0, y1, width=L.LINE_W, **kw):
    rect(ax, x - width / 2, y0, width, y1 - y0, **kw)


def dim_h(ax, x0, x1, y, text, size=7):
    ax.annotate("", (x0, y), (x1, y), arrowprops=dict(arrowstyle="<->", lw=0.8, color="#444"))
    ax.text((x0 + x1) / 2, y + 1.2, text, ha="center", va="bottom", fontsize=size, color="#222")


def dim_v(ax, x, y0, y1, text, size=7):
    ax.annotate("", (x, y0), (x, y1), arrowprops=dict(arrowstyle="<->", lw=0.8, color="#444"))
    ax.text(x - 1.2, (y0 + y1) / 2, text, ha="right", va="center", fontsize=size, color="#222", rotation=90)


def arrow(ax, p0, p1):
    ax.add_patch(FancyArrowPatch(p0, p1, arrowstyle="-|>", mutation_scale=16, color=ARROW_COLOR, lw=2.2))


# ---------------- track map ----------------
def draw_track(ax, lay):
    kw = dict(facecolor=LINE_COLOR, edgecolor="none", zorder=3)
    hline(ax, lay.xl - L.LINE_W / 2, lay.xr + L.LINE_W / 2, lay.yb, **kw)
    hline(ax, lay.xl - L.LINE_W / 2, lay.xr + L.LINE_W / 2, lay.yt, **kw)
    vline(ax, lay.xl, lay.yb, lay.yt, **kw)
    vline(ax, lay.xr, lay.yb, lay.yt, **kw)
    for y in lay.left_junction_ys():
        hline(ax, lay.xl - L.B, lay.xl, y, **kw)
    for y in lay.right_junction_ys():
        hline(ax, lay.xr, lay.xr + L.B, y, **kw)


def draw_wall(ax, lay):
    x, y, w, h = lay.wall()
    rect(ax, x, y, w, h, facecolor="#f3ddd6", edgecolor="none", hatch="///", zorder=2)
    for (rx, ry, rw, rh) in [(x, y, w, L.WALL_T), (x, y + h - L.WALL_T, w, L.WALL_T),
                             (x, y, L.WALL_T, h), (x + w - L.WALL_T, y, L.WALL_T, h)]:
        rect(ax, rx, ry, rw, rh, facecolor=WALL_COLOR, edgecolor="none", zorder=4)
    ax.text(x + w / 2, y + h / 2, "WALL\n두께 1cm\n높이 30cm 이상\n(%.0f x %.0f)" % (w, h),
            ha="center", va="center", fontsize=8, color=WALL_COLOR, zorder=5,
            bbox=dict(facecolor="white", edgecolor="none", alpha=0.85, pad=2))


def draw_box(ax, x, y, labels, color):
    rect(ax, x, y, L.BOX_D, L.BOX_W, facecolor=color, edgecolor="#333", lw=1.2, zorder=4)
    ax.text(x + L.BOX_D / 2, y + L.BOX_W / 2 + 1.5, "%s | %s" % labels, ha="center", va="center",
            fontsize=11, fontweight="bold", zorder=5)
    ax.text(x + L.BOX_D / 2, y + L.BOX_W / 2 - 3.2, "2F | 1F", ha="center", va="center", fontsize=6, zorder=5)


def draw_boxes(ax, lay):
    left = [("3", "6"), ("2", "5"), ("1", "4")]     # travel order bottom -> top (position 3, 2, 1)
    right = [("A", "D"), ("B", "E"), ("C", "F")]    # travel order top -> bottom (position 1, 2, 3)
    for y, labels in zip(lay.left_junction_ys(), left):
        draw_box(ax, lay.xl - L.B - L.BOX_D, y - L.BOX_W / 2, labels, LOAD_COLOR)
    for y, labels in zip(lay.right_junction_ys(), right):
        draw_box(ax, lay.xr + L.B, y - L.BOX_W / 2, labels, UNLOAD_COLOR)
    for n, y in enumerate(lay.left_junction_ys(), 1):
        ax.text(lay.xl + 2.2, y + 1.5, "교차점 %d" % n, fontsize=6, color="#333", zorder=5)
    for n, y in enumerate(lay.right_junction_ys(), 1):
        ax.text(lay.xr - 2.2, y + 1.5, "교차점 %d" % n, fontsize=6, color="#333", ha="right", zorder=5)


def draw_start(ax, lay):
    sx, sy = lay.start
    rect(ax, sx - L.MARKER_ALONG / 2, sy - L.MARKER_ACROSS / 2, L.MARKER_ALONG, L.MARKER_ACROSS,
         facecolor=LINE_COLOR, edgecolor="none", zorder=3)
    rect(ax, sx - L.FORKLIFT_L / 2, sy - L.FORKLIFT_W / 2, L.FORKLIFT_L, L.FORKLIFT_W,
         facecolor="none", edgecolor="#d98200", lw=1.6, ls="--", zorder=6)
    ax.text(sx, sy - L.FORKLIFT_W / 2 - 2.5, "START / 대기 (표식 %.0fx%.0f)" % (L.MARKER_ACROSS, L.MARKER_ALONG),
            ha="center", va="top", fontsize=7, zorder=6)
    ax.text(sx, sy + L.FORKLIFT_W / 2 + 1.5, "지게차 40x12 (축척 확인용)", ha="center", va="bottom",
            fontsize=6.5, color="#d98200", zorder=6)


def draw_direction(ax, lay):
    off = 6.0   # arrows run beside the line, on the wall side (the bottom one sits higher: forklift outline)
    side = 14.0  # left/right arrows sit farther in so they do not cover the junction labels
    mx = (lay.xl + lay.xr) / 2
    my = (lay.yb + lay.yt) / 2
    arrow(ax, (mx + 14, lay.yb + 13), (mx - 14, lay.yb + 13))        # bottom: west
    arrow(ax, (lay.xl + side, my - 9), (lay.xl + side, my + 9))        # left: north
    arrow(ax, (mx - 14, lay.yt - off), (mx + 14, lay.yt - off))      # top: east
    arrow(ax, (lay.xr - side, my + 9), (lay.xr - side, my - 9))        # right: south
    ax.text(mx + 20, lay.yb + 13, "구간 1 (아래)", fontsize=7, color=ARROW_COLOR, va="center")
    ax.text(lay.xl + side + 2, my - 14, "구간 2\n(왼쪽)", fontsize=7, color=ARROW_COLOR, va="top")
    ax.text(mx - 20, lay.yt - off - 1.5, "구간 3 (위)", fontsize=7, color=ARROW_COLOR, va="top", ha="right")
    ax.text(lay.xr - side - 2, my + 14, "구간 4\n(오른쪽)", fontsize=7, color=ARROW_COLOR, va="bottom", ha="right")
    ax.text(mx, lay.yt - 17, "시계 방향 주행", fontsize=9, color=ARROW_COLOR, ha="center", fontweight="bold")


def draw_leds(ax):
    spots = [(L.LED_INSET, L.LED_INSET, "LED0 좌하"), (L.LED_INSET, L.BOARD_H - L.LED_INSET, "LED1 좌상"),
             (L.BOARD_W - L.LED_INSET, L.BOARD_H - L.LED_INSET, "LED2 우상"),
             (L.BOARD_W - L.LED_INSET, L.LED_INSET, "LED3 우하")]
    for (x, y, name) in spots:
        ax.add_patch(Circle((x, y), 2.4, facecolor="#ffd400", edgecolor="#555", zorder=6))
        ax.text(x, y - 3.5, name, ha="center", va="top", fontsize=6.5, zorder=6)


def draw_dimensions(ax, lay):
    chain = [(0, L.MARGIN_X), (L.MARGIN_X, L.BOX_D), (L.MARGIN_X + L.BOX_D, L.B)]
    x = 0.0
    top = L.BOARD_H + 5
    for width in [L.MARGIN_X, L.BOX_D, L.B, L.C, lay.wall()[2], L.C, L.B, L.BOX_D, L.MARGIN_X]:
        dim_h(ax, x, x + width, top, "%g" % width, size=6.5)
        x += width
    dim_h(ax, 0, L.BOARD_W, top + 7, "전체 %g cm" % L.BOARD_W, size=8)
    dim_v(ax, L.BOARD_W + 9, 0, L.BOARD_H, "전체 %g cm" % L.BOARD_H, size=8)
    dim_h(ax, lay.xl, lay.xr, -5, "루프 가로 %g (선 중심 간격)" % lay.width, size=7)
    ys = [lay.yb] + lay.left_junction_ys() + [lay.yt]
    for y0, y1 in zip(ys, ys[1:]):
        dim_v(ax, -4, y0, y1, "%g" % (y1 - y0), size=6.5)
    dim_v(ax, -10, lay.yb, lay.yt, "루프 세로 %g (선 중심 간격)" % lay.height, size=7)


def draw_map(lay, out_path):
    fig = plt.figure(figsize=(19, 11.5))
    ax = fig.add_axes([0.03, 0.04, 0.64, 0.90])
    rect(ax, 0, 0, L.BOARD_W, L.BOARD_H, facecolor=BOARD_COLOR, edgecolor="#555", lw=1.5, zorder=0)
    draw_wall(ax, lay)
    draw_track(ax, lay)
    draw_boxes(ax, lay)
    draw_start(ax, lay)
    draw_direction(ax, lay)
    draw_leds(ax)
    draw_dimensions(ax, lay)
    ax.set_xlim(-18, 196)
    ax.set_ylim(-9, 168)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("소형 지게차 맵 배치도 (제안, 단위 cm) - 위에서 본 모습", fontsize=14, loc="left")
    side_panel(fig, lay)
    fig.savefig(out_path, dpi=110)
    plt.close(fig)


def side_panel(fig, lay):
    ax = fig.add_axes([0.69, 0.04, 0.30, 0.90])
    ax.axis("off")
    lines = [
        ("[사용자 확정]", "b"),
        ("지게차 40 x 12 cm / 선 폭 19 mm / 벽 두께 1cm, 높이 30cm 이상", "n"),
        ("판 60x90 4장 + 45x60 2장 (단위 cm로 가정) = 27,000 cm2", "n"),
        ("상차 왼쪽 1~6, 하차 오른쪽 A~F (칸마다 2층 x 3곳), 시계 방향", "n"),
        ("", "n"),
        ("[제안] 판 6장 = %g x %g 직사각형 한 개" % (L.BOARD_W, L.BOARD_H), "b"),
        ("규칙(모두 제 기준, 실측 아님):", "n"),
    ]
    lines += [(("O " if ok else "X ") + text, "n") for ok, text in L.check_rules(lay)]
    lines += [
        ("", "n"),
        ("[미정 / 사용자 확인 필요]", "b"),
        ("- 칸(상자) 크기 20x15: 짐 크기, 포크 삽입 깊이를 몰라 임시값", "n"),
        ("- 회전 중심이 몸체 가운데라는 가정 (바퀴 축 위치 확인 필요)", "n"),
        ("- 라인 센서가 몸체 어디에 있는지 (후진 때 본선 인식 위치)", "n"),
        ("- 시작 표식 크기 (미결정 20번)", "n"),
        ("- LED 위치/높이, 선반 높이 (미결정 2번)", "n"),
        ("", "n"),
        ("범례: 파랑 = 상차 칸 (위 | 아래 = 2층 | 1층)", "n"),
        ("         주황 = 하차 칸, 노랑 점 = 안내 LED", "n"),
        ("         주황 점선 = 지게차 몸체 크기 (시작 위치)", "n"),
    ]
    y = 1.0
    for text, weight in lines:
        ax.text(0, y, text, fontsize=9, fontweight="bold" if weight == "b" else "normal", va="top",
                transform=ax.transAxes)
        y -= 0.035
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)


# ---------------- board layout ----------------
def draw_board_layout(lay, out_path):
    fig = plt.figure(figsize=(19, 11.5))
    ax = fig.add_axes([0.03, 0.04, 0.64, 0.90])
    colors = {"A": "#d9e8d0", "B": "#f6e3a8"}
    for (name, x, y, w, h) in L.boards():
        rect(ax, x, y, w, h, facecolor=colors[name[0]], edgecolor="#222", lw=1.8, zorder=1)
        ax.text(x + w / 2, y + h - 4, name, ha="center", va="top", fontsize=10, fontweight="bold", zorder=5)
        ax.text(x + w / 2, y + h - 11, "%g x %g (가로 x 세로)" % (w, h), ha="center", va="top", fontsize=7, zorder=5)
    draw_track(ax, lay)
    for y in lay.left_junction_ys():
        rect(ax, lay.xl - L.B - L.BOX_D, y - L.BOX_W / 2, L.BOX_D, L.BOX_W, facecolor="none", edgecolor="#555", ls=":", zorder=4)
    for y in lay.right_junction_ys():
        rect(ax, lay.xr + L.B, y - L.BOX_W / 2, L.BOX_D, L.BOX_W, facecolor="none", edgecolor="#555", ls=":", zorder=4)
    wx, wy, ww, wh = lay.wall()
    rect(ax, wx, wy, ww, wh, facecolor="none", edgecolor=WALL_COLOR, lw=2.0, ls="-", zorder=4)
    for (sa, sb, p) in L.crossings(lay):
        ax.add_patch(Circle(p, 2.2, facecolor="none", edgecolor="#d00000", lw=2, zorder=7))
    ax.add_patch(Rectangle((lay.start[0] - L.MARKER_ALONG / 2, lay.start[1] - L.MARKER_ACROSS / 2),
                           L.MARKER_ALONG, L.MARKER_ACROSS, facecolor=LINE_COLOR, zorder=6))
    dim_h(ax, 0, L.BOARD_W, L.BOARD_H + 5, "%g cm" % L.BOARD_W, size=9)
    dim_v(ax, L.BOARD_W + 5, 0, L.BOARD_H, "%g cm" % L.BOARD_H, size=9)
    ax.set_xlim(-8, 190)
    ax.set_ylim(-6, 160)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title("판 배치도: 60x90 4장 + 45x60 2장 -> 180 x 150 cm (단위 cm)", fontsize=14, loc="left")
    board_panel(fig, lay)
    fig.savefig(out_path, dpi=110)
    plt.close(fig)


def board_panel(fig, lay):
    ax = fig.add_axes([0.69, 0.04, 0.30, 0.90])
    ax.axis("off")
    lines = ["[판을 맞대는 이음선과 검은 선이 만나는 곳]  (빨간 원)"]
    for (sa, sb, p) in L.crossings(lay):
        lines.append("  이음선 %s-%s 가 선과 만남: x=%g, y=%g" % (sa, sb, p[0], p[1]))
    lines += [
        "",
        "시작 표식 (%g, %g)은 이음선에서 30cm 이상 떨어져 있음" % lay.start,
        "교차점 y 값(45, 75, 105)과 이음선 y=90 은 15cm 떨어짐",
        "벽 아래쪽 면(y=45)이 B1/B2 이음선 위에 놓임 (선은 아님)",
        "점선 사각형 = 칸 상자 자리 (이음선과 겹치지 않음)",
        "",
        "[조립 메모 - 제 제안]",
        "- 이음선은 뒷면에 테이프로 단차 없이 붙이기",
        "- 빨간 원 5곳은 선 위로 이음선이 지나가므로",
        "  주행 중 센서가 흔들릴 수 있어 단차를 특히 확인",
        "- 판 두께/재질이 모두 같아야 함 (다르면 단차 생김)",
        "- 선(검정 테이프)은 판을 다 맞춘 뒤에 붙이면 이음선을 가로질러 연속",
        "",
        "[왜 이 배치인가 - 비교]",
        "A 안: 6장 모두 사용 -> 180 x 150 (선택)",
        "  장점 코너 회전 반경(20cm) 확보, 칸 상자 여유",
        "  단점 이음선 5개, 이음선과 선 교차 5곳",
        "B 안: 60x90 4장만 사용 -> 180 x 120",
        "  장점 이음선 적음",
        "  단점 세로 120 이라 코너 회전 반경 20cm 확보 시",
        "  루프 세로가 80 이하 -> 교차점 3개 + 칸 상자 배치 불가",
        "  (루프 세로 110 필요: E 25 x 2 + P 30 x 2)",
    ]
    y = 1.0
    for text in lines:
        ax.text(0, y, text, fontsize=9, va="top", transform=ax.transAxes)
        y -= 0.036
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)


def main(argv):
    out_dir = argv[1] if len(argv) > 1 else os.path.dirname(os.path.abspath(__file__))
    os.makedirs(out_dir, exist_ok=True)
    lay = L.make_layout()
    failed = [text for ok, text in L.check_rules(lay) if not ok]
    if failed:
        print("RULE CHECK FAILED:\n  " + "\n  ".join(failed))
        return 1
    draw_map(lay, os.path.join(out_dir, "map_layout.png"))
    draw_board_layout(lay, os.path.join(out_dir, "board_layout.png"))
    print("wrote map_layout.png and board_layout.png to", out_dir)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
