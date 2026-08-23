#!/usr/bin/env python3
"""Generate C++ CLI help metadata files."""

import argparse
import logging
import pathlib
import sys

# Support direct script execution without PYTHONPATH set
_SCRIPTS_DIR = pathlib.Path(__file__).resolve().parent.parent
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.cli.generate import generate
from autoinput_tools.paths import (
    DEFAULT_CLI_HELP_METADATA_FILE,
    GENERATED_CLI_HELP_HEADER,
    GENERATED_CLI_HELP_SOURCE,
)

logger = logging.getLogger(__name__)


def get_parser() -> argparse.ArgumentParser:
    """Create the command-line parser."""
    parser = argparse.ArgumentParser(
        description="Generate C++ CLI help metadata files.",
    )
    parser.add_argument(
        "--metadata",
        "--toml",
        dest="metadata",
        type=pathlib.Path,
        default=DEFAULT_CLI_HELP_METADATA_FILE,
        help=f"Path to CLI help TOML metadata file (default: {DEFAULT_CLI_HELP_METADATA_FILE})",
    )
    parser.add_argument(
        "--header",
        type=pathlib.Path,
        default=GENERATED_CLI_HELP_HEADER,
        help=f"Path to output header file (default: {GENERATED_CLI_HELP_HEADER})",
    )
    parser.add_argument(
        "--source",
        type=pathlib.Path,
        default=GENERATED_CLI_HELP_SOURCE,
        help=f"Path to output source file (default: {GENERATED_CLI_HELP_SOURCE})",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Check whether generated files are up to date without modifying them.",
    )
    return parser


def main() -> int:
    """Run the CLI help metadata generation command."""
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    args = get_parser().parse_args()

    if not generate(
        metadata_path=args.metadata,
        header_path=args.header,
        source_path=args.source,
        check_only=args.check,
    ):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
