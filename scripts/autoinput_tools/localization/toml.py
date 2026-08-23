"""TOML localization file loading helpers."""
import logging
import tomllib
import pathlib

from .model import (
    TomlKeyValue,
)
from ..paths import (
    LOCALIZATION_DIR,
    DEFAULT_LOCALIZATION_FILE,
)
from ..file_io import (
    get_flatten_toml,
    get_toml_keys,
)

logger = logging.getLogger(__name__)


class LocalizationFile:
    """Localization file loaded from a TOML file."""
    def __init__(self, local: str, loc_data: dict[str, str] | None):
        self.local: str = local
        self.data: dict[str, str] | None = loc_data
        self.flattened_data: dict[str, str] = get_flatten_toml(self.data)

    def get_all_keys(self, prefix: str | None = None) -> set[str]:
        if self.data is None:
            return set()
        return get_toml_keys(self.data, prefix)

    def get_all_sorted_keys(self, prefix: str | None = None) -> list[str]:
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
        filepath = LOCALIZATION_DIR / f'{local}.toml'
        return cls.load_from_filepath(filepath)

    @classmethod
    def load_from_filepath(cls, filepath: pathlib.Path) -> 'LocalizationFile':
        if not filepath.exists():
            logger.error(f"Localization file not found at {filepath}")
            return cls(DEFAULT_LOCALIZATION_FILE.stem, None)

        try:
            with filepath.open('rb') as f:
                loc_data = tomllib.load(f)
        except Exception as e:
            logger.error(f"Error parsing TOML at {filepath}: {e}")
            return cls(filepath.stem, None)
        return cls(filepath.stem, loc_data)
