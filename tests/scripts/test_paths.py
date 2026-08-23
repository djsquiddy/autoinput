#!/usr/bin/env python3
"""Unit tests for autoinput_tools.paths."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.paths import (
    AUTOCOMPLETE_BASH_FILE,
    AUTOCOMPLETE_DIR,
    AUTOCOMPLETE_LUA_FILE,
    AUTOCOMPLETE_ZSH_FILE,
    BUILD_DIR,
    CLI_RESOURCES_DIR,
    COMMANDS_DIR,
    CONFIGS_DIR,
    DEFAULT_CLI_HELP_METADATA_FILE,
    DEFAULT_HELP_TOML,
    DEFAULT_LOCALIZATION_FILE,
    GENERATED_CLI_HELP_HEADER,
    GENERATED_CLI_HELP_SOURCE,
    GENERATED_DIR,
    GENERATED_LOCALIZATION_HEADER,
    GENERATED_LOCALIZATION_SOURCE,
    LOCALIZATION_DIR,
    PROJECT_ROOT,
    RESOURCE_DIR,
    RESOURCES_DIR,
    ROOT_DIR,
    SCRIPT_DIR,
    SCRIPTS_DIR,
    SRC_DIR,
    TOOLS_DIR,
    UI_SRC_DIR,
    generated_path,
    get_python_filepath,
    get_python_module_path,
    project_path,
    resource_path,
)


def test_directory_constants() -> None:
    assert PROJECT_ROOT.exists()
    assert ROOT_DIR == PROJECT_ROOT
    assert SCRIPTS_DIR == PROJECT_ROOT / "scripts"
    assert SCRIPT_DIR == SCRIPTS_DIR
    assert TOOLS_DIR == SCRIPTS_DIR / "autoinput_tools"
    assert COMMANDS_DIR == SCRIPTS_DIR / "commands"
    assert SRC_DIR == PROJECT_ROOT / "src"
    assert UI_SRC_DIR == SRC_DIR / "autoinput_ui"
    assert RESOURCES_DIR == PROJECT_ROOT / "resources"
    assert RESOURCE_DIR == RESOURCES_DIR
    assert CONFIGS_DIR == PROJECT_ROOT / "configs"
    assert LOCALIZATION_DIR == RESOURCES_DIR / "localization"
    assert CLI_RESOURCES_DIR == RESOURCES_DIR / "cli"
    assert AUTOCOMPLETE_DIR == SCRIPTS_DIR / "autocomplete"


def test_file_constants() -> None:
    assert DEFAULT_LOCALIZATION_FILE == LOCALIZATION_DIR / "en-US.toml"
    assert DEFAULT_CLI_HELP_METADATA_FILE == CLI_RESOURCES_DIR / "help.toml"
    assert DEFAULT_HELP_TOML == DEFAULT_CLI_HELP_METADATA_FILE

    assert GENERATED_LOCALIZATION_HEADER == GENERATED_DIR / "autoinput" / "support" / "localizationIds.h"
    assert GENERATED_LOCALIZATION_SOURCE == GENERATED_DIR / "autoinput" / "support" / "localizationIds.cpp"

    assert GENERATED_CLI_HELP_HEADER == GENERATED_DIR / "autoinput" / "cli" / "cliHelpMetadata.h"
    assert GENERATED_CLI_HELP_SOURCE == GENERATED_DIR / "autoinput" / "cli" / "cliHelpMetadata.cpp"

    assert AUTOCOMPLETE_ZSH_FILE == AUTOCOMPLETE_DIR / "_autoinput"
    assert AUTOCOMPLETE_BASH_FILE == AUTOCOMPLETE_DIR / "autoinput_completion.bash"
    assert AUTOCOMPLETE_LUA_FILE == AUTOCOMPLETE_DIR / "autoinput_completion.lua"


def test_helper_path_functions() -> None:
    assert project_path("foo", "bar") == PROJECT_ROOT / "foo" / "bar"
    assert resource_path("cli", "help.toml") == RESOURCES_DIR / "cli" / "help.toml"
    assert generated_path("out.h") == GENERATED_DIR / "out.h"


def test_get_python_filepath() -> None:
    assert get_python_filepath("test") == SCRIPT_DIR / "test.py"
    assert get_python_filepath("test.py") == SCRIPT_DIR / "test.py"
    assert get_python_filepath("commands/build.py") == SCRIPT_DIR / "commands" / "build.py"


def test_get_python_module_path() -> None:
    assert get_python_module_path("build.py") == f"{SCRIPT_DIR.stem}.build"
    assert get_python_module_path("foo.bar") == f"{SCRIPT_DIR.stem}.foo"
