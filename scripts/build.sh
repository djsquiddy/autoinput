#!/usr/bin/env bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if command -v python3 >/dev/null 2>&1; then
    python3 "$SCRIPT_DIR/build.py" "$@"
elif command -v python >/dev/null 2>&1; then
    python "$SCRIPT_DIR/build.py" "$@"
else
    echo "Error: Python is required to run build.py" >&2
    exit 1
fi
