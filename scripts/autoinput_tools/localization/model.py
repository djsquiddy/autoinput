import enum
from dataclasses import dataclass, field

"""Localization model."""
TomlKeyValue = tuple[str, str]

class LocKeySettings(enum.IntFlag):
    """Localization key settings."""
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


@dataclass
class LocalizationData:
    definitions: list[str] = field(default_factory=list)
    key_array_entries: list[str] = field(default_factory=list)

    @classmethod
    def generate(cls, loc_ids: list[LocalizationId]) -> 'LocalizationData':
        indent_space = 4
        definitions: list[str] = []
        key_array_entries: list[str] = []
        id_prefix = ' ' * indent_space
        key_array_prefix = ' ' * (indent_space * 2)
        def get_settings_format_str(settings: LocKeySettings) -> str:
            if settings != LocKeySettings.Empty:
                results: list[str] = []
                if settings & LocKeySettings.Format:
                    results.append("LocKeySettings::Format")
                return "|".join(results)
            return "LocKeySettings::None"

        for loc_id in loc_ids:
            definitions.append(f"{id_prefix}inline constexpr LocId {loc_id.id_name} = {loc_id.value};")
            loc_value_str = f"{{ /*index*/{loc_id.id_name}, /*keyName*/\"{loc_id.name}\", /*settings*/{get_settings_format_str(loc_id.settings)} }}"
            definitions.append(f"{id_prefix}inline constexpr LocKey {loc_id.key_name}{loc_value_str};")
            key_array_entries.append(f'{key_array_prefix}{loc_id.key_name},')

        return cls(
            definitions=definitions,
            key_array_entries=key_array_entries,
        )

    def gen_defs_str(self, eol: str = "\n") -> str:
        return eol.join(self.definitions)

    def gen_key_array_entries_str(self, eol: str = "\n") -> str:
        return eol.join(self.key_array_entries)
