"""Clang-format helper integration for AutoInput."""

import logging
import pathlib
from typing import Any

from autoinput_tools.cmake import (
    find_clang_format,
    find_cmake,
    is_cmake_configured,
    load_cmake_presets,
    resolve_build_dir,
)
from autoinput_tools.paths import PROJECT_ROOT
from autoinput_tools.process import run_command

logger = logging.getLogger(__name__)


def build_format_configure_command(
    build_dir: pathlib.Path | str | None = None,
    preset: str | None = None,
    extra_args: list[str] | None = None,
    project_root: pathlib.Path | None = None,
) -> list[str]:
    """Construct the CMake command to configure the build directory for formatting."""
    root = project_root if project_root is not None else PROJECT_ROOT
    cmd: list[str] = ["cmake"]

    if preset is not None:
        cmd.extend(["--preset", preset])
    else:
        resolved_dir = resolve_build_dir(build_dir=build_dir, project_root=root)
        cmd.extend(["-B", str(resolved_dir), "-S", str(root)])

    if extra_args:
        cmd.extend(extra_args)

    return cmd


def build_format_command(
    build_dir: pathlib.Path | str | None = None,
    preset: str | None = None,
    check_only: bool = False,
    extra_args: list[str] | None = None,
    project_root: pathlib.Path | None = None,
) -> list[str]:
    """Construct the CMake command to run the formatting target."""
    root = project_root if project_root is not None else PROJECT_ROOT
    target = "format-check" if check_only else "format"
    resolved_dir = resolve_build_dir(build_dir=build_dir, preset=preset, project_root=root)

    cmd: list[str] = ["cmake", "--build", str(resolved_dir), "--target", target]
    if extra_args:
        cmd.extend(extra_args)

    return cmd


def run_format(
    build_dir: pathlib.Path | str | None = None,
    preset: str | None = None,
    check_only: bool = False,
    configure_if_missing: bool = True,
    extra_args: list[str] | None = None,
    project_root: pathlib.Path | None = None,
) -> int:
    """Execute the project's CMake clang-format integration."""
    root = project_root if project_root is not None else PROJECT_ROOT

    if not find_cmake():
        logger.error("CMake executable not found in PATH. Please install CMake.")
        return 1

    if not find_clang_format():
        logger.error("clang-format executable not found in PATH. CMake formatting targets require clang-format.")
        return 1

    if preset is not None:
        presets = load_cmake_presets(root)
        if preset not in presets:
            available = ", ".join(sorted(presets.keys())) if presets else "none"
            logger.error(f"CMake preset '{preset}' was not found. Available presets: {available}")
            return 1

    resolved_dir = resolve_build_dir(build_dir=build_dir, preset=preset, project_root=root)

    if not is_cmake_configured(resolved_dir):
        if not configure_if_missing:
            logger.error(
                f"Build directory '{resolved_dir}' is not configured with CMake. "
                "Run cmake configuration first or omit --no-configure."
            )
            return 1

        logger.info(f"Configuring CMake for formatting in '{resolved_dir}'...")
        configure_cmd = build_format_configure_command(
            build_dir=build_dir,
            preset=preset,
            project_root=root,
        )
        conf_code = run_command(configure_cmd, cwd=root)
        if conf_code != 0:
            logger.error(f"CMake configuration failed with exit code {conf_code}.")
            return conf_code

    build_cmd = build_format_command(
        build_dir=build_dir,
        preset=preset,
        check_only=check_only,
        extra_args=extra_args,
        project_root=root,
    )

    action_label = "Checking formatting" if check_only else "Formatting source files"
    logger.info(f"{action_label} using CMake target '{build_cmd[4]}'...")

    ret_code = run_command(build_cmd, cwd=root)
    if ret_code != 0:
        logger.error(f"Formatting failed with exit code {ret_code}.")
        return ret_code

    logger.info("Formatting completed successfully.")
    return 0
