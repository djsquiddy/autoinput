import logging
import pathlib
import re
import shutil
import subprocess
from typing import Dict, List, Optional

logger = logging.getLogger(__name__)


def extract_class_and_function_docs(header_content: str) -> List[Dict[str, str]]:
    """Extract doc comments and following declarations from C++ header content."""
    entries = []
    pattern = re.compile(
        r"/\*\*(?P<doc>[\s\S]*?)\*/\s*(?P<decl>[^;{]+[;{])",
        re.MULTILINE,
    )
    for match in pattern.finditer(header_content):
        doc_raw = match.group("doc")
        doc_lines = [
            re.sub(r"^\s*\* ?", "", line)
            for line in doc_raw.strip().splitlines()
        ]
        doc_clean = "\n".join(doc_lines).strip()
        decl_clean = match.group("decl").strip().rstrip("{").strip()

        entries.append({
            "declaration": decl_clean,
            "description": doc_clean,
        })
    return entries


def generate_markdown_doc(header_file: pathlib.Path) -> str:
    """Generate Markdown documentation for a specific header file."""
    content = header_file.read_text(encoding="utf-8")
    entries = extract_class_and_function_docs(content)

    md_lines = [
        f"### `{header_file.name}`",
        "",
        f"Source: `{header_file.as_posix()}`",
        "",
    ]

    if not entries:
        md_lines.append("_No documented symbols found._\n")
        return "\n".join(md_lines)

    for entry in entries:
        md_lines.extend([
            f"#### `{entry['declaration']}`",
            "",
            entry["description"],
            "",
        ])

    return "\n".join(md_lines)


def generate_markdown_docs(
    src_dir: pathlib.Path,
    output_dir: pathlib.Path,
    check_only: bool = False,
) -> bool:
    """Fallback generator: Extract Markdown documentation files across the codebase."""
    output_dir.mkdir(parents=True, exist_ok=True)
    success = True

    for header in src_dir.rglob("*.h"):
        doc_content = generate_markdown_doc(header)
        out_file = output_dir / f"{header.stem}.md"

        if check_only:
            if not out_file.exists() or out_file.read_text(encoding="utf-8") != doc_content:
                success = False
        else:
            out_file.write_text(doc_content, encoding="utf-8")

    logger.info("Generated Markdown documentation in `%s`", output_dir)
    return success


def run_doxygen(config_file: pathlib.Path) -> bool:
    """Run Doxygen with the specified configuration file."""
    doxygen_bin = shutil.which("doxygen")
    if not doxygen_bin:
        logger.warning("Doxygen executable not found in PATH.")
        return False

    if not config_file.exists():
        logger.warning("Doxygen config file `%s` not found.", config_file)
        return False

    result = subprocess.run(
        [doxygen_bin, str(config_file)],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        logger.error("Doxygen failed with error:\n%s", result.stderr)
        return False

    logger.info("Documentation generated successfully using Doxygen.")
    return True


def generate_docs(
    src_dir: pathlib.Path,
    output_dir: pathlib.Path,
    doxygen_config: Optional[pathlib.Path] = None,
    force_markdown: bool = False,
    check_only: bool = False,
) -> bool:
    """Generate documentation using Doxygen if available, falling back to Markdown extraction."""
    if not force_markdown and doxygen_config and doxygen_config.exists() and shutil.which("doxygen"):
        logger.info("Generating documentation via Doxygen...")
        if run_doxygen(doxygen_config):
            return True
        logger.warning("Doxygen generation failed; falling back to Markdown documentation.")

    logger.info("Generating documentation via Markdown fallback...")
    return generate_markdown_docs(src_dir=src_dir, output_dir=output_dir, check_only=check_only)
