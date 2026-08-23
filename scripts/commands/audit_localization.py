#!/usr/bin/env python3
"""Audit localization keys used in C++ source files against en-US.toml."""

import io
import logging
import os
import pathlib
import re
import sys

_SCRIPTS_DIR = pathlib.Path(__file__).resolve().parent.parent
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.localization.toml import LocalizationFile
from autoinput_tools.paths import PROJECT_ROOT, UI_SRC_DIR

logger = logging.getLogger(__name__)

AUDIT_LOC = "en-US"

# Regex to find localization keys in C++ source
KEY_PATTERNS = [
    # loc.text("key"), Localization::get().text("key"), etc.
    # We look for calls that are likely localization related.
    re.compile(r'(?:\.|\->|::)(?:text|format|textOr|has)\(\s*"([^"]+)"'),
    # UiWindow("Title", "key")
    re.compile(r'UiWindow\(\s*"[^"]*"\s*,\s*"([^"]+)"\s*\)'),
    # .labelKey = "key", .categoryKey = "key"
    re.compile(r'\.labelKey\s*=\s*"([^"]+)"'),
    re.compile(r'\.categoryKey\s*=\s*"([^"]+)"'),
    # m_titleKey = "key"
    re.compile(r'm_titleKey\s*=\s*"([^"]+)"'),
]


def audit() -> bool:
    loc_data = LocalizationFile.load(AUDIT_LOC)
    if not loc_data.is_valid():
        return False
    valid_keys = loc_data.get_all_keys()
    missing_keys: dict[str, list[str]] = {}  # key -> list of files
    used_keys: set[str] = set()

    # Scan source files
    for root, _, files in os.walk(UI_SRC_DIR):
        for file in files:
            if file.endswith((".cpp", ".h")):
                path = os.path.join(root, file)
                rel_path = os.path.relpath(path, PROJECT_ROOT)
                try:
                    with io.open(path, "r", encoding="utf-8", errors="ignore") as f:
                        content = f.read()
                        for pattern in KEY_PATTERNS:
                            matches = pattern.findall(content)
                            for match in matches:
                                # Keys shouldn't contain spaces and shouldn't contain format placeholders like {}
                                # unless they are actual keys (but we don't have keys with {} in the project)
                                if " " not in match and "{}" not in match:
                                    used_keys.add(match)
                                    if match not in valid_keys:
                                        if match not in missing_keys:
                                            missing_keys[match] = []
                                        if rel_path not in missing_keys[match]:
                                            missing_keys[match].append(rel_path)
                except Exception as e:
                    logger.error(f"Error reading {path}: {e}")

    unused_keys = sorted([k for k in valid_keys if k not in used_keys])

    logger.info(f"--- Localization Audit for en-US.toml ---")
    logger.info(f"Total keys in TOML: {len(valid_keys)}")
    logger.info(f"Total keys found in code: {len(used_keys)}")
    logger.info("")

    if missing_keys:
        logger.error(f"!!! MISSING KEYS (used in code but not in TOML) [{len(missing_keys)}]:")
        for k in sorted(missing_keys.keys()):
            files_str = ", ".join(missing_keys[k])
            logger.error(f"  - {k} (in {files_str})")
        logger.error("")
    else:
        logger.info("No missing keys found in code.")

    if unused_keys:
        logger.warning(f"??? UNUSED KEYS (in TOML but not in code) [{len(unused_keys)}]:")
        for k in unused_keys:
            logger.warning(f"  - {k}")
        logger.warning("Note: Some keys might be used dynamically or for future features.")
        logger.warning("")
    else:
        logger.info("No unused keys found in TOML.")

    return len(missing_keys) == 0


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO, format="%(message)s")
    success = audit()
    if not success:
        logger.error("Audit FAILED: Missing keys detected.")
        sys.exit(1)
    logger.info("Audit PASSED.")
    sys.exit(0)
