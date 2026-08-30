"""Clang-tidy helper integration for AutoInput."""

import logging
import pathlib
import shutil
from typing import Any

from autoinput_tools.cmake import (
    find_clang_tidy,
    find_cmake,
    is_cmake_configured,
    load_cmake_presets,
    resolve_build_dir,
)
from autoinput_tools.paths import PROJECT_ROOT
from autoinput_tools.process import run_command

logger = logging.getLogger(__name__)


def build_tidy_configure_command(
    build_dir: pathlib.Path | str | None = None,
    preset: str | None = None,
    enable_tidy_flag: bool = False,
    extra_args: list[str] | None = None,
    project_root: pathlib.Path | None = None,
) -> list[str]:
    """Construct the CMake command to configure the build directory for clang-tidy."""
    root = project_root if project_root is not None else PROJECT_ROOT
    cmd: list[str] = ["cmake"]

    if preset is not None:
        cmd.extend(["--preset", preset])
    else:
        resolved_dir = resolve_build_dir(build_dir=build_dir, project_root=root)
        cmd.extend(["-B", str(resolved_dir), "-S", str(root)])

    # Ensure compile_commands.json is always exported for clang-tidy
    cmd.append("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")

    if enable_tidy_flag:
        cmd.append("-DAUTOINPUT_ENABLE_CLANG_TIDY=ON")

    if extra_args:
        cmd.extend(extra_args)

    return cmd


def build_tidy_command(
    build_dir: pathlib.Path | str | None = None,
    preset: str | None = None,
    target: str | None = "clang-tidy-check",
    extra_args: list[str] | None = None,
    project_root: pathlib.Path | None = None,
) -> list[str]:
    """Construct the CMake build command to run clang-tidy."""
    root = project_root if project_root is not None else PROJECT_ROOT
    resolved_dir = resolve_build_dir(build_dir=build_dir, preset=preset, project_root=root)

    cmd: list[str] = ["cmake", "--build", str(resolved_dir)]
    if target:
        cmd.extend(["--target", target])

    if extra_args:
        cmd.extend(extra_args)

    return cmd


def run_clang_tidy(
    build_dir: pathlib.Path | str | None = None,
    preset: str | None = None,
    target: str | None = "clang-tidy-check",
    build_with_tidy: bool = False,
    clean: bool = False,
    configure_if_missing: bool = True,
    extra_args: list[str] | None = None,
    project_root: pathlib.Path | None = None,
) -> int:
    """Execute the project's CMake clang-tidy integration."""
    root = project_root if project_root is not None else PROJECT_ROOT

    if not find_cmake():
        logger.error("CMake executable not found in PATH. Please install CMake.")
        return 1

    if not find_clang_tidy():
        logger.error("clang-tidy executable not found in PATH. CMake clang-tidy targets require clang-tidy.")
        return 1

    if preset is not None:
        presets = load_cmake_presets(root)
        if preset not in presets:
            available = ", ".join(sorted(presets.keys())) if presets else "none"
            logger.error(f"CMake preset '{preset}' was not found. Available presets: {available}")
            return 1

    resolved_dir = resolve_build_dir(build_dir=build_dir, preset=preset, project_root=root)

    if clean and resolved_dir.exists():
        logger.info(f"Cleaning build directory '{resolved_dir}'...")
        try:
            shutil.rmtree(resolved_dir)
        except OSError as e:
            logger.error(f"Failed to clean build directory '{resolved_dir}': {e}")
            return 1

    need_configure = not is_cmake_configured(resolved_dir) or build_with_tidy

    if need_configure:
        if not configure_if_missing and not is_cmake_configured(resolved_dir):
            logger.error(
                f"Build directory '{resolved_dir}' is not configured with CMake. "
                "Run cmake configuration first or omit --no-configure."
            )
            return 1

        logger.info(f"Configuring CMake for clang-tidy in '{resolved_dir}'...")
        configure_cmd = build_tidy_configure_command(
            build_dir=build_dir,
            preset=preset,
            enable_tidy_flag=build_with_tidy,
            project_root=root,
        )
        conf_code = run_command(configure_cmd, cwd=root)
        if conf_code != 0:
            logger.error(f"CMake configuration failed with exit code {conf_code}.")
            return conf_code

    exec_target = None if (build_with_tidy and target == "clang-tidy-check") else target

    build_cmd = build_tidy_command(
        build_dir=build_dir,
        preset=preset,
        target=exec_target,
        extra_args=extra_args,
        project_root=root,
    )

    action_label = (
        f"Building with clang-tidy enabled"
        if build_with_tidy
        else f"Running clang-tidy static analysis using target '{target}'"
    )
    logger.info(f"{action_label}...")

    ret_code = run_command(build_cmd, cwd=root)
    if ret_code != 0:
        logger.error(f"Clang-tidy execution failed with exit code {ret_code}.")
        return ret_code

    logger.info("Clang-tidy analysis completed successfully.")
    return 0
