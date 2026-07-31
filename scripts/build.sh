#!/bin/bash
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Set BuildType from the first argument, default to Release
BUILD_TYPE="${1:-Release}"

# Navigate to the project root
cd "$SCRIPT_DIR/.."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "creating build directory."
    mkdir build
fi

# Navigate to build directory
cd build

echo "Running CMake..."
cmake -G Ninja -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
echo "Finished running CMake."

echo "Rebuilding..."
ninja
echo "Finished rebuilding."

echo "Running tests..."
./bin/autoinput_tests
echo "Finished running tests."
