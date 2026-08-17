import logging
import os
import re
import enum
import pathlib
import sys
import tomllib
import subprocess

from dataclasses import dataclass, field

TomlKeyValue = tuple[str, str]

logger = logging.getLogger(__name__)

# Paths relative to the script location
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
SCRIPT_DIR_NAME = SCRIPT_DIR.stem
ROOT_DIR = SCRIPT_DIR.parent
BUILD_DIR = ROOT_DIR / "build"
RESOURCE_DIR = ROOT_DIR / 'resources'
LOC_DIR = RESOURCE_DIR / 'localization'
SRC_DIR = ROOT_DIR / 'src'
CORE_SRC_DIR = SRC_DIR / 'autoinput'
TRAY_SRC_DIR = SRC_DIR / 'autoinput_tray'
UI_SRC_DIR = SRC_DIR / 'autoinput_ui'
AUTOCOMPLETE_DIR = SCRIPT_DIR / 'autocomplete'
AUTOCOMPLETE_ZSH_FILE = AUTOCOMPLETE_DIR / '_autoinput'
AUTOCOMPLETE_BASH_FILE = AUTOCOMPLETE_DIR / 'autoinput_completion.bash'
AUTOCOMPLETE_LUA_FILE = AUTOCOMPLETE_DIR / 'autoinput_completion.lua'
EXIT_SUCCESSFUL = 0


def get_python_filepath(filename: str) -> pathlib.Path:
    if filename.endswith('.py'):
        return SCRIPT_DIR / filename
    return SCRIPT_DIR / f'{filename}.py'


def get_python_module_path(filename: str) -> str:
    return f'{SCRIPT_DIR_NAME}.{pathlib.Path(filename).stem}'


