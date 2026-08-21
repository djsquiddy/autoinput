"""Shared loader/validator for the canonical CLI help metadata TOML file.

This module is intentionally free of any dependency on the rest of the
`scripts` package (aside from the standard library) so that it can be
imported both as `scripts.cli_help` and as a standalone module by the
code-generation tooling (`gen_cli_help.py`) and the autocomplete tooling
(`update_autocomplete.py`).
"""
import pathlib
import tomllib
from dataclasses import dataclass, field

# Completion values that are understood natively by the consumers of this
# metadata without needing an entry in the TOML `[completions]` table.
BUILTIN_COMPLETIONS = {"none", "file", "path", "config", "application"}


class CliHelpValidationError(ValueError):
    """Raised when the CLI help metadata TOML fails schema validation."""


@dataclass
class CliOption:
    names: list[str]
    value: bool = False
    value_name: str | None = None
    description: str = ""
    completion: str = "none"
    repeatable: bool = False


@dataclass
class CliCommand:
    name: str
    usage: str
    description: str
    options: list[CliOption] = field(default_factory=list)
    subcommands: list["CliCommand"] = field(default_factory=list)
    examples: list[str] = field(default_factory=list)
    notes: list[str] = field(default_factory=list)


@dataclass
class CliHelpMetadata:
    app_name: str
    app_summary: str
    global_options: list[CliOption]
    commands: list[CliCommand]
    completions: dict[str, list[str]]


def _parse_option(raw: dict, context: str, completions: dict[str, list[str]]) -> CliOption:
    names = raw.get("names")
    if not names or not isinstance(names, list) or any(not isinstance(n, str) or not n for n in names):
        raise CliHelpValidationError(
            f"{context}: option is missing a non-empty 'names' list: {raw!r}"
        )

    value = bool(raw.get("value", False))
    value_name = raw.get("value_name")

    if not value and value_name:
        raise CliHelpValidationError(
            f"{context}: option {names!r} has 'value_name' set ({value_name!r}) but 'value' is false. "
            "An option that does not take a value cannot have a value_name."
        )

    description = raw.get("description", "")
    if not description:
        raise CliHelpValidationError(
            f"{context}: option {names!r} is missing a non-empty 'description'."
        )

    completion = raw.get("completion", "none")
    if completion not in BUILTIN_COMPLETIONS and completion not in completions:
        raise CliHelpValidationError(
            f"{context}: option {names!r} references unknown completion '{completion}'. "
            f"Must be one of the built-ins {sorted(BUILTIN_COMPLETIONS)} or a key in [completions]."
        )

    repeatable = bool(raw.get("repeatable", False))

    return CliOption(
        names=list(names),
        value=value,
        value_name=value_name,
        description=description,
        completion=completion,
        repeatable=repeatable,
    )


def _validate_no_duplicate_names(options: list[CliOption], context: str) -> None:
    seen: dict[str, str] = {}
    for opt in options:
        for name in opt.names:
            if name in seen:
                raise CliHelpValidationError(
                    f"{context}: duplicate option name '{name}' used by both "
                    f"{seen[name]!r} and {opt.names!r}."
                )
            seen[name] = ",".join(opt.names)


def _parse_command(raw: dict, context: str, completions: dict[str, list[str]]) -> CliCommand:
    name = raw.get("name", "")
    if not name:
        raise CliHelpValidationError(f"{context}: command/subcommand is missing a non-empty 'name'.")

    full_context = f"{context}.{name}" if context else name

    usage = raw.get("usage", "")
    if not usage:
        raise CliHelpValidationError(f"{full_context}: command is missing a non-empty 'usage'.")

    description = raw.get("description", "")
    if not description:
        raise CliHelpValidationError(f"{full_context}: command is missing a non-empty 'description'.")

    options = [
        _parse_option(raw_opt, full_context, completions)
        for raw_opt in raw.get("options", [])
    ]
    _validate_no_duplicate_names(options, full_context)

    subcommands = [
        _parse_command(raw_sub, full_context, completions)
        for raw_sub in raw.get("subcommands", [])
    ]

    seen_subcommand_names: dict[str, bool] = {}
    for sub in subcommands:
        if sub.name in seen_subcommand_names:
            raise CliHelpValidationError(
                f"{full_context}: duplicate subcommand name '{sub.name}'."
            )
        seen_subcommand_names[sub.name] = True

    examples = list(raw.get("examples", []))
    notes = list(raw.get("notes", []))

    return CliCommand(
        name=name,
        usage=usage,
        description=description,
        options=options,
        subcommands=subcommands,
        examples=examples,
        notes=notes,
    )


def load_cli_help_metadata(path: pathlib.Path) -> CliHelpMetadata:
    """Load and validate CLI help metadata from the given TOML file.

    Raises:
        CliHelpValidationError: if the file fails schema validation.
        FileNotFoundError: if the file does not exist.
    """
    if not path.exists():
        raise FileNotFoundError(f"CLI help metadata file not found: {path}")

    with path.open("rb") as f:
        data = tomllib.load(f)

    app = data.get("app", {})
    app_name = app.get("name", "")
    if not app_name:
        raise CliHelpValidationError("[app]: missing non-empty 'name'.")
    app_summary = app.get("summary", "")

    completions_raw = data.get("completions", {})
    completions: dict[str, list[str]] = {}
    for key, value in completions_raw.items():
        if not isinstance(value, list) or any(not isinstance(v, str) for v in value):
            raise CliHelpValidationError(
                f"[completions]: entry '{key}' must be a list of strings."
            )
        completions[key] = list(value)

    global_options = [
        _parse_option(raw_opt, "global_options", completions)
        for raw_opt in data.get("global_options", [])
    ]
    _validate_no_duplicate_names(global_options, "global_options")

    commands = [
        _parse_command(raw_cmd, "", completions)
        for raw_cmd in data.get("commands", [])
    ]

    seen_command_names: dict[str, bool] = {}
    for cmd in commands:
        if cmd.name in seen_command_names:
            raise CliHelpValidationError(f"commands: duplicate top-level command name '{cmd.name}'.")
        seen_command_names[cmd.name] = True

    return CliHelpMetadata(
        app_name=app_name,
        app_summary=app_summary,
        global_options=global_options,
        commands=commands,
        completions=completions,
    )
