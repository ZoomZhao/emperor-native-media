#!/bin/bash
# Real shared AppMedia control path with a fake audio backend; no AppKit/AVAudioPlayer execution.
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
MODE="${1:-release}"
if [ "$#" -gt 0 ]; then shift; fi
exec /usr/bin/python3 -I -B "$ROOT/scripts/independent_runner.py" integration --mode "$MODE" "$@"
