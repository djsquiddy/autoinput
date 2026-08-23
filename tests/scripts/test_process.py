#!/usr/bin/env python3
"""Unit tests for autoinput_tools.process."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.process import run_command


def test_run_command_success() -> None:
    code = run_command([sys.executable, "-c", "import sys; sys.stdout.write('test_out\\n')"])
    # Verify command execution completes with exit code 0 on success
    assert code == 0


def test_run_command_failure() -> None:
    code = run_command([sys.executable, "-c", "import sys; sys.exit(42)"])
    # Verify non-zero exit code propagated from subprocess
    assert code == 42


def test_run_command_extra_env() -> None:
    code = run_command(
        [sys.executable, "-c", "import os, sys; sys.exit(0 if os.environ.get('TEST_VAR') == '123' else 1)"],
        extra_env={"TEST_VAR": "123"},
    )
    # Verify custom environment variables are injected into subprocess environment
    assert code == 0


def test_run_command_file_not_found() -> None:
    code = run_command(["non_existent_executable_12345"])
    # Verify missing executable returns error exit code (1)
    assert code == 1


def test_run_command_with_cwd(tmp_path: pathlib.Path) -> None:
    marker_file = tmp_path / "marker.txt"
    code = run_command(
        [sys.executable, "-c", f"import pathlib; pathlib.Path('marker.txt').write_text('ok')"],
        cwd=tmp_path,
    )
    # Verify command completes successfully when executed with specified working directory
    assert code == 0
    # Verify marker file was written in the expected working directory
    assert marker_file.exists()
    assert marker_file.read_text() == "ok"
