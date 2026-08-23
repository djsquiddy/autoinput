#!/usr/bin/env python3
"""Unit tests for autoinput_tools/icon."""

import pathlib
import struct
import pytest

from autoinput_tools.icon.ico import (
    convert_png_to_ico,
    get_png_dimensions,
    png_to_ico_bytes,
)
from autoinput_tools.icon.rc import generate_rc_content
from autoinput_tools.icon.generate import generate


def _make_dummy_png_bytes(width: int = 64, height: int = 64) -> bytes:
    """Create minimal valid PNG bytes with IHDR chunk."""
    header = b"\x89PNG\r\n\x1a\n"
    # IHDR chunk: 4 bytes len (13), 4 bytes type ('IHDR'), 13 bytes data, 4 bytes crc
    ihdr_data = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    ihdr_chunk = struct.pack(">I", 13) + b"IHDR" + ihdr_data + b"\x00\x00\x00\x00"
    # IEND chunk
    iend_chunk = struct.pack(">I", 0) + b"IEND" + b"\x00\x00\x00\x00"
    return header + ihdr_chunk + iend_chunk


def test_get_png_dimensions() -> None:
    png_bytes = _make_dummy_png_bytes(128, 256)
    w, h = get_png_dimensions(png_bytes)
    # Verify PNG width and height parsed from IHDR chunk
    assert w == 128
    assert h == 256


def test_get_png_dimensions_invalid() -> None:
    # Verify ValueError is raised when parsing invalid non-PNG data
    with pytest.raises(ValueError):
        get_png_dimensions(b"not a png")


def test_png_to_ico_bytes() -> None:
    png_bytes = _make_dummy_png_bytes(64, 64)
    ico_bytes = png_to_ico_bytes(png_bytes)
    # Verify total byte length (6-byte ICONDIR + 16-byte ICONDIRENTRY + PNG payload)
    assert len(ico_bytes) == 6 + 16 + len(png_bytes)

    # Check ICONDIR header values
    reserved, image_type, count = struct.unpack("<HHH", ico_bytes[:6])
    assert reserved == 0
    assert image_type == 1  # 1 for icon
    assert count == 1

    # Check ICONDIRENTRY header values
    w, h, color_count, reserved, planes, bpp, bytes_in_res, offset = struct.unpack(
        "<BBBBHHII", ico_bytes[6:22]
    )
    assert w == 64
    assert h == 64
    assert color_count == 0
    assert reserved == 0
    assert planes == 1
    assert bpp == 32
    assert bytes_in_res == len(png_bytes)
    assert offset == 22


def test_png_to_ico_bytes_large_dimensions() -> None:
    # Test 512x512 where 0 represents >= 256 in ICO directory header
    png_bytes = _make_dummy_png_bytes(512, 512)
    ico_bytes = png_to_ico_bytes(png_bytes)
    w, h = ico_bytes[6], ico_bytes[7]
    assert w == 0
    assert h == 0


def test_convert_png_to_ico(tmp_path: pathlib.Path) -> None:
    png_path = tmp_path / "test.png"
    ico_path = tmp_path / "test.ico"
    png_bytes = _make_dummy_png_bytes(32, 32)
    png_path.write_bytes(png_bytes)

    convert_png_to_ico(png_path, ico_path)
    # Verify output .ico file exists on disk
    assert ico_path.exists()
    # Verify written ICO bytes match conversion algorithm output
    assert ico_path.read_bytes() == png_to_ico_bytes(png_bytes)


def test_generate_rc_content() -> None:
    rc_text = generate_rc_content(
        ico_path="build/appIcon.ico",
        png_path="resources/appIcon.png",
        icon_id=101,
    )
    # Verify resource definitions in generated .rc script
    assert "#define IDI_APP_ICON 101" in rc_text
    assert 'IDI_APP_ICON ICON "build/appIcon.ico"' in rc_text
    assert 'IDI_APP_ICON RCDATA "resources/appIcon.png"' in rc_text


