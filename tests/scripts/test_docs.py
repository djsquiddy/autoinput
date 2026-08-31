#!/usr/bin/env python3
"""Unit tests for autoinput_tools.docs.generator."""

import pathlib
import sys
from unittest.mock import MagicMock
import pytest

_PROJECT_ROOT = pathlib.Path(__file__).resolve().parent.parent.parent
_SCRIPTS_DIR = _PROJECT_ROOT / "scripts"
if str(_SCRIPTS_DIR) not in sys.path:
    sys.path.insert(0, str(_SCRIPTS_DIR))

from autoinput_tools.docs.generator import (
    extract_class_and_function_docs,
    generate_docs,
    generate_markdown_doc,
    generate_markdown_docs,
    run_doxygen,
)


def test_extract_class_and_function_docs() -> None:
    header = """
/**
 * @brief Represents an input action.
 * Detailed description here.
 */
class InputAction {
public:
    /**
     * Executes the action.
     * @param ctx Execution context.
     */
    void execute(Context& ctx);
};
"""
    entries = extract_class_and_function_docs(header)
    assert len(entries) == 2
    assert "class InputAction" in entries[0]["declaration"]
    assert "@brief Represents an input action." in entries[0]["description"]
    assert "void execute(Context& ctx);" in entries[1]["declaration"]
    assert "Executes the action." in entries[1]["description"]


def test_generate_markdown_doc_with_symbols(tmp_path: pathlib.Path) -> None:
    header_file = tmp_path / "action.h"
    header_file.write_text(
        """
/**
 * Main action class.
 */
class Action;
""",
        encoding="utf-8",
    )
    doc = generate_markdown_doc(header_file)
    assert "### `action.h`" in doc
    assert "Source: `" in doc
    assert "#### `class Action;`" in doc
    assert "Main action class." in doc


def test_generate_markdown_doc_without_symbols(tmp_path: pathlib.Path) -> None:
    header_file = tmp_path / "empty.h"
    header_file.write_text("// No doc comments\nint x = 42;\n", encoding="utf-8")
    doc = generate_markdown_doc(header_file)
    assert "### `empty.h`" in doc
    assert "_No documented symbols found._" in doc


def test_generate_markdown_docs(tmp_path: pathlib.Path) -> None:
    src_dir = tmp_path / "src"
    src_dir.mkdir()
    out_dir = tmp_path / "docs" / "api"

    (src_dir / "foo.h").write_text("/** Foo */\nvoid foo();\n", encoding="utf-8")
    (src_dir / "bar.h").write_text("/** Bar */\nvoid bar();\n", encoding="utf-8")

    success = generate_markdown_docs(src_dir=src_dir, output_dir=out_dir)
    assert success is True
    assert (out_dir / "foo.md").exists()
    assert (out_dir / "bar.md").exists()

    # Test check_only when up to date
    assert generate_markdown_docs(src_dir=src_dir, output_dir=out_dir, check_only=True) is True

    # Modify source and verify check_only detects change
    (src_dir / "foo.h").write_text("/** Foo Updated */\nvoid foo();\n", encoding="utf-8")
    assert generate_markdown_docs(src_dir=src_dir, output_dir=out_dir, check_only=True) is False


def test_run_doxygen(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    config_file = tmp_path / "Doxyfile"
    config_file.write_text("PROJECT_NAME = Test\n", encoding="utf-8")

    # When doxygen not in PATH
    monkeypatch.setattr("shutil.which", lambda name: None)
    assert run_doxygen(config_file) is False

    # When doxygen in PATH but config does not exist
    monkeypatch.setattr("shutil.which", lambda name: "/usr/bin/doxygen")
    missing_config = tmp_path / "Nonexistent"
    assert run_doxygen(missing_config) is False

    # When doxygen runs successfully
    mock_run_ok = MagicMock(returncode=0, stderr="")
    monkeypatch.setattr("subprocess.run", lambda *args, **kwargs: mock_run_ok)
    assert run_doxygen(config_file) is True

    # When doxygen fails
    mock_run_err = MagicMock(returncode=1, stderr="Error parsing config")
    monkeypatch.setattr("subprocess.run", lambda *args, **kwargs: mock_run_err)
    assert run_doxygen(config_file) is False


def test_generate_docs(monkeypatch: pytest.MonkeyPatch, tmp_path: pathlib.Path) -> None:
    src_dir = tmp_path / "src"
    src_dir.mkdir()
    out_dir = tmp_path / "docs" / "api"
    config_file = tmp_path / "Doxyfile"
    config_file.write_text("PROJECT_NAME = Test\n", encoding="utf-8")
    (src_dir / "test.h").write_text("/** Test */\nvoid test();\n", encoding="utf-8")

    # 1. Doxygen available and succeeds
    monkeypatch.setattr("shutil.which", lambda name: "/usr/bin/doxygen")
    monkeypatch.setattr("autoinput_tools.docs.generator.run_doxygen", lambda cfg: True)
    assert generate_docs(src_dir, out_dir, doxygen_config=config_file) is True

    # 2. Doxygen available but fails -> fallback to markdown
    monkeypatch.setattr("autoinput_tools.docs.generator.run_doxygen", lambda cfg: False)
    assert generate_docs(src_dir, out_dir, doxygen_config=config_file) is True
    assert (out_dir / "test.md").exists()

    # 3. Force markdown mode
    monkeypatch.setattr("autoinput_tools.docs.generator.run_doxygen", lambda cfg: True)
    out_dir_forced = tmp_path / "docs" / "forced"
    assert generate_docs(src_dir, out_dir_forced, doxygen_config=config_file, force_markdown=True) is True
    assert (out_dir_forced / "test.md").exists()
