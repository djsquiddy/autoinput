#!/usr/bin/env bash
set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

pushd "${SCRIPT_DIR}" > /dev/null
if command -v python3 >/dev/null 2>&1; then
    python3 -m build "$@"
elif command -v python >/dev/null 2>&1; then
    python -m build "$@"
else
    echo "Error: Python is required to run build.py" >&2
    exit 1
fi
popd > /dev/null
