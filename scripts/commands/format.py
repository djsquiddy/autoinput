"""Run the CMake-backed clang-format workflow for AutoInput."""

import argparse
import logging
import pathlib
import sys

try:
    import _bootstrap  # noqa: F401
except ImportError:
    from . import _bootstrap  # noqa: F401

from autoinput_tools.formatting import run_format

logger = logging.getLogger(__name__)


def get_parser() -> argparse.ArgumentParser:
    """Create command-line parser for format tool."""
    parser = argparse.ArgumentParser(
        description="Run CMake-backed clang-format formatting for AutoInput.",
    )
    parser.add_argument(
        "--check",
        "--dry-run",
        dest="check_only",
        action="store_true",
        help="Check formatting without modifying files (runs the 'format-check' CMake target).",
    )
    parser.add_argument(
        "-p",
        "--preset",
        default=None,
        help="CMake configure preset to use.",
    )
    parser.add_argument(
        "-B",
        "--build-dir",
        default=None,
        type=pathlib.Path,
        help="CMake build directory (default: build/ or preset binaryDir).",
    )
    parser.add_argument(
        "--no-configure",
        dest="configure_if_missing",
        action="store_false",
        help="Do not automatically configure CMake if the build directory is not configured.",
    )
    parser.add_argument(
        "extra_cmake_args",
        nargs="*",
        help="Additional arguments to pass to CMake.",
    )
    return parser


def main() -> int:
    """Run the format command."""
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    args, extra = get_parser().parse_known_args()
    all_extra = (args.extra_cmake_args or []) + extra

    return run_format(
        build_dir=args.build_dir,
        preset=args.preset,
        check_only=args.check_only,
        configure_if_missing=args.configure_if_missing,
        extra_args=all_extra if all_extra else None,
    )


if __name__ == "__main__":
    sys.exit(main())
