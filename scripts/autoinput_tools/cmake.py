"""CMake helper utilities for AutoInput Python tooling."""

import json
import logging
import os
import pathlib
import shutil
from typing import Any

from autoinput_tools.paths import PROJECT_ROOT

logger = logging.getLogger(__name__)


def find_cmake() -> str | None:
    """Locate the cmake executable on the system PATH."""
    return shutil.which("cmake")


def find_clang_format() -> str | None:
    """Locate the clang-format executable on the system PATH."""
    return shutil.which("clang-format")


def find_clang_tidy() -> str | None:
    """Locate the clang-tidy executable on the system PATH."""
    return shutil.which("clang-tidy")


def load_cmake_presets(project_root: pathlib.Path | None = None) -> dict[str, dict[str, Any]]:
    """Loads and resolves configure presets from CMakePresets.json and CMakeUserPresets.json."""
    root = project_root if project_root is not None else PROJECT_ROOT
    preset_files = [root / "CMakePresets.json", root / "CMakeUserPresets.json"]
    raw_configure_presets: dict[str, dict[str, Any]] = {}

    for pf in preset_files:
        if pf.exists():
            try:
                with open(pf, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    for cp in data.get("configurePresets", []):
                        name = cp.get("name")
                        if name:
                            raw_configure_presets[name] = cp
            except Exception as e:
                logger.warning(f"Could not load {pf.name}: {e}")

    def resolve_preset(name: str, visited: set[str] | None = None) -> dict[str, Any]:
        if visited is None:
            visited = set()
        if name in visited or name not in raw_configure_presets:
            return {}
        visited.add(name)

        raw = raw_configure_presets[name]
        inherits = raw.get("inherits")
        merged: dict[str, Any] = {
            "name": name,
            "displayName": raw.get("displayName", name),
            "description": raw.get("description", ""),
            "hidden": raw.get("hidden", False),
            "generator": raw.get("generator"),
            "binaryDir": raw.get("binaryDir"),
            "cacheVariables": {},
        }

        if inherits:
            if isinstance(inherits, str):
                inherits = [inherits]
            for parent_name in inherits:
                parent = resolve_preset(parent_name, visited.copy())
                if parent.get("generator") and not merged["generator"]:
                    merged["generator"] = parent["generator"]
                if parent.get("binaryDir") and not merged["binaryDir"]:
                    merged["binaryDir"] = parent["binaryDir"]
                merged["cacheVariables"].update(parent.get("cacheVariables", {}))

        if raw.get("generator"):
            merged["generator"] = raw["generator"]
        if raw.get("binaryDir"):
            merged["binaryDir"] = raw["binaryDir"]
        merged["cacheVariables"].update(raw.get("cacheVariables", {}))
        return merged

    presets: dict[str, dict[str, Any]] = {}
    for name, cp in raw_configure_presets.items():
        resolved = resolve_preset(name)
        if not resolved.get("hidden", False):
            presets[name] = resolved

    return presets


def resolve_build_dir(
    build_dir: pathlib.Path | str | None = None,
    preset: str | None = None,
    project_root: pathlib.Path | None = None,
) -> pathlib.Path:
    """Resolve the CMake binary/build directory based on preset or explicit build directory path."""
    root = project_root if project_root is not None else PROJECT_ROOT

    if build_dir is not None:
        p = pathlib.Path(build_dir)
        if p.is_absolute():
            return p
        return (root / p).resolve()

    if preset is not None:
        presets = load_cmake_presets(root)
        if preset in presets:
            preset_data = presets[preset]
            binary_dir_str = preset_data.get("binaryDir")
            if binary_dir_str:
                resolved_str = binary_dir_str.replace("${sourceDir}", str(root)).replace("${presetName}", preset)
                return pathlib.Path(resolved_str).resolve()
            return (root / "build" / preset).resolve()
        # Fallback if preset is not in JSON
        return (root / "build" / preset).resolve()

    return (root / "build").resolve()


def is_cmake_configured(build_dir: pathlib.Path | str) -> bool:
    """Check if the given directory contains a valid configured CMake cache or build manifest."""
    p = pathlib.Path(build_dir)
    return (p / "CMakeCache.txt").exists() or (p / "build.ninja").exists() or (p / "Makefile").exists()
