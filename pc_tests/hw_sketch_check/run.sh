#!/usr/bin/env bash
# Structure + syntax check of the test_forklift_*/ hardware test sketches (PC only, NOT an Arduino IDE compile).
set -euo pipefail
python3 -I "$(cd "$(dirname "$0")" && pwd)/check_test_sketches.py"
