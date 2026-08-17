#!/usr/bin/env python
import argparse
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys
from dataclasses import dataclass, field

try:
    from . import utils
except ImportError:
    try:
        import utils
    except ImportError:
        sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
        import utils

logger = logging.getLogger(__name__)

# Default CLI metadata and common values
DEFAULT_LOG_LEVELS = ["d", "debug", "i", "info", "w", "warn", "warning", "e", "error", "f", "fatal"]
DEFAULT_ACTION_TYPES = ["click", "c", "hold", "h"]
DEFAULT_MOUSE_BUTTONS = ["left", "l", "right", "r", "middle", "m", "back", "forward"]
DEFAULT_NOTIFICATION_MODES = ["off", "console", "desktop", "both"]
MODIFIERS = ["ctrl+", "shift+", "alt+", "meta+"]

COMMON_KEYS = [
    "a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m",
    "n", "o", "p", "q", "r", "s", "t", "u", "v", "w", "x", "y", "z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "f10", "f11", "f12",
    "esc", "escape", "space", "tab", "enter", "return", "backspace", "ins", "insert",
    "del", "delete", "home", "end", "pageup", "pgup", "pagedown", "pgdn",
    "up", "down", "left", "right", "capslock", "numlock", "scrolllock", "printscreen", "prtsc", "pause",
]


@dataclass
class OptionInfo:
    flags: list[str]
    arg_placeholder: str | None = None
    description: str = ""


@dataclass
class CommandInfo:
    name: str
    description: str
    options: list[OptionInfo] = field(default_factory=list)
    subcommands: dict[str, "CommandInfo"] = field(default_factory=dict)


@dataclass
class CliMetadata:
    global_options: list[OptionInfo] = field(default_factory=list)
    commands: dict[str, CommandInfo] = field(default_factory=dict)
    log_levels: list[str] = field(default_factory=lambda: list(DEFAULT_LOG_LEVELS))
    action_types: list[str] = field(default_factory=lambda: list(DEFAULT_ACTION_TYPES))
    mouse_buttons: list[str] = field(default_factory=lambda: list(DEFAULT_MOUSE_BUTTONS))
    notification_modes: list[str] = field(default_factory=lambda: list(DEFAULT_NOTIFICATION_MODES))
    common_keys: list[str] = field(default_factory=lambda: list(COMMON_KEYS))
    modifiers: list[str] = field(default_factory=lambda: list(MODIFIERS))


def find_autoinput_binary(binary_arg: str | None = None) -> pathlib.Path | None:
    """Find the autoinput executable from an explicit argument or search common build locations."""
    if binary_arg:
        p = pathlib.Path(binary_arg).resolve()
        if p.exists() and p.is_file():
            return p
        logger.warning(f"Specified binary path does not exist: {binary_arg}")
        return None

    root = utils.ROOT_DIR
    exe_name = "autoinput.exe" if sys.platform == "win32" else "autoinput"

    candidates = [
        root / "build" / "debug" / "bin" / exe_name,
        root / "build" / "release" / "bin" / exe_name,
        root / "build" / "debug" / exe_name,
        root / "build" / "release" / exe_name,
        root / "build" / "bin" / exe_name,
        root / "cmake-build-debug" / "bin" / exe_name,
        root / "cmake-build-debug" / "src" / "autoinput_cli" / exe_name,
        root / "cmake-build-debug" / exe_name,
        root / "cmake-build-release" / "bin" / exe_name,
        root / "cmake-build-release" / "src" / "autoinput_cli" / exe_name,
        root / "cmake-build-release" / exe_name,
        root / "bin" / exe_name,
    ]

    for candidate in candidates:
        if candidate.exists() and candidate.is_file():
            return candidate

    which_path = shutil.which("autoinput.exe" if sys.platform == "win32" else "autoinput")
    if which_path:
        return pathlib.Path(which_path).resolve()

    return None


def run_help_command(binary: pathlib.Path, args: list[str]) -> str:
    """Run the binary with given arguments and return its stdout text."""
    try:
        res = subprocess.run(
            [str(binary)] + args,
            capture_output=True,
            text=True,
            encoding="utf-8",
            errors="replace",
            check=False,
        )
        return res.stdout
    except Exception as e:
        logger.warning(f"Failed to run '{binary} {' '.join(args)}': {e}")
        return ""


def parse_help_sections(text: str) -> dict[str, list[str]]:
    """Split help output into sections based on section headers (e.g. 'Commands:', 'Options:')."""
    sections: dict[str, list[str]] = {}
    current_section = None

    for raw_line in text.splitlines():
        line = raw_line.rstrip()
        # Look for section headers: word followed by colon on its own
        header_match = re.match(r"^([A-Z][a-zA-Z0-9_ ]*):$", line)
        if header_match:
            current_section = header_match.group(1).strip()
            sections.setdefault(current_section, [])
            continue

        if current_section and line.strip():
            sections[current_section].append(line)

    return sections


