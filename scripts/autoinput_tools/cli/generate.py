"""Generate C++ CLI help metadata files."""

import logging
import pathlib

from .cpp import generate_cli_help_content
from .toml import load_cli_help_metadata
from .validation import CliHelpValidationError
from ..file_io import check_or_write_file
from ..paths import (
    DEFAULT_CLI_HELP_METADATA_FILE,
    GENERATED_CLI_HELP_HEADER,
    GENERATED_CLI_HELP_SOURCE,
)

logger = logging.getLogger(__name__)


def generate(
    metadata_path: pathlib.Path = DEFAULT_CLI_HELP_METADATA_FILE,
    header_path: pathlib.Path = GENERATED_CLI_HELP_HEADER,
    source_path: pathlib.Path = GENERATED_CLI_HELP_SOURCE,
    check_only: bool = False,
    eol: str = "\n",
) -> bool:
    """Generate C++ CLI help metadata files."""
    try:
        metadata = load_cli_help_metadata(metadata_path)
    except FileNotFoundError:
        logger.error(f"CLI help metadata file not found: {metadata_path}")
        return False
    except CliHelpValidationError as e:
        logger.error(f"Failed to validate CLI help metadata from {metadata_path}: {e}")
        return False
    except Exception as e:
        logger.error(f"Failed to load CLI help metadata from {metadata_path}: {e}")
        return False

    header_content, source_content = generate_cli_help_content(metadata, eol=eol)

    if not check_or_write_file(header_path, header_content, check_only=check_only, filetype="header"):
        return False
    if not check_or_write_file(source_path, source_content, check_only=check_only, filetype="source"):
        return False

    return True
