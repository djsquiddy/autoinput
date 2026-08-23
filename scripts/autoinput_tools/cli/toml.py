"""TOML loading for CLI help metadata."""

import pathlib
import tomllib
from typing import Any

from .model import CliCommand, CliHelpMetadata, CliOption
from .validation import (
    CliHelpValidationError,
    validate_cli_help_metadata,
)


def _parse_option(raw: dict[str, Any], context: str) -> CliOption:
    """Parse a raw option dictionary into a CliOption instance."""
    if not isinstance(raw, dict):
        raise CliHelpValidationError(f"{context}: option entry must be a dictionary.")

    names = raw.get("names")
    if not names or not isinstance(names, list) or any(not isinstance(n, str) or not n for n in names):
        raise CliHelpValidationError(
            f"{context}: option is missing a non-empty 'names' list: {raw!r}"
        )

    value = bool(raw.get("value", False))
    value_name = raw.get("value_name")
    description = raw.get("description", "")
    completion = raw.get("completion", "none")
    repeatable = bool(raw.get("repeatable", False))

    return CliOption(
        names=list(names),
        value=value,
        value_name=value_name,
        description=description,
        completion=completion,
        repeatable=repeatable,
    )


def _parse_command(raw: dict[str, Any], context: str) -> CliCommand:
    """Parse a raw command dictionary into a CliCommand instance."""
    if not isinstance(raw, dict):
        raise CliHelpValidationError(f"{context}: command entry must be a dictionary.")

    name = raw.get("name", "")
    full_context = f"{context}.{name}" if context and name else (context or name or "command")

    usage = raw.get("usage", "")
    description = raw.get("description", "")

    options = [
        _parse_option(raw_opt, full_context)
        for raw_opt in raw.get("options", [])
    ]

    subcommands = [
        _parse_command(raw_sub, full_context)
        for raw_sub in raw.get("subcommands", [])
    ]

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


def load_cli_help_metadata(path: pathlib.Path | str) -> CliHelpMetadata:
    """Load and validate CLI help metadata from the given TOML file.

    Raises:
        CliHelpValidationError: if the file fails schema validation.
        FileNotFoundError: if the file does not exist.
    """
    if isinstance(path, str):
        path = pathlib.Path(path)

    if not path.exists():
        raise FileNotFoundError(f"CLI help metadata file not found: {path}")

    with path.open("rb") as f:
        data = tomllib.load(f)

    app = data.get("app", {})
    if not isinstance(app, dict):
        raise CliHelpValidationError("[app]: must be a table.")

    app_name = app.get("name", "")
    app_summary = app.get("summary", "")

    completions_raw = data.get("completions", {})
    completions: dict[str, list[str]] = {}
    if isinstance(completions_raw, dict):
        for key, value in completions_raw.items():
            if not isinstance(value, list) or any(not isinstance(v, str) for v in value):
                raise CliHelpValidationError(
                    f"[completions]: entry '{key}' must be a list of strings."
                )
            completions[key] = list(value)
    else:
        raise CliHelpValidationError("[completions]: must be a table.")

    global_options = [
        _parse_option(raw_opt, "global_options")
        for raw_opt in data.get("global_options", [])
    ]

    commands = [
        _parse_command(raw_cmd, "")
        for raw_cmd in data.get("commands", [])
    ]

    metadata = CliHelpMetadata(
        app_name=app_name,
        app_summary=app_summary,
        global_options=global_options,
        commands=commands,
        completions=completions,
    )

    validate_cli_help_metadata(metadata)
    return metadata