def parse_option_line(line: str) -> OptionInfo | None:
    """Parse a single option line such as '  -c, --config NAME_OR_PATH  Load a TOML configuration'."""
    stripped = line.strip()
    if not stripped.startswith("-"):
        return None

    parts = re.split(r"\s{2,}", stripped, maxsplit=1)
    spec = parts[0].strip()
    description = parts[1].strip() if len(parts) > 1 else ""

    flags = re.findall(r"-[a-zA-Z0-9]|--[a-zA-Z0-9-]+", spec)
    if not flags:
        return None

    clean_spec = spec
    for f in sorted(flags, key=len, reverse=True):
        clean_spec = clean_spec.replace(f, "")
    arg_placeholder = re.sub(r"^[,\s]+|[,\s]+$", "", clean_spec).strip() or None

    return OptionInfo(
        flags=flags,
        arg_placeholder=arg_placeholder,
        description=description,
    )


def infer_completion_type(opt: OptionInfo) -> str:
    """Infer semantic completion type from option flags and argument placeholder."""
    if not opt.arg_placeholder:
        return "none"

    placeholder = (opt.arg_placeholder or "").lower()
    flag_str = " ".join(opt.flags).lower()

    if "config" in flag_str or "name_or_path" in placeholder:
        return "config"
    elif "type" in flag_str or "click|hold" in placeholder:
        return "action_type"
    elif "button" in flag_str or "btn" in flag_str:
        return "mouse_button"
    elif "key" in flag_str or "start" in flag_str or "end" in flag_str or "play-start" in flag_str:
        return "key"
    elif "log" in flag_str or "level" in placeholder:
        return "log_level"
    elif "status-notification" in flag_str or "mode" in placeholder:
        return "notification_mode"
    elif "app" in flag_str or "application" in placeholder or "blacklist" in flag_str:
        return "application"
    elif "wait" in flag_str or "range" in placeholder:
        return "range"
    elif "sample" in flag_str or "time" in placeholder:
        return "time"

    return "other"


def infer_zsh_action(opt: OptionInfo) -> str:
    """Infer Zsh completion action specifier from OptionInfo."""
    ctype = infer_completion_type(opt)
    if ctype == "none":
        return ""
    elif ctype == "config":
        return ":config file:_autoinput_configs"
    elif ctype == "action_type":
        return ":action type:($action_types)"
    elif ctype == "mouse_button":
        return ":mouse button:($mouse_buttons)"
    elif ctype == "key":
        return ":key:($keys_with_mods)"
    elif ctype == "log_level":
        return ":log level:($log_levels)"
    elif ctype == "notification_mode":
        return ":mode:($notification_modes)"
    elif ctype == "application":
        return ":application:"
    elif ctype == "range":
        return ":range:"
    elif ctype == "time":
        return ":time:"
    else:
        label = (opt.arg_placeholder or "arg").lower()
        return f":{label}:"


def format_zsh_option(opt: OptionInfo) -> str:
    """Format OptionInfo into a Zsh _arguments option string."""
    flags = opt.flags
    if len(flags) > 1:
        exclusion = f"({' '.join(flags)})"
        flag_pattern = "{" + ",".join(flags) + "}"
    else:
        exclusion = ""
        flag_pattern = flags[0]

    desc = opt.description.replace("[", r"\[").replace("]", r"\]")
    if desc.endswith("."):
        desc = desc[:-1]
    action = infer_zsh_action(opt)

    return f"'{exclusion}{flag_pattern}[{desc}]{action}'"


def format_lua_option_flags(options: list[OptionInfo]) -> list[str]:
    """Format a list of OptionInfo into Lua set_flags entries."""
    flags_list: list[str] = []
    for opt in options:
        ctype = infer_completion_type(opt)
        for flag in opt.flags:
            if ctype == "none":
                flags_list.append(f'"{flag}"')
            elif ctype == "config":
                flags_list.append(f'"{flag}" .. clink.arg.new_parser({{configs_matcher}})')
            elif ctype == "action_type":
                flags_list.append(f'"{flag}" .. clink.arg.new_parser(action_types)')
            elif ctype == "mouse_button":
                flags_list.append(f'"{flag}" .. clink.arg.new_parser(mouse_buttons)')
            elif ctype == "key":
                flags_list.append(f'"{flag}" .. clink.arg.new_parser(keys_with_mods)')
            elif ctype == "log_level":
                flags_list.append(f'"{flag}" .. clink.arg.new_parser(log_levels)')
            elif ctype == "notification_mode":
                flags_list.append(f'"{flag}" .. clink.arg.new_parser(notification_modes)')
            else:
                flags_list.append(f'"{flag}" .. clink.arg.new_parser()')
    return flags_list


def parse_command_line(line: str) -> tuple[str, str] | None:
    """Parse a command line such as '  run [options]  Run input automation...'."""
    stripped = line.strip()
    if not stripped:
        return None

    parts = re.split(r"\s{2,}", stripped, maxsplit=1)
    spec = parts[0]
    description = parts[1].strip() if len(parts) > 1 else ""

    name = spec.split()[0]
    return name, description


