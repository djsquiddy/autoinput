#!/usr/bin/env python3
"""Unit tests for commands/build.py."""

import json
import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from commands.build import (
    EXIT_FAILED_CMAKE_CONFIGURATION,
    EXIT_FAILED_PYTHON_TESTS,
    EXIT_FAILED_SOURCE_COMPILATION,
    EXIT_FAILED_UNIT_TESTS,
    EXIT_LOCALIZATION_AUDIT,
    EXIT_SUCCESSFUL,
    BuildConfig,
    Builder,
    BuildReport,
    ExecutableInfo,
    PresetBuilder,
    StepResult,
    _list_preset,
    find_executables,
    format_duration,
    format_file_size,
    format_log_line,
    load_cmake_presets,
    main,
    parse_arguments,
)


def test_format_duration() -> None:
    # Verify duration formatting for None, negative, seconds, and minutes
    assert format_duration(None) == "-"
    assert format_duration(-5.0) == "0.00s"
    assert format_duration(12.345) == "12.35s"
    assert format_duration(65.5) == "1m 05.50s"
    assert format_duration(125.0) == "2m 05.00s"


def test_format_file_size() -> None:
    # Verify file size formatting for bytes, kilobytes, and megabytes
    assert format_file_size(500) == "500 B"
    assert format_file_size(2048) == "2.00 KB"
    assert format_file_size(5 * 1024 * 1024) == "5.00 MB"


def test_format_log_line() -> None:
    line_green = "[==========] Running tests"
    # Verify green ANSI styling for test start line
    assert format_log_line(line_green).startswith("\033[32m")

    line_red = "[  FAILED  ] Test.Case"
    # Verify red ANSI styling for test failure line
    assert format_log_line(line_red).startswith("\033[31m")

    line_yellow = "[  SKIPPED ] Test.Case"
    # Verify yellow ANSI styling for test skip line
    assert format_log_line(line_yellow).startswith("\033[33m")

    line_plain = "Regular text"
    # Verify plain log lines remain unmodified
    assert format_log_line(line_plain) == line_plain


def test_parse_arguments_defaults() -> None:
    config = parse_arguments([])
    # Verify default build configuration flags and settings
    assert config.clean is False
    assert config.build_type == "Release"
    assert config.build_tests is True
    assert config.build_tray is True
    assert config.build_ui is True
    assert config.preset is None
    assert config.audit is False
    assert config.bulk_build is False
    assert config.list_presets is False
    assert config.build_only is False
    assert config.test_only is False
    assert config.audit_only is False
    assert config.extra_cmake_args == []


def test_parse_arguments_build_only() -> None:
    config = parse_arguments(["--build-only", "all", "Release"])
    assert config.build_only is True
    assert config.test_only is False
    assert config.audit_only is False
    assert config.build_tests is True
    assert config.build_type == "Release"


def test_parse_arguments_test_only() -> None:
    config = parse_arguments(["--test-only", "all", "Release"])
    assert config.test_only is True
    assert config.build_only is False
    assert config.audit_only is False
    assert config.build_tests is True
    assert config.build_type == "Release"


def test_parse_arguments_test_only_explicit_ui() -> None:
    # If target is explicit ui, test-only still enables tests
    config = parse_arguments(["--test-only", "ui", "Release"])
    assert config.test_only is True
    assert config.build_tests is True


def test_parse_arguments_audit_only() -> None:
    config = parse_arguments(["--audit-only", "all", "Release"])
    assert config.audit_only is True
    assert config.audit is True
    assert config.build_only is False
    assert config.test_only is False
    assert config.build_ui is True


def test_parse_arguments_invalid_combinations() -> None:
    with pytest.raises(SystemExit):
        parse_arguments(["--build-only", "--test-only"])

    with pytest.raises(SystemExit):
        parse_arguments(["--build-only", "--audit-only"])

    with pytest.raises(SystemExit):
        parse_arguments(["--test-only", "--audit-only"])

    with pytest.raises(SystemExit):
        parse_arguments(["--build-only", "--test-only", "--audit-only"])


def test_parse_arguments_targets_and_clean() -> None:
    config = parse_arguments(["ui", "clean", "debug"])
    # Verify specific target selection and clean flag
    assert config.clean is True
    assert config.build_type == "Debug"
    assert config.build_ui is True
    assert config.build_tray is False
    assert config.build_tests is False