def test_generate_rc_content_no_png() -> None:
    rc_text = generate_rc_content(ico_path="build/appIcon.ico")
    # Verify resource definitions when PNG RCDATA is omitted
    assert "#define IDI_APP_ICON 101" in rc_text
    assert 'IDI_APP_ICON ICON "build/appIcon.ico"' in rc_text
    assert "RCDATA" not in rc_text


def test_generate_success_and_check_only(tmp_path: pathlib.Path) -> None:
    png_path = tmp_path / "icon.png"
    ico_path = tmp_path / "icon.ico"
    rc_path = tmp_path / "icon.rc"

    png_path.write_bytes(_make_dummy_png_bytes(48, 48))

    # Generate files: verify returns True and outputs .ico and .rc files
    assert generate(png_path=png_path, ico_path=ico_path, rc_path=rc_path, check_only=False) is True
    assert ico_path.exists()
    assert rc_path.exists()

    # Check mode should pass when files match
    assert generate(png_path=png_path, ico_path=ico_path, rc_path=rc_path, check_only=True) is True

    # Check mode should fail if .rc content is corrupted
    rc_path.write_text("corrupted", encoding="utf-8")
    assert generate(png_path=png_path, ico_path=ico_path, rc_path=rc_path, check_only=True) is False


def test_generate_missing_png(tmp_path: pathlib.Path) -> None:
    png_path = tmp_path / "nonexistent.png"
    ico_path = tmp_path / "icon.ico"
    rc_path = tmp_path / "icon.rc"
    # Verify generator returns False when source PNG file is missing
    assert generate(png_path=png_path, ico_path=ico_path, rc_path=rc_path) is False


def test_windows_pe_resources_when_built() -> None:
    import sys
    if sys.platform != "win32":
        return
    import ctypes
    from ctypes import wintypes
    kernel32 = ctypes.windll.kernel32
    kernel32.FindResourceW.argtypes = [wintypes.HMODULE, wintypes.LPCWSTR, wintypes.LPCWSTR]
    kernel32.FindResourceW.restype = wintypes.HRSRC
    kernel32.LoadLibraryExW.argtypes = [wintypes.LPCWSTR, wintypes.HANDLE, wintypes.DWORD]
    kernel32.LoadLibraryExW.restype = wintypes.HMODULE
    kernel32.FreeLibrary.argtypes = [wintypes.HMODULE]
    kernel32.FreeLibrary.restype = wintypes.BOOL

    def make_int_resource(res_id: int) -> wintypes.LPCWSTR:
        return ctypes.cast(res_id, wintypes.LPCWSTR)

    rt_group_icon = make_int_resource(14)
    rt_rcdata = make_int_resource(10)
    icon_id = make_int_resource(101)
    load_library_as_datafile = 2
    project_root = pathlib.Path(__file__).resolve().parent.parent.parent
    for exe_name in ["autoinput.exe", "autoinput-ui.exe", "autoinput-tray.exe", "autoinput-tests.exe"]:
        exe_path = project_root / "build" / "all" / "bin" / exe_name
        if not exe_path.exists():
            continue
        h = kernel32.LoadLibraryExW(str(exe_path), None, load_library_as_datafile)
        # Verify executable binary was loaded as PE resource module
        assert h is not None and h != 0, f"Could not load {exe_path}"
        try:
            r_group = kernel32.FindResourceW(h, icon_id, rt_group_icon)
            # Verify embedded icon group resource exists in binary
            assert r_group is not None and r_group != 0, f"RT_GROUP_ICON (101) not found in {exe_name}"
            r_rcdata = kernel32.FindResourceW(h, icon_id, rt_rcdata)
            # Verify embedded raw PNG resource data exists in binary
            assert r_rcdata is not None and r_rcdata != 0, f"RT_RCDATA (101) not found in {exe_name}"
        finally:
            kernel32.FreeLibrary(h)
