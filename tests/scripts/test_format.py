#!/usr/bin/env python3
"""Unit tests for formatting helper and commands/format.py."""

import json
import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.formatting import (
    build_format_command,
    build_format_configure_command,
    run_format,
)
from autoinput_tools.paths import PROJECT_ROOT
from commands.format import get_parser, main as format_main


def test_build_format_configure_command_defaults() -> None:
    cmd = build_format_configure_command()
    expected_dir = str((PROJECT_ROOT / "build").resolve())
    assert cmd == ["cmake", "-B", expected_dir, "-S", str(PROJECT_ROOT)]


def test_build_format_configure_command_preset() -> None:
    cmd = build_format_configure_command(preset="release", extra_args=["-DFOO=BAR"])
    assert cmd == ["cmake", "--preset", "release", "-DFOO=BAR"]


def test_build_format_command_defaults() -> None:
    cmd = build_format_command()
    expected_dir = str((PROJECT_ROOT / "build").resolve())
    assert cmd == ["cmake", "--build", expected_dir, "--target", "format"]


def test_build_format_command_check_only() -> None:
    cmd = build_format_command(check_only=True)
    expected_dir = str((PROJECT_ROOT / "build").resolve())
    assert cmd == ["cmake", "--build", expected_dir, "--target", "format-check"]


def test_build_format_command_custom_build_dir(tmp_path: pathlib.Path) -> None:
    custom_dir = tmp_path / "custom_build"
    cmd = build_format_command(build_dir=custom_dir, check_only=False, extra_args=["--", "-j4"])
    assert cmd == ["cmake", "--build", str(custom_dir), "--target", "format", "--", "-j4"]


def test_get_parser_options() -> None:
    parser = get_parser()
    args, extra = parser.parse_known_args(["--check", "-p", "debug", "-B", "custom-b", "--no-configure", "--verbose"])
    assert args.check_only is True
    assert args.preset == "debug"
    assert args.build_dir == pathlib.Path("custom-b")
    assert args.configure_if_missing is False
    assert extra == ["--verbose"]


def test_run_format_missing_cmake(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("autoinput_tools.formatting.find_cmake", lambda: None)
    monkeypatch.setattr("autoinput_tools.formatting.find_clang_format", lambda: "/usr/bin/clang-format")
    ret = run_format()
    assert ret == 1


def test_run_format_missing_clang_format(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("autoinput_tools.formatting.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.formatting.find_clang_format", lambda: None)
    ret = run_format()
    assert ret == 1


def test_run_format_invalid_preset(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    (tmp_path / "CMakePresets.json").write_text(json.dumps({"version": 3, "configurePresets": []}), encoding="utf-8")
    monkeypatch.setattr("autoinput_tools.formatting.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.formatting.find_clang_format", lambda: "/usr/bin/clang-format")
    ret = run_format(preset="nonexistent-preset", project_root=tmp_path)
    assert ret == 1


def test_run_format_unconfigured_without_auto_configure(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("autoinput_tools.formatting.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.formatting.find_clang_format", lambda: "/usr/bin/clang-format")
    ret = run_format(build_dir=tmp_path / "unconfigured", configure_if_missing=False, project_root=tmp_path)
    assert ret == 1


def test_run_format_auto_configure_and_format_success(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    build_dir = tmp_path / "build"
    executed_commands: list[list[str]] = []

    def mock_run_command(cmd: list[str], cwd: pathlib.Path | None = None) -> int:
        executed_commands.append(cmd)
        # Simulate cmake configuration creating CMakeCache.txt
        if cmd[0] == "cmake" and ("-B" in cmd or "--preset" in cmd):
            build_dir.mkdir(parents=True, exist_ok=True)
            (build_dir / "CMakeCache.txt").write_text("# cache", encoding="utf-8")
        return 0

    monkeypatch.setattr("autoinput_tools.formatting.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.formatting.find_clang_format", lambda: "/usr/bin/clang-format")
    monkeypatch.setattr("autoinput_tools.formatting.run_command", mock_run_command)

    ret = run_format(build_dir=build_dir, check_only=False, project_root=tmp_path)
    assert ret == 0
    assert len(executed_commands) == 2
    # First command is cmake configure
    assert executed_commands[0] == ["cmake", "-B", str(build_dir.resolve()), "-S", str(tmp_path)]
    # Second command is cmake --build ... --target format
    assert executed_commands[1] == ["cmake", "--build", str(build_dir.resolve()), "--target", "format"]


def test_run_format_failure_propagation(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    (build_dir / "CMakeCache.txt").write_text("# cache", encoding="utf-8")

    monkeypatch.setattr("autoinput_tools.formatting.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.formatting.find_clang_format", lambda: "/usr/bin/clang-format")
    monkeypatch.setattr("autoinput_tools.formatting.run_command", lambda cmd, cwd=None: 42)

    ret = run_format(build_dir=build_dir, check_only=True, project_root=tmp_path)
    assert ret == 42


def test_commands_format_main(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.argv", ["format.py", "--check"])
    monkeypatch.setattr("commands.format.run_format", lambda **kwargs: 0 if kwargs.get("check_only") else 1)
    assert format_main() == 0
