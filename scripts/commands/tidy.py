#!/usr/bin/env python3
"""Alias for commands/clang_tidy.py."""

import pathlib
import sys

try:
    import _bootstrap  # noqa: F401
except ImportError:
    from . import _bootstrap  # noqa: F401

from commands.clang_tidy import get_parser, main

if __name__ == "__main__":
    sys.exit(main())
