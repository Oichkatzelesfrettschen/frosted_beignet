#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.
# set -o pipefail # Handled manually to capture all output

echo "--- Attempting to configure the project (CMake only) after commenting out -fno-rtti ---"

BUILD_DIR="build_cmake_after_rtti_fix" # New build directory
# Clean up previous attempt if it exists
if [ -d "$BUILD_DIR" ]; then
  echo "Removing previous $BUILD_DIR directory."
  rm -rf "$BUILD_DIR"
fi
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "1. Running CMake..."
# Capture CMake output to a file and also print to stdout
cmake .. > >(tee cmake_output.log) 2> >(tee cmake_error.log >&2)
CMAKE_EXIT_CODE=${PIPESTATUS[0]}

if [ $CMAKE_EXIT_CODE -ne 0 ]; then
    echo "CMake configuration failed with exit code $CMAKE_EXIT_CODE."
    echo "CMake Output (cmake_output.log):"
    cat cmake_output.log
    echo "CMake Error (cmake_error.log):"
    cat cmake_error.log
    exit $CMAKE_EXIT_CODE
else
    echo "CMake configuration successful."
    echo "CMake Output (cmake_output.log):"
    cat cmake_output.log
    # cmake_error.log might contain warnings, so show it too
    if [ -s cmake_error.log ]; then
        echo "CMake Stderr (cmake_error.log - may contain warnings):"
        cat cmake_error.log
    fi
fi

echo ""
echo "--- CMake configuration attempt complete ---"
