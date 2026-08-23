#!/usr/bin/env python3
"""Unit tests for autoinput_tools.localization package."""

import pathlib
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.localization.cpp import generate_header_source_content
from autoinput_tools.localization.generate import generate
from autoinput_tools.localization.ids import get_localization_ids, to_loc_id_name, to_loc_key_name
from autoinput_tools.localization.model import LocKeySettings, LocalizationData, LocalizationId
from autoinput_tools.localization.toml import LocalizationFile


def test_to_loc_id_name() -> None:
    # Verify dot and dash separated localization keys are converted to uppercase C++ ID identifier format
    assert to_loc_id_name("ui.button.ok") == "UI_BUTTON_OK_ID"
    assert to_loc_id_name("app-title.main_window") == "APP_TITLE_MAIN_WINDOW_ID"


def test_to_loc_key_name() -> None:
    # Verify localization keys are converted to uppercase C++ KEY identifier format
    assert to_loc_key_name("ui.button.ok") == "UI_BUTTON_OK_KEY"
    assert to_loc_key_name("app-title.main_window") == "APP_TITLE_MAIN_WINDOW_KEY"


def test_get_localization_ids() -> None:
    key_values = [
        ("ui.title", "Main Window"),
        ("ui.welcome", "Hello {}"),
        ("ui.format", "Value: {:02d}"),
    ]
    loc_ids = get_localization_ids(key_values)

    # Verify total number of generated localization ID descriptors
    assert len(loc_ids) == 3

    # Verify plain string key has Empty format settings
    title_id = next(i for i in loc_ids if i.name == "ui.title")
    assert title_id.id_name == "UI_TITLE_ID"
    assert title_id.key_name == "UI_TITLE_KEY"
    assert title_id.settings == LocKeySettings.Empty

    # Verify positional format string has LocKeySettings.Format enabled
    welcome_id = next(i for i in loc_ids if i.name == "ui.welcome")
    assert bool(welcome_id.settings & LocKeySettings.Format) is True

    # Verify complex format specifier string has LocKeySettings.Format enabled
    format_id = next(i for i in loc_ids if i.name == "ui.format")
    assert bool(format_id.settings & LocKeySettings.Format) is True


def test_localization_file_loading(tmp_path: pathlib.Path) -> None:
    loc_path = tmp_path / "en-US.toml"
    loc_path.write_text(
        """
        [ui]
        save = "Save"
        cancel = "Cancel"
        """,
        encoding="utf-8",
    )

    loc_file = LocalizationFile.load_from_filepath(loc_path)
    # Verify localization file loads as valid
    assert loc_file.is_valid()
    keys = loc_file.get_all_keys()
    # Verify all expected keys are parsed from TOML
    assert set(keys) == {"ui.save", "ui.cancel"}
    sorted_pairs = loc_file.get_sorted_key_value_pairs()
    # Verify key-value pairs are returned in alphabetical key order
    assert [p[0] for p in sorted_pairs] == ["ui.cancel", "ui.save"]


def test_localization_file_invalid(tmp_path: pathlib.Path) -> None:
    loc_path = tmp_path / "nonexistent.toml"
    loc_file = LocalizationFile.load_from_filepath(loc_path)
    # Verify missing localization file is marked invalid with empty keys
    assert not loc_file.is_valid()
    assert loc_file.get_all_keys() == set()


def test_generate_header_source_content() -> None:
    key_values = [
        ("ui.cancel", "Cancel"),
        ("ui.save", "Save"),
    ]
    header, source = generate_header_source_content(key_values)

    # Verify C++ header contains guards, namespace, constants, and lookup declarations
    assert "#pragma once" in header
    assert "namespace autoinput::LocalizationIds" in header
    assert "UI_CANCEL_ID" in header
    assert "UI_SAVE_ID" in header
    assert "idToLocKey" in header
    assert "keyToId" in header

    # Verify C++ source includes corresponding header
    assert '#include "localizationIds.h"' in source


def test_generate_end_to_end(tmp_path: pathlib.Path) -> None:
    loc_path = tmp_path / "en-US.toml"
    loc_path.write_text(
        """
        [menu]
        file = "File"
        edit = "Edit"
        """,
        encoding="utf-8",
    )
    header_path = tmp_path / "gen" / "localizationIds.h"
    source_path = tmp_path / "gen" / "localizationIds.cpp"

    # Write mode: verify returns True and outputs header and source files
    ret = generate(loc_file_path=loc_path, header_path=header_path, source_path=source_path, check_only=False)
    assert ret is True
    assert header_path.exists()
    assert source_path.exists()

    # Check mode: verify returns True when files match generated content
    ret_check = generate(loc_file_path=loc_path, header_path=header_path, source_path=source_path, check_only=True)
    assert ret_check is True


def test_generate_duplicate_id_collision(tmp_path: pathlib.Path) -> None:
    loc_path = tmp_path / "collision.toml"
    loc_path.write_text(
        """
        [ui_test]
        item = "One"
        [ui]
        test_item = "Two"
        """,
        encoding="utf-8",
    )
    header_path = tmp_path / "gen" / "localizationIds.h"
    source_path = tmp_path / "gen" / "localizationIds.cpp"

    # Verify generate returns False when C++ identifier collision occurs across keys
    ret = generate(loc_file_path=loc_path, header_path=header_path, source_path=source_path, check_only=False)
    assert ret is False
