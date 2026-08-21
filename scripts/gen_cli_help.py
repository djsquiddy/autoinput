#!/usr/bin/env python
"""Generates C++ CLI help metadata (cliHelpMetadata.h/.cpp) from resources/cli/help.toml.

Mirrors the pattern used by scripts/gen_localization_ids.py.
"""
import argparse
import logging
import pathlib
import sys

try:
    from . import utils
    from .cli_help import (
        CliCommand,
        CliHelpMetadata,
        CliHelpValidationError,
        CliOption,
        load_cli_help_metadata,
    )
except ImportError:
    try:
        import utils
        from cli_help import (
            CliCommand,
            CliHelpMetadata,
            CliHelpValidationError,
            CliOption,
            load_cli_help_metadata,
        )
    except ImportError:
        sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
        import utils
        from cli_help import (
            CliCommand,
            CliHelpMetadata,
            CliHelpValidationError,
            CliOption,
            load_cli_help_metadata,
        )

logger = logging.getLogger(__name__)

DEFAULT_HELP_TOML = utils.RESOURCE_DIR / "cli" / "help.toml"
DEFAULT_HEADER_OUTPUT_FILE: pathlib.Path = utils.BUILD_DIR / "generated" / "autoinput" / "cli" / "cliHelpMetadata.h"
DEFAULT_SOURCE_OUTPUT_FILE = DEFAULT_HEADER_OUTPUT_FILE.with_suffix(".cpp")

GENERATED_HEADER_COMMENT = (
    "AUTO-GENERATED \u2014 DO NOT EDIT. "
    "Generated from resources/cli/help.toml by scripts/gen_cli_help.py."
)


def _escape(value: str) -> str:
    """Escape a string for embedding in a C++ string literal."""
    return (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\n", "\\n")
    )


def _cpp_string_view_array(values: list[str]) -> str:
    return "{ " + ", ".join(f'"{_escape(v)}"sv' for v in values) + " }"


class CppGenerator:
    """Generates unique C++ identifiers/arrays for the metadata tree and renders source text."""

    def __init__(self, metadata: CliHelpMetadata):
        self.metadata = metadata
        self._option_array_lines: list[str] = []
        self._command_array_lines: list[str] = []
        self._option_counter = 0
        self._command_counter = 0

    def _next_option_var(self) -> str:
        name = f"g_option_{self._option_counter}"
        self._option_counter += 1
        return name

    def _next_command_var(self) -> str:
        name = f"g_command_{self._command_counter}"
        self._command_counter += 1
        return name

    def _emit_option(self, opt: CliOption) -> str:
        var = self._next_option_var()
        names_array_var = f"{var}_names"
        self._option_array_lines.append(
            f"    inline constexpr std::array<std::string_view, {len(opt.names)}> {names_array_var} = "
            f"{_cpp_string_view_array(opt.names)};"
        )
        value_name = opt.value_name or ""
        self._option_array_lines.append(
            "    inline constexpr CliOptionMetadata {var}{{ "
            "{names}, {value}, \"{value_name}\"sv, \"{description}\"sv, \"{completion}\"sv, {repeatable} }};".format(
                var=var,
                names=names_array_var,
                value="true" if opt.value else "false",
                value_name=_escape(value_name),
                description=_escape(opt.description),
                completion=_escape(opt.completion),
                repeatable="true" if opt.repeatable else "false",
            )
        )
        return var

    def _emit_command(self, cmd: CliCommand) -> str:
        option_vars = [self._emit_option(opt) for opt in cmd.options]
        subcommand_vars = [self._emit_command(sub) for sub in cmd.subcommands]

        var = self._next_command_var()

        options_array_var = f"{var}_options"
        self._command_array_lines.append(
            f"    inline constexpr std::array<CliOptionMetadata, {len(option_vars)}> {options_array_var} = "
            "{ " + ", ".join(option_vars) + " };"
        )

        subcommands_array_var = f"{var}_subcommands"
        # Forward declared below via std::array<const CliCommandMetadata*, N> to allow
        # a command's subcommands to reference fully-defined CliCommandMetadata instances.
        self._command_array_lines.append(
            f"    inline constexpr std::array<CliCommandMetadata, {len(subcommand_vars)}> {subcommands_array_var} = "
            "{ " + ", ".join(subcommand_vars) + " };"
        )

        examples_array_var = f"{var}_examples"
        self._command_array_lines.append(
            f"    inline constexpr std::array<std::string_view, {len(cmd.examples)}> {examples_array_var} = "
            f"{_cpp_string_view_array(cmd.examples)};"
        )

        notes_array_var = f"{var}_notes"
        self._command_array_lines.append(
            f"    inline constexpr std::array<std::string_view, {len(cmd.notes)}> {notes_array_var} = "
            f"{_cpp_string_view_array(cmd.notes)};"
        )

        self._command_array_lines.append(
            "    inline constexpr CliCommandMetadata {var}{{ "
            "\"{name}\"sv, \"{usage}\"sv, \"{description}\"sv, {options}, {subcommands}, {examples}, {notes} }};".format(
                var=var,
                name=_escape(cmd.name),
                usage=_escape(cmd.usage),
                description=_escape(cmd.description),
                options=options_array_var,
                subcommands=subcommands_array_var,
                examples=examples_array_var,
                notes=notes_array_var,
            )
        )
        return var

    def generate(self) -> tuple[str, str]:
        global_option_vars = [self._emit_option(opt) for opt in self.metadata.global_options]
        command_vars = [self._emit_command(cmd) for cmd in self.metadata.commands]

        self._option_array_lines.append(
            f"    inline constexpr std::array<CliOptionMetadata, {len(global_option_vars)}> g_global_options = "
            "{ " + ", ".join(global_option_vars) + " };"
        )
        self._command_array_lines.append(
            f"    inline constexpr std::array<CliCommandMetadata, {len(command_vars)}> g_all_commands = "
            "{ " + ", ".join(command_vars) + " };"
        )

        return "\n".join(self._option_array_lines), "\n".join(self._command_array_lines)


