#!/usr/bin/env python3
"""Unit tests for autoinput_tools.cmake."""

import json
import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.cmake import (
    find_clang_format,
    find_clang_tidy,
    find_cmake,
    is_cmake_configured,
    load_cmake_presets,
    resolve_build_dir,
)
from autoinput_tools.paths import PROJECT_ROOT


def test_find_tools(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("shutil.which", lambda name: f"/mock/bin/{name}" if name in ("cmake", "clang-format", "clang-tidy") else None)
    assert find_cmake() == "/mock/bin/cmake"
    assert find_clang_format() == "/mock/bin/clang-format"
    assert find_clang_tidy() == "/mock/bin/clang-tidy"

    monkeypatch.setattr("shutil.which", lambda name: None)
    assert find_cmake() is None
    assert find_clang_format() is None
    assert find_clang_tidy() is None


def test_load_cmake_presets(tmp_path: pathlib.Path) -> None:
    presets_data = {
        "version": 3,
        "configurePresets": [
            {
                "name": "base",
                "hidden": True,
                "generator": "Ninja",
                "binaryDir": "${sourceDir}/build/${presetName}",
                "cacheVariables": {"VAR_A": "1"},
            },
            {
                "name": "debug",
                "inherits": "base",
                "cacheVariables": {"VAR_B": "2"},
            },
        ],
    }
    (tmp_path / "CMakePresets.json").write_text(json.dumps(presets_data), encoding="utf-8")

    presets = load_cmake_presets(tmp_path)
    # Hidden preset should be filtered out from top-level return dict
    assert "base" not in presets
    assert "debug" in presets
    debug_preset = presets["debug"]
    assert debug_preset["generator"] == "Ninja"
    assert debug_preset["cacheVariables"]["VAR_A"] == "1"
    assert debug_preset["cacheVariables"]["VAR_B"] == "2"


def test_resolve_build_dir_defaults() -> None:
    assert resolve_build_dir() == (PROJECT_ROOT / "build").resolve()


def test_resolve_build_dir_explicit(tmp_path: pathlib.Path) -> None:
    explicit_abs = tmp_path / "custom_build"
    assert resolve_build_dir(build_dir=explicit_abs) == explicit_abs

    explicit_rel = "custom_rel_build"
    assert resolve_build_dir(build_dir=explicit_rel, project_root=tmp_path) == (tmp_path / explicit_rel).resolve()


def test_resolve_build_dir_preset(tmp_path: pathlib.Path) -> None:
    presets_data = {
        "version": 3,
        "configurePresets": [
            {
                "name": "custom-preset",
                "binaryDir": "${sourceDir}/out/${presetName}",
            },
        ],
    }
    (tmp_path / "CMakePresets.json").write_text(json.dumps(presets_data), encoding="utf-8")

    resolved = resolve_build_dir(preset="custom-preset", project_root=tmp_path)
    assert resolved == (tmp_path / "out" / "custom-preset").resolve()

    # Preset without CMakePresets entry defaults to build/<preset>
    resolved_fallback = resolve_build_dir(preset="unknown-preset", project_root=tmp_path)
    assert resolved_fallback == (tmp_path / "build" / "unknown-preset").resolve()


def test_is_cmake_configured(tmp_path: pathlib.Path) -> None:
    build_dir = tmp_path / "build"
    build_dir.mkdir()
    assert is_cmake_configured(build_dir) is False

    (build_dir / "CMakeCache.txt").write_text("# CMake cache", encoding="utf-8")
    assert is_cmake_configured(build_dir) is True
