"""make_cutplan.py - cutplan_result.json 을 읽어 도안 이미지 3장을 만든다.
  1_cut_sheets.png   : 판 6장 재단도 (부품 위치/크기, 잔여 조각, 부품표)
  2_assembly_top.png : 바닥 조립도 (이음선, 선, 벽, 칸, 지게차)
  3_wall_rack_detail.png : 벽 / 선반 조립 상세
실행: PYTHONPATH=. python3 make_cutplan.py   (먼저 python3 cutplan_search.py 로 JSON 생성)
"""
import json
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import cutplan_core as cc

plt.rcParams["font.family"] = "Noto Sans CJK KR"
plt.rcParams["axes.unicode_minus"] = False

KIND_COLOR = {"floor": "#cfe3f3", "wall": "#e9cdbd", "back": "#cfe8cf", "shelf": "#f6e3b0"}
KIND_LABEL = {"floor": "바닥", "wall": "벽", "back": "선반 뒷판", "shelf": "선반(칸)"}
BOARD_COLOR = {"A1": "#cfe3f3", "A2": "#d8d3ee", "A3": "#cfe8cf", "A4": "#f6e3b0", "A5": "#e3d6c4", "A6": "#d9e6c3", "B1": "#f3d0d6", "B2": "#d0ecec"}


def g(x):
    return ("%.1f" % x).rstrip("0").rstrip(".")


def load(path="cutplan_result.json"):
    data = json.load(open(path))
    cc.set_boards(data["extra_a"])
    d = cc.Design(**data["design"])
    parts = {p.pid: p for p in cc.make_parts(d)}
    return d, parts, data["placed"], data["free"]