def extract_metadata_from_binary(binary: pathlib.Path) -> CliMetadata:
    """Extract CLI metadata by dynamically querying the binary with --help."""
    logger.info(f"Extracting CLI metadata from binary: {binary}")
    metadata = CliMetadata()

    root_help = run_help_command(binary, ["--help"])
    root_sections = parse_help_sections(root_help)

    # 1. Parse top-level commands
    commands_lines = root_sections.get("Commands", [])
    for line in commands_lines:
        parsed = parse_command_line(line)
        if parsed:
            cmd_name, cmd_desc = parsed
            metadata.commands[cmd_name] = CommandInfo(name=cmd_name, description=cmd_desc)

    # 2. For each command, parse options, global options, and subcommands
    for cmd_name, cmd_info in list(metadata.commands.items()):
        if cmd_name == "help":
            continue

        cmd_help = run_help_command(binary, [cmd_name, "--help"])
        cmd_sections = parse_help_sections(cmd_help)

        # Parse global options if not yet populated
        if not metadata.global_options and "Global options" in cmd_sections:
            for line in cmd_sections["Global options"]:
                opt = parse_option_line(line)
                if opt:
                    metadata.global_options.append(opt)

        # Parse command options
        if "Options" in cmd_sections:
            for line in cmd_sections["Options"]:
                opt = parse_option_line(line)
                if opt:
                    cmd_info.options.append(opt)

        # Parse subcommands if present
        if "Commands" in cmd_sections:
            for line in cmd_sections["Commands"]:
                parsed = parse_command_line(line)
                if parsed:
                    sub_name, sub_desc = parsed
                    # Avoid recursive duplicate entries if root commands were listed
                    if sub_name not in metadata.commands or cmd_name in ("config", "apps"):
                        cmd_info.subcommands[sub_name] = CommandInfo(name=sub_name, description=sub_desc)

    # If global options weren't captured from command help, use defaults
    if not metadata.global_options:
        metadata.global_options = [
            OptionInfo(flags=["-h", "--help"], description="Show help"),
            OptionInfo(flags=["--examples"], description="Show examples"),
            OptionInfo(flags=["-l", "--log"], arg_placeholder="LEVEL", description="Set log level: debug, info, warning, error"),
            OptionInfo(flags=["--json"], description="Output JSON where supported"),
        ]

    # Ensure standard choice lists and keys are present
    metadata.log_levels = list(DEFAULT_LOG_LEVELS)
    metadata.action_types = list(DEFAULT_ACTION_TYPES)
    metadata.mouse_buttons = list(DEFAULT_MOUSE_BUTTONS)
    metadata.notification_modes = list(DEFAULT_NOTIFICATION_MODES)
    metadata.common_keys = list(COMMON_KEYS)
    metadata.modifiers = list(MODIFIERS)

    return metadata


def get_default_metadata() -> CliMetadata:
    """Return fallback CLI metadata when no binary is available."""
    metadata = CliMetadata()
    metadata.global_options = [
        OptionInfo(flags=["-h", "--help"], description="Show help"),
        OptionInfo(flags=["-l", "--log"], arg_placeholder="LEVEL", description="Set log level: debug, info, warning, error"),
        OptionInfo(flags=["--json"], description="Output JSON where supported"),
    ]

    run_cmd = CommandInfo(name="run", description="Run input automation from command options or a TOML configuration.")
    run_cmd.options = [
        OptionInfo(flags=["-c", "--config"], arg_placeholder="NAME_OR_PATH", description="Load a TOML configuration"),
        OptionInfo(flags=["-t", "--type"], arg_placeholder="click|hold", description="Set action type"),
        OptionInfo(flags=["-b", "--button"], arg_placeholder="BUTTON", description="Mouse button target"),
        OptionInfo(flags=["-k", "--key"], arg_placeholder="KEY", description="Keyboard key target"),
        OptionInfo(flags=["-s", "--start"], arg_placeholder="KEY", description="Start/toggle trigger"),
        OptionInfo(flags=["-e", "--end"], arg_placeholder="KEY", description="Stop trigger"),
        OptionInfo(flags=["-a", "--app"], arg_placeholder="APPLICATION", description="Only run while application is focused"),
        OptionInfo(flags=["-B", "--blacklist"], arg_placeholder="APPLICATION", description="Pause while application is focused"),
        OptionInfo(flags=["-w", "--wait"], arg_placeholder="RANGE", description="Alias for --press-wait"),
        OptionInfo(flags=["--press-wait"], arg_placeholder="RANGE", description="Delay while target is pressed"),
        OptionInfo(flags=["--release-wait"], arg_placeholder="RANGE", description="Delay between repeated actions"),
        OptionInfo(flags=["--status-notification"], arg_placeholder="MODE", description="off, console, desktop, both"),
        OptionInfo(flags=["-S", "--save-config"], arg_placeholder="NAME", description="Save current options as a user config"),
    ]

    record_cmd = CommandInfo(name="record", description="Record input events and save them as a replayable configuration.")
    record_cmd.options = [
        OptionInfo(flags=["--start"], arg_placeholder="KEY", description="Key that starts recording"),
        OptionInfo(flags=["--end"], arg_placeholder="KEY", description="Key that stops recording"),
        OptionInfo(flags=["--play-start"], arg_placeholder="KEY", description="Key used to play the recorded sequence"),
        OptionInfo(flags=["--mouse-moves"], description="Record mouse movement events"),
        OptionInfo(flags=["--mouse-sample"], arg_placeholder="TIME", description="Mouse movement sampling interval"),
        OptionInfo(flags=["--force"], description="Overwrite destination config if it exists"),
    ]

    config_cmd = CommandInfo(name="config", description="Manage autoinput configuration files.")
    config_cmd.subcommands = {
        "list": CommandInfo(name="list", description="List available configurations"),
        "validate": CommandInfo(name="validate", description="Validate a configuration file"),
        "duplicate": CommandInfo(name="duplicate", description="Duplicate a configuration into the user config directory"),
        "copy": CommandInfo(name="copy", description="Alias for duplicate"),
        "path": CommandInfo(name="path", description="Print the path to the configuration."),
    }

    apps_cmd = CommandInfo(name="apps", description="Inspect running applications.")
    apps_cmd.subcommands = {
        "list": CommandInfo(name="list", description="List currently running application names"),
    }

    serve_cmd = CommandInfo(name="serve", description="Starts the automation runtime server.")
    serve_cmd.options = [
        OptionInfo(flags=["--stdio"], description="Use standard input/output for the protocol."),
    ]

    help_cmd = CommandInfo(name="help", description="Show help for autoinput commands.")

    metadata.commands = {
        "run": run_cmd,
        "record": record_cmd,
        "config": config_cmd,
        "apps": apps_cmd,
        "serve": serve_cmd,
        "help": help_cmd,
    }
    return metadata


