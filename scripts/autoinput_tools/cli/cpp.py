"""C++ rendering for CLI help metadata."""

import logging
import pathlib
from .model import CliCommand, CliHelpMetadata, CliOption

logger = logging.getLogger(__name__)

GENERATED_HEADER_COMMENT = (
    "AUTO-GENERATED \u2014 DO NOT EDIT. "
    "Generated from resources/cli/help.toml by scripts/gen_cli_help.py."
)


def _escape(value: str) -> str:
    """Escape a string for embedding in a C++ string literal."""
    return (
        value.replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace("\r", "\\r")
        .replace("\n", "\\n")
        .replace("\t", "\\t")
    )


def _cpp_string_view_array(values: list[str]) -> str:
    """Render a list of strings as a C++ initializer list of string_view literals."""
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


def generate_cli_help_header(metadata: CliHelpMetadata | None = None, eol: str = "\n") -> str:
    """Generate the C++ header file content for CLI help metadata."""
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


def generate_cli_help_source(metadata: CliHelpMetadata, eol: str = "\n") -> str:
    """Generate the C++ source file content for CLI help metadata."""
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


def generate_cli_help_content(
    metadata: CliHelpMetadata,
    eol: str = "\n",
) -> tuple[str, str]:
    """Generate C++ header and source content for CLI help metadata."""
    header = generate_cli_help_header(metadata, eol=eol)
    source = generate_cli_help_source(metadata, eol=eol)
    return header, source
