#!/bin/bash
set -e # Exit immediately if a command exits with a non-zero status.
set -o pipefail # Causes a pipeline to return the exit status of the last command in the pipe that returned a non-zero return value.

echo "--- Verifying LLVM installation and configuration ---"

# Check if llvm-config is accessible and get its version
echo "1. Checking llvm-config accessibility and version..."
if command -v llvm-config &> /dev/null; then
    LLVM_CONFIG_PATH=$(command -v llvm-config)
    echo "llvm-config found at: $LLVM_CONFIG_PATH"
    LLVM_VERSION=$(llvm-config --version)
    echo "LLVM version: $LLVM_VERSION"

    # Check for required LLVM components (Clang libraries)
    echo ""
    echo "2. Checking for required LLVM/Clang components..."
    MISSING_LIBS=()
    REQUIRED_LIBS=(
        "clangCodeGen"
        "clangFrontend"
        "clangSerialization"
        "clangDriver"
        "clangSema"
        "clangStaticAnalyzerFrontend"
        "clangStaticAnalyzerCheckers"
        "clangStaticAnalyzerCore"
        "clangAnalysis"
        "clangEdit"
        "clangAST"
        "clangParse"
        "clangSema"
        "clangLex"
        "clangBasic"
    )

    # Get all available LLVM libraries
    ALL_LIBS=$(llvm-config --libs --link-static 2>/dev/null || llvm-config --libs 2>/dev/null)
    echo "Available LLVM libraries (from llvm-config --libs):"
    echo "$ALL_LIBS"
    echo ""
    echo "Checking specific required Clang libraries:"

    LLVM_CFLAGS=$($LLVM_CONFIG_PATH --cflags 2>/dev/null || echo "")
    HAS_CLANG_HEADERS_VIA_LLVM_CONFIG=false
    if echo "$LLVM_CFLAGS" | grep -qi "clang" &> /dev/null; then # Check if cflags include a path with "clang" (case-insensitive)
        HAS_CLANG_HEADERS_VIA_LLVM_CONFIG=true
        echo "llvm-config --cflags includes Clang header paths. Assuming Clang libraries are available."
        echo "CFLAGS: $LLVM_CFLAGS"
    else
        echo "llvm-config --cflags does not seem to include specific Clang header paths."
        echo "CFLAGS: $LLVM_CFLAGS"
        # Additionally, check if a general Clang library like libclang-cpp is directly reported by llvm-config --libs
        # This is less likely given the typical output of llvm-config --libs for LLVM >= 9
        if echo "$ALL_LIBS" | grep -q -- "-lclang-cpp" || echo "$ALL_LIBS" | grep -q "libclang-cpp" ; then
             HAS_CLANG_HEADERS_VIA_LLVM_CONFIG=true # Re-purpose flag for simplicity, means "found some evidence of Clang"
             echo "Found libclang-cpp or -lclang-cpp in llvm-config --libs. Assuming Clang libraries are available."
        fi
    fi

    for LIB_NAME in "${REQUIRED_LIBS[@]}"; do
        # Check for direct evidence of the library (e.g., -lclangParse or libclangParse.a/so)
        if echo "$ALL_LIBS" | grep -q -- "-l${LIB_NAME}" || \
           echo "$ALL_LIBS" | grep -q "lib${LIB_NAME}\." ; then
            echo "Found (direct match in llvm-config --libs): $LIB_NAME"
        # Check for a generic Clang library entry in ALL_LIBS (e.g., -lclang, libclang.so)
        # This is a common way for newer llvm-config to list Clang components.
        elif echo "$ALL_LIBS" | grep -q -- "-lclang" || \
             echo "$ALL_LIBS" | grep -q "libclang\." ; then
            echo "Found (covered by generic -lclang or libclang.* in llvm-config --libs): $LIB_NAME"
        elif [ "$HAS_CLANG_HEADERS_VIA_LLVM_CONFIG" = true ]; then
            # If llvm-config cflags point to Clang headers, or if a specific clang-cpp lib was in --libs (already checked)
            # assume the specific Clang libraries are also available (might be bundled or found by linker)
            echo "Assumed present (due to Clang headers in llvm-config cflags or specific Clang lib like clang-cpp in --libs): $LIB_NAME"
        else
            echo "Possibly missing (not directly found or covered by generic Clang lib in llvm-config --libs, and no Clang header paths in llvm-config --cflags): $LIB_NAME"
            MISSING_LIBS+=("$LIB_NAME")
        fi
    done

    if [ ${#MISSING_LIBS[@]} -ne 0 ]; then
        echo ""
        echo "Warning: The following Clang libraries might be missing or not reported by llvm-config:"
        for MISSING_LIB in "${MISSING_LIBS[@]}"; do
            echo "  - $MISSING_LIB"
        done
        echo "This might lead to build issues."
    else
        echo "All explicitly checked Clang libraries appear to be available or covered by generic Clang library."
    fi

else
    echo "llvm-config not found. Please ensure LLVM is installed and llvm-config is in your PATH."
    echo "Alternatively, set the LLVM_INSTALL_DIR CMake variable to the directory containing llvm-config."
    exit 1
fi

echo ""
echo "--- LLVM verification complete ---"