def generate_zsh_completion(metadata: CliMetadata, eol: str = "\n") -> str:
    """Generate Zsh completion script (_autoinput) from CLI metadata."""
    lines = [
        "#compdef autoinput",
        "",
        "_autoinput() {",
        "    local line",
        "    local -a log_levels action_types mouse_buttons notification_modes common_keys modifiers all_pos",
        f"    log_levels=({' '.join(metadata.log_levels)})",
        f"    action_types=({' '.join(metadata.action_types)})",
        f"    mouse_buttons=({' '.join(metadata.mouse_buttons)})",
        f"    notification_modes=({' '.join(metadata.notification_modes)})",
        f"    modifiers=({' '.join(metadata.modifiers)})",
        "    common_keys=(",
        "        " + " ".join(metadata.common_keys[:26]),
        "        " + " ".join(metadata.common_keys[26:36]),
        "        " + " ".join(metadata.common_keys[36:48]),
        "        " + " ".join(metadata.common_keys[48:57]),
        "        " + " ".join(metadata.common_keys[57:65]),
        "        " + " ".join(metadata.common_keys[65:]),
        "    )",
        "",
        "    local -a keys_with_mods",
        "    keys_with_mods=($common_keys)",
        "",
        "    for mod in $modifiers; do",
        "        for k in $common_keys; do",
        '            keys_with_mods+=("${mod}${k}")',
        "        done",
        "    done",
        "",
        "    _arguments -C \\",
    ]

    root_args = [format_zsh_option(opt) for opt in metadata.global_options]
    root_args.extend(["'1: :->command'", "'*:: :->args'"])
    for i, arg_str in enumerate(root_args):
        sep = " \\" if i < len(root_args) - 1 else ""
        lines.append(f"        {arg_str}{sep}")

    lines.extend([
        "",
        "    case $state in",
        "        command)",
        "            local -a subcommands",
        "            subcommands=(",
    ])

    for cmd_name, cmd_info in metadata.commands.items():
        desc = cmd_info.description.split(".")[0].strip() or cmd_name
        lines.append(f"                '{cmd_name}:{desc}'")

    lines.extend([
        "            )",
        "            _describe 'command' subcommands",
        "            ;;",
        "        args)",
        "            case $line[1] in",
    ])

    # Command: run
    run_cmd = metadata.commands.get("run", CommandInfo("run", ""))
    run_opts = [format_zsh_option(opt) for opt in run_cmd.options]
    lines.append("                run)")
    lines.append("                    _arguments \\")
    for i, opt_str in enumerate(run_opts):
        sep = " \\" if i < len(run_opts) - 1 else ""
        lines.append(f"                        {opt_str}{sep}")
    lines.append("                    ;;")

    # Command: record
    record_cmd = metadata.commands.get("record", CommandInfo("record", ""))
    record_opts = ["':name: '"] + [format_zsh_option(opt) for opt in record_cmd.options]
    lines.append("                record)")
    lines.append("                    _arguments \\")
    for i, opt_str in enumerate(record_opts):
        sep = " \\" if i < len(record_opts) - 1 else ""
        lines.append(f"                        {opt_str}{sep}")
    lines.append("                    ;;")

    # Command: config
    lines.append("                config)")
    lines.append("                    if (( CURRENT == 2 )); then")
    lines.append("                        local -a config_subcommands")
    lines.append("                        config_subcommands=(")
    config_subcmds = metadata.commands.get("config", CommandInfo("config", "")).subcommands
    for sub_name, sub_info in config_subcmds.items():
        desc = sub_info.description.split(".")[0].strip() or sub_name
        lines.append(f"                            '{sub_name}:{desc}'")
    lines.extend([
        "                        )",
        "                        _describe 'config command' config_subcommands",
        "                    else",
        "                        case $line[2] in",
        "                            validate|path)",
        "                                _arguments ':config file:_autoinput_configs'",
        "                                ;;",
        "                            duplicate|copy)",
        "                                _arguments \\",
        "                                    ':source config:_autoinput_configs' \\",
        "                                    ':destination name: ' \\",
        "                                    '--force[Overwrite destination if it already exists]'",
        "                                ;;",
        "                        esac",
        "                    fi",
        "                    ;;",
    ])

    # Command: apps
    apps_subcmds = metadata.commands.get("apps", CommandInfo("apps", "")).subcommands
    lines.append("                apps)")
    lines.append("                    if (( CURRENT == 2 )); then")
    lines.append("                        local -a apps_subcommands")
    lines.append("                        apps_subcommands=(")
    for sub_name, sub_info in apps_subcmds.items():
        desc = sub_info.description.split(".")[0].strip() or sub_name
        lines.append(f"                            '{sub_name}:{desc}'")
    lines.extend([
        "                        )",
        "                        _describe 'apps command' apps_subcommands",
        "                    fi",
        "                    ;;",
    ])

    # Command: serve
    serve_cmd = metadata.commands.get("serve", CommandInfo("serve", ""))
    serve_opts = [format_zsh_option(opt) for opt in serve_cmd.options]
    lines.append("                serve)")
    lines.append("                    _arguments \\")
    for i, opt_str in enumerate(serve_opts):
        sep = " \\" if i < len(serve_opts) - 1 else ""
        lines.append(f"                        {opt_str}{sep}")
    lines.append("                    ;;")

    # Command: help
    help_commands = " ".join(c for c in metadata.commands.keys() if c != "help")
    config_subs_str = " ".join(config_subcmds.keys())
    lines.extend([
        "                help)",
        "                    if (( CURRENT == 2 )); then",
        f"                        _arguments ':command:({help_commands})'",
        '                    elif (( CURRENT == 3 )) && [[ $line[2] == "config" ]]; then',
        f"                        _arguments ':subcommand:({config_subs_str})'",
        "                    fi",
        "                    ;;",
        "            esac",
        "            ;;",
        "    esac",
        "}",
        "",
        "_autoinput_configs() {",
        "    local -a configs",
        "    # Program looks for configs/ directory relative to its executable.",
        "    # For shell completion, we assume it's in the current directory, ../configs, or ~/.autoinput.",
        "    configs=(",
        "        configs/*.toml(N:t:r)",
        "        ../configs/*.toml(N:t:r)",
        "        ~/.autoinput/*.toml(N:t:r)",
        "    )",
        "    typeset -U configs",
        "    if [[ ${#configs} -gt 0 ]]; then",
        "        _describe 'configs' configs",
        "    fi",
        "}",
        "",
        '_autoinput "$@"',
        "",
    ])
    return eol.join(lines)


