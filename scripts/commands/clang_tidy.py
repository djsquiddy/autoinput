"""Run the CMake-backed clang-tidy workflow for AutoInput."""

import argparse
import logging
import pathlib
import sys

try:
    import _bootstrap  # noqa: F401
except ImportError:
    from . import _bootstrap  # noqa: F401

from autoinput_tools.tidy import run_clang_tidy

logger = logging.getLogger(__name__)


def get_parser() -> argparse.ArgumentParser:
    """Create command-line parser for clang-tidy tool."""
    parser = argparse.ArgumentParser(
        description="Run CMake-backed clang-tidy static analysis for AutoInput.",
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
        "-t",
        "--target",
        default="clang-tidy-check",
        help="CMake target to build (default: clang-tidy-check).",
    )
    parser.add_argument(
        "--build",
        dest="build_with_tidy",
        action="store_true",
        help="Configure with -DAUTOINPUT_ENABLE_CLANG_TIDY=ON and compile targets with clang-tidy enabled.",
    )
    parser.add_argument(
        "-c",
        "--clean",
        action="store_true",
        help="Clean the build directory before running.",
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
    """Run the clang-tidy command."""
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    args, extra = get_parser().parse_known_args()
    all_extra = (args.extra_cmake_args or []) + extra

    return run_clang_tidy(
        build_dir=args.build_dir,
        preset=args.preset,
        target=args.target,
        build_with_tidy=args.build_with_tidy,
        clean=args.clean,
        configure_if_missing=args.configure_if_missing,
        extra_args=all_extra if all_extra else None,
    )


if __name__ == "__main__":
    sys.exit(main())
