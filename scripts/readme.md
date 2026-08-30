# Project scripts

## Commands

- `commands/build.py` — configure/build project and execute tests/audits
- `commands/format.py` (or `commands/clang_format.py`) — run CMake-backed clang-format workflow
- `commands/clang_tidy.py` (or `commands/tidy.py`) — run CMake-backed clang-tidy static analysis
- `commands/gen_localization_ids.py` — generate localization C++ IDs
- `commands/gen_cli_help.py` — generate CLI help resources
- `commands/update_autocomplete.py` — update shell autocomplete resources

### Build Automation (`commands/build.py`)

The build script supports splitting the workflow into distinct phases:

- `--build-only`: Configure CMake and compile project targets without running tests, localization audits, or autocomplete generation.
- `--test-only`: Run C++ unit tests and Python test suites against an existing build directory without reconfiguring or rebuilding.
- `--audit-only`: Run the localization audit against an existing build directory without cleaning, configuring, building, testing, or updating autocomplete.
- `--audit`: Run the localization audit as part of a full build.
- `--clean`: Clean the build directory before building.

Note: `--build-only`, `--test-only`, and `--audit-only` are mutually exclusive.

#### Example Usage

Full build and test (default):
```bash
python scripts/commands/build.py all Release
python scripts/commands/build.py --clean all Release
python scripts/commands/build.py --audit all Release
```

CI workflow example (split build, test, and audit steps):
```bash
# Step 1: Build only
python scripts/commands/build.py --clean --build-only all Release

# Step 2: Test only
python scripts/commands/build.py --test-only all Release

# Step 3: Audit only
python scripts/commands/build.py --audit-only all Release
```

### Formatting (`commands/format.py`)

The format helper invokes the project's CMake `format` or `format-check` targets defined in `cmake/autoinputFormat.cmake` using the repository-level `.clang-format` configuration.

Options:
- `--check` / `--dry-run`: Verify formatting without modifying source files (runs `format-check`).
- `-p` / `--preset`: Target a specific CMake configure preset (e.g. `release`, `debug`).
- `-B` / `--build-dir`: Target a custom CMake build directory.
- `--no-configure`: Skip automatic CMake configuration when the build directory is missing a cache.

Examples:
```bash
# Format codebase in-place
python scripts/commands/format.py

# Check formatting in CI (returns non-zero if changes needed)
python scripts/commands/format.py --check

# Format using a specific preset
python scripts/commands/format.py --preset release
```

### Static Analysis (`commands/clang_tidy.py`)

The clang-tidy helper invokes the project's CMake clang-tidy setup defined in `cmake/configure_clang_tidy.cmake` using the repository-level `.clang-tidy` configuration.

Options:
- `-p` / `--preset`: Target a specific CMake configure preset.
- `-B` / `--build-dir`: Target a custom CMake build directory.
- `-t` / `--target`: Target to build (default: `clang-tidy-check`).
- `--build`: Configure with `-DAUTOINPUT_ENABLE_CLANG_TIDY=ON` to run clang-tidy during compilation.
- `-c` / `--clean`: Clean build directory before analyzing.
- `--no-configure`: Skip automatic CMake configuration if cache is missing.

Examples:
```bash
# Run clang-tidy static analysis across the codebase
python scripts/commands/clang_tidy.py

# Run clang-tidy with a specific preset
python scripts/commands/clang_tidy.py --preset debug

# Perform a clean build with clang-tidy compiler integration enabled
python scripts/commands/clang_tidy.py --clean --build
```

## Shared package

Reusable Python code lives in `autoinput_tools/`.