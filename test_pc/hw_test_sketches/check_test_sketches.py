#!/usr/bin/env python3
"""Structure + syntax checks for test_forklift_board_hardware/<sketch>/ (PC only, NOT an Arduino IDE compile).
Checks per sketch folder: exactly one .ino, named like the folder (single-file sketch); no .h files; exactly one
setup()/loop(); one "SERIAL TEST SECTION" marker, and the code above it (pins/hardware functions) never touches
USB Serial; string literals are ASCII (English); the file passes g++ -fsyntax-only -Wall -Wextra against the
fake headers in ./fake_arduino_headers."""
import re, subprocess, sys, tempfile
from pathlib import Path

here = Path(__file__).resolve().parent
root = here.parent.parent / "test_forklift_board_hardware"
stubs = here / "fake_arduino_headers"
SERIAL_MARK = "// SERIAL TEST SECTION"  # banner line; the file header only mentions the name inside a block comment
MAX_LINES = 300  # PROBLEM_1PAGER.md rule: file <= 300 lines


def strip_comments(src):
    out, i, n, mode = [], 0, len(src), None
    while i < n:
        c, d = src[i], src[i:i + 2]
        if mode is None:
            if d == "//": mode = "line"; i += 2; continue
            if d == "/*": mode = "block"; i += 2; continue
            if c == '"': mode = "str"
            out.append(c)
        elif mode == "line":
            if c == "\n": mode = None; out.append(c)
        elif mode == "block":
            if d == "*/": mode = None; i += 2; continue
            if c == "\n": out.append(c)
        elif mode == "str":
            out.append(c)
            if c == "\\": out.append(src[i + 1]); i += 2; continue
            if c == '"': mode = None
        i += 1
    return "".join(out)


fails = 0
def fail(msg):
    global fails
    fails += 1
    print("FAIL", msg)

sketches = sorted(p for p in root.iterdir() if p.is_dir())
if not sketches:
    fail("no sketch folders in %s" % root)
for d in sketches:
    main = d / (d.name + ".ino")
    if not main.exists():
        fail("%s: main file %s.ino missing" % (d.name, d.name)); continue
    if list(d.glob("*.h")) or list(d.glob("*.cpp")):
        fail("%s: .h/.cpp present (project rule: no separate headers)" % d.name)
    tabs = [main]
    others = sorted(p.name for p in d.glob("*.ino") if p != main)
    if others:
        fail("%s: extra .ino tabs %s (single-file sketch expected)" % (d.name, others))
    raw = main.read_text(encoding="utf-8")
    if len(raw.splitlines()) > MAX_LINES:
        fail("%s: %d lines (> %d)" % (d.name, len(raw.splitlines()), MAX_LINES))
    if len(re.findall(r"^" + re.escape(SERIAL_MARK), raw, re.M)) != 1:
        fail("%s: marker %r must appear exactly once" % (d.name, SERIAL_MARK))
        head = raw
    else:
        head = raw.split("\n" + SERIAL_MARK)[0]
    if re.search(r"\bSerial\b", strip_comments(head)):
        fail("%s: USB Serial used in code above the serial test section" % d.name)
    code = strip_comments(raw)
    for name in ("setup", "loop"):
        if len(re.findall(r"\bvoid\s+%s\s*\(" % name, code)) != 1:
            fail("%s: %s() must be defined exactly once" % (d.name, name))
    for lit in re.findall(r'"((?:[^"\\]|\\.)*)"', code):
        if any(ord(ch) > 127 for ch in lit):
            fail("%s: non-ASCII string literal %r" % (d.name, lit))
    with tempfile.TemporaryDirectory() as tmp:
        cpp = Path(tmp) / "sketch.cpp"
        cpp.write_text('#include <Arduino.h>\n' + "\n".join(p.read_text(encoding="utf-8") for p in tabs), encoding="utf-8")
        r = subprocess.run(["g++", "-std=gnu++11", "-fsyntax-only", "-x", "c++", "-Wall", "-Wextra", "-I", str(stubs), str(cpp)],
                           capture_output=True, text=True)
        if r.returncode != 0 or r.stderr.strip():
            fail("%s: g++ syntax check\n%s" % (d.name, r.stderr))
    if not fails:
        pass
    print("checked", d.name, [p.name for p in tabs])
print("sketches checked: %d, failures: %d" % (len(sketches), fails))
sys.exit(1 if fails else 0)