def generate_bash_completion(metadata: CliMetadata, eol: str = "\n") -> str:
    """Generate Bash completion script (autoinput_completion.bash) from CLI metadata."""
    commands_str = " ".join(metadata.commands.keys())
    global_opts_str = " ".join(f for opt in metadata.global_options for f in opt.flags)
    log_levels_str = " ".join(metadata.log_levels)
    action_types_str = " ".join(metadata.action_types)
    mouse_buttons_str = " ".join(metadata.mouse_buttons)
    notification_modes_str = " ".join(metadata.notification_modes)
    common_keys_str = " ".join(metadata.common_keys)

    run_cmd = metadata.commands.get("run", CommandInfo("run", ""))
    run_flags = " ".join(f for opt in run_cmd.options for f in opt.flags)

    # Group run flags by completion type for case "$prev"
    run_config_flags = "|".join(f for opt in run_cmd.options if infer_completion_type(opt) == "config" for f in opt.flags)
    run_action_flags = "|".join(f for opt in run_cmd.options if infer_completion_type(opt) == "action_type" for f in opt.flags)
    run_mouse_flags = "|".join(f for opt in run_cmd.options if infer_completion_type(opt) == "mouse_button" for f in opt.flags)
    run_key_flags = "|".join(f for opt in run_cmd.options if infer_completion_type(opt) == "key" for f in opt.flags)
    run_notify_flags = "|".join(f for opt in run_cmd.options if infer_completion_type(opt) == "notification_mode" for f in opt.flags)
    run_log_flags = "|".join(f for opt in run_cmd.options if infer_completion_type(opt) == "log_level" for f in opt.flags)

    record_cmd = metadata.commands.get("record", CommandInfo("record", ""))
    record_flags = " ".join(f for opt in record_cmd.options for f in opt.flags)
    record_key_flags = "|".join(f for opt in record_cmd.options if infer_completion_type(opt) == "key" for f in opt.flags)

    config_subcmds = metadata.commands.get("config", CommandInfo("config", "")).subcommands
    config_subs_str = " ".join(config_subcmds.keys())

    apps_subcmds = metadata.commands.get("apps", CommandInfo("apps", "")).subcommands
    apps_subs_str = " ".join(apps_subcmds.keys())

    serve_cmd = metadata.commands.get("serve", CommandInfo("serve", ""))
    serve_flags = " ".join(f for opt in serve_cmd.options for f in opt.flags)

    help_cmds_str = " ".join(c for c in metadata.commands.keys() if c != "help")

    lines = [
        "# autoinput bash completion",
        "",
        "_autoinput_configs() {",
        '    local cur="${COMP_WORDS[COMP_CWORD]}"',
        "    local configs=( $(ls configs/*.toml ../configs/*.toml ~/.autoinput/*.toml 2>/dev/null | xargs -n1 basename | sed 's/\\.toml$//' | sort -u) )",
        '    COMPREPLY=( $(compgen -W "${configs[*]}" -- "$cur") )',
        "}",
        "",
        "_autoinput() {",
        "    local cur prev words cword",
        "    _get_comp_words_by_ref -n : cur prev words cword",
        "",
        f'    local commands="{commands_str}"',
        f'    local global_opts="{global_opts_str}"',
        "    ",
        f'    local log_levels="{log_levels_str}"',
        f'    local action_types="{action_types_str}"',
        f'    local mouse_buttons="{mouse_buttons_str}"',
        f'    local notification_modes="{notification_modes_str}"',
        f'    local common_keys="{common_keys_str}"',
        "    ",
        "    # Simple check for command",
        '    local command=""',
        "    local i",
        "    for ((i=1; i < cword; i++)); do",
        '        if [[ " ${commands} " == *" ${words[i]} "* ]]; then',
        '            command="${words[i]}"',
        "            break",
        "        fi",
        "    done",
        "",
        '    if [[ -z "$command" ]]; then',
        '        if [[ "$cur" == -* ]]; then',
        '            COMPREPLY=( $(compgen -W "${global_opts}" -- "$cur") )',
        "        else",
        '            COMPREPLY=( $(compgen -W "${commands}" -- "$cur") )',
        "        fi",
        "        return 0",
        "    fi",
        "",
        '    case "$command" in',
        "        run)",
        '            case "$prev" in',
    ]

    if run_config_flags:
        lines.extend([
            f"                {run_config_flags})",
            "                    _autoinput_configs",
            "                    return 0",
            "                    ;;",
        ])
    if run_action_flags:
        lines.extend([
            f"                {run_action_flags})",
            '                    COMPREPLY=( $(compgen -W "${action_types}" -- "$cur") )',
            "                    return 0",
            "                    ;;",
        ])
    if run_mouse_flags:
        lines.extend([
            f"                {run_mouse_flags})",
            '                    COMPREPLY=( $(compgen -W "${mouse_buttons}" -- "$cur") )',
            "                    return 0",
            "                    ;;",
        ])
    if run_key_flags:
        lines.extend([
            f"                {run_key_flags})",
            '                    COMPREPLY=( $(compgen -W "${common_keys}" -- "$cur") )',
            "                    return 0",
            "                    ;;",
        ])
    if run_notify_flags:
        lines.extend([
            f"                {run_notify_flags})",
            '                    COMPREPLY=( $(compgen -W "${notification_modes}" -- "$cur") )',
            "                    return 0",
            "                    ;;",
        ])
    if run_log_flags:
        lines.extend([
            f"                {run_log_flags})",
            '                    COMPREPLY=( $(compgen -W "${log_levels}" -- "$cur") )',
            "                    return 0",
            "                    ;;",
        ])

    lines.extend([
        "            esac",
        f'            COMPREPLY=( $(compgen -W "{run_flags}" -- "$cur") )',
        "            ;;",
        "        record)",
        '            case "$prev" in',
    ])

    if record_key_flags:
        lines.extend([
            f"                {record_key_flags})",
            '                    COMPREPLY=( $(compgen -W "${common_keys}" -- "$cur") )',
            "                    return 0",
            "                    ;;",
        ])

    lines.extend([
        "            esac",
        f'            COMPREPLY=( $(compgen -W "{record_flags}" -- "$cur") )',
        "            ;;",
        "        config)",
        f'            local subcommands="{config_subs_str}"',
        '            local subcmd=""',
        "            for ((i=1; i < cword; i++)); do",
        '                if [[ " ${subcommands} " == *" ${words[i]} "* ]]; then',
        '                    subcmd="${words[i]}"',
        "                    break",
        "                fi",
        "            done",
        "",
        '            if [[ -z "$subcmd" ]]; then',
        '                COMPREPLY=( $(compgen -W "${subcommands}" -- "$cur") )',
        "            else",
        '                case "$subcmd" in',
        "                    validate|path)",
        '                        if [[ "$prev" == "$subcmd" ]]; then',
        "                            _autoinput_configs",
        "                            return 0",
        "                        fi",
        "                        ;;",
        "                    duplicate|copy)",
        '                        if [[ "$prev" == "$subcmd" ]]; then',
        "                            _autoinput_configs",
        "                            return 0",
        "                        fi",
        "                        ;;",
        "                esac",
        '                if [[ "$subcmd" == "duplicate" || "$subcmd" == "copy" ]]; then',
        '                    COMPREPLY=( $(compgen -W "--force" -- "$cur") )',
        "                fi",
        "            fi",
        "            ;;",
        "        apps)",
        f'            COMPREPLY=( $(compgen -W "{apps_subs_str}" -- "$cur") )',
        "            ;;",
        "        serve)",
        f'            COMPREPLY=( $(compgen -W "{serve_flags}" -- "$cur") )',
        "            ;;",
        "        help)",
        "            if [[ $cword -eq 2 ]]; then",
        f'                COMPREPLY=( $(compgen -W "{help_cmds_str}" -- "$cur") )',
        '            elif [[ $cword -eq 3 && "${words[1]}" == "config" ]]; then',
        f'                COMPREPLY=( $(compgen -W "{config_subs_str}" -- "$cur") )',
        "            fi",
        "            ;;",
        "    esac",
        "}",
        "",
        "complete -F _autoinput autoinput",
        "",
    ])
    return eol.join(lines)


