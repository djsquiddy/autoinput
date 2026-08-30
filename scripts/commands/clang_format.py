#!/usr/bin/env python3
"""Alias for commands/format.py."""

import pathlib
import sys

try:
    import _bootstrap  # noqa: F401
except ImportError:
    from . import _bootstrap  # noqa: F401

from commands.format import get_parser, main

if __name__ == "__main__":
    sys.exit(main())
