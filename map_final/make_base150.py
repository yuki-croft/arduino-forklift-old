"""make_base150.py - base150_result.json 으로 최종 맵 도안 2장을 이 폴더에 만든다.
  map_final_assembly_top.png : 150x150 풍차 베이스 위 맵 조립도 (구멍, 이음선, 선, 벽, 칸)
  map_final_cut_sheets.png   : 남는 판(추가 60x90 + 45x60 2장) 재단도
(벽/선반 상세 그림은 선반 배치가 바뀐 뒤 갱신되지 않아 만들지 않는다.)
실행: python3 make_base150.py   (어느 폴더에서 실행해도 됨)
"""
import json
import sys
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import cutplan_core as cc
import base150_plan as bp
import make_cutplan as mk
from make_cutplan import g, dim_arrow, KIND_COLOR, KIND_LABEL

plt.rcParams["font.family"] = "Noto Sans CJK KR"
plt.rcParams["axes.unicode_minus"] = False
BASE_COLOR = {"A1": "#cfe3f3", "A2": "#d8d3ee", "A3": "#cfe8cf", "A4": "#f6e3b0"}
SHEET_COLOR = {"A5": "#e3d6c4", "A6": "#d9e6c3", "B1": "#f3d0d6", "B2": "#d0ecec"}


HERE = Path(__file__).resolve().parent


def load(path=HERE / "base150_result.json"):
    data = json.load(open(path))
    cc.BOARDS = [tuple(b) for b in data["boards"]]
    d = cc.Design(**data["design"])
    parts = {p.pid: p for p in bp.make_parts(d)}
    return d, parts, data["placed"], data["free"]


