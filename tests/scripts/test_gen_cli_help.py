#!/usr/bin/env python3
"""Unit tests for commands/gen_cli_help.py."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.paths import (
    DEFAULT_CLI_HELP_METADATA_FILE,
    GENERATED_CLI_HELP_HEADER,
    GENERATED_CLI_HELP_SOURCE,
)
from commands.gen_cli_help import get_parser, main


def test_parser_defaults() -> None:
    parser = get_parser()
    args = parser.parse_args([])
    assert args.metadata == DEFAULT_CLI_HELP_METADATA_FILE
    assert args.header == GENERATED_CLI_HELP_HEADER
    assert args.source == GENERATED_CLI_HELP_SOURCE
    assert args.check is False


def test_parser_custom_args() -> None:
    parser = get_parser()
    args = parser.parse_args(["--metadata", "custom.toml", "--header", "out.h", "--source", "out.cpp", "--check"])
    assert args.metadata == pathlib.Path("custom.toml")
    assert args.header == pathlib.Path("out.h")
    assert args.source == pathlib.Path("out.cpp")
    assert args.check is True


def test_gen_cli_help_main_success(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    metadata_file = tmp_path / "help.toml"
    metadata_file.write_text(
        """
        [app]
        name = "testapp"
        summary = "Summary"

        [[global_options]]
        names = ["-h", "--help"]
        value = false
        description = "Show help"
        """,
        encoding="utf-8",
    )
    header_file = tmp_path / "help.h"
    source_file = tmp_path / "help.cpp"

    test_args = [
        "gen_cli_help.py",
        "--metadata", str(metadata_file),
        "--header", str(header_file),
        "--source", str(source_file),
    ]
    monkeypatch.setattr(sys, "argv", test_args)
    ret = main()
    assert ret == 0
    assert header_file.exists()
    assert source_file.exists()


def test_gen_cli_help_main_check_mode(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    metadata_file = tmp_path / "help.toml"
    metadata_file.write_text(
        """
        [app]
        name = "testapp"
        summary = "Summary"

        [[global_options]]
        names = ["-h", "--help"]
        value = false
        description = "Show help"
        """,
        encoding="utf-8",
    )
    header_file = tmp_path / "help.h"
    source_file = tmp_path / "help.cpp"

    # First generate
    monkeypatch.setattr(sys, "argv", ["gen_cli_help.py", "--metadata", str(metadata_file), "--header", str(header_file), "--source", str(source_file)])
    assert main() == 0

    # Then check
    monkeypatch.setattr(sys, "argv", ["gen_cli_help.py", "--metadata", str(metadata_file), "--header", str(header_file), "--source", str(source_file), "--check"])
    assert main() == 0
