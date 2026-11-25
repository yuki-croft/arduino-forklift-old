"""Forklift map geometry (all lengths in cm). Pure computation, no drawing.

Given by the user: forklift 40 x 12 cm, line width 19 mm, center wall 1 cm thick / >= 30 cm high,
boards 60x90 (x4) and 45x60 (x2), 3 positions x 2 levels on each side, clockwise travel, start bottom center.
Everything marked PROPOSAL below is my choice, with the rule that produced it (see check_rules()).
"""
from dataclasses import dataclass

# ---- given by the user ----
FORKLIFT_L, FORKLIFT_W = 40.0, 12.0
LINE_W = 1.9
WALL_T = 1.0
BOARD_W, BOARD_H = 180.0, 150.0   # PROPOSAL: the only rectangle these six boards tile (see boards())

# ---- PROPOSALS (change here, rerun make_map.py) ----
C = 25.0         # main line centerline -> wall face.      Rule: >= L/2 + 5 (pivot at body center assumed)
B = 25.0         # main line centerline -> box face.       Rule: >= L/2 + 5
BOX_D = 20.0     # box depth (x direction).  PLACEHOLDER: cargo size and fork insertion depth unknown
BOX_W = 15.0     # box width (along the line). PLACEHOLDER: cargo size unknown
P = 30.0         # junction pitch.                          Rule: >= BOX_W + forklift width + 3
E = 25.0         # corner centerline -> nearest junction.   Rule: >= L/2 + 5
MARGIN_X = 5.0   # board edge -> box outer face
MARKER_ACROSS, MARKER_ALONG = 8.0, 3.0   # start bar: across the line / along the line. PROPOSAL (open item 20)
LED_INSET = 8.0  # LED distance from the board corner. PROPOSAL (sketch only says "outer corners")


@dataclass(frozen=True)
class Layout:
    xl: float
    xr: float
    yb: float
    yt: float

    @property
    def width(self): return self.xr - self.xl          # loop centerline width

    @property
    def height(self): return self.yt - self.yb         # loop centerline height

    @property
    def start(self): return ((self.xl + self.xr) / 2, self.yb)

    def wall(self):
        """(x, y, w, h) of the wall's outer faces."""
        return (self.xl + C, self.yb + C, self.width - 2 * C, self.height - 2 * C)

    def left_junction_ys(self):
        """Junction y values in travel order (bottom -> top), position 3, 2, 1 of the left side."""
        return [self.yb + E + i * P for i in range(3)]

    def right_junction_ys(self):
        """Junction y values in travel order (top -> bottom), position 1, 2, 3 of the right side."""
        return [self.yt - E - i * P for i in range(3)]


def make_layout():
    xl = MARGIN_X + BOX_D + B
    xr = BOARD_W - xl
    height = 2 * E + 2 * P
    yb = (BOARD_H - height) / 2
    return Layout(xl, xr, yb, yb + height)


def boards():
    """Six boards tiling BOARD_W x BOARD_H: (name, x, y, w, h). 'A' = 60x90, 'B' = 45x60."""
    return [
        ("A1 60x90", 0, 90, 90, 60), ("A2 60x90", 90, 90, 90, 60),    # top band, boards laid sideways
        ("A3 60x90", 0, 0, 60, 90), ("A4 60x90", 120, 0, 60, 90),     # lower band left / right
        ("B1 45x60", 60, 45, 60, 45), ("B2 45x60", 60, 0, 60, 45),    # lower band middle, two boards stacked
    ]


def seams():
    """Internal joints between boards: ((x0, y0), (x1, y1))."""
    return [((0, 90), (180, 90)), ((90, 90), (90, 150)), ((60, 0), (60, 90)),
            ((120, 0), (120, 90)), ((60, 45), (120, 45))]


def track_segments(lay):
    """Line centerlines as ((x0, y0), (x1, y1)): the loop and the six branches."""
    xl, xr, yb, yt = lay.xl, lay.xr, lay.yb, lay.yt
    loop = [((xl, yb), (xr, yb)), ((xl, yb), (xl, yt)), ((xl, yt), (xr, yt)), ((xr, yb), (xr, yt))]
    left = [((xl - B, y), (xl, y)) for y in lay.left_junction_ys()]
    right = [((xr, y), (xr + B, y)) for y in lay.right_junction_ys()]
    return loop + left + right


def crossings(lay):
    """Points where a seam crosses a track line (axis-aligned segments only)."""
    points = []
    for (sa, sb) in seams():
        for (ta, tb) in track_segments(lay):
            p = _intersection(sa, sb, ta, tb)
            if p is not None:
                points.append((sa, sb, p))
    return points


def _intersection(sa, sb, ta, tb):
    s_vertical = sa[0] == sb[0]
    t_vertical = ta[0] == tb[0]
    if s_vertical == t_vertical:
        return None   # parallel (a seam lying exactly on a line is not expected here)
    v, h = ((sa, sb), (ta, tb)) if s_vertical else ((ta, tb), (sa, sb))
    x, y = v[0][0], h[0][1]
    in_v = min(v[0][1], v[1][1]) <= y <= max(v[0][1], v[1][1])
    in_h = min(h[0][0], h[1][0]) <= x <= max(h[0][0], h[1][0])
    return (x, y) if (in_v and in_h) else None


def check_rules(lay):
    """Returns [(ok, text)]. Every rule is my own sizing heuristic, not a measured requirement."""
    half = FORKLIFT_L / 2
    wx, wy, ww, wh = lay.wall()
    area = sum(w * h for (_, _, _, w, h) in boards())
    return [
        (ww > 0 and wh > 0, "wall has positive size: %.0f x %.0f" % (ww, wh)),
        (C >= half + 5, "C (line -> wall) %.0f >= L/2+5 = %.0f" % (C, half + 5)),
        (B >= half + 5, "B (line -> box) %.0f >= L/2+5 = %.0f" % (B, half + 5)),
        (E >= half + 5, "E (corner -> 1st junction) %.0f >= L/2+5 = %.0f" % (E, half + 5)),
        (P >= BOX_W + FORKLIFT_W + 3, "P (pitch) %.0f >= box width + 12 + 3 = %.0f" % (P, BOX_W + FORKLIFT_W + 3)),
        (lay.yb >= half and BOARD_H - lay.yt >= half,
         "corner turn circle r=%.0f stays on the boards (margin %.0f)" % (half, lay.yb)),
        (MARGIN_X >= 0 and lay.xl - B - BOX_D >= 0, "boxes inside the boards (margin %.0f)" % MARGIN_X),
        (abs(area - BOARD_W * BOARD_H) < 1e-6, "boards area %.0f == map area %.0f" % (area, BOARD_W * BOARD_H)),
    ]
