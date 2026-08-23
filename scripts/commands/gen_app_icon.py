#!/usr/bin/env python3
"""Generate app icon (.ico) and Windows resource (.rc) files."""

import argparse
import logging
import pathlib
import sys

# Support direct script execution without PYTHONPATH set
_SCRIPTS_DIR = pathlib.Path(__file__).resolve().parent.parent
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.icon.generate import generate
from autoinput_tools.paths import (
    DEFAULT_APP_ICON_PNG,
    GENERATED_APP_ICON_ICO,
    GENERATED_APP_ICON_RC,
)

logger = logging.getLogger(__name__)


def get_parser() -> argparse.ArgumentParser:
    """Create the command-line parser."""
    parser = argparse.ArgumentParser(
        description="Generate app icon (.ico) and Windows resource (.rc) files.",
    )
    parser.add_argument(
        "--png",
        "--icon",
        dest="png",
        type=pathlib.Path,
        default=DEFAULT_APP_ICON_PNG,
        help=f"Path to source PNG icon file (default: {DEFAULT_APP_ICON_PNG})",
    )
    parser.add_argument(
        "--ico",
        type=pathlib.Path,
        default=GENERATED_APP_ICON_ICO,
        help=f"Path to output ICO file (default: {GENERATED_APP_ICON_ICO})",
    )
    parser.add_argument(
        "--rc",
        type=pathlib.Path,
        default=GENERATED_APP_ICON_RC,
        help=f"Path to output RC file (default: {GENERATED_APP_ICON_RC})",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check whether generated files are up to date without modifying them.",
    )
    return parser


def main() -> int:
    """Run the app icon generation command."""
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    args = get_parser().parse_args()

    if not generate(
        png_path=args.png,
        ico_path=args.ico,
        rc_path=args.rc,
        check_only=args.check,
    ):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
