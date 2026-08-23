"""CLI help metadata models."""

from dataclasses import dataclass, field

# Completion values that are understood natively by the consumers of this
# metadata without needing an entry in the TOML `[completions]` table.
BUILTIN_COMPLETIONS = {"none", "file", "path", "config", "application"}


@dataclass
class CliOption:
    """Represents a CLI option/flag."""

    names: list[str]
    value: bool = False
    value_name: str | None = None
    description: str = ""
    completion: str = "none"
    repeatable: bool = False


@dataclass
class CliCommand:
    """Represents a CLI command or subcommand."""

    name: str
    usage: str
    description: str
    options: list[CliOption] = field(default_factory=list)
    subcommands: list["CliCommand"] = field(default_factory=list)
    examples: list[str] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


@dataclass
class CliHelpMetadata:
    """Top-level CLI help metadata container."""

    app_name: str
    app_summary: str
    global_options: list[CliOption]
    commands: list[CliCommand]
    completions: dict[str, list[str]] = field(default_factory=dict)
