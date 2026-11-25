// PC test of forklift_line_pattern.h (pure detector) and of the forklift_sense.ino glue, on SYNTHETIC sensor traces.
// The sensor values below are ASSUMPTIONS (see the header comment), not measurements from the real forklift.
#include "Arduino.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "forklift_line_pattern.h"

// ---- things the real sketch defines (forklift_sense.ino expects them) ----
const uint8_t PIN_LINE_LEFT = A2, PIN_LINE_CENTER = A3, PIN_LINE_RIGHT = A4;
const int LINE_WHITE_MAX = 29;
const long IR_FREQ_HZ = 38000;
const long REAR_STOP_CM = 5;
const unsigned long REAR_CHECK_MS = 100, REAR_HOLD_MS = 1000;
unsigned long rearCheckMs = 0, rearSinceMs = 0;
bool rearClose = false;
uint8_t segment = 0;
struct FakeUltrasonic { int read() { return 0; } } ultrasonic;
void irScan(long, byte &l, byte &f, byte &r) { l = f = r = 1; }

// ---- fake clock and analog inputs ----
static unsigned long g_now = 1000;
static int g_a[3] = {26, 26, 26};
unsigned long millis() { return g_now; }
int analogRead(uint8_t pin) { return g_a[pin == PIN_LINE_LEFT ? 0 : pin == PIN_LINE_CENTER ? 1 : 2]; }

#include SRC_SENSE   // the real forklift_sense.ino

// ---- synthetic values (ASSUMPTIONS) ----
const int WHITE = 26, EDGE = 33, FULL = 46;      // white floor / sensor on the edge of the line / sensor fully on black
struct Seg { int l, c, r, n; };
static unsigned lcg = 12345;
static int noise(int amp) { lcg = lcg * 1103515245u + 12345u; return amp == 0 ? 0 : (int)((lcg >> 16) % (2 * amp + 1)) - amp; }
static std::vector<LineSample> build(const std::vector<Seg> &segs, int amp) {
  std::vector<LineSample> out;
  for (const Seg &s : segs) for (int i = 0; i < s.n; i++) out.push_back({s.l + noise(amp), s.c + noise(amp), s.r + noise(amp)});
  return out;
}
static Seg straight(int n) { return {EDGE, FULL, EDGE, n}; }
static Seg leftBranch(int n) { return {FULL, FULL, EDGE, n}; }
static Seg rightBranch(int n) { return {EDGE, FULL, FULL, n}; }
static Seg cornerBurst(int n) { return {WHITE, FULL, FULL, n}; }
static Seg lost(int n) { return {WHITE, WHITE, WHITE, n}; }
static Seg marker(int n) { return {FULL, FULL, FULL, n}; }

struct Result { std::vector<LineEvent> events; int markerRises = 0; int markerSamples = 0; };
static Result runDetector(const std::vector<LineSample> &tr) {
  LineDetector d; Result r; bool prev = false;
  for (const LineSample &s : tr) {
    LineEvent e = lineDetectorStep(d, s);
    if (e != EV_NONE) r.events.push_back(e);
    if (d.markerOn && !prev) r.markerRises++;
    if (d.markerOn) r.markerSamples++;
    prev = d.markerOn;
  }
  return r;
}

static int checks = 0, fails = 0;
static void expect(const char *what, bool ok) { checks++; if (!ok) fails++; printf("%s  %s\n", ok ? "PASS" : "FAIL", what); }
static bool is(const Result &r, std::vector<LineEvent> want) { return r.events == want; }