def draw_sheets(d, parts, placed, free, path):
    nb = len(cc.BOARDS)
    fig = plt.figure(figsize=(21, 13), dpi=100)
    gs = fig.add_gridspec(2, 3, width_ratios=[60, 60, 80], height_ratios=[90, 60], wspace=0.12, hspace=0.18)
    pos = {}
    slots = [gs[0, 0], gs[0, 1], gs[1, 0], gs[1, 1]]
    for i in range(nb):
        pos[i] = slots[i]
    for i, (name, W, H) in enumerate(cc.BOARDS):
        ax = fig.add_subplot(pos[i])
        ax.set_xlim(-1, W + 1)
        ax.set_ylim(-1, H + 1)
        ax.set_aspect("equal")
        ax.set_anchor("N")
        ax.set_xticks([])
        ax.set_yticks([])
        for s in ax.spines.values():
            s.set_visible(False)
        ax.add_patch(Rectangle((0, 0), W, H, fc="white", ec="black", lw=2))
        for si, x, y, w, h in free:
            if si == i:
                ax.add_patch(Rectangle((x, y), w, h, fc="#eeeeee", ec="#999999", lw=0.6, hatch="////"))
                if w >= 6 and h >= 6:
                    ax.text(x + w / 2, y + h / 2, "잔여\n%s×%s" % (g(w), g(h)), ha="center", va="center", fontsize=8, color="#555555")
        for pid, si, x, y, w, h, rot in placed:
            if si != i:
                continue
            p = parts[pid]
            ax.add_patch(Rectangle((x, y), w, h, fc=KIND_COLOR[p.kind], ec="#333333", lw=1))
            txt = "%s\n%s×%s" % (pid, g(w), g(h)) if min(w, h) >= 9 else pid
            ax.text(x + w / 2, y + h / 2, txt, ha="center", va="center", fontsize=9 if min(w, h) >= 9 else 7,
                    rotation=90 if (h > 2.2 * w and min(w, h) < 9) else 0)
        used = sum(w * h for pid, si, x, y, w, h, r in placed if si == i)
        ax.set_title("판 %s  %s×%s cm  (사용 %.0f%%)" % (name, g(W), g(H), 100 * used / (W * H)), fontsize=12)
    ax = fig.add_subplot(gs[:, 2])
    ax.axis("off")
    groups = {}
    for p in parts.values():
        groups.setdefault((p.kind, p.name, p.w, p.h), []).append(p.pid)
    rows = []
    for (kind, name, w, h), ids in groups.items():
        rows.append([name, "%s×%s" % (g(w), g(h)), str(len(ids)), ("%s" % ",".join(ids)) if len(ids) <= 2 else "%s~%s" % (ids[0], ids[-1]), "%.0f" % (w * h * len(ids))])
    total = cc.parts_area(list(parts.values()))
    rows.append(["합계", "", str(len(parts)), "", "%.0f" % total])
    tb = ax.table(cellText=rows, colLabels=["부품", "치수(cm)", "수량", "번호", "면적"], loc="upper center", cellLoc="center",
                  colWidths=[0.30, 0.2, 0.1, 0.18, 0.16])
    tb.auto_set_font_size(False)
    tb.set_fontsize(9.5)
    tb.scale(1, 1.9)
    cap = sum(w * h for _, w, h in cc.BOARDS)
    leftover = sum(w * h for _, _, _, w, h in free)
    kerf_loss = cap - total - leftover
    notes = [
        "자르는 판 %d장 합계 %d cm² 중 부품 %.0f cm² (%.1f%%)" % (nb, cap, total, 100 * total / cap),
        "잔여 조각 %.0f cm² (%.1f%%) + 톱날 %.0f cm² (%.1f%%)" % (leftover, 100 * leftover / cap, kerf_loss, 100 * kerf_loss / cap),
        "바닥 판 A1~A4 (60×90 4장)는 자르지 않음",
        "",
        "가정 (사용자 확인 필요)",
        "· 톱날 두께 %.1f cm 반영, 모두 직선(기요틴) 재단" % cc.KERF,
        "· 판은 결이 없어 90도 돌려 배치해도 됨",
        "· 벽은 베이스 위에 서므로 높이 %g (판 두께 보정 없음)" % cc.WALL_H,
        "· 벽 가로 %g = 선 루프 100 - 선->벽 %g x2" % (d.wall_w, cc.C),
        "· 칸 폭 %s, 깊이 %s cm 는 임시값(짐 크기 미확인)" % (g(d.rack_w), g(cc.RACK_D)),
    ]
    ax.text(0.0, 0.42, "\n".join(notes), va="top", ha="left", fontsize=10.5, linespacing=1.6)
    handles = [Rectangle((0, 0), 1, 1, fc=KIND_COLOR[k], ec="#333333") for k in ("wall", "back", "shelf")]
    ax.legend(handles, [KIND_LABEL[k] for k in ("wall", "back", "shelf")], loc="lower left", ncol=3, fontsize=10, frameon=False)
    fig.suptitle("재단도 - 추가 60×90 %d장 + 45×60 2장 (단위 cm, 위에서 본 모습)" % (nb - 2), fontsize=16, x=0.02, ha="left")
    fig.savefig(path, bbox_inches="tight", facecolor="white")
    plt.close(fig)