def format_log_line(line: str) -> str:
    """Format and colorize log lines if needed (e.g., GoogleTest markers on Windows pipes)."""
    # Don't re-colorize if already prefixed with ANSI escape sequences
    if line.startswith("\x1b["):
        return line

    if line.startswith(("[==========]", "[----------]", "[ RUN      ]", "[       OK ]", "[  PASSED  ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[32m{tag}\033[0m{rest}"
    if line.startswith(("[  FAILED  ]", "[  TIMEOUT ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[31m{tag}\033[0m{rest}"
    if line.startswith(("[  SKIPPED ]", "[ DISABLED ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[33m{tag}\033[0m{rest}"
    if line.startswith("Note: Google Test") or line.startswith("Note: This is test shard"):
        return f"\033[33m{line}\033[0m"
    if " YOU HAVE " in line and " DISABLED " in line:
        return f"\033[33m{line}\033[0m"
    if re.search(r"^\s*\d+ FAILED TESTS?", line):
        return f"\033[31m{line}\033[0m"

    return line


def run_command(cmd: list[str | os.PathLike | pathlib.Path],
                cwd: pathlib.Path | None = None,
                extra_env: dict[str, str] | None = None,) -> int:
    cmd_str = [str(arg) for arg in cmd]
    env = os.environ.copy()
    env["CLICOLOR_FORCE"] = "1"
    env["FORCE_COLOR"] = "1"
    env["GTEST_COLOR"] = "yes"
    env["CMAKE_COLOR_DIAGNOSTICS"] = "ON"
    python_path = [str(p) for p in sys.path]
    python_path.append(str(ROOT_DIR))
    python_path.append(str(SCRIPT_DIR))
    env["PYTHONPATH"] = os.pathsep.join(python_path)
    if extra_env:
        env.update(extra_env)

    try:
        with subprocess.Popen(
                cmd_str,
                cwd=cwd,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                encoding="utf-8",
                errors="replace",
        ) as process:
            if process.stdout:
                for line in iter(process.stdout.readline, ""):
                    if line:
                        logger.info(format_log_line(line.rstrip("\r\n")))
            return process.wait()
    except FileNotFoundError:
        logger.error(f"Error: '{cmd_str[0]}' executable not found in PATH.")
        return 1
    except Exception as e:
        logger.error(f"Failed to execute command {' '.join(cmd_str)}: {e}")
        return 1


def get_toml_keys(data, prefix: str | None = None) -> set[str]:
    if prefix is None:
        prefix = ''
    keys = set()
    for k, v in data.items():
        full_key = f'{prefix}{k}'
        if isinstance(v, dict):
            keys.update(get_toml_keys(v, f'{full_key}.'))
        else:
            keys.add(full_key)
    return keys


def get_flatten_toml(data, prefix: str | None = None) -> dict[str, str]:
    if prefix is None:
        prefix = ''
    results = {}
    for k, v in data.items():
        full_key = f'{prefix}{k}'
        if isinstance(v, dict):
            results.update(get_flatten_toml(v, f'{full_key}.'))
        else:
            results[full_key] = v
    return results


class LocKeySettings(enum.IntFlag):
    Empty = enum.auto()
    Format = enum.auto()


@dataclass(frozen=True)
class LocalizationId:
    """Represents a localization key ID."""
    name: str
    id_name: str
    key_name: str
    value: int
    settings: LocKeySettings = LocKeySettings.Empty


def to_loc_id_name(key: str) -> str:
    """Convert a dotted/camelCase localization key (e.g., 'app.name', 'windows.configEditor')
    into a C++ identifier constant name (e.g., 'APP_NAME_ID', 'WINDOWS_CONFIG_EDITOR_ID').
    """
    parts = key.split(".")
    processed_parts = []
    for part in parts:
        part = part.replace("-", "_")
        # insert underscore before capital letters (camelCase -> snake_case)
        part = re.sub(r"([a-z0-9])([A-Z])", r"\1_\2", part)
        processed_parts.append(part)
    combined = "_".join(processed_parts)
    combined = re.sub(r"_+", "_", combined).strip("_")
    return f"{combined.upper()}_ID"

def to_loc_key_name(key: str) -> str:
    """Convert a dotted/camelCase localization key (e.g., 'app.name', 'windows.configEditor')
    into a C++ identifier constant name (e.g., 'APP_NAME_KEY', 'WINDOWS_CONFIG_EDITOR_KEY').
    """
    id_name = to_loc_id_name(key)
    return id_name[:-2] + 'KEY'


def get_localization_ids(key_values: list[TomlKeyValue]) -> list[LocalizationId]:
    loc_ids: list[LocalizationId] = []
    for idx, p in enumerate(key_values):
        key, value = p
        id_name = to_loc_id_name(key)
        key_name = to_loc_key_name(key)
        settings = LocKeySettings.Empty
        if '{}' in value or '{:' in value:
            settings |= LocKeySettings.Format
        loc_ids.append(
            LocalizationId(
                name=key,
                id_name=id_name,
                key_name=key_name,
                value=idx,
                settings=settings
            )
        )

    return loc_ids


class LocalizationFile:
    def __init__(self, local: str, loc_data: dict[str, str] | None):
        self.local: str = local
        self.data: dict[str, str] | None = loc_data
        self.flattened_data: dict[str, str] | None = get_flatten_toml(self.data)

    def get_all_keys(self, prefix: str = None) -> set[str]:
        if self.data is None:
            return set()
        return get_toml_keys(self.data, prefix)

    def get_all_sorted_keys(self, prefix: str = None) -> list[str]:
        if self.data is None:
            return []
        return sorted(list(get_toml_keys(self.data, prefix)))

    def get_sorted_key_value_pairs(self) -> list[TomlKeyValue]:
        if self.data is None:
            return []
        return [(k, self.flattened_data[k]) for k in self.get_all_sorted_keys()]

    def is_valid(self):
        return self.data is not None

    @classmethod
    def load(cls, local: str) -> 'LocalizationFile':
        filepath = LOC_DIR / f'{local}.toml'
        return cls.load_from_filepath(filepath)

    @classmethod
    def load_from_filepath(cls, filepath: pathlib.Path) -> 'LocalizationFile':
        if not filepath.exists():
            logger.error(f"Localization file not found at {filepath}")
            return cls(local, None)

        try:
            with filepath.open('rb') as f:
                loc_data = tomllib.load(f)
        except Exception as e:
            logger.error(f"Error parsing TOML at {filepath}: {e}")
            return cls(filepath.stem, None)
        return cls(filepath.stem, loc_data)
