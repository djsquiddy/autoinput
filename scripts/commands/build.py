#!/usr/bin/env python3
"""Project build automation script for AutoInput."""

import argparse
import json
import logging
import os
import pathlib
import re
import shutil
import stat
import subprocess
import sys
import time
from dataclasses import dataclass, field

_SCRIPTS_DIR = pathlib.Path(__file__).resolve().parent.parent
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.paths import (
    BUILD_DIR,
    COMMANDS_DIR,
    PROJECT_ROOT,
)
from autoinput_tools.process import run_command

EXIT_SUCCESSFUL = 0

logger = logging.getLogger(__name__)


# exit codes
EXIT_FAILED_CLEAN_BUILD_DIRECTORY = 1000
EXIT_FAILED_CREATE_BUILD_DIRECTORY = 1001
EXIT_FAILED_CMAKE_CONFIGURATION = 1002
EXIT_FAILED_SOURCE_COMPILATION = 1003
EXIT_FAILED_UNIT_TESTS = 1004
EXIT_LOCALIZATION_AUDIT = 1005
EXIT_FAILED_TO_CREATE_BUILDER = 1006
EXIT_FAILED_AUTOCOMPLETE = 1007
EXIT_FAILED_PYTHON_TESTS = 1008


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
    audit: bool = False
    bulk_build: bool = False
    list_presets: bool = False
    build_only: bool = False
    test_only: bool = False
    audit_only: bool = False
    extra_cmake_args: list[str] = field(default_factory=list)


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
    preset_files = [PROJECT_ROOT / "CMakePresets.json", PROJECT_ROOT / "CMakeUserPresets.json"]
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
    _ = parser.add_argument(
        "-p",
        "--preset",
        default=None,
        help="CMake preset to use for configuring and building.",
    )
    _ = parser.add_argument(
        "--list-presets",
        action="store_true",
        help="List available CMake configure presets and exit.",
    )
    _ = parser.add_argument(
        "-c",
        "--clean",
        action="store_true",
        help="Clean build directory before building.",
    )
    _ = parser.add_argument(
        "-b",
        "--build-type",
        default=None,
        help="CMake build type (Release, Debug, RelWithDebInfo, MinSizeRel).",
    )
    _ = parser.add_argument(
        "--audit",
        action="store_true",
        help="Run static analysis on the codebase. Run the localization audit to check for missing or incorrect translations.",
    )
    _ = parser.add_argument(
        "--bulk-build",
        action="store_true",
        help="Build all targets in parallel.",
    )
    _ = parser.add_argument(
        "--build-only",
        action="store_true",
        help="Configure and build the project only without running tests, audit, or updating autocomplete.",
    )
    _ = parser.add_argument(
        "--test-only",
        action="store_true",
        help="Run tests only without cleaning, configuring, building, or auditing.",
    )
    _ = parser.add_argument(
        "--audit-only",
        action="store_true",
        help="Run localization audit only without cleaning, configuring, building, running tests, or updating autocomplete.",
    )
    _ = parser.add_argument(
        "targets_and_type",
        nargs="*",
        help="Build targets (ui, tray, tests, all) and/or build type (Release, Debug, etc.) or /clean",
    )
    return parser


