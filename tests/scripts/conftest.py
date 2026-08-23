"""Pytest configuration and shared fixtures for scripts test suite."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"

if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))
