#!/usr/bin/env python3
"""Generate code documentation with Doxygen and Markdown fallback."""

import argparse
import logging
import pathlib
import sys

try:
    import _bootstrap  # noqa: F401
except ImportError:
    from . import _bootstrap  # noqa: F401

from autoinput_tools.paths import SRC_DIR, PROJECT_ROOT, BUILD_DIR
from autoinput_tools.docs.generator import generate_docs

logger = logging.getLogger(__name__)


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate code documentation (Doxygen with Markdown fallback).",
    )
    parser.add_argument(
        "--src-dir",
        type=pathlib.Path,
        default=SRC_DIR,
        help="Path to source directory to scan (default: src/)",
    )
    parser.add_argument(
        "--output-dir",
        type=pathlib.Path,
        default=PROJECT_ROOT / "docs" / "api",
        help="Path to output directory for Markdown docs (default: docs/api)",
    )
    parser.add_argument(
        "--config",
        type=pathlib.Path,
        default=BUILD_DIR / "Doxyfile",
        help="Path to generated Doxygen configuration file",
    )
    parser.add_argument(
        "--markdown-only",
        action="store_true",
        help="Force Markdown documentation extraction without invoking Doxygen",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Verify documentation is up to date without writing changes",
    )
    return parser


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    args = get_parser().parse_args()

    success = generate_docs(
        src_dir=args.src_dir,
        output_dir=args.output_dir,
        doxygen_config=args.config,
        force_markdown=args.markdown_only,
        check_only=args.check,
    )
    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
