"""Generate C++ localization ID files."""

import logging
import pathlib

from .ids import to_loc_id_name
from .cpp import generate_header_source_content
from .toml import LocalizationFile
from ..file_io import check_or_write_file
from ..paths import (
    DEFAULT_LOCALIZATION_FILE,
    GENERATED_LOCALIZATION_HEADER,
    GENERATED_LOCALIZATION_SOURCE,
)

logger = logging.getLogger(__name__)


def generate(
    loc_file_path: pathlib.Path = DEFAULT_LOCALIZATION_FILE,
    header_path: pathlib.Path = GENERATED_LOCALIZATION_HEADER,
    source_path: pathlib.Path = GENERATED_LOCALIZATION_SOURCE,
    check_only: bool = False,
    eol: str = "\n",
) -> bool:
    """Generate localizationIds.h and localizationIds.cpp from the given TOML file."""
    loc_file = LocalizationFile.load_from_filepath(loc_file_path)
    if not loc_file.is_valid():
        logger.error(f"Localization file not found or invalid: {loc_file_path}")
        return False

    key_values = loc_file.get_sorted_key_value_pairs()
    if not key_values:
        logger.error(f"No localization keys found in {loc_file_path}")
        return False

    # Check for duplicate generated ID names
    seen_ids: dict[str, str] = {}
    for key, _ in key_values:
        id_name = to_loc_id_name(key)
        if id_name in seen_ids:
            logger.error(f"ID name collision: '{key}' and '{seen_ids[id_name]}' both produce '{id_name}'")
            return False
        seen_ids[id_name] = key

    header_content, source_content = generate_header_source_content(key_values, eol=eol)

    if not check_or_write_file(header_path, header_content, check_only=check_only, filetype="header"):
        return False
    if not check_or_write_file(source_path, source_content, check_only=check_only, filetype="source"):
        return False

    return True
