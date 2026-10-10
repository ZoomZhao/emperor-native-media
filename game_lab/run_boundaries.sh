#!/bin/bash
# Pure synthetic media suite; excludes original assets, SG3/map and App/device execution.
set -euo pipefail
DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$DIR/.." && pwd)"
MODE="${1:-release}"
if [ "$#" -gt 0 ]; then shift; fi
exec /usr/bin/python3 -I -B "$ROOT/scripts/independent_runner.py" media --mode "$MODE" "$@"
