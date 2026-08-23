"""Validation for CLI help metadata."""

from typing import Any
from .model import (
    BUILTIN_COMPLETIONS,
    CliCommand,
    CliHelpMetadata,
    CliOption,
)


class CliHelpValidationError(ValueError):
    """Raised when CLI help metadata is invalid."""


def validate_completion_reference(
    option: CliOption,
    completions: dict[str, list[str]],
    scope: str,
) -> None:
    """Validate that an option's completion reference is known."""
    completion = option.completion
    if completion not in BUILTIN_COMPLETIONS and completion not in completions:
        raise CliHelpValidationError(
            f"{scope}: option {option.names!r} references unknown completion '{completion}'. "
            f"Must be one of the built-ins {sorted(BUILTIN_COMPLETIONS)} or a key in [completions]."
        )


def validate_options(
    options: list[CliOption],
    completions: dict[str, list[str]],
    scope: str,
) -> None:
    """Validate a list of CLI options within a given scope."""
    seen_flags: dict[str, list[str]] = {}

    for opt in options:
        if not opt.names or not isinstance(opt.names, list) or any(not isinstance(n, str) or not n for n in opt.names):
            raise CliHelpValidationError(
                f"{scope}: option is missing a non-empty 'names' list: {opt!r}"
            )

        if not opt.value and opt.value_name:
            raise CliHelpValidationError(
                f"{scope}: option {opt.names!r} has 'value_name' set ({opt.value_name!r}) but 'value' is false. "
                "An option that does not take a value cannot have a value_name."
            )

        if not opt.description:
            raise CliHelpValidationError(
                f"{scope}: option {opt.names!r} is missing a non-empty 'description'."
            )

        validate_completion_reference(opt, completions, scope)

        for name in opt.names:
            if name in seen_flags:
                raise CliHelpValidationError(
                    f"{scope}: duplicate option name '{name}' used by both "
                    f"{seen_flags[name]!r} and {opt.names!r}."
                )
            seen_flags[name] = opt.names


def validate_unique_command_names(commands: list[CliCommand], scope: str) -> None:
    """Validate that command names are unique within a scope."""
    seen: set[str] = set()
    for cmd in commands:
        if cmd.name in seen:
            raise CliHelpValidationError(f"{scope}: duplicate command name '{cmd.name}'.")
        seen.add(cmd.name)


def validate_command(
    command: CliCommand,
    completions: dict[str, list[str]],
    scope: str = "",
) -> None:
    """Validate a command and its subcommands recursively."""
    name = command.name
    if not name:
        raise CliHelpValidationError(f"{scope or 'commands'}: command/subcommand is missing a non-empty 'name'.")

    full_context = f"{scope}.{name}" if scope else name

    if not command.usage:
        raise CliHelpValidationError(f"{full_context}: command is missing a non-empty 'usage'.")

    if not command.description:
        raise CliHelpValidationError(f"{full_context}: command is missing a non-empty 'description'.")

    validate_options(command.options, completions, full_context)

    seen_subcommands: set[str] = set()
    for sub in command.subcommands:
        if sub.name in seen_subcommands:
            raise CliHelpValidationError(f"{full_context}: duplicate subcommand name '{sub.name}'.")
        seen_subcommands.add(sub.name)
        validate_command(sub, completions, full_context)


def validate_cli_help_metadata(metadata: CliHelpMetadata) -> None:
    """Validate complete CLI help metadata structure."""
    if not metadata.app_name:
        raise CliHelpValidationError("[app]: missing non-empty 'name'.")

    if metadata.completions:
        for key, value in metadata.completions.items():
            if not isinstance(value, list) or any(not isinstance(v, str) for v in value):
                raise CliHelpValidationError(
                    f"[completions]: entry '{key}' must be a list of strings."
                )

    validate_options(metadata.global_options, metadata.completions, "global_options")

    seen_commands: set[str] = set()
    for cmd in metadata.commands:
        if cmd.name in seen_commands:
            raise CliHelpValidationError(f"commands: duplicate top-level command name '{cmd.name}'.")
        seen_commands.add(cmd.name)
        validate_command(cmd, metadata.completions, "")