// ---- full synthetic lap: start marker, 4 corners, 3 left branches, 3 right branches, return to the marker ----
static std::vector<Seg> lap() {
  std::vector<Seg> v;
  v.push_back(marker(8)); v.push_back(straight(30));
  v.push_back(cornerBurst(6)); v.push_back(lost(4));                       // corner 1 (bottom -> left)
  for (int i = 0; i < 3; i++) { v.push_back(straight(25)); v.push_back(leftBranch(6)); }
  v.push_back(straight(25)); v.push_back(cornerBurst(6)); v.push_back(lost(4));   // corner 2 (left -> top)
  v.push_back(straight(60)); v.push_back(cornerBurst(6)); v.push_back(lost(4));   // corner 3 (top -> right)
  for (int i = 0; i < 3; i++) { v.push_back(straight(25)); v.push_back(rightBranch(6)); }
  v.push_back(straight(25)); v.push_back(cornerBurst(6)); v.push_back(lost(4));   // corner 4 (right -> bottom)
  v.push_back(straight(30)); v.push_back(marker(8)); v.push_back(straight(10));
  return v;
}

int main() {
  // 1. straight line, assumed noise up to +-5: nothing is detected
  for (int amp = 0; amp <= 5; amp++) {
    Result r = runDetector(build({straight(2000)}, amp));
    char name[96]; snprintf(name, sizeof(name), "straight line, noise +-%d: no event, no marker", amp);
    expect(name, r.events.empty() && r.markerRises == 0);
  }
  // 2. single events
  expect("left branch -> one EV_JUNCTION_LEFT", is(runDetector(build({straight(20), leftBranch(6), straight(20)}, 2)), {EV_JUNCTION_LEFT}));
  expect("right branch -> one EV_JUNCTION_RIGHT", is(runDetector(build({straight(20), rightBranch(6), straight(20)}, 2)), {EV_JUNCTION_RIGHT}));
  expect("right burst then the line ahead is lost -> EV_CORNER", is(runDetector(build({straight(20), cornerBurst(6), lost(8)}, 2)), {EV_CORNER}));
  expect("corner where the line ahead is lost during the burst -> EV_CORNER", is(runDetector(build({straight(20), {WHITE, FULL, FULL, 4}, {WHITE, WHITE, FULL, 3}, lost(6)}, 1)), {EV_CORNER}));
  Result m = runDetector(build({straight(20), marker(8), straight(20)}, 2));
  expect("marker -> level on for the whole stay, no junction/corner event", m.markerRises == 1 && m.events.empty() && m.markerSamples >= 6);
  // 3. glitches
  expect("1-sample spike on the left sensor: ignored", runDetector(build({straight(20), {50, FULL, EDGE, 1}, straight(20)}, 0)).events.empty());
  expect("2-sample spike: ignored (needs LINE_CONFIRM_SAMPLES=3)", runDetector(build({straight(20), {50, FULL, EDGE, 2}, straight(20)}, 0)).events.empty());
  expect("2-sample all-high spike (seam step): no marker", runDetector(build({straight(20), marker(2), straight(20)}, 0)).markerRises == 0);
  expect("KNOWN LIMIT: a 3-sample spike counts as a junction", is(runDetector(build({straight(20), {50, FULL, EDGE, 3}, straight(20)}, 0)), {EV_JUNCTION_LEFT}));
  {  // steering wobble: side sensors swing between 30 and 38, never reach 40
    std::vector<Seg> w; for (int i = 0; i < 100; i++) w.push_back({30 + (i % 9), FULL, 38 - (i % 9), 1});
    expect("steering wobble (sides 30..38): nothing detected", runDetector(build(w, 0)).events.empty());
  }
  expect("line lost completely (all white): nothing detected", runDetector(build({straight(20), lost(50), straight(20)}, 2)).events.empty());
  // 4. marker approach / departure with the sides not switching at the same sample
  expect("marker entered with right side first (4 samples earlier): marker, no junction", [&] {
    Result r = runDetector(build({straight(20), {EDGE, FULL, FULL, 4}, marker(8), {FULL, FULL, EDGE, 4}, straight(20)}, 0));
    return r.markerRises == 1 && r.events.empty(); }());
  // 5. full lap
  for (int amp = 0; amp <= 5; amp += 5) {
    Result r = runDetector(build(lap(), amp));
    std::vector<LineEvent> want = {EV_CORNER, EV_JUNCTION_LEFT, EV_JUNCTION_LEFT, EV_JUNCTION_LEFT, EV_CORNER, EV_CORNER,
                                   EV_JUNCTION_RIGHT, EV_JUNCTION_RIGHT, EV_JUNCTION_RIGHT, EV_CORNER};
    char name[96]; snprintf(name, sizeof(name), "full lap, noise +-%d: 4 corners, 3 left + 3 right junctions in order, 2 markers", amp);
    expect(name, is(r, want) && r.markerRises == 2);
  }

  // ---------- glue (real forklift_sense.ino): fake analog pins + clock, called the way the state machine calls it ----------
  {
    auto feed = [&](const LineSample &s) { g_a[0] = s.l; g_a[1] = s.c; g_a[2] = s.r; g_now += LINE_SAMPLE_MS; };
    auto trace = build(lap(), 2);
    int corners = 0, junctions[4] = {0, 0, 0, 0}, markerRises = 0; bool prevMarker = false;
    segment = 0; lineDet = LineDetector(); lineEvent = EV_NONE;
    for (const LineSample &s : trace) {
      feed(s);
      if (senseCorner()) { corners++; segment = (segment + 1) % 4; }       // like handleCorner()
      if (senseJunction()) junctions[segment]++;                           // like countJunction() (it counts only the target segment)
      bool mk = senseMarker(); if (mk && !prevMarker) markerRises++; prevMarker = mk;
    }
    expect("glue, full lap: 4 corners reported", corners == 4);
    expect("glue, full lap: 3 junctions on the left segment, 3 on the right, none on bottom/top", junctions[1] == 3 && junctions[3] == 3 && junctions[0] == 0 && junctions[2] == 0);
    expect("glue, full lap: marker level rose twice (start and end)", markerRises == 2);
    expect("glue: after the lap segment is back to 0", segment == 0);
  }
  {  // events that nobody takes are dropped after LINE_EVENT_TTL_MS; wrong-segment junctions are not delivered
    auto feed = [&](const LineSample &s) { g_a[0] = s.l; g_a[1] = s.c; g_a[2] = s.r; g_now += LINE_SAMPLE_MS; };
    segment = 1; lineDet = LineDetector(); lineEvent = EV_NONE;
    for (const LineSample &s : build({straight(10), leftBranch(6), straight(4)}, 0)) { feed(s); senseMarker(); }   // only polls
    g_now += LINE_EVENT_TTL_MS + 50;
    expect("glue: untaken junction event expires after the TTL", !senseJunction());
    segment = 2; lineDet = LineDetector(); lineEvent = EV_NONE;
    bool any = false;
    for (const LineSample &s : build({straight(10), leftBranch(6), straight(4)}, 0)) { feed(s); any = any || senseJunction(); }
    expect("glue: no junction is delivered on the top segment", !any);
    segment = 1; lineDet = LineDetector(); lineEvent = EV_NONE; any = false;
    for (const LineSample &s : build({straight(10), rightBranch(6), straight(4)}, 0)) { feed(s); any = any || senseJunction(); }
    expect("glue: a right branch is not a junction on the left segment", !any);
    segment = 0; lineDet = LineDetector(); lineEvent = EV_NONE;
    bool mk = false;
    for (int i = 0; i < 4; i++) { feed({FULL, FULL, FULL}); mk = senseMarker(); }   // one poll per sample, like loop() does
    expect("glue: starting ON the marker, senseMarker() is true after LINE_CONFIRM_SAMPLES polls (level, for markerArmed)", mk);
    for (int i = 0; i < 2; i++) { feed({EDGE, FULL, EDGE}); mk = senseMarker(); }
    expect("glue: leaving the marker, senseMarker() turns false", !mk);
  }

  printf("%d checks, %d failed\n", checks, fails);
  if (fails) return 1;
  puts("all passed");
  return 0;
}
