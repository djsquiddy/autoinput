"""Windows Resource Script (.rc) generation."""

import pathlib
from typing import Union


def generate_rc_content(
    ico_path: Union[str, pathlib.Path],
    png_path: Union[str, pathlib.Path] | None = None,
    icon_id: int = 101,
) -> str:
    """Generate .rc file content referencing the given icon and optional PNG resource."""
    ico_posix = pathlib.Path(ico_path).as_posix()
    lines = [
        f"#define IDI_APP_ICON {icon_id}",
        f'IDI_APP_ICON ICON "{ico_posix}"',
    ]
    if png_path is not None:
        png_posix = pathlib.Path(png_path).as_posix()
        lines.append(f'IDI_APP_ICON RCDATA "{png_posix}"')
    lines.append("")
    return "\n".join(lines)