def parse_arguments(argv: list[str]) -> BuildConfig:
    parser = get_parser()
    args, unknown = parser.parse_known_args(argv)

    list_presets: bool = args.list_presets
    preset: str = args.preset
    clean: bool = args.clean
    build_type: str | None = args.build_type
    audit: bool = args.audit
    bulk_build: bool = args.bulk_build
    build_only: bool = args.build_only
    test_only: bool = args.test_only
    audit_only: bool = args.audit_only
    explicit_targets: set[str] = set()
    extra_cmake_args: list[str] = []

    item: str
    for item in list(args.targets_and_type) + unknown:
        item_lower = item.lower()
        if item_lower == "--list-presets":
            list_presets = True
        elif item.startswith("--preset="):
            preset = item.split("=", 1)[1]
        elif item_lower in ("--clean", "/clean", "-clean", "clean"):
            clean = True
        elif item_lower in ("--build-only", "/build-only", "-build-only"):
            build_only = True
        elif item_lower in ("--test-only", "/test-only", "-test-only"):
            test_only = True
        elif item_lower in ("--audit-only", "/audit-only", "-audit-only"):
            audit_only = True
        elif item_lower in ("ui", "tray", "tests", "all", "python-tests", "python_tests", "pytest", "pytests"):
            if item_lower in ("python-tests", "python_tests", "pytest", "pytests"):
                explicit_targets.add("tests")
            else:
                explicit_targets.add(item_lower)
        elif item_lower in KNOWN_BUILD_TYPES:
            if build_type is None:
                build_type = KNOWN_BUILD_TYPES[item_lower]
        elif item.startswith("-D") or item.startswith("/D"):
            extra_cmake_args.append(item)
            if item.startswith("-DCMAKE_UNITY_BUILD") or item.startswith("/DCMAKE_UNITY_BUILD"):
                if "=" in item:
                    val = item.split("=", 1)[1].upper()
                    bulk_build = val in ("ON", "1", "TRUE", "YES", "Y")
                else:
                    bulk_build = True
        elif not item.startswith("-") and not item.startswith("/"):
            if build_type is None:
                build_type = item

    flag_count = sum([bool(build_only), bool(test_only), bool(audit_only)])
    if flag_count > 1:
        parser.error("Flags --build-only, --test-only, and --audit-only are mutually exclusive.")

    if list_presets:
        return BuildConfig(
            clean=clean,
            build_type="Release",
            build_tests=False,
            build_tray=False,
            build_ui=False,
            audit=audit or audit_only,
            bulk_build=bulk_build,
            preset=preset,
            list_presets=True,
            build_only=build_only,
            test_only=test_only,
            audit_only=audit_only,
            extra_cmake_args=extra_cmake_args,
        )

    if preset:
        build_tests = ("tests" in explicit_targets) or test_only
        build_tray = "tray" in explicit_targets
        build_ui = ("ui" in explicit_targets) or (audit_only and not explicit_targets)
        return BuildConfig(
            clean=clean,
            build_type=build_type or "Release",
            build_tests=build_tests,
            build_tray=build_tray,
            build_ui=build_ui,
            audit=audit or audit_only,
            bulk_build=bulk_build,
            preset=preset,
            list_presets=False,
            build_only=build_only,
            test_only=test_only,
            audit_only=audit_only,
            extra_cmake_args=extra_cmake_args,
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
        build_tests = ("tests" in explicit_targets) or test_only
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
        audit=audit or audit_only,
        bulk_build=bulk_build,
        list_presets=False,
        build_only=build_only,
        test_only=test_only,
        audit_only=audit_only,
        extra_cmake_args=extra_cmake_args,
    )

def _terminal_setup() -> None:
    if sys.platform != 'win32':
        return
    # Enable ANSI escape code support on Windows consoles
    from ctypes import windll, byref, create_string_buffer
    # Define constants
    STD_OUTPUT_HANDLE = -11
    ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004

    # Get handle to stdout
    handle = windll.kernel32.GetStdHandle(STD_OUTPUT_HANDLE)

    # Get current console mode
    mode = create_string_buffer(4) # DWORD is 4 bytes
    if windll.kernel32.GetConsoleMode(handle, byref(mode)):
        current_mode = int.from_bytes(mode.raw, 'little')

        # Enable virtual terminal processing if not already set
        if not (current_mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING):
            windll.kernel32.SetConsoleMode(handle, current_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)

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

