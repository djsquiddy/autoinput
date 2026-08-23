#!/usr/bin/env python3
"""Unit tests for commands/audit_localization.py."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.localization.toml import LocalizationFile
from commands.audit_localization import KEY_PATTERNS, audit


def test_key_patterns_matching() -> None:
    samples = [
        ('.text("button.ok")', "button.ok"),
        ('->format("dialog.message")', "dialog.message"),
        ('::textOr("fallback.key")', "fallback.key"),
        ('UiWindow("Title", "window.key")', "window.key"),
        ('.labelKey = "label.test"', "label.test"),
        ('.categoryKey = "cat.test"', "cat.test"),
        ('m_titleKey = "title.test"', "title.test"),
    ]
    matched = set()
    for text, expected in samples:
        for pat in KEY_PATTERNS:
            for m in pat.finditer(text):
                matched.add(m.group(1))
    # Verify all expected localization key patterns match C++ source code idioms
    assert "button.ok" in matched
    assert "dialog.message" in matched
    assert "fallback.key" in matched
    assert "window.key" in matched
    assert "label.test" in matched
    assert "cat.test" in matched
    assert "title.test" in matched


def test_audit_success(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    src_dir = tmp_path / "src"
    src_dir.mkdir()
    (src_dir / "view.cpp").write_text(
        """
        loc.text("ui.save");
        loc.text("ui.cancel");
        """,
        encoding="utf-8",
    )

    mock_loc_file = LocalizationFile(
        "en-US",
        {"ui": {"save": "Save", "cancel": "Cancel"}},
    )

    monkeypatch.setattr("commands.audit_localization.UI_SRC_DIR", src_dir)
    monkeypatch.setattr("commands.audit_localization.PROJECT_ROOT", tmp_path)
    monkeypatch.setattr("commands.audit_localization.LocalizationFile.load", lambda name: mock_loc_file)

    # Verify audit succeeds when all C++ source localization keys exist in TOML
    assert audit() is True


def test_audit_missing_keys(tmp_path: pathlib.Path, monkeypatch: pytest.MonkeyPatch) -> None:
    src_dir = tmp_path / "src"
    src_dir.mkdir()
    (src_dir / "view.cpp").write_text(
        """
        loc.text("ui.save");
        loc.text("ui.nonexistent");
        """,
        encoding="utf-8",
    )

    mock_loc_file = LocalizationFile(
        "en-US",
        {"ui": {"save": "Save"}},
    )

    monkeypatch.setattr("commands.audit_localization.UI_SRC_DIR", src_dir)
    monkeypatch.setattr("commands.audit_localization.PROJECT_ROOT", tmp_path)
    monkeypatch.setattr("commands.audit_localization.LocalizationFile.load", lambda name: mock_loc_file)

    # Verify audit returns False when referenced keys are missing from localization TOML
    assert audit() is False


def test_audit_invalid_loc_data(monkeypatch: pytest.MonkeyPatch) -> None:
    invalid_loc = LocalizationFile("en-US", None)
    monkeypatch.setattr("commands.audit_localization.LocalizationFile.load", lambda name: invalid_loc)
    # Verify audit fails when localization file cannot be loaded
    assert audit() is False
