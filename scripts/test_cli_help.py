#!/usr/bin/env python
"""Plain-assert validation tests for scripts/cli_help.py.

Run with: python scripts/test_cli_help.py
(There is no pytest setup in this project; this mirrors that convention.)
"""
import pathlib
import sys
import tempfile

try:
    from . import utils
    from .cli_help import load_cli_help_metadata, CliHelpValidationError
except ImportError:
    sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
    import utils
    from cli_help import load_cli_help_metadata, CliHelpValidationError

REAL_HELP_TOML = utils.RESOURCE_DIR / "cli" / "help.toml"


def _load_toml_text(text: str):
    with tempfile.TemporaryDirectory() as tmp:
        path = pathlib.Path(tmp) / "help.toml"
        path.write_text(text, encoding="utf-8")
        return load_cli_help_metadata(path)


def _assert_raises(text: str, expected_substring: str, test_name: str) -> None:
    try:
        _load_toml_text(text)
    except CliHelpValidationError as e:
        assert expected_substring.lower() in str(e).lower(), (
            f"{test_name}: expected error message to contain {expected_substring!r}, got: {e}"
        )
        print(f"PASS: {test_name}")
        return
    raise AssertionError(f"{test_name}: expected CliHelpValidationError, but none was raised")


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

    print("PASS: test_real_help_toml_loads")


def test_missing_names() -> None:
    _assert_raises(
        """
        [app]
        name = "autoinput"
        summary = "test"

        [[global_options]]
        value = false
        description = "Show help"
        """,
        "names",
        "test_missing_names",
    )


def test_empty_names() -> None:
    _assert_raises(
        """
        [app]
        name = "autoinput"
        summary = "test"

        [[global_options]]
        names = []
        value = false
        description = "Show help"
        """,
        "names",
        "test_empty_names",
    )


def test_duplicate_global_option_names() -> None:
    _assert_raises(
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
        "duplicate",
        "test_duplicate_global_option_names",
    )


def test_duplicate_option_names_within_command() -> None:
    _assert_raises(
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
        "duplicate",
        "test_duplicate_option_names_within_command",
    )


def test_value_false_with_value_name() -> None:
    _assert_raises(
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
        "value_name",
        "test_value_false_with_value_name",
    )


def test_unknown_completion() -> None:
    _assert_raises(
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
        "completion",
        "test_unknown_completion",
    )


def test_completion_reference_to_completions_table() -> None:
    metadata = _load_toml_text(
        """
        [app]
        name = "autoinput"
        summary = "test"

        [[global_options]]
        names = ["-l", "--log"]
        value = true
        value_name = "LEVEL"
        description = "Set log level"
        completion = "log_levels"

        [completions]
        log_levels = ["debug", "info"]
        """
    )
    assert metadata.global_options[0].completion == "log_levels"
    print("PASS: test_completion_reference_to_completions_table")


def test_duplicate_top_level_command_names() -> None:
    _assert_raises(
        """
        [app]
        name = "autoinput"
        summary = "test"

        [[commands]]
        name = "run"
        usage = "run"
        description = "Run stuff."

        [[commands]]
        name = "run"
        usage = "run"
        description = "Run stuff again."
        """,
        "duplicate",
        "test_duplicate_top_level_command_names",
    )


def test_duplicate_subcommand_names() -> None:
    _assert_raises(
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
        "duplicate",
        "test_duplicate_subcommand_names",
    )


def test_missing_command_description() -> None:
    _assert_raises(
        """
        [app]
        name = "autoinput"
        summary = "test"

        [[commands]]
        name = "run"
        usage = "run"
        description = ""
        """,
        "description",
        "test_missing_command_description",
    )


def test_missing_option_description() -> None:
    _assert_raises(
        """
        [app]
        name = "autoinput"
        summary = "test"

        [[global_options]]
        names = ["-h", "--help"]
        description = ""
        """,
        "description",
        "test_missing_option_description",
    )


def test_missing_subcommand_description() -> None:
    _assert_raises(
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
        "description",
        "test_missing_subcommand_description",
    )


def main() -> int:
    tests = [
        test_real_help_toml_loads,
        test_missing_names,
        test_empty_names,
        test_duplicate_global_option_names,
        test_duplicate_option_names_within_command,
        test_value_false_with_value_name,
        test_unknown_completion,
        test_completion_reference_to_completions_table,
        test_duplicate_top_level_command_names,
        test_duplicate_subcommand_names,
        test_missing_command_description,
        test_missing_option_description,
        test_missing_subcommand_description,
    ]

    for test in tests:
        test()

    print(f"\nAll {len(tests)} cli_help tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
