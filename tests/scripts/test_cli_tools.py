#!/usr/bin/env python3
"""Unit tests for autoinput_tools.cli package."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.cli.cpp import (
    CppGenerator,
    _cpp_string_view_array,
    _escape,
    generate_cli_help_content,
    generate_cli_help_header,
    generate_cli_help_source,
)
from autoinput_tools.cli.generate import generate
from autoinput_tools.cli.model import BUILTIN_COMPLETIONS, CliCommand, CliHelpMetadata, CliOption
from autoinput_tools.cli.toml import load_cli_help_metadata
from autoinput_tools.cli.validation import CliHelpValidationError, validate_cli_help_metadata


@pytest.fixture
def sample_metadata() -> CliHelpMetadata:
    return CliHelpMetadata(
        app_name="testapp",
        app_summary="A test application.",
        global_options=[
            CliOption(
                names=["-h", "--help"],
                value=False,
                value_name=None,
                description="Show help.",
                completion="none",
            ),
            CliOption(
                names=["-l", "--log"],
                value=True,
                value_name="LEVEL",
                description="Set log level.",
                completion="log_levels",
            ),
        ],
        commands=[
            CliCommand(
                name="run",
                usage="run [options]",
                description="Run the application.",
                options=[
                    CliOption(
                        names=["-c", "--config"],
                        value=True,
                        value_name="PATH",
                        description="Configuration file.",
                        completion="file",
                    )
                ],
                subcommands=[
                    CliCommand(
                        name="task",
                        usage="task [options]",
                        description="Run specific task.",
                        options=[],
                        subcommands=[],
                    )
                ],
            )
        ],
        completions={
            "log_levels": ["debug", "info", "warn", "error"],
        },
    )


def test_builtin_completions() -> None:
    # Verify standard built-in completion generator identifiers are present
    assert "file" in BUILTIN_COMPLETIONS
    assert "path" in BUILTIN_COMPLETIONS
    assert "none" in BUILTIN_COMPLETIONS
    assert "config" in BUILTIN_COMPLETIONS
    assert "application" in BUILTIN_COMPLETIONS


def test_cpp_escape() -> None:
    # Verify quotes and backslashes are escaped for C++ string literals
    assert _escape(r'hello "world" \ test') == r'hello \"world\" \\ test'


def test_cpp_string_view_array() -> None:
    empty_arr = _cpp_string_view_array([])
    # Verify empty string view array representation
    assert "{  }" in empty_arr
    items_arr = _cpp_string_view_array(["-h", "--help"])
    # Verify string_view literal suffixes ("sv") are appended to array elements
    assert '"-h"sv, "--help"sv' in items_arr


def test_cpp_generator_header(sample_metadata: CliHelpMetadata) -> None:
    header = generate_cli_help_header(sample_metadata)
    # Verify header guard, namespace, and struct definitions exist in generated C++ header
    assert "#pragma once" in header
    assert "namespace autoinput::cli::HelpMetadata" in header
    assert "struct CliOptionMetadata" in header
    assert "struct CliCommandMetadata" in header
    assert "findCommand" in header


def test_cpp_generator_source(sample_metadata: CliHelpMetadata) -> None:
    source = generate_cli_help_source(sample_metadata)
    # Verify namespace, app name, summary, and commands exist in generated C++ source
    assert "namespace autoinput::cli::HelpMetadata" in source
    assert "testapp" in source
    assert "A test application." in source
    assert "run" in source
    assert "task" in source


def test_generate_cli_help_content(sample_metadata: CliHelpMetadata) -> None:
    header, source = generate_cli_help_content(sample_metadata)
    # Verify both header and source contents are generated simultaneously
    assert "struct CliOptionMetadata" in header
    assert "namespace autoinput::cli::HelpMetadata" in source


def test_validate_cli_help_metadata_valid(sample_metadata: CliHelpMetadata) -> None:
    # Should not raise any validation error for valid metadata
    validate_cli_help_metadata(sample_metadata)


def test_validate_invalid_app_name() -> None:
    meta = CliHelpMetadata(app_name="", app_summary="sum", global_options=[], commands=[], completions={})
    # Verify validation fails when application name is empty
    with pytest.raises(CliHelpValidationError, match=r"(?i)name"):
        validate_cli_help_metadata(meta)


def test_validate_invalid_option_empty_names() -> None:
    meta = CliHelpMetadata(
        app_name="app",
        app_summary="sum",
        global_options=[CliOption(names=[], value=False, value_name=None, description="desc", completion="none")],
        commands=[],
        completions={},
    )
    # Verify validation fails when option has no flag names defined
    with pytest.raises(CliHelpValidationError, match=r"(?i)names"):
        validate_cli_help_metadata(meta)


def test_validate_invalid_command_usage() -> None:
    meta = CliHelpMetadata(
        app_name="app",
        app_summary="sum",
        global_options=[],
        commands=[CliCommand(name="run", usage="", description="desc", options=[], subcommands=[])],
        completions={},
    )
    # Verify validation fails when command usage string is empty
    with pytest.raises(CliHelpValidationError, match=r"(?i)usage"):
        validate_cli_help_metadata(meta)


def test_generate_end_to_end(tmp_path: pathlib.Path) -> None:
    metadata_file = tmp_path / "help.toml"
    metadata_file.write_text(
        """
        [app]
        name = "testapp"
        summary = "Test App Summary"

        [[global_options]]
        names = ["-v", "--version"]
        value = false
        description = "Print version"

        [[commands]]
        name = "ping"
        usage = "ping"
        description = "Ping server"
        """,
        encoding="utf-8",
    )

    header_file = tmp_path / "generated" / "help.h"
    source_file = tmp_path / "generated" / "help.cpp"

    # Run generator in write mode: verify returns True and files are created
    ret = generate(metadata_path=metadata_file, header_path=header_file, source_path=source_file, check_only=False)
    assert ret is True
    assert header_file.exists()
    assert source_file.exists()

    # Run generator in check mode: verify returns True when files match
    ret_check = generate(metadata_path=metadata_file, header_path=header_file, source_path=source_file, check_only=True)
    assert ret_check is True

    # Modify header and check mode: verify returns False when content differs
    header_file.write_text("corrupted", encoding="utf-8")
    ret_check_fail = generate(metadata_path=metadata_file, header_path=header_file, source_path=source_file, check_only=True)
    assert ret_check_fail is False


def test_generate_missing_metadata(tmp_path: pathlib.Path) -> None:
    ret = generate(
        metadata_path=tmp_path / "missing.toml",
        header_path=tmp_path / "help.h",
        source_path=tmp_path / "help.cpp",
        check_only=False,
    )
    # Verify generator returns False when input metadata TOML file is missing
    assert ret is False


def test_generate_invalid_metadata(tmp_path: pathlib.Path) -> None:
    invalid_file = tmp_path / "invalid.toml"
    invalid_file.write_text("[app]\nname = ''\n", encoding="utf-8")
    ret = generate(
        metadata_path=invalid_file,
        header_path=tmp_path / "help.h",
        source_path=tmp_path / "help.cpp",
        check_only=False,
    )
    # Verify generator returns False when metadata fails validation
    assert ret is False
