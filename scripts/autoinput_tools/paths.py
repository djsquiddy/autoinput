"""Shared project path constants for AutoInput Python tooling."""

import pathlib

# Paths relative to the script location
TOOLS_DIR: pathlib.Path = pathlib.Path(__file__).resolve().parent
SCRIPTS_DIR: pathlib.Path = TOOLS_DIR.parent
SCRIPT_DIR: pathlib.Path = SCRIPTS_DIR
COMMANDS_DIR: pathlib.Path = SCRIPTS_DIR / "commands"
PROJECT_ROOT: pathlib.Path = SCRIPTS_DIR.parent
ROOT_DIR: pathlib.Path = PROJECT_ROOT

SRC_DIR: pathlib.Path = PROJECT_ROOT / "src"
UI_SRC_DIR: pathlib.Path = SRC_DIR / "autoinput_ui"
BUILD_DIR: pathlib.Path = PROJECT_ROOT / "build"
RESOURCES_DIR: pathlib.Path = PROJECT_ROOT / "resources"
RESOURCE_DIR: pathlib.Path = RESOURCES_DIR
CONFIGS_DIR: pathlib.Path = PROJECT_ROOT / "configs"

LOCALIZATION_DIR: pathlib.Path = RESOURCES_DIR / "localization"
CLI_RESOURCES_DIR: pathlib.Path = RESOURCES_DIR / "cli"

DEFAULT_LOCALIZATION_FILE: pathlib.Path = LOCALIZATION_DIR / "en-US.toml"
DEFAULT_CLI_HELP_METADATA_FILE: pathlib.Path = CLI_RESOURCES_DIR / "help.toml"
DEFAULT_HELP_TOML: pathlib.Path = DEFAULT_CLI_HELP_METADATA_FILE
DEFAULT_APP_ICON_PNG: pathlib.Path = RESOURCES_DIR / "appIcon.png"
DEFAULT_APP_ICON_FILE: pathlib.Path = DEFAULT_APP_ICON_PNG

GENERATED_DIR: pathlib.Path = BUILD_DIR / "generated"

GENERATED_LOCALIZATION_HEADER: pathlib.Path = GENERATED_DIR / "autoinput" / "support" / "localizationIds.h"
GENERATED_LOCALIZATION_SOURCE: pathlib.Path = GENERATED_LOCALIZATION_HEADER.with_suffix(".cpp")

GENERATED_CLI_HELP_HEADER: pathlib.Path = GENERATED_DIR / "autoinput" / "cli" / "cliHelpMetadata.h"
GENERATED_CLI_HELP_SOURCE: pathlib.Path = GENERATED_CLI_HELP_HEADER.with_suffix(".cpp")

GENERATED_APP_ICON_ICO: pathlib.Path = GENERATED_DIR / "autoinput" / "resources" / "appIcon.ico"
GENERATED_APP_ICON_RC: pathlib.Path = GENERATED_DIR / "autoinput" / "resources" / "appIcon.rc"

AUTOCOMPLETE_DIR: pathlib.Path = SCRIPTS_DIR / "autocomplete"
AUTOCOMPLETE_ZSH_FILE: pathlib.Path = AUTOCOMPLETE_DIR / "_autoinput"
AUTOCOMPLETE_BASH_FILE: pathlib.Path = AUTOCOMPLETE_DIR / "autoinput_completion.bash"
AUTOCOMPLETE_LUA_FILE: pathlib.Path = AUTOCOMPLETE_DIR / "autoinput_completion.lua"


def project_path(*parts: str) -> pathlib.Path:
    return PROJECT_ROOT.joinpath(*parts)


def resource_path(*parts: str) -> pathlib.Path:
    return RESOURCES_DIR.joinpath(*parts)


def generated_path(*parts: str) -> pathlib.Path:
    return GENERATED_DIR.joinpath(*parts)


def get_python_filepath(filename: str) -> pathlib.Path:
    if filename.endswith(".py"):
        return SCRIPT_DIR / filename
    return SCRIPT_DIR / f"{filename}.py"


def get_python_module_path(filename: str) -> str:
    return f"{SCRIPT_DIR.stem}.{pathlib.Path(filename).stem}"