def format_duration(seconds: float | None) -> str:
    if seconds is None:
        return "-"
    if seconds < 0:
        return "0.00s"
    if seconds >= 60:
        minutes = int(seconds // 60)
        remaining_seconds = seconds % 60
        return f"{minutes}m {remaining_seconds:05.2f}s"
    return f"{seconds:.2f}s"


def format_file_size(size_bytes: int) -> str:
    if size_bytes >= 1024 * 1024:
        return f"{size_bytes / (1024 * 1024):.2f} MB"
    if size_bytes >= 1024:
        return f"{size_bytes / 1024:.2f} KB"
    return f"{size_bytes} B"


@dataclass
class ExecutableInfo:
    name: str
    path: pathlib.Path
    size: int


def find_executables(build_dir: pathlib.Path) -> list[ExecutableInfo]:
    executables: list[ExecutableInfo] = []
    bin_dir = build_dir / "bin"
    search_dirs = [bin_dir] if bin_dir.exists() else [build_dir]
    seen_names: set[str] = set()

    for d in search_dirs:
        if not d.exists() or not d.is_dir():
            continue
        for item in d.iterdir():
            if not item.is_file() or item.name in seen_names:
                continue
            if sys.platform == "win32":
                if item.suffix.lower() == ".exe":
                    try:
                        seen_names.add(item.name)
                        executables.append(
                            ExecutableInfo(
                                name=item.name,
                                path=item,
                                size=item.stat().st_size,
                            )
                        )
                    except OSError:
                        pass
            else:
                if os.access(item, os.X_OK) and item.suffix.lower() not in (
                    ".so", ".a", ".dylib", ".log", ".txt", ".json", ".toml", ".py", ".sh", ".cmake", ".ninja"
                ):
                    try:
                        seen_names.add(item.name)
                        executables.append(
                            ExecutableInfo(
                                name=item.name,
                                path=item,
                                size=item.stat().st_size,
                            )
                        )
                    except OSError:
                        pass

    return sorted(executables, key=lambda x: x.name.lower())


@dataclass
class StepResult:
    name: str
    status: str  # "PASSED", "FAILED", "SKIPPED"
    duration: float | None = None


class BuildReport:
    def __init__(self, config: BuildConfig, build_dir: pathlib.Path):
        self.config: BuildConfig = config
        self.build_dir: pathlib.Path = build_dir
        self.steps: dict[str, StepResult] = {
            "Clean": StepResult("Clean", "SKIPPED"),
            "CMake Configure": StepResult("CMake Configure", "SKIPPED"),
            "Build": StepResult("Build", "SKIPPED"),
            "Unit Tests": StepResult("Unit Tests", "SKIPPED"),
            "Python Tests": StepResult("Python Tests", "SKIPPED"),
            "Localization Audit": StepResult("Localization Audit", "SKIPPED"),
            "Update Autocomplete": StepResult("Update Autocomplete", "SKIPPED"),
        }
        self.executables: list[ExecutableInfo] = []

    def record_step(self, name: str, status: str, duration: float | None = None) -> None:
        self.steps[name] = StepResult(name=name, status=status, duration=duration)

    def collect_executables(self) -> None:
        self.executables = find_executables(self.build_dir)

    def print_summary(self, total_duration: float, success: bool) -> None:
        width = 62
        divider = "=" * width
        thin_divider = "-" * width

        logger.info("")
        logger.info(divider)
        logger.info(f"{'BUILD SUMMARY':^{width}}")
        logger.info(divider)
        if self.config.preset:
            logger.info(f"  Preset:           {self.config.preset}")
        else:
            logger.info(f"  Build Type:       {self.config.build_type}")
        logger.info(f"  Build Directory:  {self.build_dir}")
        logger.info(thin_divider)
        logger.info(f"  {'Step':<24}{'Status':<20}{'Time':>14}")
        logger.info(thin_divider)

        for step in self.steps.values():
            time_str = format_duration(step.duration)
            status_padded = f"{step.status:<20}"
            if step.status == "PASSED":
                colored_status = f"\033[32m{status_padded}\033[0m"
            elif step.status == "FAILED":
                colored_status = f"\033[31m{status_padded}\033[0m"
            elif step.status == "SKIPPED":
                colored_status = f"\033[33m{status_padded}\033[0m"
            else:
                colored_status = status_padded

            logger.info(f"  {step.name:<24}{colored_status}{time_str:>14}")

        if self.executables:
            logger.info(thin_divider)
            logger.info("  Executables:")
            for exe in self.executables:
                size_str = format_file_size(exe.size)
                logger.info(f"    {exe.name:<38}{size_str:>18}")

        logger.info(thin_divider)
        logger.info(f"  {'Total Time:':<44}{format_duration(total_duration):>14}")
        result_colored = "\033[32m\033[1mSUCCESS\033[0m" if success else "\033[31m\033[1mFAILED\033[0m"
        logger.info(f"  {'Overall Result:':<24}{result_colored}")
        logger.info(divider)


class Builder:
    def __init__(self, config: BuildConfig, build_dir: pathlib.Path):
        self.config: BuildConfig = config
        self.build_dir: pathlib.Path = build_dir

    def clean_build(self) -> tuple[bool, float]:
        if not self.config.clean or not self.build_dir.exists():
            return True, 0.0
        logger.info("Cleaning build directory...")
        start = time.perf_counter()
        try:
            def on_rm_error(func, path, exc_info):
                try:
                    os.chmod(path, stat.S_IWRITE)
                    func(path)
                except Exception:
                    pass

            shutil.rmtree(self.build_dir, onerror=on_rm_error)
            duration = time.perf_counter() - start
            logger.info(f"Finished cleaning build directory ({format_duration(duration)}).")
            return True, duration
        except Exception as e:
            duration = time.perf_counter() - start
            logger.error(f"Failed to clean build directory: {e}")
            return False, duration

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

    def run_cmake_configure(self) -> tuple[int, float]:
        logger.info("Running CMake...")
        start = time.perf_counter()
        cmake_cmd = [
            "cmake",
            "-G",
            "Ninja",
            f"-DCMAKE_BUILD_TYPE={self.config.build_type}",
            "-DCMAKE_COLOR_DIAGNOSTICS=ON",
            f"-DAUTOINPUT_BUILD_TESTS={'ON' if self.config.build_tests else 'OFF'}",
            f"-DAUTOINPUT_BUILD_TRAY={'ON' if self.config.build_tray else 'OFF'}",
            f"-DAUTOINPUT_BUILD_UI={'ON' if self.config.build_ui else 'OFF'}",
            f"-DCMAKE_UNITY_BUILD={'ON' if self.config.bulk_build else 'OFF'}",
        ]
        for arg in self.config.extra_cmake_args:
            if not arg.startswith(("-DCMAKE_UNITY_BUILD", "/DCMAKE_UNITY_BUILD")):
                cmake_cmd.append(arg)
        cmake_cmd.append("..")
        ret = run_command([str(c) for c in cmake_cmd], cwd=self.build_dir)
        duration = time.perf_counter() - start
        if ret != 0:
            logger.error(f"CMake failed with return code {ret}.")
            return ret, duration
        logger.info(f"Finished running CMake ({format_duration(duration)}).")
        return EXIT_SUCCESSFUL, duration

    # noinspection method-may-be-static
    def build(self) -> tuple[int, float]:
        logger.info("Building...")
        start = time.perf_counter()
        ret = run_command(["ninja"], cwd=self.build_dir)
        duration = time.perf_counter() - start
        if ret != 0:
            logger.error(f"Ninja build failed with return code {ret}.")
            return ret, duration
        logger.info(f"Finished building ({format_duration(duration)}).")
        return EXIT_SUCCESSFUL, duration

    def run_tests(self) -> tuple[int, float | None, bool]:
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
                start = time.perf_counter()
                ret = run_command([test_bin], cwd=self.build_dir)
                duration = time.perf_counter() - start
                if ret != 0:
                    logger.error(f"Tests failed with return code {ret}.")
                    return ret, duration, True
                logger.info(f"Finished running tests ({format_duration(duration)}).")
                return EXIT_SUCCESSFUL, duration, True
            else:
                logger.info("Test binary not found, skipping tests.")
                return EXIT_SUCCESSFUL, None, False
        else:
            logger.info("Tests are disabled, skipping.")
            return EXIT_SUCCESSFUL, None, False

    def run_python_tests(self) -> tuple[int, float | None, bool]:
        logger.info("Running Python tests...")
        if self.config.build_tests:
            import importlib.util
            if importlib.util.find_spec("pytest") is None:
                logger.warning('pytest is not installed. Skipping running python tests.')
                return EXIT_SUCCESSFUL, None, False
            test_dir = PROJECT_ROOT / "tests" / "scripts"
            if not test_dir.exists():
                logger.info("Python test directory not found, skipping Python tests.")
                return EXIT_SUCCESSFUL, None, False

            start = time.perf_counter()
            ret = run_command(
                [sys.executable, "-m", "pytest", str(test_dir)],
                cwd=PROJECT_ROOT,
            )
            duration = time.perf_counter() - start
            if ret != 0:
                logger.error(f"Python tests failed with return code {ret}.")
                return ret, duration, True
            logger.info(f"Finished running Python tests ({format_duration(duration)}).")
            return EXIT_SUCCESSFUL, duration, True
        else:
            logger.info("Tests are disabled, skipping Python tests.")
            return EXIT_SUCCESSFUL, None, False

    def run_localization_audit(self) -> tuple[int, float | None, bool]:
        if not self.config.audit and not self.config.audit_only:
            return EXIT_SUCCESSFUL, None, False
        # Running localization audit
        logger.info("Running localization audit...")
        if self.config.audit_only:
            if not self.build_dir.exists():
                logger.error(f"Build directory does not exist: {self.build_dir}. A prior build is required for audit.")
                return EXIT_LOCALIZATION_AUDIT, None, True
            if not self.config.build_ui:
                logger.error("Localization audit requires UI to be enabled/built. Target 'ui' or 'all' is required.")
                return EXIT_LOCALIZATION_AUDIT, None, True

        if self.config.build_ui:
            start = time.perf_counter()
            audit_script = COMMANDS_DIR / "audit_localization.py"
            ret = run_command(
                [sys.executable, str(audit_script)],
                cwd=self.build_dir
            )
            duration = time.perf_counter() - start
            if ret != 0:
                logger.error(f"Localization audit failed with return code {ret}.")
                return ret, duration, True
            logger.info(f"Finished running localization audit ({format_duration(duration)}).")
            return EXIT_SUCCESSFUL, duration, True
        else:
            logger.info("UI is disabled, skipping localization audit.")
            return EXIT_SUCCESSFUL, None, False

    def _find_autoinput_executable(self) -> pathlib.Path | None:
        executables = find_executables(self.build_dir)
        for exe in executables:
            if exe.name.lower() in ("autoinput.exe", "autoinput"):
                return exe.path
        exe_name = "autoinput.exe" if sys.platform == "win32" else "autoinput"
        direct_candidates = [
            self.build_dir / "bin" / exe_name,
            self.build_dir / exe_name,
            self.build_dir / "src" / "autoinput_cli" / exe_name,
        ]
        for candidate in direct_candidates:
            if candidate.exists() and candidate.is_file():
                return candidate
        return None

    def run_update_autocomplete(self) -> tuple[int, float | None, bool]:
        # The primary metadata source is now resources/cli/help.toml (--source toml, the
        # default), so a missing binary no longer blocks the update. The binary is only
        # passed along as a fallback hint in case --source binary is ever used.
        logger.info("Updating autocomplete scripts from resources/cli/help.toml...")
        start = time.perf_counter()
        update_script = COMMANDS_DIR / "update_autocomplete.py"
        command = [sys.executable, str(update_script)]

        autoinput_bin = self._find_autoinput_executable()
        if autoinput_bin:
            command.extend(["--binary", str(autoinput_bin)])

        ret = run_command(command, cwd=PROJECT_ROOT)
        duration = time.perf_counter() - start
        if ret != 0:
            logger.error(f"Updating autocomplete scripts failed with return code {ret}.")
            return ret, duration, True
        logger.info(f"Finished updating autocomplete scripts ({format_duration(duration)}).")
        return EXIT_SUCCESSFUL, duration, True

    def run(self) -> int:
        total_start = time.perf_counter()
        exit_code = EXIT_SUCCESSFUL
        report = BuildReport(self.config, self.build_dir)

        try:
            # Clean
            if self.config.test_only or self.config.audit_only:
                report.record_step("Clean", "SKIPPED")
            elif self.config.clean:
                clean_ok, clean_dur = self.clean_build()
                if not clean_ok:
                    report.record_step("Clean", "FAILED", clean_dur)
                    exit_code = EXIT_FAILED_CLEAN_BUILD_DIRECTORY
                    return exit_code
                report.record_step("Clean", "PASSED", clean_dur)
            else:
                report.record_step("Clean", "SKIPPED")

            # Create build directory, CMake configure, Build
            if not self.config.test_only and not self.config.audit_only:
                # Create build directory
                if not self.create_build_directory():
                    exit_code = EXIT_FAILED_CREATE_BUILD_DIRECTORY
                    return exit_code

                # CMake configuration
                ret, cmake_dur = self.run_cmake_configure()
                if ret != EXIT_SUCCESSFUL:
                    report.record_step("CMake Configure", "FAILED", cmake_dur)
                    exit_code = EXIT_FAILED_CMAKE_CONFIGURATION
                    return exit_code
                report.record_step("CMake Configure", "PASSED", cmake_dur)

                # Build
                ret, build_dur = self.build()
                report.config = self.config
                if ret != EXIT_SUCCESSFUL:
                    report.record_step("Build", "FAILED", build_dur)
                    exit_code = EXIT_FAILED_SOURCE_COMPILATION
                    return exit_code
                report.record_step("Build", "PASSED", build_dur)
            else:
                report.record_step("CMake Configure", "SKIPPED")
                report.record_step("Build", "SKIPPED")

            # Running tests
            if self.config.build_only or self.config.audit_only:
                report.record_step("Unit Tests", "SKIPPED")
                report.record_step("Python Tests", "SKIPPED")
            else:
                # Running tests
                test_ret, test_dur, test_run = self.run_tests()
                if not test_run:
                    report.record_step("Unit Tests", "SKIPPED")
                elif test_ret != EXIT_SUCCESSFUL:
                    report.record_step("Unit Tests", "FAILED", test_dur)
                    exit_code = EXIT_FAILED_UNIT_TESTS
                    return exit_code
                else:
                    report.record_step("Unit Tests", "PASSED", test_dur)

                # Running Python tests
                py_test_ret, py_test_dur, py_test_run = self.run_python_tests()
                if not py_test_run:
                    report.record_step("Python Tests", "SKIPPED")
                elif py_test_ret != EXIT_SUCCESSFUL:
                    report.record_step("Python Tests", "FAILED", py_test_dur)
                    exit_code = EXIT_FAILED_PYTHON_TESTS
                    return exit_code
                else:
                    report.record_step("Python Tests", "PASSED", py_test_dur)

            # Localization audit
            if self.config.build_only or self.config.test_only:
                report.record_step("Localization Audit", "SKIPPED")
            else:
                audit_ret, audit_dur, audit_run = self.run_localization_audit()
                if not audit_run:
                    report.record_step("Localization Audit", "SKIPPED")
                elif audit_ret != EXIT_SUCCESSFUL:
                    report.record_step("Localization Audit", "FAILED", audit_dur)
                    exit_code = EXIT_LOCALIZATION_AUDIT
                    return exit_code
                else:
                    report.record_step("Localization Audit", "PASSED", audit_dur)

            # Update autocomplete
            if self.config.build_only or self.config.test_only or self.config.audit_only:
                report.record_step("Update Autocomplete", "SKIPPED")
            else:
                auto_ret, auto_dur, auto_run = self.run_update_autocomplete()
                if not auto_run:
                    report.record_step("Update Autocomplete", "SKIPPED")
                elif auto_ret != EXIT_SUCCESSFUL:
                    report.record_step("Update Autocomplete", "FAILED", auto_dur)
                    exit_code = EXIT_FAILED_AUTOCOMPLETE
                    return exit_code
                else:
                    report.record_step("Update Autocomplete", "PASSED", auto_dur)

            return EXIT_SUCCESSFUL
        finally:
            total_duration = time.perf_counter() - total_start
            report.collect_executables()
            report.print_summary(total_duration, exit_code == EXIT_SUCCESSFUL)


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
        resolved = raw_binary_dir.replace("${sourceDir}", str(PROJECT_ROOT)).replace("${presetName}", preset_name)
        return pathlib.Path(resolved).resolve()

    def run_cmake_configure(self) -> tuple[int, float]:
        logger.info(f"Running CMake with preset '{self.preset_name}'...")
        start = time.perf_counter()
        cmake_cmd = [
            "cmake",
            "--preset",
            self.preset_name,
            "-DCMAKE_COLOR_DIAGNOSTICS=ON",
        ]
        if self.config.bulk_build:
            cmake_cmd.append("-DCMAKE_UNITY_BUILD=ON")
        elif any(arg.startswith(("-DCMAKE_UNITY_BUILD=OFF", "-DCMAKE_UNITY_BUILD=0", "-DCMAKE_UNITY_BUILD=FALSE")) for arg in self.config.extra_cmake_args):
            cmake_cmd.append("-DCMAKE_UNITY_BUILD=OFF")
        for arg in self.config.extra_cmake_args:
            if not arg.startswith(("-DCMAKE_UNITY_BUILD", "/DCMAKE_UNITY_BUILD")):
                cmake_cmd.append(arg)
        ret = run_command(cmake_cmd, cwd=PROJECT_ROOT)
        duration = time.perf_counter() - start
        if ret != 0:
            logger.error(f"CMake failed with return code {ret}.")
            return ret, duration
        logger.info(f"Finished running CMake ({format_duration(duration)}).")
        return EXIT_SUCCESSFUL, duration

    def build(self) -> tuple[int, float]:
        logger.info(f"Building with preset '{self.preset_name}'...")
        start = time.perf_counter()
        build_cmd = ["cmake", "--build", "--preset", self.preset_name]
        ret = run_command(build_cmd, cwd=PROJECT_ROOT)
        duration = time.perf_counter() - start
        if ret != 0:
            logger.error(f"Build failed with return code {ret}.")
            return ret, duration
        logger.info(f"Finished building ({format_duration(duration)}).")

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
            audit=self.config.audit,
            bulk_build=self.config.bulk_build,
            list_presets=self.config.list_presets,
            build_only=self.config.build_only,
            test_only=self.config.test_only,
            audit_only=self.config.audit_only,
            extra_cmake_args=self.config.extra_cmake_args,
        )
        return EXIT_SUCCESSFUL, duration


def main() -> int:
    _terminal_setup()
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    config = parse_arguments(sys.argv[1:])

    if config.list_presets:
        return _list_preset()

    builder = PresetBuilder.create(config) if config.preset else Builder(config, BUILD_DIR)
    if builder is None:
        return EXIT_FAILED_TO_CREATE_BUILDER
    return builder.run()


if __name__ == "__main__":
    sys.exit(main())
