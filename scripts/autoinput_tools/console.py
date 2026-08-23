
import re

def format_log_line(line: str) -> str:
    """Format and colorize log lines if needed (e.g., GoogleTest markers on Windows pipes)."""
    # Don't re-colorize if already prefixed with ANSI escape sequences
    if line.startswith("\x1b["):
        return line

    if line.startswith(("[==========]", "[----------]", "[ RUN      ]", "[       OK ]", "[  PASSED  ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[32m{tag}\033[0m{rest}"
    if line.startswith(("[  FAILED  ]", "[  TIMEOUT ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[31m{tag}\033[0m{rest}"
    if line.startswith(("[  SKIPPED ]", "[ DISABLED ]")):
        tag = line[:12]
        rest = line[12:]
        return f"\033[33m{tag}\033[0m{rest}"
    if line.startswith("Note: Google Test") or line.startswith("Note: This is test shard"):
        return f"\033[33m{line}\033[0m"
    if " YOU HAVE " in line and " DISABLED " in line:
        return f"\033[33m{line}\033[0m"
    if re.search(r"^\s*\d+ FAILED TESTS?", line):
        return f"\033[31m{line}\033[0m"

    return line