def draw_assembly(d, parts, path):
    d = bp.abs_geom(d)
    fig = plt.figure(figsize=(21, 13), dpi=100)
    gs = fig.add_gridspec(1, 2, width_ratios=[3.1, 1], wspace=0.02)
    ax = fig.add_subplot(gs[0])
    ax.set_aspect("equal")
    ax.axis("off")
    for name, (x, y, w, h) in bp.PINWHEEL.items():
        ax.add_patch(Rectangle((x, y), w, h, fc=BASE_COLOR[name], ec="#555555", lw=1.4, zorder=1))
        ax.text(x + 4, y + 4 if name in ("A1", "A4") else y + h - 4, "%s %s×%s" % (name, g(w), g(h)), fontsize=9, va="bottom" if name in ("A1", "A4") else "top", zorder=6,
                bbox=dict(fc="white", ec="none", alpha=0.75, pad=1))
    hx, hy, hw, hh = bp.HOLE
    ax.add_patch(Rectangle((hx, hy), hw, hh, fc="#7a5a3a", ec="#333333", lw=1.2, hatch="xx", zorder=2, alpha=0.55))
    ax.text(hx + hw / 2, hy - 2.5, "구멍 %s×%s (유지)" % (g(hw), g(hh)), ha="center", va="top", fontsize=9, color="#4a2f14", zorder=7)
    xl, xr, yb, yt = d.xl, d.xr, d.yb, d.yt
    lw = cc.LINE_W
    black = dict(fc="#111111", ec="none", zorder=3)
    ax.add_patch(Rectangle((xl - lw / 2, yb - lw / 2), lw, d.loop_h + lw, **black))
    ax.add_patch(Rectangle((xr - lw / 2, yb - lw / 2), lw, d.loop_h + lw, **black))
    ax.add_patch(Rectangle((xl, yb - lw / 2), d.loop_w, lw, **black))
    ax.add_patch(Rectangle((xl, yt - lw / 2), d.loop_w, lw, **black))
    labels_l = ["3|6", "2|5", "1|4"]
    labels_r = ["C|F", "B|E", "A|D"]
    for yj, ll, lr in zip(d.junction_ys, labels_l, labels_r):
        ax.add_patch(Rectangle((xl - cc.B, yj - lw / 2), cc.B, lw, **black))
        ax.add_patch(Rectangle((xr, yj - lw / 2), cc.B, lw, **black))
        bl = (xl - cc.B - cc.RACK_D, yj - d.rack_w / 2)
        br = (xr + cc.B, yj - d.rack_w / 2)
        for (bx, by), lab, col in ((bl, ll, "#cfe0f5"), (br, lr, "#f8dcc0")):
            ax.add_patch(Rectangle((bx, by), cc.RACK_D, d.rack_w, fc=col, ec="#333333", lw=1, zorder=4, alpha=0.9))
            ax.text(bx + cc.RACK_D / 2, by + d.rack_w / 2, lab, ha="center", va="center", fontsize=9, zorder=7)
    back_len = 2 * cc.P + d.rack_w
    by0 = d.junction_ys[0] - d.rack_w / 2
    ax.add_patch(Rectangle((xl - cc.B - cc.RACK_D, by0), cc.BOARD_T * 1.2, back_len, fc="#5f9f5f", ec="#2f5f2f", zorder=5))
    ax.add_patch(Rectangle((xr + cc.B + cc.RACK_D - cc.BOARD_T * 1.2, by0), cc.BOARD_T * 1.2, back_len, fc="#5f9f5f", ec="#2f5f2f", zorder=5))
    ax.text(xl - cc.B - cc.RACK_D - 2, by0 + back_len / 2, "뒷판 R1 (상차)", rotation=90, ha="right", va="center", fontsize=8.5, color="#2f5f2f")
    ax.text(xr + cc.B + cc.RACK_D + 2, by0 + back_len / 2, "뒷판 R2 (하차)", rotation=90, ha="left", va="center", fontsize=8.5, color="#2f5f2f")
    wx, wy, ww, wl = xl + cc.C, yb + cc.C, d.wall_w, d.loop_h - 2 * cc.C
    ax.add_patch(Rectangle((wx, wy), ww, wl, fc="#e9cdbd", ec="#8a3b24", lw=2, hatch="//", zorder=2, alpha=0.55))
    ax.text(wx + ww / 2, wy + wl - 4, "벽 (베이스 위)\n%s×%s 높이 %s" % (g(ww), g(wl), g(cc.WALL_H)), ha="center", va="top", fontsize=9,
            color="#6a2b14", bbox=dict(fc="white", ec="none", alpha=0.85), zorder=7)
    cx = (xl + xr) / 2
    ax.add_patch(Rectangle((cx - cc.FORK_L / 2, yb - cc.FORK_W / 2), cc.FORK_L, cc.FORK_W, fc="none", ec="#e08a00", ls="--", lw=1.5, zorder=8))
    ax.add_patch(Rectangle((cx - 1.5, yb - 4), 3, 8, fc="#111111", zorder=4))
    ax.text(cx + 2, yb - cc.STRAIGHT_HALF - 1.5, "시작 위치 · 지게차 %s×%s (점선)" % (g(cc.FORK_L), g(cc.FORK_W)), ha="left", va="top", fontsize=8.5, color="#a86400", zorder=8)
    # 이음선 (판끼리 맞닿는 곳)
    seams = [((0, 90), (60, 90)), ((90, 90), (90, 150)), ((90, 60), (150, 60)), ((60, 0), (60, 60))]
    for (x1, y1), (x2, y2) in seams:
        ax.plot([x1, x2], [y1, y2], color="#d62728", lw=2.2, ls="--", zorder=9)
    ax.text(2, 91.5, "이음선", fontsize=8.5, color="#d62728", va="bottom", zorder=9)
    ax.text(148, 61.5, "이음선", fontsize=8.5, color="#d62728", va="bottom", ha="right", zorder=9)
    ax.text(91.5, 148, "이음선", fontsize=8.5, color="#d62728", va="top", rotation=90, zorder=9)
    ax.text(58.5, 2, "이음선", fontsize=8.5, color="#d62728", va="bottom", ha="right", rotation=90, zorder=9)
    ax.set_xlim(-cc.RACK_D - 14, bp.BASE + cc.RACK_D + 14)
    ax.set_ylim(-17, bp.BASE + 8)
    dim_arrow(ax, (0, -9), (bp.BASE, -9), "베이스 폭 %s" % g(bp.BASE))
    dim_arrow(ax, (-cc.RACK_D - 8, 0), (-cc.RACK_D - 8, bp.BASE), "베이스 높이 %s" % g(bp.BASE), rot=90)
    ax.set_title("150×150 풍차 베이스 위 맵 조립도 (위에서 본 모습, 단위 cm)", fontsize=14, loc="left")
    tx = fig.add_subplot(gs[1])
    tx.axis("off")
    over = d.xl - cc.B
    lines = [
        "베이스 (자르지 않음)",
        "· 60×90 판 4장(A1~A4)을 풍차 모양으로 놓으면",
        "  150×150, 가운데 구멍 30×30 (계산값).",
        "· 사진은 거울 방향일 수 있음: 이 도안은 좌우 대칭이라",
        "  거울 배치도 점검 결과가 같다 (시험으로 확인).",
        "",
        "맵 배치 (제 가정, 베이스 한가운데)",
        "· 검은 선 루프 가로 %s × 세로 %s (사용자 요청 정사각형)," % (g(d.loop_w), g(d.loop_h)),
        "  베이스 가장자리와 사방 %s 떨어짐" % g(d.xl),
        "· 벽 %s×%s 가 구멍(60~90)을 둘러싸고, 벽은 구멍에서 9cm 떨어짐" % (g(d.wall_w), g(d.loop_h - 2 * cc.C)),
        "· 이음선이 선과 만나는 곳: 교차점·모서리에서 15cm 이상,",
        "  시작 표식(폭 8) 가장자리에서 11cm (둘 다 기준 8 이상)",
        "",
        "칸(선반)",
        ("· 칸 앞면 = 베이스 가장자리와 같은 위치 (베이스 위에 걸치지 않음)" if abs(over) < 1e-9 else "· 칸 앞면은 베이스 가장자리 안쪽 %s" % g(over)),
        "· 뒷판은 베이스 바깥 %s cm 에 서므로 좌우로 각 %s cm 필요 (총 폭 %s)" % (g(cc.RACK_D - max(over, 0)), g(cc.RACK_D - max(over, 0)), g(bp.BASE + 2 * (cc.RACK_D - max(over, 0)))),
        "",
        "규칙 점검 (제 기준, 실측 아님)",
        "· 선→벽 %s, 선→칸 %s, 교차점 간격 %s: 기존 규칙 그대로" % (g(cc.C), g(cc.B), g(cc.P)),
        "· 모서리→첫 교차점 %s: 기존 25 에서 줄임! 지게차 반길이 %s + 여유 5 = %s 에 미달" % (g(cc.E), g(cc.FORK_L / 2), g(cc.FORK_L / 2 + 5)),
        "  (세로 100 을 맞추려면 E 또는 교차점 간격을 줄여야 함. 모서리 회전 시험 필요)",
        "· 모서리 회전 원(반경 %s)이 베이스 안에 있음" % g(cc.CORNER_R),
    ]
    tx.text(0, 1, "\n".join(lines), va="top", ha="left", fontsize=10.5, linespacing=1.55)
    fig.savefig(path, bbox_inches="tight", facecolor="white")
    plt.close(fig)


if __name__ == "__main__":
    d, parts, placed, free = load()
    draw_sheets(d, parts, placed, free, str(HERE / "map_final_cut_sheets.png"))
    draw_assembly(d, parts, str(HERE / "map_final_assembly_top.png"))
    print("done", d)
