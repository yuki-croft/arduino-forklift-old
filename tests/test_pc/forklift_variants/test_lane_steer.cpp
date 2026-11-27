// PC test of the variant B lane steering (forklift_hw.ino lineSteer) with fake inputs. Direction rule is an ASSUMPTION.
#include <cstdio>
#include <cstdlib>
const int LINE_BLACK_MIN = 30, LINE_BASE_SPEED = 100, LINE_TRIM_1 = 5, LINE_TRIM_2 = 15, LANE_DIFF_1 = 1, LANE_DIFF_2 = 3;
const bool LANE_STEER_INVERT = false;
#include "lane_steer.inc"
static int fails = 0;
static void expect(const char *n, bool ok) { printf("%s  %s\n", ok ? "PASS" : "FAIL", n); if (!ok) fails++; }
int main() {
  int l, r;
  lineSteer(32, 27, 32, l, r); expect("B steer: both lines equal -> straight", l == 100 && r == 100);
  lineSteer(29, 27, 32, l, r); expect("B steer: left value lower by 3 -> turn left a little (left wheel slower by trim 1)", l == 95 && r == 100);
  lineSteer(27, 27, 32, l, r); expect("B steer: left value lower by 5 -> turn left more (trim 2)", l == 85 && r == 100);
  lineSteer(32, 27, 28, l, r); expect("B steer: right value lower -> turn right (right wheel slower)", l == 100 && r == 85);
  lineSteer(27, 32, 27, l, r); expect("B steer: center on a line -> keep the last direction (right) strongly", l == 100 && r == 85);
  printf("%s\n", fails ? "SOME TESTS FAILED" : "all passed");
  return fails ? 1 : 0;
}
