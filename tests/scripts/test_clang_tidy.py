#!/usr/bin/env python3
"""Unit tests for clang-tidy helper and commands/clang_tidy.py."""

import json
import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.paths import PROJECT_ROOT
from autoinput_tools.tidy import (
    build_tidy_command,
    build_tidy_configure_command,
    run_clang_tidy,
)
from commands.clang_tidy import get_parser, main as tidy_main


def test_build_tidy_configure_command_defaults() -> None:
    cmd = build_tidy_configure_command()
    expected_dir = str((PROJECT_ROOT / "build").resolve())
    assert cmd == [
        "cmake",
        "-B",
        expected_dir,
        "-S",
        str(PROJECT_ROOT),
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]


def test_build_tidy_configure_command_with_enable_tidy() -> None:
    cmd = build_tidy_configure_command(enable_tidy_flag=True)
    expected_dir = str((PROJECT_ROOT / "build").resolve())
    assert cmd == [
        "cmake",
        "-B",
        expected_dir,
        "-S",
        str(PROJECT_ROOT),
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DAUTOINPUT_ENABLE_CLANG_TIDY=ON",
    ]


def test_build_tidy_configure_command_preset() -> None:
    cmd = build_tidy_configure_command(preset="debug", enable_tidy_flag=True, extra_args=["-DFOO=BAR"])
    assert cmd == [
        "cmake",
        "--preset",
        "debug",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DAUTOINPUT_ENABLE_CLANG_TIDY=ON",
        "-DFOO=BAR",
    ]


def test_build_tidy_command_defaults() -> None:
    cmd = build_tidy_command()
    expected_dir = str((PROJECT_ROOT / "build").resolve())
    assert cmd == ["cmake", "--build", expected_dir, "--target", "clang-tidy-check"]


def test_build_tidy_command_custom_target(tmp_path: pathlib.Path) -> None:
    custom_dir = tmp_path / "custom_tidy"
    cmd = build_tidy_command(build_dir=custom_dir, target="custom-tidy-target", extra_args=["--", "-j2"])
    assert cmd == ["cmake", "--build", str(custom_dir), "--target", "custom-tidy-target", "--", "-j2"]


def test_build_tidy_command_no_target(tmp_path: pathlib.Path) -> None:
    custom_dir = tmp_path / "custom_tidy"
    cmd = build_tidy_command(build_dir=custom_dir, target=None)
    assert cmd == ["cmake", "--build", str(custom_dir)]


def test_get_parser_options() -> None:
    parser = get_parser()
    args, extra = parser.parse_known_args(["-p", "release", "-B", "custom-dir", "-t", "my-target", "--build", "-c", "--no-configure", "--verbose"])
    assert args.preset == "release"
    assert args.build_dir == pathlib.Path("custom-dir")
    assert args.target == "my-target"
    assert args.build_with_tidy is True
    assert args.clean is True
    assert args.configure_if_missing is False
    assert extra == ["--verbose"]


def test_run_clang_tidy_missing_cmake(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("autoinput_tools.tidy.find_cmake", lambda: None)
    monkeypatch.setattr("autoinput_tools.tidy.find_clang_tidy", lambda: "/usr/bin/clang-tidy")
    ret = run_clang_tidy()
    assert ret == 1


def test_run_clang_tidy_missing_clang_tidy(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("autoinput_tools.tidy.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.tidy.find_clang_tidy", lambda: None)
    ret = run_clang_tidy()
    assert ret == 1


def test_run_clang_tidy_invalid_preset(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    (tmp_path / "CMakePresets.json").write_text(json.dumps({"version": 3, "configurePresets": []}), encoding="utf-8")
    monkeypatch.setattr("autoinput_tools.tidy.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.tidy.find_clang_tidy", lambda: "/usr/bin/clang-tidy")
    ret = run_clang_tidy(preset="invalid-preset", project_root=tmp_path)
    assert ret == 1


def test_run_clang_tidy_unconfigured_without_auto_configure(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("autoinput_tools.tidy.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.tidy.find_clang_tidy", lambda: "/usr/bin/clang-tidy")
    ret = run_clang_tidy(build_dir=tmp_path / "unconfigured", configure_if_missing=False, project_root=tmp_path)
    assert ret == 1


def test_run_clang_tidy_auto_configure_and_run_success(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    build_dir = tmp_path / "build"
    executed_commands: list[list[str]] = []

    def mock_run_command(cmd: list[str], cwd: pathlib.Path | None = None) -> int:
        executed_commands.append(cmd)
        if cmd[0] == "cmake" and ("-B" in cmd or "--preset" in cmd):
            build_dir.mkdir(parents=True, exist_ok=True)
            (build_dir / "CMakeCache.txt").write_text("# cache", encoding="utf-8")
        return 0

    monkeypatch.setattr("autoinput_tools.tidy.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.tidy.find_clang_tidy", lambda: "/usr/bin/clang-tidy")
    monkeypatch.setattr("autoinput_tools.tidy.run_command", mock_run_command)

    ret = run_clang_tidy(build_dir=build_dir, project_root=tmp_path)
    assert ret == 0
    assert len(executed_commands) == 2
    # Configure command with compile commands export
    assert executed_commands[0] == [
        "cmake",
        "-B",
        str(build_dir.resolve()),
        "-S",
        str(tmp_path),
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]
    # Build target clang-tidy-check
    assert executed_commands[1] == [
        "cmake",
        "--build",
        str(build_dir.resolve()),
        "--target",
        "clang-tidy-check",
    ]


def test_run_clang_tidy_with_build_mode(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    build_dir = tmp_path / "build"
    executed_commands: list[list[str]] = []

    def mock_run_command(cmd: list[str], cwd: pathlib.Path | None = None) -> int:
        executed_commands.append(cmd)
        return 0

    monkeypatch.setattr("autoinput_tools.tidy.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.tidy.find_clang_tidy", lambda: "/usr/bin/clang-tidy")
    monkeypatch.setattr("autoinput_tools.tidy.run_command", mock_run_command)

    ret = run_clang_tidy(build_dir=build_dir, build_with_tidy=True, project_root=tmp_path)
    assert ret == 0
    assert len(executed_commands) == 2
    # Configure command with enable tidy flag
    assert "-DAUTOINPUT_ENABLE_CLANG_TIDY=ON" in executed_commands[0]
    # Build command without clang-tidy-check target (builds default)
    assert "--target" not in executed_commands[1]


def test_run_clang_tidy_clean(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    marker = build_dir / "old_file.txt"
    marker.write_text("old", encoding="utf-8")

    def mock_run_command(cmd: list[str], cwd: pathlib.Path | None = None) -> int:
        return 0

    monkeypatch.setattr("autoinput_tools.tidy.find_cmake", lambda: "/usr/bin/cmake")
    monkeypatch.setattr("autoinput_tools.tidy.find_clang_tidy", lambda: "/usr/bin/clang-tidy")
    monkeypatch.setattr("autoinput_tools.tidy.run_command", mock_run_command)

    ret = run_clang_tidy(build_dir=build_dir, clean=True, project_root=tmp_path)
    assert ret == 0
    # Marker should be removed by clean
    assert not marker.exists()


def test_commands_clang_tidy_main(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.argv", ["clang_tidy.py", "--clean"])
    monkeypatch.setattr("commands.clang_tidy.run_clang_tidy", lambda **kwargs: 0 if kwargs.get("clean") else 1)
    assert tidy_main() == 0
