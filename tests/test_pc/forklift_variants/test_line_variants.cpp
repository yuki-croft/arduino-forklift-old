// PC test of the two forklift line variants (2026-11-28): detector rules and steering direction, on SYNTHETIC values
// inside the user's ranges (white 25-29, black 30-34). NOT measurements, NOT an Arduino compile, no board.
#include <cstdio>
#include <cstdlib>
#include <vector>
#if VARIANT_B
#include "pattern_b.h"
#else
#include "pattern_a.h"
#endif
static int fails = 0;
static void expect(const char *n, bool ok) { printf("%s  %s\n", ok ? "PASS" : "FAIL", n); if (!ok) fails++; }
static std::vector<LineEvent> run(std::vector<LineSample> tr, bool *marker = nullptr) {
  LineDetector d; std::vector<LineEvent> evs; bool m = false;
  for (auto &s : tr) { LineEvent e = lineDetectorStep(d, s); if (e != EV_NONE) evs.push_back(e); m = m || d.markerOn; }
  if (marker) *marker = m;
  return evs;
}
static std::vector<LineSample> rep(LineSample s, int n) { return std::vector<LineSample>(n, s); }
static std::vector<LineSample> cat(std::vector<std::vector<LineSample>> parts) { std::vector<LineSample> o; for (auto &p : parts) o.insert(o.end(), p.begin(), p.end()); return o; }
int main() {
#if VARIANT_B
  const LineSample lane = {32, 27, 32}, leftGap = {27, 27, 32}, rightGap = {32, 27, 27}, cross = {33, 33, 33}, outerAhead = {32, 32, 27};
  bool m;
  expect("B: steady lane -> no event", run(rep(lane, 50), &m).empty() && !m);
  auto e = run(cat({rep(lane, 10), rep(leftGap, 6), rep(lane, 10)}));
  expect("B: left line broken for 6 samples then back -> JUNCTION_LEFT", e.size() == 1 && e[0] == EV_JUNCTION_LEFT);
  e = run(cat({rep(lane, 10), rep(rightGap, 6), rep(lane, 10)}));
  expect("B: right line broken then back -> CORNER (user: corner = right-branch pattern)", e.size() == 1 && e[0] == EV_CORNER);
  e = run(cat({rep(lane, 10), rep(rightGap, 6), rep(outerAhead, 4)}));
  expect("B: right line broken, then center meets the outer line -> CORNER", e.size() == 1 && e[0] == EV_CORNER);
  e = run(cat({rep(lane, 10), rep(leftGap, 2), rep(lane, 10)}));
  expect("B: 2-sample wobble (below LINE_CONFIRM_SAMPLES) -> ignored", e.empty());
  e = run(cat({rep(lane, 10), rep(cross, 5), rep(leftGap, 4), rep(lane, 10)}), &m);
  expect("B: cross tape -> marker, and the left break right after it is inside the guard -> ignored", m && e.empty());
#else
  const LineSample onLine = {28, 32, 27}, leftBranch = {32, 32, 27}, rightBranch = {27, 32, 33}, cornerOff = {27, 27, 33}, marker = {32, 33, 32};
  bool m;
  expect("A: black threshold is 30 (user range 30-34), STRONG = 30", LINE_ON_MIN == 30 && LINE_STRONG_MIN == 30);
  expect("A: steady line with sides at 27-28 -> no event", run(rep(onLine, 50), &m).empty() && !m);
  auto e = run(cat({rep(onLine, 10), rep(leftBranch, 5), rep(onLine, 10)}));
  expect("A: left side black for 5 samples, center stays black -> JUNCTION_LEFT", e.size() == 1 && e[0] == EV_JUNCTION_LEFT);
  e = run(cat({rep(onLine, 10), rep(rightBranch, 5), rep(onLine, 10)}));
  expect("A: right side black, center stays black -> CORNER (user: corner = right-branch pattern)", e.size() == 1 && e[0] == EV_CORNER);
  e = run(cat({rep(onLine, 10), rep(rightBranch, 3), rep(cornerOff, 3), rep({27, 27, 27}, 2)}));
  expect("A: right side black, center leaves the line, then right leaves too -> CORNER", e.size() == 1 && e[0] == EV_CORNER);
  e = run(cat({rep(onLine, 10), rep(rightBranch, 2), rep(onLine, 10)}));
  expect("A: 2-sample edge touch -> ignored", e.empty());
  run(cat({rep(onLine, 5), rep(marker, 4), rep(onLine, 5)}), &m);
  expect("A: all three black for 4 samples -> marker", m);
  e = run(cat({rep(onLine, 10), rep({30, 32, 27}, 5), rep(onLine, 3)}));
  expect("A: value exactly 30 counts as black (old rule needed 40 and never fired)", e.size() == 1 && e[0] == EV_JUNCTION_LEFT);
#endif
  printf("%s\n", fails ? "SOME TESTS FAILED" : "all passed");
  return fails ? 1 : 0;
}
