"""File I/O helpers for AutoInput Python tooling."""

import logging
import pathlib
from typing import Any

logger = logging.getLogger(__name__)


def read_text(path: pathlib.Path | str) -> str:
    """Read text from a UTF-8 encoded file."""
    if isinstance(path, str):
        path = pathlib.Path(path)
    return path.read_text(encoding="utf-8", errors="replace")


def write_text(path: pathlib.Path | str, content: str) -> None:
    """Write text to a file using UTF-8 encoding and LF newlines."""
    if isinstance(path, str):
        path = pathlib.Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8", newline="\n")


def write_text_if_changed(path: pathlib.Path | str, content: str) -> bool:
    """Write text to a file only if the content has changed or the file does not exist.

    Returns:
        True if the file was written, False if it already had identical content.
    """
    if isinstance(path, str):
        path = pathlib.Path(path)

    if path.exists():
        try:
            if path.read_text(encoding="utf-8", errors="replace") == content:
                return False
        except OSError:
            pass

    write_text(path, content)
    return True


def check_or_write_file(
    path: pathlib.Path | str,
    content: str,
    check_only: bool = False,
    filetype: str = "file",
) -> bool:
    """Check if file matches content, or write content if modified.

    In check_only mode:
        Returns True if file exists and matches content.
        Returns False if file does not exist or differs from content.
    In write mode:
        Writes file if missing or content differs (without touching if already identical).
        Returns True on success, False on write failure.
    """
    if isinstance(path, str):
        path = pathlib.Path(path)

    if check_only:
        if not path.exists():
            logger.error(f"Output {filetype} file does not exist: {path}")
            return False
        try:
            existing = path.read_text(encoding="utf-8", errors="replace")
            if existing == content:
                logger.info(f"{filetype.capitalize()} {path} is up to date.")
                return True
            logger.error(f"Output {filetype} {path} is out of date.")
            return False
        except OSError as e:
            logger.error(f"Failed to read existing {filetype} {path}: {e}")
            return False

    try:
        if path.exists():
            try:
                existing = path.read_text(encoding="utf-8", errors="replace")
                if existing == content:
                    logger.info(f"{filetype.capitalize()} {path} is already up to date.")
                    return True
            except OSError:
                pass

        write_text(path, content)
        logger.info(f"Successfully generated {path}.")
        return True
    except OSError as e:
        logger.error(f"Failed to write {filetype} to {path}: {e}")
        return False


def ensure_dir(path: pathlib.Path | str) -> pathlib.Path:
    """Ensure a directory exists."""
    if isinstance(path, str):
        path = pathlib.Path(path)
    path.mkdir(parents=True, exist_ok=True)
    return path


def file_exists(path: pathlib.Path | str) -> bool:
    """Check if a file exists."""
    if isinstance(path, str):
        path = pathlib.Path(path)
    return path.is_file()


def dir_exists(path: pathlib.Path | str) -> bool:
    """Check if a directory exists."""
    if isinstance(path, str):
        path = pathlib.Path(path)
    return path.is_dir()


def get_toml_keys(data: dict[str, Any], prefix: str | None = None) -> set[str]:
    """Get all nested keys from a TOML dictionary."""
    if prefix is None:
        prefix = ""
    keys: set[str] = set()
    for k, v in data.items():
        full_key = f"{prefix}{k}"
        if isinstance(v, dict):
            keys.update(get_toml_keys(v, f"{full_key}."))
        else:
            keys.add(full_key)
    return keys


def get_flatten_toml(data: dict[str, Any] | None, prefix: str | None = None) -> dict[str, Any]:
    """Flatten a nested TOML dictionary."""
    if data is None:
        return {}
    if prefix is None:
        prefix = ""
    results: dict[str, Any] = {}
    for k, v in data.items():
        full_key = f"{prefix}{k}"
        if isinstance(v, dict):
            results.update(get_flatten_toml(v, f"{full_key}."))
        else:
            results[full_key] = v
    return results