def test_parse_arguments_preset() -> None:
    config = parse_arguments(["--preset", "ninja-debug", "tests"])
    # Verify preset name parsing and target filtering
    assert config.preset == "ninja-debug"
    assert config.build_tests is True
    assert config.build_ui is False
    assert config.build_tray is False


def test_parse_arguments_list_presets() -> None:
    config = parse_arguments(["--list-presets"])
    # Verify list presets flag parsing
    assert config.list_presets is True


def test_parse_arguments_bulk_build_and_cmake_args() -> None:
    config = parse_arguments(["--bulk-build", "-DFOO=BAR", "-DCMAKE_UNITY_BUILD=ON"])
    # Verify bulk build flag and passthrough CMake arguments
    assert config.bulk_build is True
    assert "-DFOO=BAR" in config.extra_cmake_args
    assert "-DCMAKE_UNITY_BUILD=ON" in config.extra_cmake_args


def test_load_cmake_presets_inherits(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    presets_data = {
        "version": 3,
        "configurePresets": [
            {
                "name": "base",
                "generator": "Ninja",
                "binaryDir": "${sourceDir}/build/base",
                "cacheVariables": {"VAR_A": "1"},
            },
            {
                "name": "derived",
                "inherits": "base",
                "displayName": "Derived Preset",
                "cacheVariables": {"VAR_B": "2"},
            },
            {
                "name": "hidden-preset",
                "hidden": True,
            },
        ],
    }
    preset_file = tmp_path / "CMakePresets.json"
    preset_file.write_text(json.dumps(presets_data), encoding="utf-8")

    monkeypatch.setattr("commands.build.PROJECT_ROOT", tmp_path)
    presets = load_cmake_presets()

    # Verify preset inheritance and exclusion of hidden presets
    assert "base" in presets
    assert "derived" in presets
    assert "hidden-preset" not in presets
    assert presets["derived"]["generator"] == "Ninja"
    assert presets["derived"]["cacheVariables"]["VAR_A"] == "1"
    assert presets["derived"]["cacheVariables"]["VAR_B"] == "2"


def test_find_executables(tmp_path: pathlib.Path) -> None:
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    exe1 = bin_dir / "autoinput.exe"
    exe1.write_bytes(b"x" * 1024)
    txt1 = bin_dir / "note.txt"
    txt1.write_text("hello")

    executables = find_executables(tmp_path)
    if sys.platform == "win32":
        # Verify discovering executable files and file sizes in bin directory
        assert len(executables) == 1
        assert executables[0].name == "autoinput.exe"
        assert executables[0].size == 1024


def test_build_report_print_summary(tmp_path: pathlib.Path) -> None:
    config = BuildConfig(
        clean=False,
        build_type="Debug",
        build_tests=True,
        build_tray=False,
        build_ui=True,
    )
    report = BuildReport(config, tmp_path)
    report.record_step("Clean", "SKIPPED")
    report.record_step("CMake Configure", "PASSED", 1.5)
    report.record_step("Build", "PASSED", 3.2)
    report.record_step("Unit Tests", "PASSED", 0.8)
    report.record_step("Python Tests", "PASSED", 0.5)
    report.executables = [ExecutableInfo(name="app.exe", path=tmp_path / "app.exe", size=2048)]

    # Calling print_summary should run without error
    report.print_summary(total_duration=5.5, success=True)
    report.print_summary(total_duration=5.5, success=False)


def test_builder_clean_build(tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    (build_dir / "test.txt").write_text("data")

    config = BuildConfig(clean=True, build_type="Release", build_tests=False, build_tray=False, build_ui=False)
    builder = Builder(config, build_dir)

    ok, duration = builder.clean_build()
    # Verify clean_build succeeds and removes build directory
    assert ok is True
    assert not build_dir.exists()


def test_builder_create_build_directory(tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "new_build"
    config = BuildConfig(clean=False, build_type="Release", build_tests=False, build_tray=False, build_ui=False)
    builder = Builder(config, build_dir)

    # Verify build directory creation on first and subsequent calls
    assert builder.create_build_directory() is True
    assert build_dir.exists()
    assert builder.create_build_directory() is True


def test_builder_run_success(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    config = BuildConfig(
        clean=False,
        build_type="Release",
        build_tests=True,
        build_tray=True,
        build_ui=True,
        audit=False,
    )
    builder = Builder(config, build_dir)

    monkeypatch.setattr("commands.build.run_command", lambda *args, **kwargs: 0)
    ret = builder.run()
    # Verify full build workflow returns success exit code
    assert ret == EXIT_SUCCESSFUL


def test_builder_run_cmake_failure(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    config = BuildConfig(
        clean=False,
        build_type="Release",
        build_tests=False,
        build_tray=False,
        build_ui=False,
    )
    builder = Builder(config, build_dir)

    monkeypatch.setattr("commands.build.run_command", lambda *args, **kwargs: 1)
    ret = builder.run()
    # Verify CMake configuration failure return code
    assert ret == EXIT_FAILED_CMAKE_CONFIGURATION


def test_builder_run_build_failure(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    config = BuildConfig(
        clean=False,
        build_type="Release",
        build_tests=False,
        build_tray=False,
        build_ui=False,
    )
    builder = Builder(config, build_dir)

    call_count = 0

    def mock_run(cmd, *args, **kwargs):
        nonlocal call_count
        call_count += 1
        if call_count == 1:
            return 0  # cmake config passes
        return 1  # build fails

    monkeypatch.setattr("commands.build.run_command", mock_run)
    ret = builder.run()
    # Verify compilation step failure return code
    assert ret == EXIT_FAILED_SOURCE_COMPILATION


def test_builder_run_tests_failure(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    bin_dir = build_dir / "bin"
    bin_dir.mkdir(parents=True)
    test_exe_name = "autoinput-tests.exe" if sys.platform == "win32" else "autoinput-tests"
    (bin_dir / test_exe_name).write_bytes(b"dummy")

    config = BuildConfig(
        clean=False,
        build_type="Release",
        build_tests=True,
        build_tray=False,
        build_ui=False,
    )
    builder = Builder(config, build_dir)

    call_count = 0

    def mock_run(cmd, *args, **kwargs):
        nonlocal call_count
        call_count += 1
        if call_count <= 2:
            return 0  # configure and build pass
        return 1  # tests fail

    monkeypatch.setattr("commands.build.run_command", mock_run)
    ret = builder.run()
    # Verify C++ unit test failure return code
    assert ret == EXIT_FAILED_UNIT_TESTS


def test_builder_run_python_tests(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    config_disabled = BuildConfig(clean=False, build_type="Release", build_tests=False, build_tray=False, build_ui=False)
    builder_disabled = Builder(config_disabled, tmp_path)
    ret, dur, ran = builder_disabled.run_python_tests()
    # Verify Python tests are skipped when build_tests is disabled
    assert ret == EXIT_SUCCESSFUL
    assert ran is False

    config_enabled = BuildConfig(clean=False, build_type="Release", build_tests=True, build_tray=False, build_ui=False)
    builder_enabled = Builder(config_enabled, tmp_path)
    monkeypatch.setattr("commands.build.run_command", lambda *args, **kwargs: 0)
    ret, dur, ran = builder_enabled.run_python_tests()
    # Verify Python tests are executed when build_tests is enabled
    assert ret == EXIT_SUCCESSFUL
    assert ran is True


def test_builder_run_python_tests_failure(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    bin_dir = build_dir / "bin"
    bin_dir.mkdir(parents=True)
    test_exe_name = "autoinput-tests.exe" if sys.platform == "win32" else "autoinput-tests"
    (bin_dir / test_exe_name).write_bytes(b"dummy")

    config = BuildConfig(
        clean=False,
        build_type="Release",
        build_tests=True,
        build_tray=False,
        build_ui=False,
    )
    builder = Builder(config, build_dir)

    call_count = 0

    def mock_run(cmd, *args, **kwargs):
        nonlocal call_count
        call_count += 1
        if call_count <= 3:
            return 0  # configure, build, unit tests pass
        return 1  # python tests fail

    monkeypatch.setattr("commands.build.run_command", mock_run)
    ret = builder.run()
    # Verify Python test failure returns EXIT_FAILED_PYTHON_TESTS
    assert ret == EXIT_FAILED_PYTHON_TESTS


def test_preset_builder_create(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    mock_presets = {
        "ninja-debug": {
            "name": "ninja-debug",
            "displayName": "Ninja Debug",
            "binaryDir": "${sourceDir}/build/ninja-debug",
            "cacheVariables": {"AUTOINPUT_BUILD_TESTS": "ON"},
        }
    }
    monkeypatch.setattr("commands.build.load_cmake_presets", lambda: mock_presets)

    config = BuildConfig(clean=False, build_type="Debug", build_tests=False, build_tray=False, build_ui=False, preset="ninja-debug")
    builder = PresetBuilder.create(config)
    # Verify PresetBuilder is instantiated for valid preset name
    assert builder is not None
    assert builder.preset_name == "ninja-debug"

    config_invalid = BuildConfig(clean=False, build_type="Debug", build_tests=False, build_tray=False, build_ui=False, preset="nonexistent")
    # Verify PresetBuilder returns None for unknown preset
    assert PresetBuilder.create(config_invalid) is None


def test_list_preset(monkeypatch: pytest.MonkeyPatch) -> None:
    mock_presets = {
        "ninja-debug": {
            "name": "ninja-debug",
            "displayName": "Ninja Debug",
            "description": "Debug preset",
        }
    }
    monkeypatch.setattr("commands.build.load_cmake_presets", lambda: mock_presets)
    # Verify _list_preset exits with code 0
    assert _list_preset() == 0


def test_build_main_list_presets(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(sys, "argv", ["build.py", "--list-presets"])
    monkeypatch.setattr("commands.build.load_cmake_presets", lambda: {})
    # Verify main execution with --list-presets flag returns 0
    assert main() == 0


def test_builder_run_build_only(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    config = BuildConfig(
        clean=True,
        build_type="Release",
        build_tests=True,
        build_tray=True,
        build_ui=True,
        build_only=True,
    )
    builder = Builder(config, build_dir)

    executed_commands = []

    def mock_run(cmd, *args, **kwargs):
        executed_commands.append(cmd)
        return 0

    monkeypatch.setattr("commands.build.run_command", mock_run)
    ret = builder.run()
    assert ret == EXIT_SUCCESSFUL
    # Verify CMake configure and ninja build were called, but tests/audit/autocomplete were not
    assert len(executed_commands) == 2
    assert "cmake" in str(executed_commands[0][0])
    assert executed_commands[1] == ["ninja"]


def test_builder_run_test_only(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    bin_dir = build_dir / "bin"
    bin_dir.mkdir(parents=True)
    test_exe_name = "autoinput-tests.exe" if sys.platform == "win32" else "autoinput-tests"
    (bin_dir / test_exe_name).write_bytes(b"dummy")

    config = BuildConfig(
        clean=True,
        build_type="Release",
        build_tests=True,
        build_tray=True,
        build_ui=True,
        test_only=True,
    )
    builder = Builder(config, build_dir)

    executed_commands = []

    def mock_run(cmd, *args, **kwargs):
        executed_commands.append(cmd)
        return 0

    monkeypatch.setattr("commands.build.run_command", mock_run)
    ret = builder.run()
    assert ret == EXIT_SUCCESSFUL
    # Verify no cmake/ninja was called, but test binaries/pytest were run
    assert len(executed_commands) > 0
    for cmd in executed_commands:
        assert cmd != ["ninja"]
        assert "cmake" not in str(cmd[0])


def test_builder_run_audit_only_success(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    build_dir.mkdir(parents=True)

    config = BuildConfig(
        clean=True,
        build_type="Release",
        build_tests=True,
        build_tray=True,
        build_ui=True,
        audit=True,
        audit_only=True,
    )
    builder = Builder(config, build_dir)

    executed_commands = []

    def mock_run(cmd, *args, **kwargs):
        executed_commands.append(cmd)
        return 0

    monkeypatch.setattr("commands.build.run_command", mock_run)
    ret = builder.run()
    assert ret == EXIT_SUCCESSFUL
    # Verify only audit script was executed
    assert len(executed_commands) == 1
    assert "audit_localization.py" in str(executed_commands[0][1])


def test_builder_run_audit_only_missing_build_dir(tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "nonexistent"
    config = BuildConfig(
        clean=False,
        build_type="Release",
        build_tests=False,
        build_tray=False,
        build_ui=True,
        audit=True,
        audit_only=True,
    )
    builder = Builder(config, build_dir)
    ret = builder.run()
    assert ret == EXIT_LOCALIZATION_AUDIT


def test_builder_run_audit_only_missing_ui(tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    build_dir.mkdir(parents=True)
    config = BuildConfig(
        clean=False,
        build_type="Release",
        build_tests=False,
        build_tray=False,
        build_ui=False,
        audit=True,
        audit_only=True,
    )
    builder = Builder(config, build_dir)
    ret = builder.run()
    assert ret == EXIT_LOCALIZATION_AUDIT
