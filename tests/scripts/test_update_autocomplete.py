#!/usr/bin/env python3
"""Unit tests for commands/update_autocomplete.py."""

import argparse
import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from commands.update_autocomplete import (
    ALL_SHELL_TARGETS,
    CliMetadata,
    CommandInfo,
    OptionInfo,
    _resolve_active_targets,
    detect_default_targets,
    detect_file_eol,
    find_autoinput_binary,
    format_lua_option_flags,
    format_zsh_option,
    generate_bash_completion,
    generate_lua_completion,
    generate_zsh_completion,
    get_parser,
    infer_completion_type,
    infer_zsh_action,
    load_metadata_from_toml_file,
    main,
    metadata_from_toml,
    parse_help_sections,
    parse_option_line,
    read_file_normalized,
    write_file,
)


@pytest.fixture
def sample_cli_metadata() -> CliMetadata:
    meta = CliMetadata()
    meta.global_options = [
        OptionInfo(flags=["-h", "--help"], description="Show help"),
        OptionInfo(flags=["-l", "--log"], arg_placeholder="LEVEL", description="Set log level"),
    ]
    meta.commands = {
        "run": CommandInfo(
            name="run",
            description="Run automation script",
            options=[
                OptionInfo(flags=["-c", "--config"], arg_placeholder="NAME_OR_PATH", description="Config file"),
                OptionInfo(flags=["-t", "--type"], arg_placeholder="click|hold", description="Action type"),
            ],
            subcommands={},
        ),
        "config": CommandInfo(
            name="config",
            description="Manage configuration",
            options=[],
            subcommands={
                "list": CommandInfo(name="list", description="List configs", options=[], subcommands={}),
            },
        ),
    }
    return meta


def test_parser_options() -> None:
    parser = get_parser()
    args = parser.parse_args(["--source", "toml", "--shell", "bash", "--check"])
    assert args.source == "toml"
    assert args.shell == ["bash"]
    assert args.check is True


def test_detect_file_eol(tmp_path: pathlib.Path) -> None:
    crlf_file = tmp_path / "crlf.txt"
    crlf_file.write_bytes(b"hello\r\nworld\r\n")
    assert detect_file_eol(crlf_file) == "\r\n"

    lf_file = tmp_path / "lf.txt"
    lf_file.write_bytes(b"hello\nworld\n")
    assert detect_file_eol(lf_file) == "\n"

    missing_file = tmp_path / "missing.txt"
    assert detect_file_eol(missing_file) == "\n"


def test_read_and_write_file_normalized(tmp_path: pathlib.Path) -> None:
    test_file = tmp_path / "test.txt"
    write_file(test_file, "Line 1\nLine 2\n", eol="\n")
    content = read_file_normalized(test_file)
    assert "Line 1\nLine 2\n" in content


def test_resolve_active_targets() -> None:
    ns1 = argparse.Namespace(shell=["bash", "lua"], all=False)
    targets1 = _resolve_active_targets(ns1)
    assert targets1 == {"bash", "lua"}

    ns2 = argparse.Namespace(shell=["all"], all=False)
    targets2 = _resolve_active_targets(ns2)
    assert targets2 == ALL_SHELL_TARGETS


def test_detect_default_targets() -> None:
    defaults = detect_default_targets()
    assert isinstance(defaults, set)
    assert len(defaults) > 0


def test_load_metadata_from_toml(tmp_path: pathlib.Path) -> None:
    toml_path = tmp_path / "help.toml"
    toml_path.write_text(
        """
        [app]
        name = "testapp"
        summary = "A test"

        [[global_options]]
        names = ["-v", "--version"]
        value = false
        description = "Print version"
        """,
        encoding="utf-8",
    )
    meta = load_metadata_from_toml_file(toml_path)
    assert meta is not None
    assert len(meta.global_options) == 1
    assert meta.global_options[0].flags == ["-v", "--version"]


def test_parse_help_sections() -> None:
    help_text = """Options:
  -h, --help     Show help
  -v, --version  Show version

Commands:
  run            Run script
  config         Manage config
"""
    sections = parse_help_sections(help_text)
    assert "Options" in sections
    assert "Commands" in sections


def test_parse_option_line() -> None:
    line = "  -c, --config NAME_OR_PATH   Path to configuration file"
    opt = parse_option_line(line)
    assert opt is not None
    assert opt.flags == ["-c", "--config"]
    assert opt.arg_placeholder == "NAME_OR_PATH"
    assert opt.description == "Path to configuration file"


def test_infer_completion_type() -> None:
    opt_cfg = OptionInfo(flags=["-c", "--config"], arg_placeholder="NAME_OR_PATH")
    assert infer_completion_type(opt_cfg) == "config"

    opt_act = OptionInfo(flags=["--action-type"], arg_placeholder="click|hold")
    assert infer_completion_type(opt_act) == "action_type"

    opt_btn = OptionInfo(flags=["-b", "--button"], arg_placeholder="BTN")
    assert infer_completion_type(opt_btn) == "mouse_button"

    opt_log = OptionInfo(flags=["-l", "--log"], arg_placeholder="LEVEL")
    assert infer_completion_type(opt_log) == "log_level"

    opt_flag = OptionInfo(flags=["-f", "--force"])
    assert infer_completion_type(opt_flag) == "none"


def test_infer_zsh_action() -> None:
    opt_log = OptionInfo(flags=["-l", "--log"], arg_placeholder="LEVEL")
    action = infer_zsh_action(opt_log)
    assert "($log_levels)" in action


def test_format_zsh_option() -> None:
    opt = OptionInfo(flags=["-h", "--help"], description="Show help.")
    formatted = format_zsh_option(opt)
    assert "{-h,--help}[Show help]" in formatted


def test_format_lua_option_flags() -> None:
    opts = [OptionInfo(flags=["-c", "--config"], arg_placeholder="NAME_OR_PATH", description="Config")]
    flags = format_lua_option_flags(opts)
    assert any('"-c"' in f for f in flags)
    assert any('"--config"' in f for f in flags)


def test_generate_completion_scripts(sample_cli_metadata: CliMetadata) -> None:
    zsh = generate_zsh_completion(sample_cli_metadata)
    assert "#compdef autoinput" in zsh
    assert "_autoinput()" in zsh

    bash = generate_bash_completion(sample_cli_metadata)
    assert "_autoinput()" in bash
    assert "complete -F _autoinput autoinput" in bash

    lua = generate_lua_completion(sample_cli_metadata)
    assert "autoinput_parser" in lua
    assert 'clink.arg.register_parser("autoinput", autoinput_parser)' in lua


def test_find_autoinput_binary(tmp_path: pathlib.Path) -> None:
    exe_file = tmp_path / "custom_autoinput.exe"
    exe_file.write_bytes(b"dummy")

    found = find_autoinput_binary(str(exe_file))
    assert found == exe_file.resolve()

    missing = find_autoinput_binary(str(tmp_path / "nonexistent.exe"))
    assert missing is None


def test_update_autocomplete_main_check_mode(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(sys, "argv", ["update_autocomplete.py", "--source", "toml", "--check"])
    ret = main(["--source", "toml", "--check"])
    assert ret == 0
