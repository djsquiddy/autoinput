#!/usr/bin/env python3
"""Pytest tests for AutoInput CLI help generation and validation."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.cli.toml import load_cli_help_metadata
from autoinput_tools.cli.validation import CliHelpValidationError
from autoinput_tools.paths import DEFAULT_CLI_HELP_METADATA_FILE

REAL_HELP_TOML = DEFAULT_CLI_HELP_METADATA_FILE


def _load_toml_text(tmp_path: pathlib.Path, text: str):
    file_path = tmp_path / "help.toml"
    file_path.write_text(text, encoding="utf-8")
    return load_cli_help_metadata(file_path)


def test_real_help_toml_loads() -> None:
    metadata = load_cli_help_metadata(REAL_HELP_TOML)
    assert metadata.app_name == "autoinput"
    assert metadata.app_summary
    assert len(metadata.global_options) == 4
    command_names = {c.name for c in metadata.commands}
    assert command_names == {"run", "record", "config", "apps", "serve", "help"}

    config_cmd = next(c for c in metadata.commands if c.name == "config")
    subcommand_names = {s.name for s in config_cmd.subcommands}
    assert subcommand_names == {"list", "validate", "duplicate", "copy", "path"}

    assert "log_levels" in metadata.completions
    assert "action_types" in metadata.completions
    assert "mouse_buttons" in metadata.completions
    assert "notification_modes" in metadata.completions


def test_missing_names(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)names"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[global_options]]
            value = false
            description = "Show help"
            """,
        )


def test_empty_names(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)names"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[global_options]]
            names = []
            value = false
            description = "Show help"
            """,
        )


def test_duplicate_global_option_names(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)duplicate"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[global_options]]
            names = ["-h", "--help"]
            description = "Show help"

            [[global_options]]
            names = ["--help"]
            description = "Also help"
            """,
        )


def test_duplicate_option_names_within_command(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)duplicate"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[commands]]
            name = "run"
            usage = "run"
            description = "Run stuff."

            [[commands.options]]
            names = ["-c", "--config"]
            description = "Config"

            [[commands.options]]
            names = ["--config"]
            description = "Duplicate config"
            """,
        )


def test_value_false_with_value_name(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)value_name"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[global_options]]
            names = ["--json"]
            value = false
            value_name = "MODE"
            description = "Output JSON"
            """,
        )


def test_unknown_completion(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)completion"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[global_options]]
            names = ["-l", "--log"]
            value = true
            value_name = "LEVEL"
            description = "Set log level"
            completion = "not_a_real_completion"
            """,
        )


def test_completion_reference_to_completions_table(tmp_path: pathlib.Path) -> None:
    metadata = _load_toml_text(
        tmp_path,
        """
        [app]
        name = "autoinput"
        summary = "test"

        [completions]
        custom_types = ["foo", "bar"]

        [[global_options]]
        names = ["--custom"]
        value = true
        value_name = "TYPE"
        description = "Custom type"
        completion = "custom_types"
        """,
    )
    assert metadata.global_options[0].completion == "custom_types"


def test_duplicate_top_level_command_names(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)duplicate"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[commands]]
            name = "run"
            usage = "run"
            description = "Run commands."

            [[commands]]
            name = "run"
            usage = "run again"
            description = "Duplicate run command."
            """,
        )


def test_duplicate_subcommand_names(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)duplicate"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[commands]]
            name = "config"
            usage = "config"
            description = "Manage config."

            [[commands.subcommands]]
            name = "list"
            usage = "list"
            description = "List configs."

            [[commands.subcommands]]
            name = "list"
            usage = "list"
            description = "List configs again."
            """,
        )


def test_missing_command_description(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)description"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[commands]]
            name = "run"
            usage = "run"
            description = ""
            """,
        )


def test_missing_option_description(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)description"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[global_options]]
            names = ["-h", "--help"]
            description = ""
            """,
        )


def test_missing_subcommand_description(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)description"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[commands]]
            name = "config"
            usage = "config"
            description = "Manage config."

            [[commands.subcommands]]
            name = "list"
            usage = "list"
            description = ""
            """,
        )


def test_missing_app_name(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)name"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = ""
            summary = "test"
            """,
        )


def test_missing_command_usage(tmp_path: pathlib.Path) -> None:
    with pytest.raises(CliHelpValidationError, match=r"(?i)usage"):
        _load_toml_text(
            tmp_path,
            """
            [app]
            name = "autoinput"
            summary = "test"

            [[commands]]
            name = "run"
            usage = ""
            description = "Run commands."
            """,
        )


def test_file_not_found(tmp_path: pathlib.Path) -> None:
    non_existent = tmp_path / "does_not_exist.toml"
    with pytest.raises(FileNotFoundError):
        load_cli_help_metadata(non_existent)