def generate_header_content(eol: str = "\n") -> str:
    header_lines = [
        "/**",
        " * @file cliHelpMetadata.h",
        f" * @brief {GENERATED_HEADER_COMMENT}",
        " */",
        "#ifndef INCLUDE_AUTOINPUT_CLI_CLIHELPMETADATA_H",
        "#define INCLUDE_AUTOINPUT_CLI_CLIHELPMETADATA_H",
        "#pragma once",
        "",
        "#include <array>",
        "#include <span>",
        "#include <string_view>",
        "",
        "namespace autoinput::cli::HelpMetadata",
        "{",
        "    struct CliOptionMetadata",
        "    {",
        "        std::span<const std::string_view> names;",
        "        bool value;",
        "        std::string_view valueName;",
        "        std::string_view description;",
        "        std::string_view completion;",
        "        bool repeatable;",
        "    };",
        "",
        "    struct CliCommandMetadata",
        "    {",
        "        std::string_view name;",
        "        std::string_view usage;",
        "        std::string_view description;",
        "        std::span<const CliOptionMetadata> options;",
        "        std::span<const CliCommandMetadata> subcommands;",
        "        std::span<const std::string_view> examples;",
        "        std::span<const std::string_view> notes;",
        "    };",
        "",
        "    extern const std::string_view APP_NAME;",
        "    extern const std::string_view APP_SUMMARY;",
        "",
        "    /**",
        "     * @brief The global (application-wide) CLI options, e.g. -h/--help, --log, --json.",
        "     */",
        "    extern const std::span<const CliOptionMetadata> GLOBAL_OPTIONS;",
        "",
        "    /**",
        "     * @brief All top-level CLI commands (run, record, config, apps, serve, help).",
        "     */",
        "    extern const std::span<const CliCommandMetadata> ALL_COMMANDS;",
        "",
        "    /**",
        "     * @brief Finds a top-level command by name.",
        "     * @param name The command name.",
        "     * @return Pointer to the CliCommandMetadata, or nullptr if not found.",
        "     */",
        "    [[nodiscard]] const CliCommandMetadata* findCommand(std::string_view name);",
        "",
        "    /**",
        "     * @brief Finds a subcommand within a command's subcommand list by name.",
        "     * @param command The parent command.",
        "     * @param name The subcommand name.",
        "     * @return Pointer to the CliCommandMetadata, or nullptr if not found.",
        "     */",
        "    [[nodiscard]] const CliCommandMetadata* findSubcommand(const CliCommandMetadata& command, std::string_view name);",
        "}",
        "",
        "#endif // INCLUDE_AUTOINPUT_CLI_CLIHELPMETADATA_H",
        "",
    ]
    return eol.join(header_lines)


