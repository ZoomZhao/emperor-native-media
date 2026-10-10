#!/bin/bash
# Exact synthetic data/TLV suite, with per-process deadline and task-local inputs.
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
MODE="${1:-release}"
if [ "$#" -gt 0 ]; then shift; fi
exec /usr/bin/python3 -I -B "$ROOT/scripts/independent_runner.py" data --mode "$MODE" "$@"
