#!/usr/bin/env python3
"""Unit tests for autoinput_tools.file_io."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.file_io import (
    check_or_write_file,
    dir_exists,
    ensure_dir,
    file_exists,
    get_flatten_toml,
    get_toml_keys,
    read_text,
    write_text,
    write_text_if_changed,
)


def test_read_and_write_text(tmp_path: pathlib.Path) -> None:
    test_file = tmp_path / "test.txt"
    content = "Hello, World!\nLine 2\n"

    write_text(test_file, content)
    # Verify written file exists on disk
    assert test_file.exists()
    # Verify file contents match written string exactly
    assert read_text(test_file) == content


def test_write_text_if_changed(tmp_path: pathlib.Path) -> None:
    test_file = tmp_path / "changed.txt"
    content_v1 = "Initial content"
    content_v2 = "Updated content"

    # Verify initial write returns True indicating file changed
    assert write_text_if_changed(test_file, content_v1) is True
    # Verify file contains initial content
    assert read_text(test_file) == content_v1

    # Verify identical write returns False indicating no change
    assert write_text_if_changed(test_file, content_v1) is False

    # Verify write with new content returns True indicating modification
    assert write_text_if_changed(test_file, content_v2) is True
    # Verify file contains updated content
    assert read_text(test_file) == content_v2


def test_check_or_write_file_write_mode(tmp_path: pathlib.Path) -> None:
    target = tmp_path / "sub" / "output.txt"
    content = "Sample payload"

    # Initial write: verify returns True and creates target file with content
    ok = check_or_write_file(target, content, check_only=False, filetype="test file")
    assert ok is True
    assert target.exists()
    assert read_text(target) == content

    # Repeated write with same content: verify returns True without failure
    ok = check_or_write_file(target, content, check_only=False, filetype="test file")
    assert ok is True


def test_check_or_write_file_check_mode(tmp_path: pathlib.Path) -> None:
    target = tmp_path / "check.txt"
    content = "Expected payload"

    # Non-existent file in check_only mode: verify returns False
    ok = check_or_write_file(target, content, check_only=True, filetype="missing file")
    assert ok is False

    # Create file with matching content: verify check_only returns True
    write_text(target, content)
    ok = check_or_write_file(target, content, check_only=True, filetype="matching file")
    assert ok is True

    # Modify file so it differs: verify check_only returns False
    write_text(target, "Differing payload")
    ok = check_or_write_file(target, content, check_only=True, filetype="differing file")
    assert ok is False


def test_ensure_dir(tmp_path: pathlib.Path) -> None:
    nested = tmp_path / "a" / "b" / "c"
    # Verify nested directory does not exist before ensure_dir
    assert not nested.exists()
    ensure_dir(nested)
    # Verify nested directory structure was created
    assert nested.exists()
    assert nested.is_dir()


def test_file_and_dir_exists(tmp_path: pathlib.Path) -> None:
    f = tmp_path / "file.txt"
    d = tmp_path / "dir"
    f.write_text("hello", encoding="utf-8")
    d.mkdir()

    # Verify file_exists returns True for regular file, False for directory or missing path
    assert file_exists(f) is True
    assert file_exists(d) is False
    assert file_exists(tmp_path / "nonexistent") is False

    # Verify dir_exists returns True for directory, False for regular file or missing path
    assert dir_exists(d) is True
    assert dir_exists(f) is False
    assert dir_exists(tmp_path / "nonexistent_dir") is False


def test_get_toml_keys() -> None:
    data = {
        "app": {
            "name": "autoinput",
            "version": "1.0",
        },
        "commands": {
            "run": {
                "desc": "run something",
            }
        },
        "flag": True,
    }
    keys = get_toml_keys(data)
    # Verify nested dictionary keys are flattened into dot-separated paths
    assert "app.name" in keys
    assert "app.version" in keys
    assert "commands.run.desc" in keys
    assert "flag" in keys


def test_get_flatten_toml() -> None:
    data = {
        "ui": {
            "button": {
                "ok": "OK",
                "cancel": "Cancel",
            },
            "title": "Main Window",
        },
        "count": 42,
    }
    flattened = get_flatten_toml(data)
    # Verify flattened dictionary maps dot-separated keys to their respective values
    assert flattened["ui.button.ok"] == "OK"
    assert flattened["ui.button.cancel"] == "Cancel"
    assert flattened["ui.title"] == "Main Window"
    assert flattened["count"] == 42

    # None data: verify returns empty dict
    assert get_flatten_toml(None) == {}