def generate_lua_completion(metadata: CliMetadata, eol: str = "\n") -> str:
    """Generate Clink Lua completion script (autoinput_completion.lua) from CLI metadata."""
    log_levels_str = ", ".join(f'"{x}"' for x in metadata.log_levels)
    action_types_str = ", ".join(f'"{x}"' for x in metadata.action_types)
    mouse_buttons_str = ", ".join(f'"{x}"' for x in metadata.mouse_buttons)
    notification_modes_str = ", ".join(f'"{x}"' for x in metadata.notification_modes)
    modifiers_str = ", ".join(f'"{x}"' for x in metadata.modifiers)

    def quote_list(lst: list[str]) -> str:
        return ", ".join(f'"{x}"' for x in lst)

    lines = [
        "-- autoinput clink completion script",
        "-- To use, place this file in a directory that Clink scans for scripts,",
        "-- or add that directory to Clink's lua.path.",
        "",
        "local function configs_matcher(word)",
        "    local configs = {}",
        '    local paths = {"configs", "../configs"}',
        "    ",
        '    local home = os.getenv("USERPROFILE") or os.getenv("HOME")',
        "    if home then",
        '        table.insert(paths, home .. "/.autoinput")',
        "    end",
        "",
        "    for _, path in ipairs(paths) do",
        '        local cmd = \'dir /b "\' .. path .. \'" 2>nul\'',
        "        local handle = io.popen(cmd)",
        "        if handle then",
        "            for line in handle:lines() do",
        '                local name = line:match("^(.*)%.toml$")',
        "                if name then",
        "                    configs[name] = true",
        "                end",
        "            end",
        "            handle:close()",
        "        end",
        "    end",
        "",
        "    local results = {}",
        "    for name, _ in pairs(configs) do",
        "        table.insert(results, name)",
        "    end",
        "    return results",
        "end",
        "",
        f"local log_levels = {{{log_levels_str}}}",
        f"local action_types = {{{action_types_str}}}",
        f"local mouse_buttons = {{{mouse_buttons_str}}}",
        f"local notification_modes = {{{notification_modes_str}}}",
        f"local modifiers = {{{modifiers_str}}}",
        "",
        "local function with_modifiers(base_completions)",
        "    local results = {}",
        "    for _, base in ipairs(base_completions) do",
        "        table.insert(results, base)",
        "        for _, mod in ipairs(modifiers) do",
        "            table.insert(results, mod..base)",
        "        end",
        "    end",
        "    return results",
        "end",
        "",
        "local common_keys = {",
        "    " + quote_list(metadata.common_keys[:26]) + ",",
        "    " + quote_list(metadata.common_keys[26:36]) + ",",
        "    " + quote_list(metadata.common_keys[36:48]) + ",",
        "    " + quote_list(metadata.common_keys[48:57]) + ",",
        "    " + quote_list(metadata.common_keys[57:65]) + ",",
        "    " + quote_list(metadata.common_keys[65:]),
        "}",
        "local keys_with_mods = with_modifiers(common_keys)",
        "",
        "-- Commands",
        "local run_parser = clink.arg.new_parser()",
        "run_parser:set_flags(",
    ]

    run_cmd = metadata.commands.get("run", CommandInfo("run", ""))
    run_lua_flags = format_lua_option_flags(run_cmd.options)
    for i, flag_entry in enumerate(run_lua_flags):
        sep = "," if i < len(run_lua_flags) - 1 else ""
        lines.append(f"    {flag_entry}{sep}")
    lines.extend([
        ")",
        "",
        "local record_parser = clink.arg.new_parser()",
        "record_parser:set_arguments({",
        "    clink.arg.new_parser() -- name",
        "})",
        "record_parser:set_flags(",
    ])

    record_cmd = metadata.commands.get("record", CommandInfo("record", ""))
    record_lua_flags = format_lua_option_flags(record_cmd.options)
    for i, flag_entry in enumerate(record_lua_flags):
        sep = "," if i < len(record_lua_flags) - 1 else ""
        lines.append(f"    {flag_entry}{sep}")
    lines.extend([
        ")",
        "",
        "local config_parser = clink.arg.new_parser()",
        "config_parser:set_arguments({",
        "    {",
        '        "list",',
        '        "validate" .. clink.arg.new_parser({configs_matcher}),',
        '        "duplicate" .. clink.arg.new_parser({configs_matcher}, clink.arg.new_parser()),',
        '        "copy" .. clink.arg.new_parser({configs_matcher}, clink.arg.new_parser()),',
        '        "path" .. clink.arg.new_parser({configs_matcher})',
        "    }",
        "})",
        'config_parser:set_flags("--force")',
        "",
        "local apps_parser = clink.arg.new_parser()",
        "apps_parser:set_arguments({",
        '    {"list"}',
        "})",
        "",
        "local serve_parser = clink.arg.new_parser()",
        "serve_parser:set_flags(",
    ])

    serve_cmd = metadata.commands.get("serve", CommandInfo("serve", ""))
    serve_lua_flags = format_lua_option_flags(serve_cmd.options)
    for i, flag_entry in enumerate(serve_lua_flags):
        sep = "," if i < len(serve_lua_flags) - 1 else ""
        lines.append(f"    {flag_entry}{sep}")
    lines.extend([
        ")",
        "",
        "local help_config_parser = clink.arg.new_parser()",
        "help_config_parser:set_arguments({",
        "    {" + ", ".join(f'"{k}"' for k in metadata.commands.get("config", CommandInfo("config", "")).subcommands.keys()) + "}",
        "})",
        "",
        "local help_parser = clink.arg.new_parser()",
        "help_parser:set_arguments({",
        "    {",
    ])

    for cmd_name in metadata.commands.keys():
        if cmd_name == "help":
            continue
        if cmd_name == "config":
            lines.append('        "config" .. help_config_parser,')
        else:
            lines.append(f'        "{cmd_name}",')
    if lines[-1].endswith(","):
        lines[-1] = lines[-1][:-1]

    lines.extend([
        "    }",
        "})",
        "",
        "local autoinput_parser = clink.arg.new_parser()",
        "autoinput_parser:set_flags(",
    ])

    global_lua_flags = format_lua_option_flags(metadata.global_options)
    for i, flag_entry in enumerate(global_lua_flags):
        sep = "," if i < len(global_lua_flags) - 1 else ""
        lines.append(f"    {flag_entry}{sep}")

    lines.extend([
        ")",
        "",
        "autoinput_parser:set_arguments({",
        "    {",
    ])

    for cmd_name in metadata.commands.keys():
        lines.append(f'        "{cmd_name}" .. {cmd_name}_parser,')
    if lines[-1].endswith(","):
        lines[-1] = lines[-1][:-1]

    lines.extend([
        "    }",
        "})",
        "",
        'clink.arg.register_parser("autoinput", autoinput_parser)',
        'clink.arg.register_parser("autoinput.exe", autoinput_parser)',
        "",
    ])
    return eol.join(lines)


