"""cutplan_search.py - 원래 치수(벽 30x60, 칸 폭 15, 바닥 띠 25, 뒷판 16)를 유지하면서
추가 판(60x90)을 가장 적게 쓰는 재단 배치를 찾아 JSON 에 저장한다.
  python3 cutplan_search.py        -> 링형 바닥 (선 둘레만)  : cutplan_result.json
  python3 cutplan_search.py full   -> 가운데까지 이은 바닥   : cutplan_result_full.json
"""
import os
import sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, "..", "..", "map_final"))   # cutplan_core lives in map_final/
os.chdir(HERE)
import json
import sys
import cutplan_core as cc

if __name__ == "__main__":
    full = len(sys.argv) > 1 and sys.argv[1] == "full"
    out = "cutplan_result_full.json" if full else "cutplan_result.json"
    done = False
    for back_h in (16.0, 14.0, 12.0):
        design = cc.Design(band_out=25.0, back_h=back_h, wall_w=30.0, rack_w=15.0, fill_center=full)
        parts = cc.make_parts(design)
        print("뒷판 높이 %g: 부품 면적 %.0f cm2 (%d개)" % (back_h, cc.parts_area(parts), len(parts)))
        for extra in (0, 1, 2):                       # 사용자: 60x90 2장까지 추가 가능
            cc.set_boards(extra)
            res = cc.search(parts, 20000, 1)
            print("  추가 판 %d장 (합계 %.0f cm2): %s" % (extra, cc.TOTAL_AREA, "가능" if res else "못 넣음"))
            if res is None:
                continue
            res2 = cc.search(parts, 60000, 2)
            if res2 and res2[0] > res[0]:
                res = res2
            _, placed, free = res
            print("  잔여 조각 합계 %.0f cm2" % sum(w * h for *_, w, h in free))
            json.dump({"extra_a": extra, "design": design.__dict__, "placed": placed, "free": free},
                      open(out, "w"), ensure_ascii=False, indent=1)
            done = True
            break
        if done:
            break
    if not done:
        raise SystemExit("추가 판 2장, 뒷판 12까지 줄여도 못 넣음")
