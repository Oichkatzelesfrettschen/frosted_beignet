#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.
# set -o pipefail # Handled manually to capture all output

echo "--- Attempting to configure and build the project ---"

BUILD_DIR="build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo ""
echo "1. Running CMake..."
# Capture CMake output to a file and also print to stdout
cmake .. -DOCLICD_COMPAT=0 > >(tee cmake_output.log) 2> >(tee cmake_error.log >&2)
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
echo "2. Running make..."
# Capture make output similarly
make -j$(nproc) > >(tee make_output.log) 2> >(tee make_error.log >&2)
MAKE_EXIT_CODE=${PIPESTATUS[0]}

if [ $MAKE_EXIT_CODE -ne 0 ]; then
    echo "Make build failed with exit code $MAKE_EXIT_CODE."
    echo "Make Output (make_output.log):"
    cat make_output.log
    echo "Make Error (make_error.log):"
    cat make_error.log
    exit $MAKE_EXIT_CODE
else
    echo "Make build successful."
    echo "Make Output (make_output.log):"
    cat make_output.log
    if [ -s make_error.log ]; then
        echo "Make Stderr (make_error.log - may contain warnings):"
        cat make_error.log
    fi
fi

echo ""
echo "--- Build attempt complete ---"
