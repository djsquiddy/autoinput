#!/usr/bin/env python3
"""Unit tests for commands/gen_localization_ids.py."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.paths import (
    DEFAULT_LOCALIZATION_FILE,
    GENERATED_LOCALIZATION_HEADER,
    GENERATED_LOCALIZATION_SOURCE,
)
from commands.gen_localization_ids import get_parser, main


def test_parser_defaults() -> None:
    parser = get_parser()
    args = parser.parse_args([])
    assert args.loc == DEFAULT_LOCALIZATION_FILE
    assert args.header == GENERATED_LOCALIZATION_HEADER
    assert args.source == GENERATED_LOCALIZATION_SOURCE
    assert args.check is False


def test_parser_custom_args() -> None:
    parser = get_parser()
    args = parser.parse_args(["--loc", "custom.toml", "--header", "out.h", "--source", "out.cpp", "--check"])
    assert args.loc == pathlib.Path("custom.toml")
    assert args.header == pathlib.Path("out.h")
    assert args.source == pathlib.Path("out.cpp")
    assert args.check is True


def test_gen_localization_ids_main_success(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    loc_file = tmp_path / "en-US.toml"
    loc_file.write_text(
        """
        [ui]
        ok = "OK"
        cancel = "Cancel"
        """,
        encoding="utf-8",
    )
    header_file = tmp_path / "loc.h"
    source_file = tmp_path / "loc.cpp"

    test_args = [
        "gen_localization_ids.py",
        "--loc", str(loc_file),
        "--header", str(header_file),
        "--source", str(source_file),
    ]
    monkeypatch.setattr(sys, "argv", test_args)
    ret = main()
    assert ret == 0
    assert header_file.exists()
    assert source_file.exists()


def test_gen_localization_ids_main_check_mode(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    loc_file = tmp_path / "en-US.toml"
    loc_file.write_text(
        """
        [ui]
        ok = "OK"
        """,
        encoding="utf-8",
    )
    header_file = tmp_path / "loc.h"
    source_file = tmp_path / "loc.cpp"

    # First generate
    monkeypatch.setattr(sys, "argv", ["gen_localization_ids.py", "--loc", str(loc_file), "--header", str(header_file), "--source", str(source_file)])
    assert main() == 0

    # Then check
    monkeypatch.setattr(sys, "argv", ["gen_localization_ids.py", "--loc", str(loc_file), "--header", str(header_file), "--source", str(source_file), "--check"])
    assert main() == 0
