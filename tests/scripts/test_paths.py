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
    CLANG_FORMAT_CONFIG_FILE,
    CLANG_TIDY_CONFIG_FILE,
    CLI_RESOURCES_DIR,
    CMAKE_PRESETS_FILE,
    CMAKE_USER_PRESETS_FILE,
    COMMANDS_DIR,
    CONFIGS_DIR,
    DEFAULT_APP_ICON_FILE,
    DEFAULT_APP_ICON_PNG,
    DEFAULT_CLI_HELP_METADATA_FILE,
    DEFAULT_HELP_TOML,
    DEFAULT_LOCALIZATION_FILE,
    DOCS_API_DIR,
    DOCS_DIR,
    GENERATED_APP_ICON_ICO,
    GENERATED_APP_ICON_RC,
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
    # Verify project root exists on disk
    assert PROJECT_ROOT.exists()
    # Verify ROOT_DIR alias resolves to PROJECT_ROOT
    assert ROOT_DIR == PROJECT_ROOT
    # Verify scripts directory path
    assert SCRIPTS_DIR == PROJECT_ROOT / "scripts"
    # Verify SCRIPT_DIR alias resolves to SCRIPTS_DIR
    assert SCRIPT_DIR == SCRIPTS_DIR
    # Verify tools directory path
    assert TOOLS_DIR == SCRIPTS_DIR / "autoinput_tools"
    # Verify commands directory path
    assert COMMANDS_DIR == SCRIPTS_DIR / "commands"
    # Verify source directory path
    assert SRC_DIR == PROJECT_ROOT / "src"
    # Verify UI source directory path
    assert UI_SRC_DIR == SRC_DIR / "autoinput_ui"
    # Verify resources directory path
    assert RESOURCES_DIR == PROJECT_ROOT / "resources"
    # Verify RESOURCE_DIR alias resolves to RESOURCES_DIR
    assert RESOURCE_DIR == RESOURCES_DIR
    # Verify configs directory path
    assert CONFIGS_DIR == PROJECT_ROOT / "configs"
    # Verify localization resources directory path
    assert LOCALIZATION_DIR == RESOURCES_DIR / "localization"
    # Verify CLI resources directory path
    assert CLI_RESOURCES_DIR == RESOURCES_DIR / "cli"
    # Verify documentation directory paths
    assert DOCS_DIR == PROJECT_ROOT / "docs"
    assert DOCS_API_DIR == DOCS_DIR / "api"
    # Verify autocomplete directory path
    assert AUTOCOMPLETE_DIR == SCRIPTS_DIR / "autocomplete"


def test_file_constants() -> None:
    # Verify default localization TOML file path
    assert DEFAULT_LOCALIZATION_FILE == LOCALIZATION_DIR / "en-US.toml"
    # Verify default CLI help metadata file path
    assert DEFAULT_CLI_HELP_METADATA_FILE == CLI_RESOURCES_DIR / "help.toml"
    # Verify DEFAULT_HELP_TOML alias resolves to DEFAULT_CLI_HELP_METADATA_FILE
    assert DEFAULT_HELP_TOML == DEFAULT_CLI_HELP_METADATA_FILE
    # Verify default app icon PNG path
    assert DEFAULT_APP_ICON_PNG == RESOURCES_DIR / "appIcon.png"
    # Verify DEFAULT_APP_ICON_FILE alias resolves to DEFAULT_APP_ICON_PNG
    assert DEFAULT_APP_ICON_FILE == DEFAULT_APP_ICON_PNG

    # Verify generated localization C++ header and source file paths
    assert GENERATED_LOCALIZATION_HEADER == GENERATED_DIR / "autoinput" / "support" / "localizationIds.h"
    assert GENERATED_LOCALIZATION_SOURCE == GENERATED_DIR / "autoinput" / "support" / "localizationIds.cpp"

    # Verify generated CLI help C++ header and source file paths
    assert GENERATED_CLI_HELP_HEADER == GENERATED_DIR / "autoinput" / "cli" / "cliHelpMetadata.h"
    assert GENERATED_CLI_HELP_SOURCE == GENERATED_DIR / "autoinput" / "cli" / "cliHelpMetadata.cpp"

    # Verify generated Windows app icon (.ico) and resource script (.rc) paths
    assert GENERATED_APP_ICON_ICO == GENERATED_DIR / "autoinput" / "resources" / "appIcon.ico"
    assert GENERATED_APP_ICON_RC == GENERATED_DIR / "autoinput" / "resources" / "appIcon.rc"

    # Verify shell completion file paths
    assert AUTOCOMPLETE_ZSH_FILE == AUTOCOMPLETE_DIR / "_autoinput"
    assert AUTOCOMPLETE_BASH_FILE == AUTOCOMPLETE_DIR / "autoinput_completion.bash"
    assert AUTOCOMPLETE_LUA_FILE == AUTOCOMPLETE_DIR / "autoinput_completion.lua"

    # Verify tool config and CMake preset file paths
    assert CLANG_FORMAT_CONFIG_FILE == PROJECT_ROOT / ".clang-format"
    assert CLANG_TIDY_CONFIG_FILE == PROJECT_ROOT / ".clang-tidy"
    assert CMAKE_PRESETS_FILE == PROJECT_ROOT / "CMakePresets.json"
    assert CMAKE_USER_PRESETS_FILE == PROJECT_ROOT / "CMakeUserPresets.json"


def test_helper_path_functions() -> None:
    # Verify project_path joins components relative to PROJECT_ROOT
    assert project_path("foo", "bar") == PROJECT_ROOT / "foo" / "bar"
    # Verify resource_path joins components relative to RESOURCES_DIR
    assert resource_path("cli", "help.toml") == RESOURCES_DIR / "cli" / "help.toml"
    # Verify generated_path joins components relative to GENERATED_DIR
    assert generated_path("out.h") == GENERATED_DIR / "out.h"


def test_get_python_filepath() -> None:
    # Verify getting python file path without .py extension
    assert get_python_filepath("test") == SCRIPT_DIR / "test.py"
    # Verify getting python file path with .py extension
    assert get_python_filepath("test.py") == SCRIPT_DIR / "test.py"
    # Verify getting nested python file path
    assert get_python_filepath("commands/build.py") == SCRIPT_DIR / "commands" / "build.py"


def test_get_python_module_path() -> None:
    # Verify module path derived from script file name
    assert get_python_module_path("build.py") == f"{SCRIPT_DIR.stem}.build"
    # Verify module path derived from dotted string
    assert get_python_module_path("foo.bar") == f"{SCRIPT_DIR.stem}.foo"
