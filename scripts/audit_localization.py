#!/usr/bin/env python3
import os
import re
import tomllib
import sys

# Paths relative to the script location
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ROOT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
LOC_FILE = os.path.join(ROOT_DIR, "resources", "localization", "en-US.toml")
SRC_DIR = os.path.join(ROOT_DIR, "src", "autoinput_ui")

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

def get_toml_keys(data, prefix=""):
    keys = set()
    for k, v in data.items():
        full_key = f"{prefix}{k}"
        if isinstance(v, dict):
            keys.update(get_toml_keys(v, f"{full_key}."))
        else:
            keys.add(full_key)
    return keys

def audit():
    if not os.path.exists(LOC_FILE):
        print(f"Error: Localization file not found at {LOC_FILE}")
        return False

    try:
        with open(LOC_FILE, "rb") as f:
            loc_data = tomllib.load(f)
    except Exception as e:
        print(f"Error parsing TOML at {LOC_FILE}: {e}")
        return False
    
    valid_keys = get_toml_keys(loc_data)
    missing_keys = {} # key -> list of files
    used_keys = set()

    # Scan source files
    for root, _, files in os.walk(SRC_DIR):
        for file in files:
            if file.endswith((".cpp", ".h")):
                path = os.path.join(root, file)
                rel_path = os.path.relpath(path, ROOT_DIR)
                try:
                    with open(path, "r", encoding="utf-8", errors="ignore") as f:
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
                    print(f"Error reading {path}: {e}")

    unused_keys = sorted([k for k in valid_keys if k not in used_keys])

    print(f"--- Localization Audit for en-US.toml ---")
    print(f"Total keys in TOML: {len(valid_keys)}")
    print(f"Total keys found in code: {len(used_keys)}")
    print()

    if missing_keys:
        print(f"!!! MISSING KEYS (used in code but not in TOML) [{len(missing_keys)}]:")
        for k in sorted(missing_keys.keys()):
            files_str = ", ".join(missing_keys[k])
            print(f"  - {k} (in {files_str})")
        print()
    else:
        print("No missing keys found in code.")

    if unused_keys:
        print(f"??? UNUSED KEYS (in TOML but not in code) [{len(unused_keys)}]:")
        for k in unused_keys:
            print(f"  - {k}")
        print("Note: Some keys might be used dynamically or for future features.")
        print()
    else:
        print("No unused keys found in TOML.")

    return len(missing_keys) == 0

if __name__ == "__main__":
    success = audit()
    if not success:
        print("Audit FAILED: Missing keys detected.")
        sys.exit(1)
    print("Audit PASSED.")
    sys.exit(0)
