"""ICO file format generation from PNG."""

import pathlib
import struct
from typing import Union


def get_png_dimensions(png_bytes: bytes) -> tuple[int, int]:
    """Extract width and height from PNG bytes."""
    if len(png_bytes) < 24 or png_bytes[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("Invalid PNG signature")
    width, height = struct.unpack(">II", png_bytes[16:24])
    return width, height


def png_to_ico_bytes(png_bytes: bytes) -> bytes:
    """Wrap raw PNG bytes into a standard Windows ICO format."""
    width, height = get_png_dimensions(png_bytes)

    # In ICO ICONDIRENTRY: 0 represents 256 (or >= 256 for Vista+ PNG icons)
    b_width = width if 1 <= width <= 255 else 0
    b_height = height if 1 <= height <= 255 else 0

    # ICONDIR header: 6 bytes
    # idReserved: 0 (uint16)
    # idType: 1 (uint16, 1 = icon)
    # idCount: 1 (uint16, 1 image)
    header = struct.pack("<HHH", 0, 1, 1)

    # ICONDIRENTRY: 16 bytes
    # bWidth (uint8)
    # bHeight (uint8)
    # bColorCount (uint8, 0 for >=8bpp/32bpp)
    # bReserved (uint8, 0)
    # wPlanes (uint16, 1)
    # wBitCount (uint16, 32)
    # dwBytesInRes (uint32, size of png data)
    # dwImageOffset (uint32, offset = 6 + 16 = 22)
    entry = struct.pack("<BBBBHHII", b_width, b_height, 0, 0, 1, 32, len(png_bytes), 22)

    return header + entry + png_bytes


def convert_png_to_ico(png_path: Union[str, pathlib.Path], ico_path: Union[str, pathlib.Path]) -> None:
    """Convert a PNG file to an ICO file."""
    png_p = pathlib.Path(png_path)
    ico_p = pathlib.Path(ico_path)
    png_data = png_p.read_bytes()
    ico_data = png_to_ico_bytes(png_data)
    ico_p.parent.mkdir(parents=True, exist_ok=True)
    ico_p.write_bytes(ico_data)
