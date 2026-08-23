#!/usr/bin/env python3
"""Unit tests for commands/gen_app_icon.py."""

import pathlib
import struct
import sys
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.paths import (
    DEFAULT_APP_ICON_PNG,
    GENERATED_APP_ICON_ICO,
    GENERATED_APP_ICON_RC,
)
from commands.gen_app_icon import get_parser, main


def _make_dummy_png_bytes(width: int = 32, height: int = 32) -> bytes:
    header = b"\x89PNG\r\n\x1a\n"
    ihdr_data = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    ihdr_chunk = struct.pack(">I", 13) + b"IHDR" + ihdr_data + b"\x00\x00\x00\x00"
    iend_chunk = struct.pack(">I", 0) + b"IEND" + b"\x00\x00\x00\x00"
    return header + ihdr_chunk + iend_chunk


def test_parser_defaults() -> None:
    parser = get_parser()
    args = parser.parse_args([])
    # Verify default argument paths and check flag
    assert args.png == DEFAULT_APP_ICON_PNG
    assert args.ico == GENERATED_APP_ICON_ICO
    assert args.rc == GENERATED_APP_ICON_RC
    assert args.check is False


def test_parser_custom_args() -> None:
    parser = get_parser()
    args = parser.parse_args(["--png", "custom.png", "--ico", "out.ico", "--rc", "out.rc", "--check"])
    # Verify custom parsed argument values
    assert args.png == pathlib.Path("custom.png")
    assert args.ico == pathlib.Path("out.ico")
    assert args.rc == pathlib.Path("out.rc")
    assert args.check is True


def test_gen_app_icon_main_success(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    png_file = tmp_path / "appIcon.png"
    png_file.write_bytes(_make_dummy_png_bytes(64, 64))
    ico_file = tmp_path / "appIcon.ico"
    rc_file = tmp_path / "appIcon.rc"

    test_args = [
        "gen_app_icon.py",
        "--png", str(png_file),
        "--ico", str(ico_file),
        "--rc", str(rc_file),
    ]
    monkeypatch.setattr(sys, "argv", test_args)
    ret = main()
    # Verify CLI execution returns 0 and produces both .ico and .rc files
    assert ret == 0
    assert ico_file.exists()
    assert rc_file.exists()


def test_gen_app_icon_main_check_mode(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    png_file = tmp_path / "appIcon.png"
    png_file.write_bytes(_make_dummy_png_bytes(64, 64))
    ico_file = tmp_path / "appIcon.ico"
    rc_file = tmp_path / "appIcon.rc"

    # First generate: verify returns 0
    monkeypatch.setattr(
        sys,
        "argv",
        ["gen_app_icon.py", "--png", str(png_file), "--ico", str(ico_file), "--rc", str(rc_file)],
    )
    assert main() == 0

    # Then check: verify check mode succeeds with return code 0
    monkeypatch.setattr(
        sys,
        "argv",
        ["gen_app_icon.py", "--png", str(png_file), "--ico", str(ico_file), "--rc", str(rc_file), "--check"],
    )
    assert main() == 0