def generate_source_content(metadata: CliHelpMetadata, eol: str = "\n") -> str:
    generator = CppGenerator(metadata)
    option_defs, command_defs = generator.generate()

    source_lines = [
        "/**",
        " * @file cliHelpMetadata.cpp",
        f" * @brief {GENERATED_HEADER_COMMENT}",
        " */",
        '#include "autoinput/cli/cliHelpMetadata.h"',
        "#include <algorithm>",
        "",
        "using namespace std::string_view_literals;",
        "",
        "namespace autoinput::cli::HelpMetadata",
        "{",
        f'    const std::string_view APP_NAME = "{_escape(metadata.app_name)}"sv;',
        f'    const std::string_view APP_SUMMARY = "{_escape(metadata.app_summary)}"sv;',
        "",
        "namespace",
        "{",
        option_defs,
        "",
        command_defs,
        "}",
        "",
        "    const std::span<const CliOptionMetadata> GLOBAL_OPTIONS = g_global_options;",
        "    const std::span<const CliCommandMetadata> ALL_COMMANDS = g_all_commands;",
        "",
        "    const CliCommandMetadata* findCommand(const std::string_view name)",
        "    {",
        "        const auto it = std::ranges::find_if(ALL_COMMANDS, [name](const CliCommandMetadata& cmd) { return cmd.name == name; });",
        "        return it != ALL_COMMANDS.end() ? &(*it) : nullptr;",
        "    }",
        "",
        "    const CliCommandMetadata* findSubcommand(const CliCommandMetadata& command, const std::string_view name)",
        "    {",
        "        const auto it = std::ranges::find_if(command.subcommands, [name](const CliCommandMetadata& cmd) { return cmd.name == name; });",
        "        return it != command.subcommands.end() ? &(*it) : nullptr;",
        "    }",
        "}",
        "",
    ]
    return eol.join(source_lines)


def check_and_update_file_contents(filepath: pathlib.Path, contents: str, filetype: str, check_only: bool) -> bool:
    if check_only:
        if not filepath.exists():
            logger.error(f"Output {filetype} file does not exist: {filepath}")
            return False
        try:
            existing = filepath.read_text(encoding="utf-8")
            if existing == contents:
                logger.info(f"CLI help {filetype} {filepath} is up to date.")
                return True
            logger.error(f"CLI help {filetype} {filepath} is out of date.")
            return False
        except Exception as e:
            logger.error(f"Failed to read existing {filetype} {filepath}: {e}")
            return False

    try:
        filepath.parent.mkdir(parents=True, exist_ok=True)
        if filepath.exists():
            try:
                existing = filepath.read_text(encoding="utf-8")
                if existing == contents:
                    logger.info(f"CLI help {filetype} {filepath} is already up to date.")
                    return True
            except Exception:
                pass

        with filepath.open("w", encoding="utf-8", newline="\n") as f:
            f.write(contents)
        logger.info(f"Successfully generated {filepath}.")
        return True
    except Exception as e:
        logger.error(f"Failed to write {filetype} to {filepath}: {e}")
        return False


def generate(
    toml_path: pathlib.Path = DEFAULT_HELP_TOML,
    header_path: pathlib.Path = DEFAULT_HEADER_OUTPUT_FILE,
    source_path: pathlib.Path = DEFAULT_SOURCE_OUTPUT_FILE,
    check_only: bool = False,
    eol: str = "\n",
) -> bool:
    try:
        metadata = load_cli_help_metadata(toml_path)
    except (FileNotFoundError, CliHelpValidationError) as e:
        logger.error(f"Failed to load CLI help metadata from {toml_path}: {e}")
        return False

    header_content = generate_header_content(eol=eol)
    source_content = generate_source_content(metadata, eol=eol)

    if not check_and_update_file_contents(header_path, header_content, "header", check_only):
        return False
    return check_and_update_file_contents(source_path, source_content, "source", check_only)


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Generate C++ CLI help metadata (cliHelpMetadata.h/.cpp) from resources/cli/help.toml.",
    )
    _ = parser.add_argument(
        "--toml",
        type=pathlib.Path,
        default=DEFAULT_HELP_TOML,
        help=f"Path to source TOML file (default: {DEFAULT_HELP_TOML})",
    )
    _ = parser.add_argument(
        "--header",
        type=pathlib.Path,
        default=DEFAULT_HEADER_OUTPUT_FILE,
        help=f"Path to output header file (default: {DEFAULT_HEADER_OUTPUT_FILE})",
    )
    _ = parser.add_argument(
        "--source",
        type=pathlib.Path,
        default=DEFAULT_SOURCE_OUTPUT_FILE,
        help=f"Path to output source file (default: {DEFAULT_SOURCE_OUTPUT_FILE})",
    )
    _ = parser.add_argument(
        "--check",
        action="store_true",
        help="Check if output files are up to date without modifying them.",
    )
    return parser


def main():
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    args = get_parser().parse_args()
    success = generate(
        toml_path=args.toml,
        header_path=args.header,
        source_path=args.source,
        check_only=args.check,
    )
    if not success:
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
