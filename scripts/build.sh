#!/usr/bin/env bash
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Set BuildType from the first argument, default to Release
BUILD_TYPE="${1:-Release}"
BUILD_TESTS="OFF"
BUILD_TRAY="OFF"
BUILD_UI="OFF"

# Navigate to the project root
pushd "$SCRIPT_DIR/.." > /dev/null

# Create build directory if it doesn't exist
if ! [[ -d "build" ]]; then
    echo "creating build directory."
    mkdir build
fi

# Navigate to build directory
pushd build > /dev/null

echo "Running CMake..."
cmake -G Ninja \
    -DCMAKE_BUILD_TYPE="${BuildType}" \
    -DAUTOINPUT_BUILD_TESTS="${BUILD_TESTS}" \
    -DAUTOINPUT_BUILD_TRAY="${BUILD_TRAY}" \
    -DAUTOINPUT_BUILD_UI="${BUILD_UI}" \
    ..
echo "Finished running CMake."

echo "Rebuilding..."
ninja
echo "Finished rebuilding."

echo "Running tests..."
./bin/autoinput-tests
echo "Finished running tests."
popd > /dev/null
popd > /dev/null
