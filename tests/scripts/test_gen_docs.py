#!/usr/bin/env python3
"""Unit tests for commands/gen_docs.py."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.paths import (
    BUILD_DIR,
    DOCS_API_DIR,
    SRC_DIR,
)
from commands.gen_docs import get_parser, main


def test_parser_defaults() -> None:
    parser = get_parser()
    args = parser.parse_args([])
    # Verify default arguments
    assert args.src_dir == SRC_DIR
    assert args.output_dir == DOCS_API_DIR
    assert args.config == BUILD_DIR / "Doxyfile"
    assert args.markdown_only is False
    assert args.check is False


def test_parser_custom_args() -> None:
    parser = get_parser()
    args = parser.parse_args([
        "--src-dir", "custom/src",
        "--output-dir", "custom/docs",
        "--config", "custom/Doxyfile",
        "--markdown-only",
        "--check",
    ])
    assert args.src_dir == pathlib.Path("custom/src")
    assert args.output_dir == pathlib.Path("custom/docs")
    assert args.config == pathlib.Path("custom/Doxyfile")
    assert args.markdown_only is True
    assert args.check is True


def test_gen_docs_main_markdown_fallback(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    src_dir = tmp_path / "src"
    src_dir.mkdir()
    out_dir = tmp_path / "docs" / "api"
    (src_dir / "test.h").write_text("/** Main header */\nclass Test {};\n", encoding="utf-8")

    test_args = [
        "gen_docs.py",
        "--src-dir", str(src_dir),
        "--output-dir", str(out_dir),
        "--markdown-only",
    ]
    monkeypatch.setattr(sys, "argv", test_args)
    ret = main()
    assert ret == 0
    assert (out_dir / "test.md").exists()


def test_gen_docs_main_check_mode(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    src_dir = tmp_path / "src"
    src_dir.mkdir()
    out_dir = tmp_path / "docs" / "api"
    (src_dir / "test.h").write_text("/** Main header */\nclass Test {};\n", encoding="utf-8")

    # Generate first
    monkeypatch.setattr(sys, "argv", [
        "gen_docs.py",
        "--src-dir", str(src_dir),
        "--output-dir", str(out_dir),
        "--markdown-only",
    ])
    assert main() == 0

    # Check matches
    monkeypatch.setattr(sys, "argv", [
        "gen_docs.py",
        "--src-dir", str(src_dir),
        "--output-dir", str(out_dir),
        "--markdown-only",
        "--check",
    ])
    assert main() == 0

    # Out of date check
    (src_dir / "test.h").write_text("/** Modified header */\nclass Test {};\n", encoding="utf-8")
    assert main() == 1
