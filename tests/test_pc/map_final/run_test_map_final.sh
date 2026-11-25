#!/usr/bin/env bash
# Checks the final map plan in map/map_final/ (placement, kerf, geometry, 100x100 loop). Needs python3.
set -euo pipefail
python3 -I "$(cd "$(dirname "$0")" && pwd)/test_base150.py"
