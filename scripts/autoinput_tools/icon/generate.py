"""Generate app icon (.ico) and Windows resource script (.rc)."""

import logging
import pathlib

from .ico import png_to_ico_bytes
from .rc import generate_rc_content
from ..file_io import check_or_write_file
from ..paths import (
    DEFAULT_APP_ICON_PNG,
    GENERATED_APP_ICON_ICO,
    GENERATED_APP_ICON_RC,
)

logger = logging.getLogger(__name__)


def generate(
    png_path: pathlib.Path = DEFAULT_APP_ICON_PNG,
    ico_path: pathlib.Path = GENERATED_APP_ICON_ICO,
    rc_path: pathlib.Path = GENERATED_APP_ICON_RC,
    check_only: bool = False,
) -> bool:
    """Generate appIcon.ico and appIcon.rc from the given PNG file."""
    if not png_path.exists():
        logger.error(f"App icon PNG file not found: {png_path}")
        return False

    try:
        png_bytes = png_path.read_bytes()
        ico_bytes = png_to_ico_bytes(png_bytes)
    except Exception as e:
        logger.error(f"Failed to generate ICO bytes from {png_path}: {e}")
        return False

    # Check or write ICO file
    if check_only:
        if not ico_path.exists():
            logger.error(f"Output ICO file does not exist: {ico_path}")
            return False
        try:
            if ico_path.read_bytes() != ico_bytes:
                logger.error(f"Output ICO file {ico_path} is out of date.")
                return False
            logger.info(f"ICO {ico_path} is up to date.")
        except OSError as e:
            logger.error(f"Failed to read existing ICO {ico_path}: {e}")
            return False
    else:
        try:
            if ico_path.exists():
                try:
                    if ico_path.read_bytes() == ico_bytes:
                        logger.info(f"ICO {ico_path} is already up to date.")
                    else:
                        ico_path.write_bytes(ico_bytes)
                        logger.info(f"Successfully generated {ico_path}.")
                except OSError:
                    ico_path.write_bytes(ico_bytes)
                    logger.info(f"Successfully generated {ico_path}.")
            else:
                ico_path.parent.mkdir(parents=True, exist_ok=True)
                ico_path.write_bytes(ico_bytes)
                logger.info(f"Successfully generated {ico_path}.")
        except OSError as e:
            logger.error(f"Failed to write ICO to {ico_path}: {e}")
            return False

    rc_content = generate_rc_content(ico_path=ico_path, png_path=png_path)
    if not check_or_write_file(rc_path, rc_content, check_only=check_only, filetype="resource"):
        return False

    return True
