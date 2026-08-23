#!/usr/bin/env python3
"""Generate C++ localization ID files."""

import argparse
import logging
import pathlib
import sys

# Support direct script execution without PYTHONPATH set
_SCRIPTS_DIR = pathlib.Path(__file__).resolve().parent.parent
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.localization.generate import generate
from autoinput_tools.paths import (
    DEFAULT_LOCALIZATION_FILE,
    GENERATED_LOCALIZATION_HEADER,
    GENERATED_LOCALIZATION_SOURCE,
)

logger = logging.getLogger(__name__)


def get_parser() -> argparse.ArgumentParser:
    """Create the command-line parser."""
    parser = argparse.ArgumentParser(
        description="Generate C++ header and source files with constexpr localization IDs.",
    )
    parser.add_argument(
        "--loc",
        "--toml",
        dest="loc",
        type=pathlib.Path,
        default=DEFAULT_LOCALIZATION_FILE,
        help=f"Path to source TOML file (default: {DEFAULT_LOCALIZATION_FILE})",
    )
    parser.add_argument(
        "--header",
        type=pathlib.Path,
        default=GENERATED_LOCALIZATION_HEADER,
        help=f"Path to output header file (default: {GENERATED_LOCALIZATION_HEADER})",
    )
    parser.add_argument(
        "--source",
        type=pathlib.Path,
        default=GENERATED_LOCALIZATION_SOURCE,
        help=f"Path to output source file (default: {GENERATED_LOCALIZATION_SOURCE})",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check if output files are up to date without modifying them.",
    )
    return parser


def main() -> int:
    """Run the localization ID generation command."""
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    args = get_parser().parse_args()

    if not generate(
        loc_file_path=args.loc,
        header_path=args.header,
        source_path=args.source,
        check_only=args.check,
    ):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
