# Project scripts

## Commands

- `commands/build.py` — configure/build project and execute tests/audits
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

## Shared package

Reusable Python code lives in `autoinput_tools/`.