def detect_file_eol(file_path: pathlib.Path) -> str:
    """Detect newline style (LF or CRLF) of an existing file."""
    if not file_path.exists():
        return "\n"
    try:
        with open(file_path, "rb") as f:
            content = f.read(4096)
            if b"\r\n" in content:
                return "\r\n"
    except OSError:
        pass
    return "\n"


def read_file_normalized(file_path: pathlib.Path) -> str:
    """Read a text file with newline normalization."""
    if not file_path.exists():
        return ""
    with open(file_path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()


def write_file(file_path: pathlib.Path, content: str, eol: str = "\n") -> bool:
    """Write content to file ensuring correct directory and EOL."""
    file_path.parent.mkdir(parents=True, exist_ok=True)
    normalized = content.replace("\r\n", "\n").replace("\n", eol)
    with open(file_path, "wb") as f:
        f.write(normalized.encode("utf-8"))
    return True


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Update or verify shell autocomplete scripts for autoinput (Zsh, Bash, Clink/Lua).",
        add_help=True,
    )
    _ = parser.add_argument(
        "--binary",
        "-b",
        default=None,
        help="Path to autoinput binary for dynamic CLI discovery.",
    )
    _ = parser.add_argument(
        "--check",
        "-c",
        action="store_true",
        help="Check if completion files are up-to-date without modifying them.",
    )
    _ = parser.add_argument(
        "--zsh",
        default=None,
        help="Output path for Zsh completion script (default: scripts/autocomplete/_autoinput).",
    )
    _ = parser.add_argument(
        "--bash",
        default=None,
        help="Output path for Bash completion script (default: scripts/autocomplete/autoinput_completion.bash).",
    )
    _ = parser.add_argument(
        "--lua",
        default=None,
        help="Output path for Clink Lua completion script (default: scripts/autocomplete/autoinput_completion.lua).",
    )
    _ = parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Enable verbose logging.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = get_parser()
    args = parser.parse_args(argv)

    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.INFO,
        format="%(levelname)s: %(message)s" if not args.verbose else "%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    )

    zsh_path = pathlib.Path(args.zsh).resolve() if args.zsh else utils.AUTOCOMPLETE_ZSH_FILE
    bash_path = pathlib.Path(args.bash).resolve() if args.bash else utils.AUTOCOMPLETE_BASH_FILE
    lua_path = pathlib.Path(args.lua).resolve() if args.lua else utils.AUTOCOMPLETE_LUA_FILE

    binary_path = find_autoinput_binary(args.binary)
    if binary_path:
        logger.info(f"Using binary '{binary_path}' for dynamic option discovery.")
        metadata = extract_metadata_from_binary(binary_path)
    else:
        if args.binary:
            logger.error(f"Binary not found: {args.binary}")
            return 1
        logger.info("No autoinput binary found; using static CLI metadata fallback.")
        metadata = get_default_metadata()

    zsh_eol = detect_file_eol(zsh_path)
    bash_eol = detect_file_eol(bash_path)
    lua_eol = detect_file_eol(lua_path)

    generated_zsh = generate_zsh_completion(metadata, eol=zsh_eol)
    generated_bash = generate_bash_completion(metadata, eol=bash_eol)
    generated_lua = generate_lua_completion(metadata, eol=lua_eol)

    targets = [
        ("Zsh", zsh_path, generated_zsh, zsh_eol),
        ("Bash", bash_path, generated_bash, bash_eol),
        ("Clink/Lua", lua_path, generated_lua, lua_eol),
    ]

    if args.check:
        all_up_to_date = True
        for name, path, content, _ in targets:
            existing = read_file_normalized(path)
            # Compare normalized content
            if existing.replace("\r\n", "\n") != content.replace("\r\n", "\n"):
                logger.error(f"Autocomplete script for {name} ({path}) is outdated!")
                all_up_to_date = False
            else:
                logger.info(f"Autocomplete script for {name} ({path}) is up to date.")

        if not all_up_to_date:
            logger.error("Autocomplete verification failed. Run 'python scripts/update_autocomplete.py' to update.")
            return 1

        logger.info("All autocomplete scripts are up to date.")
        return 0

    for name, path, content, eol in targets:
        write_file(path, content, eol=eol)
        logger.info(f"Updated {name} completion script at {path}")

    logger.info("All autocomplete scripts updated successfully.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
