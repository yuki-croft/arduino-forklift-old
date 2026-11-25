"""Tests for layout.py (run: python3 -m unittest test_layout -v, from this folder)."""
import unittest

import layout as L


def overlap(a, b):
    ax, ay, aw, ah = a
    bx, by, bw, bh = b
    return ax < bx + bw and bx < ax + aw and ay < by + bh and by < ay + ah


class BoardTilingTest(unittest.TestCase):
    def test_six_boards_have_the_requested_sizes(self):
        sizes = sorted(tuple(sorted((w, h))) for (_, _, _, w, h) in L.boards())
        self.assertEqual(sizes, [(45, 60), (45, 60), (60, 90), (60, 90), (60, 90), (60, 90)])

    def test_boards_do_not_overlap_and_fill_the_rectangle(self):
        rects = [(x, y, w, h) for (_, x, y, w, h) in L.boards()]
        for i, a in enumerate(rects):
            self.assertGreaterEqual(a[0], 0)
            self.assertGreaterEqual(a[1], 0)
            self.assertLessEqual(a[0] + a[2], L.BOARD_W)
            self.assertLessEqual(a[1] + a[3], L.BOARD_H)
            for b in rects[i + 1:]:
                self.assertFalse(overlap(a, b), "%s overlaps %s" % (a, b))
        self.assertEqual(sum(w * h for (_, _, w, h) in rects), L.BOARD_W * L.BOARD_H)


class TrackTest(unittest.TestCase):
    def setUp(self):
        self.lay = L.make_layout()

    def test_all_rules_pass(self):
        for ok, text in L.check_rules(self.lay):
            self.assertTrue(ok, text)

    def test_junction_order_matches_firmware_numbering(self):
        # final_forklift_board: load junction count = 4 - position (position 3 is met first on the left side)
        self.assertEqual(self.lay.left_junction_ys(), sorted(self.lay.left_junction_ys()))      # bottom -> top
        self.assertEqual(self.lay.right_junction_ys(), sorted(self.lay.right_junction_ys(), reverse=True))  # top -> bottom
        self.assertEqual(len(self.lay.left_junction_ys()), 3)
        self.assertEqual(len(self.lay.right_junction_ys()), 3)

    def test_start_is_bottom_center_of_the_loop(self):
        self.assertEqual(self.lay.start, ((self.lay.xl + self.lay.xr) / 2, self.lay.yb))

    def test_boxes_do_not_sit_on_a_seam(self):
        boxes = [(self.lay.xl - L.B - L.BOX_D, y - L.BOX_W / 2, L.BOX_D, L.BOX_W) for y in self.lay.left_junction_ys()]
        boxes += [(self.lay.xr + L.B, y - L.BOX_W / 2, L.BOX_D, L.BOX_W) for y in self.lay.right_junction_ys()]
        for (sa, sb) in L.seams():
            seam = (min(sa[0], sb[0]), min(sa[1], sb[1]), abs(sb[0] - sa[0]) or 1e-6, abs(sb[1] - sa[1]) or 1e-6)
            for box in boxes:
                self.assertFalse(overlap(seam, box), "seam %s crosses box %s" % ((sa, sb), box))

    def test_seams_stay_clear_of_start_marker_and_junctions(self):
        sx, sy = self.lay.start
        for (sa, sb, p) in L.crossings(self.lay):
            self.assertGreaterEqual(abs(p[0] - sx) + abs(p[1] - sy), 30)    # far from the start bar
        junction_ys = self.lay.left_junction_ys() + self.lay.right_junction_ys()
        for (sa, sb, p) in L.crossings(self.lay):
            for y in junction_ys:
                if p[0] in (self.lay.xl, self.lay.xr):
                    self.assertGreaterEqual(abs(p[1] - y), 10)

    def test_five_seam_crossings(self):
        self.assertEqual(len(L.crossings(self.lay)), 5)

    def test_rule_check_fails_when_the_wall_would_vanish(self):
        original = L.C
        try:
            L.C = 45.0   # corridors too wide for this loop width
            self.assertTrue(any(not ok for ok, _ in L.check_rules(L.make_layout())))
        finally:
            L.C = original


if __name__ == "__main__":
    unittest.main()
