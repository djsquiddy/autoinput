
import re
from .model import (
    TomlKeyValue,
    LocKeySettings,
    LocalizationId,
)

def to_loc_id_name(key: str) -> str:
    """Convert a localization key into a C++ localization ID constant name."""
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
    """Convert a localization key into a C++ localization ID constant key name."""
    id_name = to_loc_id_name(key)
    return id_name[:-2] + 'KEY'



def get_localization_ids(key_values: list[TomlKeyValue]) -> list[LocalizationId]:
    """Get localization IDs from a list of key-value pairs."""
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

