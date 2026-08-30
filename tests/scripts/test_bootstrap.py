#!/usr/bin/env python3
"""Tests for the scripts/commands/_bootstrap.py mechanism."""

import importlib
import os
import pathlib
import subprocess
import sys

from autoinput_tools.paths import PROJECT_ROOT


def test_bootstrap_adds_scripts_dir_to_sys_path() -> None:
    scripts_dir = PROJECT_ROOT / "scripts"
    scripts_dir_str = str(scripts_dir)

    # Remove scripts_dir from sys.path if present
    original_path = list(sys.path)
    try:
        sys.path = [p for p in sys.path if p != scripts_dir_str]

        # Re-import or reload _bootstrap
        if "commands._bootstrap" in sys.modules:
            del sys.modules["commands._bootstrap"]
        if "_bootstrap" in sys.modules:
            del sys.modules["_bootstrap"]

        import commands._bootstrap  # noqa: F401

        assert scripts_dir_str in sys.path
        assert sys.path[0] == scripts_dir_str
    finally:
        sys.path = original_path


def test_direct_script_execution_without_pythonpath() -> None:
    """Verify that command scripts can be run directly without PYTHONPATH configured."""
    scripts_to_test = [
        "scripts/commands/build.py",
        "scripts/commands/format.py",
        "scripts/commands/clang_format.py",
        "scripts/commands/clang_tidy.py",
        "scripts/commands/tidy.py",
        "scripts/commands/gen_app_icon.py",
        "scripts/commands/gen_cli_help.py",
        "scripts/commands/gen_localization_ids.py",
    ]

    clean_env = os.environ.copy()
    clean_env.pop("PYTHONPATH", None)

    for script_rel_path in scripts_to_test:
        script_full_path = PROJECT_ROOT / script_rel_path
        proc = subprocess.run(
            [sys.executable, str(script_full_path), "--help"],
            cwd=str(PROJECT_ROOT),
            env=clean_env,
            capture_output=True,
            text=True,
        )
        assert proc.returncode == 0, f"Failed running {script_rel_path}: {proc.stderr}"
        assert "usage:" in proc.stdout.lower() or "options:" in proc.stdout.lower()