def draw_sheets(d, parts, placed, free, path):
    nb = len(cc.BOARDS)
    rows = (nb + 2) // 3
    hr = [max(H for _, _, H in cc.BOARDS[r * 3:r * 3 + 3]) for r in range(rows)]
    fig = plt.figure(figsize=(22, 22 * sum(hr) / 258.0 * 0.93 + 1.6), dpi=100)
    gs = fig.add_gridspec(rows, 4, width_ratios=[60, 60, 60, 78], height_ratios=hr, wspace=0.12, hspace=0.14)
    for i, (name, W, H) in enumerate(cc.BOARDS):
        ax = fig.add_subplot(gs[i // 3, i % 3])
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
    # 부품표
    ax = fig.add_subplot(gs[:, 3])
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
    leftover = sum(w * h for _, _, _, w, h in free)
    kerf_loss = cc.TOTAL_AREA - total - leftover
    notes = [
        "판 %d장 합계 %d cm² 중 부품 %.0f cm² (%.1f%%)" % (nb, cc.TOTAL_AREA, total, 100 * total / cc.TOTAL_AREA),
        "잔여 조각 %.0f cm² (%.1f%%) + 톱날 %.0f cm² (%.1f%%)" % (leftover, 100 * leftover / cc.TOTAL_AREA, kerf_loss, 100 * kerf_loss / cc.TOTAL_AREA),
        "",
        "가정 (사용자 확인 필요)",
        "· 톱날 두께 %.1f cm 반영, 모두 직선(기요틴) 재단" % cc.KERF,
        "· 판은 결이 없어 90도 돌려 배치해도 됨",
        ("· 판 두께 %.0f cm: 사용자 확인함 (벽은 바닥 위라 높이 보정 없음)" % cc.BOARD_T) if d.fill_center else ("· 판 두께 %.0f cm: 사용자 확인함 (벽 높이에 +%.0f 반영)" % (cc.BOARD_T, cc.BOARD_T)),
        ("· 바닥은 가운데까지 모두 깔고 벽도 바닥 위에 섬" if d.fill_center else "· 바닥은 선 둘레만 깔고 가운데는 탁자 (벽이 탁자 위에 섬)"),
        "· 칸 폭 %s, 깊이 %s cm 는 임시값(짐 크기 미확인)" % (g(d.rack_w), g(cc.RACK_D)),
    ]
    ax.text(0.0, 0.40, "\n".join(notes), va="top", ha="left", fontsize=10.5, linespacing=1.6)
    handles = [Rectangle((0, 0), 1, 1, fc=c, ec="#333333") for c in KIND_COLOR.values()]
    ax.legend(handles, KIND_LABEL.values(), loc="lower left", ncol=2, fontsize=10, frameon=False)
    fig.suptitle("재단도 - 60×90 판 %d장 + 45×60 판 2장 (단위 cm, 위에서 본 모습)" % (nb - 2), fontsize=16, x=0.02, ha="left")
    fig.savefig(path, bbox_inches="tight", facecolor="white")
    plt.close(fig)


def dim_arrow(ax, p1, p2, text, off=(0, 0), fs=9, rot=0):
    ax.annotate("", xy=p2, xytext=p1, arrowprops=dict(arrowstyle="<->", lw=0.8, color="#444444"))
    ax.text((p1[0] + p2[0]) / 2 + off[0], (p1[1] + p2[1]) / 2 + off[1], text, ha="center", va="center", fontsize=fs,
            rotation=rot, bbox=dict(fc="white", ec="none", pad=0.8))


def draw_assembly(d, parts, placed, path):
    src = {pid: cc.BOARDS[si][0] for pid, si, *_ in placed}
    fig = plt.figure(figsize=(21, 13), dpi=100)
    gs = fig.add_gridspec(1, 2, width_ratios=[3.1, 1], wspace=0.02)
    ax = fig.add_subplot(gs[0])
    ax.set_aspect("equal")
    ax.axis("off")
    fl = cc.floor_layout(d)
    for pid, (x, y, w, h) in fl.items():
        ax.add_patch(Rectangle((x, y), w, h, fc=BOARD_COLOR[src[pid]], ec="#555555", lw=1.2, zorder=1))
        # 라벨 위치: 선/가지선/시작 표식과 겹치지 않는 자리 (띠는 선 바깥쪽 모서리 쪽, 다리는 선 위쪽)
        if pid in ("F1", "F3"):
            lx, ly, txt = (d.xl / 2 if pid == "F1" else d.xr + d.band_out / 2), 8.0, "%s (판 %s)\n%s×%s" % (pid, src[pid], g(w), g(h))
        elif pid in ("F2", "F4"):
            lx, ly, txt = (d.xl / 2 if pid == "F2" else d.xr + d.band_out / 2), d.floor_h - 8.0, "%s (판 %s)\n%s×%s" % (pid, src[pid], g(w), g(h))
        else:
            if d.fill_center:
                lx, ly, txt = x + 1.5, (37.0 if pid == "F5" else d.yt + 8.0), "%s (판 %s)\n%s×%s" % (pid, src[pid], g(w), g(h))
            else:
                lx, ly, txt = x + 1.5, (d.yb + 8.5 if pid == "F5" else d.yt + 6), "%s·%s" % (pid, src[pid])
        ax.text(lx, ly, txt, ha="center" if pid in ("F1", "F2", "F3", "F4") else "left", va="center", fontsize=8.5, color="#222222", zorder=6,
                bbox=dict(fc="white", ec="none", alpha=0.75, pad=1))
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
            ax.add_patch(Rectangle((bx, by), cc.RACK_D, d.rack_w, fc=col, ec="#333333", lw=1, zorder=4))
            ax.text(bx + cc.RACK_D / 2, by + d.rack_w / 2, lab, ha="center", va="center", fontsize=9, zorder=7)
    back_len = 2 * cc.P + d.rack_w
    by0 = d.junction_ys[0] - d.rack_w / 2
    ax.add_patch(Rectangle((xl - cc.B - cc.RACK_D, by0), cc.BOARD_T * 1.2, back_len, fc="#5f9f5f", ec="#2f5f2f", zorder=5))
    ax.add_patch(Rectangle((xr + cc.B + cc.RACK_D - cc.BOARD_T * 1.2, by0), cc.BOARD_T * 1.2, back_len, fc="#5f9f5f", ec="#2f5f2f", zorder=5))
    ax.text(xl - cc.B - cc.RACK_D - 2, by0 + back_len / 2, "뒷판 R1 (상차)", rotation=90, ha="right", va="center", fontsize=8.5, color="#2f5f2f")
    ax.text(xr + cc.B + cc.RACK_D + 2, by0 + back_len / 2, "뒷판 R2 (하차)", rotation=90, ha="left", va="center", fontsize=8.5, color="#2f5f2f")
    wx, wy, ww, wl = xl + cc.C, yb + cc.C, d.wall_w, d.loop_h - 2 * cc.C
    wall_h = cc.WALL_H + (0.0 if d.fill_center else cc.BOARD_T)
    ax.add_patch(Rectangle((wx, wy), ww, wl, fc="#e9cdbd", ec="#8a3b24", lw=2, hatch="//", zorder=2))
    ax.text(wx + ww / 2, wy + wl / 2, ("벽 (바닥 위)" if d.fill_center else "벽 (탁자 위)") + "\n%s×%s\n높이 %s" % (g(ww), g(wl), g(wall_h)), ha="center", va="center", fontsize=9.5,
            color="#6a2b14", bbox=dict(fc="white", ec="none", alpha=0.85), zorder=7)
    cx = (xl + xr) / 2
    ax.add_patch(Rectangle((cx - cc.FORK_L / 2, yb - cc.FORK_W / 2), cc.FORK_L, cc.FORK_W, fc="none", ec="#e08a00", ls="--", lw=1.5, zorder=8))
    ax.add_patch(Rectangle((cx - 1.5, yb - 4), 3, 8, fc="#111111", zorder=4))
    ax.text(cx, yb - cc.STRAIGHT_HALF - 1.5, "시작 위치 · 지게차 %s×%s (점선)" % (g(cc.FORK_L), g(cc.FORK_W)), ha="center", va="top", fontsize=8.5, color="#a86400", zorder=8)
    for xx in (0, d.floor_w - d.band_w):
        ax.plot([xx, xx + d.band_w], [cc.SEAM_Y, cc.SEAM_Y], color="#d62728", lw=2, ls="--", zorder=9)
    if d.fill_center:       # 가운데 띠: 세로 이음선 2줄 + 가로 이음선 1줄 (좌우 띠 이음선과 어긋남)
        for sx in (d.band_w, d.floor_w - d.band_w):
            ax.plot([sx, sx], [0, d.floor_h], color="#d62728", lw=2, ls="--", zorder=9)
        ax.plot([d.band_w, d.floor_w - d.band_w], [cc.CENTER_SEAM_Y, cc.CENTER_SEAM_Y], color="#d62728", lw=2, ls="--", zorder=9)
    ax.text(2, cc.SEAM_Y + 1.5, "이음선", fontsize=8.5, color="#d62728", va="bottom", zorder=9)
    ax.text(d.floor_w - 2, cc.SEAM_Y + 1.5, "이음선", fontsize=8.5, color="#d62728", va="bottom", ha="right", zorder=9)
    fw, fh = d.floor_w, d.floor_h
    ax.set_xlim(-cc.RACK_D - 14, fw + cc.RACK_D + 14)
    ax.set_ylim(-17, fh + 14)
    dim_arrow(ax, (0, -9), (fw, -9), "바닥 전체 폭 %s" % g(fw))
    dim_arrow(ax, (-cc.RACK_D - 8, 0), (-cc.RACK_D - 8, fh), "바닥 전체 높이 %s" % g(fh), rot=90)
    dim_arrow(ax, (0, fh + 6), (d.band_w, fh + 6), "띠 %s" % g(d.band_w))
    dim_arrow(ax, (fw - d.band_w, fh + 6), (fw, fh + 6), "띠 %s" % g(d.band_w))
    dim_arrow(ax, (xl + cc.CORNER_R, fh + 6), (xr - cc.CORNER_R, fh + 6), ("가운데 %s" if d.fill_center else "다리 %s") % g(d.loop_w - 2 * cc.CORNER_R))
    ax.set_title("바닥 조립도 (위에서 본 모습, 단위 cm) - " + ("가운데까지 모두 깐 통판형 바닥" if d.fill_center else "선 둘레만 깔고 가운데는 탁자"), fontsize=14, loc="left")
    # 설명 패널
    tx = fig.add_subplot(gs[1])
    tx.axis("off")
    corner_cov = "O"
    lines = [
        "조립 순서",
        "1. 바닥 6조각을 위 그림 위치에 놓고 뒷면을 테이프로 잇는다.",
        ("2. 이음선(빨간 점선): 세로 2줄(띠↔가운데) + 가로 3곳" if d.fill_center else "2. 이음선(빨간 점선)은 좌우 띠에 각 1곳."),
        ("   좌우 띠 y=%s, 가운데 y=%s 로 어긋나게(튼튼). 세로 이음선은" % (g(cc.SEAM_Y), g(cc.CENTER_SEAM_Y))) if d.fill_center else "   가지선(교차점)에서 15cm 떨어져 있어 선 감지에 영향 최소.",
        ("   모서리/시작 표식에서 16cm 이상, 가로선에서도 40cm 떨어짐." if d.fill_center else "   검은 선이 이음선을 지나는 곳은 테이프를 선 위에 겹치지 않게."),
        "3. 검은 선(폭 1.9cm)은 조립 후 위 그림처럼 붙인다 (이음선 위는 테이프 두께 주의).",
        ("4. 벽 4면을 바닥 위 해칭 자리에 세운다 (상세는 3번 이미지)." if d.fill_center else "4. 벽 4면을 탁자 위 해칭 자리에 세운다 (상세는 3번 이미지)."),
        "5. 칸 상자(선반)는 바닥 바깥 탁자 위에 놓는다.",
        "",
        "규칙 점검 (제 기준, 실측 아님)",
        "· 선→벽 %s ≥ 지게차 반길이+5 = %s" % (g(cc.C), g(cc.FORK_L / 2 + 5)),
        "· 선→칸 %s ≥ %s,  모서리→첫 교차점 %s ≥ %s" % (g(cc.B), g(cc.FORK_L / 2 + 5), g(cc.E), g(cc.FORK_L / 2 + 5)),
        "· 교차점 간격 %s ≥ 칸 폭 %s + 12 + 3 = %s" % (g(cc.P), g(d.rack_w), g(d.rack_w + 15)),
        "· 모서리 회전 원(반경 %s)이 바닥 안에 있음" % g(cc.CORNER_R),
        "· 직선 구간 바닥 폭 %s (몸체 12 + 여유)" % g(2 * cc.STRAIGHT_HALF),
        "",
        "이전 안(180×150 통판)과 달라진 점",
        ("· 선 둘레(띠 2)뿐 아니라 가운데(2조각)까지 깔아 %s×%s 한 장처럼 이음" % (g(d.floor_w), g(d.floor_h))) if d.fill_center else
        "· 바닥을 통판 대신 선 둘레(띠 2 + 다리 2)만 깔아 판을 아낌",
        ("· 벽은 바닥 위에 서므로 높이 30 (판 두께 보정 +1 불필요)") if d.fill_center else "· 벽은 탁자 위에 서므로 높이 31 (바닥 판 두께 +1)",
        ("· 벽 30×60, 루프 가로 %s, 선→벽 25 는 이전 안 그대로" % g(d.loop_w)) if d.wall_w == 30 else
        ("· 벽 가로 30 → %s (루프 가로 80 → %s): 판이 모자라 줄임" % (g(d.wall_w), g(d.loop_w))),
        "· 선반 뒷판 높이 %s: 선반 윗면은 약 %s cm 까지 가능" % (g(d.back_h), g(d.back_h - 2 * cc.BOARD_T)),
        "  (탁자-바닥 판 두께 %s + 선반 판 두께 %s 제외)" % (g(cc.BOARD_T), g(cc.BOARD_T)),
    ]
    tx.text(0, 1, "\n".join(lines), va="top", ha="left", fontsize=10.5, linespacing=1.55)
    fig.savefig(path, bbox_inches="tight", facecolor="white")
    plt.close(fig)


def draw_detail(d, parts, path):
    fig, axes = plt.subplots(1, 2, figsize=(19, 8.5), dpi=100, gridspec_kw={"width_ratios": [1, 1.15]})
    # --- 벽
    ax = axes[0]
    ax.set_aspect("equal")
    ax.axis("off")
    wh = cc.WALL_H + (0.0 if d.fill_center else cc.BOARD_T)
    ll, sl = d.loop_h - 2 * cc.C, d.wall_w - 2 * cc.BOARD_T
    y = 0
    for pid, w, name in (("W1", ll, "긴 면"), ("W2", ll, "긴 면")):
        ax.add_patch(Rectangle((0, y), w, wh, fc=KIND_COLOR["wall"], ec="#333333", lw=1.2))
        ax.text(w / 2, y + wh / 2, "%s %s  %s×%s" % (pid, name, g(w), g(wh)), ha="center", va="center", fontsize=10)
        y += wh + 5
    x = 0
    for pid in ("W3", "W4"):
        ax.add_patch(Rectangle((x, y), sl, wh, fc=KIND_COLOR["wall"], ec="#333333", lw=1.2))
        ax.text(x + sl / 2, y + wh / 2, "%s\n짧은 면\n%s×%s" % (pid, g(sl), g(wh)), ha="center", va="center", fontsize=9)
        x += sl + 5
    ax.text(0, y + wh + 3, "벽 펼친 모습 (4면, 뚜껑 없음)", fontsize=12, fontweight="bold")
    # 위에서 본 조립
    ox, oy = ll + 14, 0
    sc = 1.0
    ax.add_patch(Rectangle((ox, oy), d.wall_w, ll, fc="none", ec="#8a3b24", lw=1))
    t = cc.BOARD_T
    ax.add_patch(Rectangle((ox, oy), t, ll, fc="#c9987f", ec="none"))
    ax.add_patch(Rectangle((ox + d.wall_w - t, oy), t, ll, fc="#c9987f", ec="none"))
    ax.add_patch(Rectangle((ox + t, oy), sl, t, fc="#e0b9a3", ec="none"))
    ax.add_patch(Rectangle((ox + t, oy + ll - t), sl, t, fc="#e0b9a3", ec="none"))
    ax.text(ox + d.wall_w / 2, oy + ll / 2, "위에서 본 모습\n긴 면 2장이 바깥,\n짧은 면 2장이 사이\n(바깥 %s×%s)" % (g(d.wall_w), g(ll)), ha="center", va="center", fontsize=8.5)
    ax.text(ox, oy - 4, "접착: 모서리 안쪽에 풀/테이프", fontsize=8.5, va="top")
    ax.set_xlim(-2, ll + d.wall_w + 22)
    ax.set_ylim(-8, y + wh + 9)
    ax.set_title("벽: 두께 1cm, 높이 %s (바닥 판 위 30cm)" % g(wh), fontsize=13, loc="left")
    # --- 선반 단면
    ax = axes[1]
    ax.set_aspect("equal")
    ax.axis("off")
    depth = cc.RACK_D
    T = cc.BOARD_T
    ax.add_patch(Rectangle((-14, -T), 14 + depth + 14, T, fc="#bbbbbb", ec="#555555"))          # 탁자 표면 (표시용)
    ax.text(depth / 2, -T - 1.2, "탁자 (바닥 판 윗면이 판 두께 %s 만큼 더 높음)" % g(T), ha="center", va="top", fontsize=9)
    ax.add_patch(Rectangle((-14, 0), 14, T, fc="#cfe3f3", ec="#555555"))
    ax.text(-7, T + 0.4, "바닥 판 (윗면 = 높이 0)", ha="center", va="bottom", fontsize=8)
    # 뒷판
    ax.add_patch(Rectangle((depth - T, 0), T, d.back_h, fc="#5f9f5f", ec="#2f5f2f"))
    ax.text(depth + 2, d.back_h / 2, "뒷판 %s×%s\n(높이 %s)" % (g(2 * cc.P + d.rack_w), g(d.back_h), g(d.back_h)), va="center", ha="left", fontsize=9.5, color="#2f5f2f")
    z1, z2 = 4.0, 9.0
    for z, lab in ((z1, "1층 선반"), (z2, "2층 선반")):
        ax.add_patch(Rectangle((0, z), depth - T, T, fc=KIND_COLOR["shelf"], ec="#333333"))
        ax.text(-1, z + T / 2, "%s (높이 예시)" % lab, ha="right", va="center", fontsize=8.5)
    dim_arrow(ax, (0, -T - 5), (depth, -T - 5), "깊이 %s" % g(depth), off=(0, -1.5))
    ax.annotate("", xy=(depth + 18, 0), xytext=(depth + 18, d.back_h), arrowprops=dict(arrowstyle="<->", lw=0.8))
    ax.text(depth + 19, d.back_h * 0.5, "뒷판 높이 %s" % g(d.back_h), rotation=90, va="center", ha="left", fontsize=9)
    ax.plot([-26, -26], [4.5, 12.5], color="#e08a00", lw=4)
    ax.text(-27.5, 8.5, "포크 높이 범위\n4.5 ~ 12.5\n(뒷판 %s 안쪽)" % g(d.back_h), ha="right", va="center", fontsize=8.5, color="#a86400")
    ax.text(-14, d.back_h + 9, "선반 옆모습 (선 방향에서 본 단면)", fontsize=12, fontweight="bold")
    ax.text(-14, d.back_h + 6, "선반 판 %s×%s 를 뒷판에 직각으로 붙임 (캔틸레버, 하중 미확인)" % (g(d.rack_w), g(depth)), fontsize=8.5)
    ax.set_xlim(-62, depth + 28)
    ax.set_ylim(-12, d.back_h + 12)
    ax.set_title("칸 선반: 뒷판 1장 + 선반 판 6장 (상차 쪽), 하차 쪽도 같음", fontsize=13, loc="left")
    fig.text(0.02, 0.02, "선반 높이(1층/2층), 짐 크기, 하중은 미확인이므로 그림의 선반 높이는 예시입니다. 확정 전에는 뒷판에 높이를 연필로 표시한 뒤 붙이세요.", fontsize=10)
    fig.savefig(path, bbox_inches="tight", facecolor="white")
    plt.close(fig)


if __name__ == "__main__":
    import sys
    full = len(sys.argv) > 1 and sys.argv[1] == "full"
    pre = "full_" if full else ""
    d, parts, placed, free = load("cutplan_result_full.json" if full else "cutplan_result.json")
    draw_sheets(d, parts, placed, free, pre + "1_cut_sheets.png")
    draw_assembly(d, parts, placed, pre + "2_assembly_top.png")
    draw_detail(d, parts, pre + "3_wall_rack_detail.png")
    print("done", d)
