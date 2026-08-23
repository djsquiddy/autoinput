"""
Process utilities.
"""
import os
import sys
import pathlib
import subprocess
import logging
from .paths import (
    PROJECT_ROOT,
    SCRIPT_DIR
)
from .console import format_log_line

logger = logging.getLogger(__name__)

def run_command(cmd: list[str | os.PathLike | pathlib.Path],
                cwd: pathlib.Path | None = None,
                extra_env: dict[str, str] | None = None,) -> int:
    cmd_str = [str(arg) for arg in cmd]
    env = os.environ.copy()
    env["CLICOLOR_FORCE"] = "1"
    env["FORCE_COLOR"] = "1"
    env["GTEST_COLOR"] = "yes"
    env["CMAKE_COLOR_DIAGNOSTICS"] = "ON"
    python_path = [str(p) for p in sys.path]
    python_path.append(str(PROJECT_ROOT))
    python_path.append(str(SCRIPT_DIR))
    env["PYTHONPATH"] = os.pathsep.join(python_path)
    if extra_env:
        env.update(extra_env)

    try:
        with subprocess.Popen(
                cmd_str,
                cwd=cwd,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                encoding="utf-8",
                errors="replace",
        ) as process:
            if process.stdout:
                for line in iter(process.stdout.readline, ""):
                    if line:
                        logger.info(format_log_line(line.rstrip("\r\n")))
            return process.wait()
    except FileNotFoundError:
        logger.error(f"Error: '{cmd_str[0]}' executable not found in PATH.")
        return 1
    except Exception as e:
        logger.error(f"Failed to execute command {' '.join(cmd_str)}: {e}")
        return 1
