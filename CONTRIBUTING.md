# Contributing to AutoInput

First off, thank you for considering contributing to AutoInput! It's people like you that make it a great tool.

## How Can I Contribute?

### Reporting Bugs

- Ensure the bug was not already reported by searching on GitHub under [Issues](https://github.com/placeholder/autoinput/issues).
- If you're unable to find an open issue addressing the problem, open a new one. Be sure to include a title and clear description, as much relevant information as possible, and a code sample or an executable test case demonstrating the expected behavior that is not occurring.

### Suggesting Enhancements

- Open a new issue with the "Enhancement" label.
- Provide a clear and detailed description of the suggested enhancement.
- Explain why this enhancement would be useful to most AutoInput users.

### Pull Requests

- Fork the repository and create your branch from `main`.
- If you've added code that should be tested, add tests.
- If you've changed APIs, update the documentation.
- Ensure the test suite passes.
- Make sure your code follows the existing style (see [Formatting](#formatting)).

## Development Setup

### Build and Test

AutoInput uses CMake and Ninja.

```bash
# Configure with tests enabled
cmake --preset tests

# Build
cmake --build --preset tests

# Run tests
ctest --preset tests
```

You can also use the scripts in the `scripts/` directory.

### Formatting

The project uses `clang-format` to maintain a consistent code style. Formatting rules are defined in `.clang-format`.

You can format the codebase using the `format` target:

```bash
cmake --build build --target format
```

### Linting

The project uses `clang-tidy` for static analysis. Rules are defined in `.clang-tidy`.

You can run linting using:

```bash
cmake --build build --target clang-tidy-check
```

## Pull Request Guidance

1. Use a clear and descriptive title for your PR.
2. Link any related issues in the PR description.
3. Keep PRs focused on a single change or a set of highly related changes.
4. Follow the C++23 standard and maintain compatibility with both Windows and Linux.
