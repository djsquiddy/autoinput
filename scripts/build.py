#!/usr/bin/env python
import argparse
import json
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys

from dataclasses import dataclass
import utils

logger = logging.getLogger(__name__)


# exit codes
EXIT_FAILED_CLEAN_BUILD_DIRECTORY = 1000
EXIT_FAILED_CREATE_BUILD_DIRECTORY = 1001
EXIT_FAILED_CMAKE_CONFIGURATION = 1002
EXIT_FAILED_SOURCE_COMPILATION = 1003
EXIT_FAILED_UNIT_TESTS = 1004
EXIT_LOCALIZATION_AUDIT = 1005
EXIT_FAILED_TO_CREATE_BUILDER = 1006


KNOWN_BUILD_TYPES = {
    "release": "Release",
    "debug": "Debug",
    "relwithdebinfo": "RelWithDebInfo",
    "minsizerel": "MinSizeRel",
}


@dataclass
class BuildConfig:
    clean: bool
    build_type: str
    build_tests: bool
    build_tray: bool
    build_ui: bool
    preset: str | None = None
    list_presets: bool = False


def format_log_line(line: str) -> str:
    """Format and colorize log lines if needed (e.g., GoogleTest markers on Windows pipes)."""
    # Don't re-colorize if already prefixed with ANSI escape sequences
    if line.startswith("\x1b["):
        return line

    if line.startswith(("[==========]", "[----------]", "[ RUN      ]", "[       OK ]", "[  PASSED  ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[32m{tag}\033[0m{rest}"
    if line.startswith(("[  FAILED  ]", "[  TIMEOUT ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[31m{tag}\033[0m{rest}"
    if line.startswith(("[  SKIPPED ]", "[ DISABLED ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[33m{tag}\033[0m{rest}"
    if line.startswith("Note: Google Test") or line.startswith("Note: This is test shard"):
        return f"\033[33m{line}\033[0m"
    if " YOU HAVE " in line and " DISABLED " in line:
        return f"\033[33m{line}\033[0m"
    if re.search(r"^\s*\d+ FAILED TESTS?", line):
        return f"\033[31m{line}\033[0m"

    return line


def load_cmake_presets() -> dict[str, dict]:
    """Loads and resolves configure presets from CMakePresets.json and CMakeUserPresets.json."""
    preset_files = [utils.ROOT_DIR / "CMakePresets.json", utils.ROOT_DIR / "CMakeUserPresets.json"]
    raw_configure_presets = {}

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

    def resolve_preset(name: str, visited: set | None = None) -> dict:
        if visited is None:
            visited = set()
        if name in visited or name not in raw_configure_presets:
            return {}
        visited.add(name)

        raw = raw_configure_presets[name]
        inherits = raw.get("inherits")
        merged = {
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

    presets = {}
    for name, cp in raw_configure_presets.items():
        resolved = resolve_preset(name)
        if not resolved.get("hidden", False):
            presets[name] = resolved

    return presets


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Build script for autoinput project.",
        add_help=True,
    )
    parser.add_argument(
        "-p",
        "--preset",
        default=None,
        help="CMake preset to use for configuring and building.",
    )
    parser.add_argument(
        "--list-presets",
        action="store_true",
        help="List available CMake configure presets and exit.",
    )
    parser.add_argument(
        "-c",
        "--clean",
        action="store_true",
        help="Clean build directory before building.",
    )
    parser.add_argument(
        "-b",
        "--build-type",
        default=None,
        help="CMake build type (Release, Debug, RelWithDebInfo, MinSizeRel).",
    )
    parser.add_argument(
        "targets_and_type",
        nargs="*",
        help="Build targets (ui, tray, tests, all) and/or build type (Release, Debug, etc.) or /clean",
    )
    return parser


def parse_arguments(argv: list[str]) -> BuildConfig:
    parser = get_parser()
    args, unknown = parser.parse_known_args(argv)

    list_presets = args.list_presets
    preset = args.preset
    clean = args.clean
    build_type = args.build_type
    explicit_targets = set()

    for item in list(args.targets_and_type) + unknown:
        item_lower = item.lower()
        if item_lower == "--list-presets":
            list_presets = True
        elif item.startswith("--preset="):
            preset = item.split("=", 1)[1]
        elif item_lower in ("--clean", "/clean", "-clean", "clean"):
            clean = True
        elif item_lower in ("ui", "tray", "tests", "all"):
            explicit_targets.add(item_lower)
        elif item_lower in KNOWN_BUILD_TYPES:
            if build_type is None:
                build_type = KNOWN_BUILD_TYPES[item_lower]
        elif not item.startswith("-") and not item.startswith("/"):
            if build_type is None:
                build_type = item

    if list_presets:
        return BuildConfig(
            clean=clean,
            build_type="Release",
            build_tests=False,
            build_tray=False,
            build_ui=False,
            preset=preset,
            list_presets=True,
        )

    if preset:
        build_tests = "tests" in explicit_targets
        build_tray = "tray" in explicit_targets
        build_ui = "ui" in explicit_targets
        return BuildConfig(
            clean=clean,
            build_type=build_type or "Release",
            build_tests=build_tests,
            build_tray=build_tray,
            build_ui=build_ui,
            preset=preset,
            list_presets=False,
        )

    if build_type is None:
        build_type = "Release"

    found_build_target = len(explicit_targets) > 0
    found_all = "all" in explicit_targets

    if not found_build_target:
        logger.info("No build target selected, building all.")
        found_all = True

    if found_all:
        logger.info("Enabling building autoinput tests")
        build_tests = True
        logger.info("Enabling building autoinput system tray")
        build_tray = True
        logger.info("Enabling building autoinput ui app")
        build_ui = True
    else:
        build_tests = "tests" in explicit_targets
        if build_tests:
            logger.info("Enabling building autoinput tests")
        build_tray = "tray" in explicit_targets
        if build_tray:
            logger.info("Enabling building autoinput system tray")
        build_ui = "ui" in explicit_targets
        if build_ui:
            logger.info("Enabling building autoinput ui app")

    return BuildConfig(
        clean=clean,
        build_type=build_type,
        build_tests=build_tests,
        build_tray=build_tray,
        build_ui=build_ui,
        preset=None,
        list_presets=False,
    )


def run_command(cmd: list[str | os.PathLike | pathlib.Path],
                cwd: pathlib.Path | None = None,
                extra_env: dict[str, str] | None = None,) -> int:
    cmd_str = [str(arg) for arg in cmd]
    env = os.environ.copy()
    env["CLICOLOR_FORCE"] = "1"
    env["FORCE_COLOR"] = "1"
    env["GTEST_COLOR"] = "yes"
    env["CMAKE_COLOR_DIAGNOSTICS"] = "ON"
    if extra_env:
        env.update(extra_env)

    try:
        with subprocess.Popen(
            cmd_str,
            cwd=cwd,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            encoding="utf-8",
            errors="replace",
        ) as process:
            if process.stdout:
                for line in iter(process.stdout.readline, ""):
                    if line:
                        logger.info(format_log_line(line.rstrip("\r\n")))
            return process.wait()
    except FileNotFoundError:
        logger.error(f"Error: '{cmd_str[0]}' executable not found in PATH.")
        return 1
    except Exception as e:
        logger.error(f"Failed to execute command {' '.join(cmd_str)}: {e}")
        return 1


def _terminal_setup() -> None:
    # Enable ANSI escape code support on Windows consoles
    if sys.platform == 'win32':
        os.system('')

def _list_preset() -> int:
    logger.info("Available configure presets:\n")
    presets = load_cmake_presets()
    for name, p_data in presets.items():
        display = p_data.get("displayName") or name
        desc = p_data.get("description")
        if desc:
            logger.info(f"  \"{name}\" - {display} ({desc})")
        else:
            logger.info(f"  \"{name}\" - {display}")
    return 0

class Builder:
    def __init__(self, config: BuildConfig, build_dir: pathlib.Path):
        self.config: BuildConfig = config
        self.build_dir: pathlib.Path = build_dir

    def clean_build(self) -> bool:
        if not self.config.clean or not self.build_dir.exists():
            return True
        logger.info("Cleaning build directory...")
        try:
            shutil.rmtree(self.build_dir)
            return True
        except Exception as e:
            logger.error(f"Failed to clean build directory: {e}")
            return False

    def create_build_directory(self) -> bool:
        if self.build_dir.exists():
            return True
        logger.info("creating build directory.")
        try:
            self.build_dir.mkdir(parents=True, exist_ok=True)
            return True
        except Exception as e:
            logger.error(f"Failed to create build directory: {e}")
            return False

    def run_cmake_configure(self) -> int:
        logger.info("Running CMake...")
        cmake_cmd = [
            "cmake",
            "-G",
            "Ninja",
            f"-DCMAKE_BUILD_TYPE={self.config.build_type}",
            "-DCMAKE_COLOR_DIAGNOSTICS=ON",
            f"-DAUTOINPUT_BUILD_TESTS={'ON' if self.config.build_tests else 'OFF'}",
            f"-DAUTOINPUT_BUILD_TRAY={'ON' if self.config.build_tray else 'OFF'}",
            f"-DAUTOINPUT_BUILD_UI={'ON' if self.config.build_ui else 'OFF'}",
            "..",
        ]
        ret = run_command(cmake_cmd, cwd=self.build_dir)
        if ret != 0:
            logger.error(f"CMake failed with return code {ret}.")
            return ret
        logger.info("Finished running CMake.")
        return utils.EXIT_SUCCESSFUL

    # noinspection method-may-be-static
    def build(self) -> int:
        logger.info("Building...")
        ret = run_command(["ninja"], cwd=self.build_dir)
        if ret != 0:
            logger.error(f"Ninja build failed with return code {ret}.")
            return ret
        logger.info("Finished building.")
        return utils.EXIT_SUCCESSFUL

    def run_tests(self) -> int:
        logger.info("Running tests...")
        if self.config.build_tests:
            test_exe_name = "autoinput-tests.exe" if sys.platform == "win32" else "autoinput-tests"
            test_bin = self.build_dir / "bin" / test_exe_name
            if not test_bin.exists() and sys.platform == "win32":
                # Fallback to non-exe name if applicable
                test_bin_alt = self.build_dir / "bin" / "autoinput-tests"
                if test_bin_alt.exists():
                    test_bin = test_bin_alt

            if test_bin.exists():
                ret = run_command([test_bin], cwd=self.build_dir)
                if ret != 0:
                    logger.error(f"Tests failed with return code {ret}.")
                    return ret
            else:
                logger.info("Test binary not found, skipping tests.")
        else:
            logger.info("Tests are disabled, skipping.")

        logger.info("Finished running tests.")
        return utils.EXIT_SUCCESSFUL

    def run_localization_audit(self) -> int:
        # Running localization audit
        logger.info("Running localization audit...")
        if self.config.build_ui:
            audit_script = utils.get_python_filepath('audit_localization')
            ret = run_command([sys.executable, audit_script], cwd=self.build_dir)
            if ret != 0:
                logger.error(f"Localization audit failed with return code {ret}.")
                return ret
        else:
            logger.info("UI is disabled, skipping localization audit.")

        logger.info("Finished running localization audit.")
        return utils.EXIT_SUCCESSFUL

    def run(self) -> int:
        if not self.clean_build():
            return EXIT_FAILED_CLEAN_BUILD_DIRECTORY
        if not self.create_build_directory():
            return EXIT_FAILED_CREATE_BUILD_DIRECTORY
        # CMake configuration
        if self.run_cmake_configure() != utils.EXIT_SUCCESSFUL:
            return EXIT_FAILED_CMAKE_CONFIGURATION
        # Build
        if self.build() != utils.EXIT_SUCCESSFUL:
            return EXIT_FAILED_SOURCE_COMPILATION
        # Running tests
        if self.run_tests() != utils.EXIT_SUCCESSFUL:
            return EXIT_FAILED_UNIT_TESTS
        if self.run_localization_audit() != utils.EXIT_SUCCESSFUL:
            return EXIT_LOCALIZATION_AUDIT
        return utils.EXIT_SUCCESSFUL


class PresetBuilder(Builder):
    def __init__(self, config: BuildConfig, preset_data: dict):
        preset_name = preset_data.get("name", config.preset)
        super().__init__(config, self._get_preset_build_dir(preset_data, preset_name))
        self.preset_data = preset_data

    @classmethod
    def create(cls, config: BuildConfig) -> 'PresetBuilder':
        presets = load_cmake_presets()
        if config.preset not in presets:
            logger.error(f"Error: Unknown preset '{config.preset}'.")
            logger.error(f"Available presets: {', '.join(presets.keys())}")
            return None
        return cls(config, presets[config.preset])

    @property
    def preset_name(self) -> str:
        if "name" not in self.preset_data:
            return self.config.preset
        return self.preset_data["name"]

    @staticmethod
    def _get_preset_build_dir(preset_data: dict, preset_name: str) -> pathlib.Path:
        raw_binary_dir = preset_data.get("binaryDir", "${sourceDir}/build/${presetName}")
        resolved = raw_binary_dir.replace("${sourceDir}", str(utils.ROOT_DIR)).replace("${presetName}", preset_name)
        return pathlib.Path(resolved).resolve()

    def run_cmake_configure(self) -> int:
        logger.info(f"Running CMake with preset '{self.preset_name}'...")
        cmake_cmd = [
            "cmake",
            "--preset",
            self.preset_name,
            "-DCMAKE_COLOR_DIAGNOSTICS=ON",
        ]
        ret = run_command(cmake_cmd, cwd=utils.ROOT_DIR)
        if ret != 0:
            logger.error(f"CMake failed with return code {ret}.")
            return ret
        logger.info("Finished running CMake.")
        return utils.EXIT_SUCCESSFUL

    def build(self) -> int:
        logger.info(f"Building with preset '{self.preset_name}'...")
        build_cmd = ["cmake", "--build", "--preset", self.preset_name]
        ret = run_command(build_cmd, cwd=utils.ROOT_DIR)
        if ret != 0:
            logger.error(f"Build failed with return code {ret}.")
            return ret
        logger.info("Finished building.")

        cache_vars = self.preset_data.get("cacheVariables", {})
        build_tests = self.config.build_tests or cache_vars.get("AUTOINPUT_BUILD_TESTS") in ("ON", "1", "TRUE", True)
        build_ui = self.config.build_ui or cache_vars.get("AUTOINPUT_BUILD_UI") in ("ON", "1", "TRUE", True)
        self.config = BuildConfig(
            clean=self.config.clean,
            build_type=self.config.build_type,
            build_tests=build_tests,
            build_tray=self.config.build_tray,
            build_ui=build_ui,
            preset=self.config.preset,
            list_presets=self.config.list_presets,
        )
        return utils.EXIT_SUCCESSFUL


def main() -> int:
    _terminal_setup()
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    config = parse_arguments(sys.argv[1:])

    if config.list_presets:
        return _list_preset()

    builder = PresetBuilder.create(config) if config.preset else Builder(config, utils.BUILD_DIR)
    if builder is None:
        return EXIT_FAILED_TO_CREATE_BUILDER
    return builder.run()


if __name__ == "__main__":
    sys.exit(main())
