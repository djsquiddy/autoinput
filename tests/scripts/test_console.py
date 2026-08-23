#!/usr/bin/env python3
"""Unit tests for autoinput_tools.console."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.console import format_log_line


def test_ansi_escaped_line_untouched() -> None:
    line = "\x1b[31mAlready colored\x1b[0m"
    # Verify that lines already containing ANSI escape sequences remain unmodified
    assert format_log_line(line) == line


@pytest.mark.parametrize(
    "prefix",
    ["[==========]", "[----------]", "[ RUN      ]", "[       OK ]", "[  PASSED  ]"],
)
def test_green_log_markers(prefix: str) -> None:
    line = f"{prefix} Test suite marker"
    formatted = format_log_line(line)
    # Verify prefix is highlighted in green (ANSI 32m)
    assert formatted.startswith(f"\033[32m{prefix}\033[0m")
    # Verify the remaining line content is preserved after the prefix
    assert formatted.endswith(" Test suite marker")


@pytest.mark.parametrize("prefix", ["[  FAILED  ]", "[  TIMEOUT ]"])
def test_red_log_markers(prefix: str) -> None:
    line = f"{prefix} Test failed"
    formatted = format_log_line(line)
    # Verify prefix is highlighted in red (ANSI 31m)
    assert formatted.startswith(f"\033[31m{prefix}\033[0m")
    # Verify the remaining line content is preserved after the prefix
    assert formatted.endswith(" Test failed")


@pytest.mark.parametrize("prefix", ["[  SKIPPED ]", "[ DISABLED ]"])
def test_yellow_log_markers(prefix: str) -> None:
    line = f"{prefix} Test skipped"
    formatted = format_log_line(line)
    # Verify prefix is highlighted in yellow (ANSI 33m)
    assert formatted.startswith(f"\033[33m{prefix}\033[0m")
    # Verify the remaining line content is preserved after the prefix
    assert formatted.endswith(" Test skipped")


def test_google_test_notes() -> None:
    note1 = "Note: Google Test filter = Foo*"
    # Verify GoogleTest filter notes are colored yellow
    assert format_log_line(note1) == f"\033[33m{note1}\033[0m"

    note2 = "Note: This is test shard 1 of 2"
    # Verify GoogleTest shard notes are colored yellow
    assert format_log_line(note2) == f"\033[33m{note2}\033[0m"


def test_disabled_summary_line() -> None:
    summary = "  YOU HAVE 5 DISABLED TESTS"
    # Verify disabled tests summary line is highlighted in yellow
    assert format_log_line(summary) == f"\033[33m{summary}\033[0m"


@pytest.mark.parametrize("line", ["1 FAILED TEST", "  12 FAILED TESTS", "0 FAILED TESTS"])
def test_failed_tests_summary_line(line: str) -> None:
    # Verify failed tests summary line is highlighted in red
    assert format_log_line(line) == f"\033[31m{line}\033[0m"


def test_plain_log_line() -> None:
    line = "Compiling src/main.cpp..."
    # Verify unformatted plain log lines pass through unchanged
    assert format_log_line(line) == line
