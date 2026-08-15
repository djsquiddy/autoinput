import logging
import os
import pathlib
import tomllib

logger = logging.getLogger(__name__)

# Paths relative to the script location
SCRIPT_DIR = pathlib.Path(__file__).resolve().parent
ROOT_DIR = SCRIPT_DIR.parent
BUILD_DIR = ROOT_DIR / "build"
RESOURCE_DIR = ROOT_DIR / 'resources'
LOC_DIR = RESOURCE_DIR / 'localization'
SRC_DIR = ROOT_DIR / 'src'
CORE_SRC_DIR = SRC_DIR / 'autoinput'
TRAY_SRC_DIR = SRC_DIR / 'autoinput_tray'
UI_SRC_DIR = SRC_DIR / 'autoinput_ui'
EXIT_SUCCESSFUL = 0


def get_python_filepath(filename: str) -> pathlib.Path:
    if filename.endswith('.py'):
        return SCRIPT_DIR / filename
    return SCRIPT_DIR / f'{filename}.py'


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


class LocalizationFile:
    def __init__(self, local: str, loc_data: dict[str, str] | None):
        self.local: str = local
        self.data: dict[str, str] | None = loc_data

    def get_all_keys(self, prefix: str = None) -> set[str]:
        if self.data is None:
            return set()
        return get_toml_keys(self.data, prefix)

    def is_valid(self):
        return self.data is not None

    @classmethod
    def load(cls, local: str) -> 'LocalizationFile':
        filepath = LOC_DIR / f'{local}.toml'
        if not filepath.exists():
            logger.error(f"Localization file not found at {filepath}")
            return cls(local, None)

        try:
            with filepath.open('rb') as f:
                loc_data = tomllib.load(f)
        except Exception as e:
            logger.error(f"Error parsing TOML at {filepath}: {e}")
            return cls(local, None)
        return cls(local, loc_data)


