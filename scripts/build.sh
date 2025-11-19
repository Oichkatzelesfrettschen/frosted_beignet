#!/bin/bash
# Frosted Beignet Build Script
# Automated build with comprehensive error checking and logging

set -e  # Exit on error

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="${PROJECT_ROOT}/build"
LOG_DIR="${PROJECT_ROOT}/logs"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_FILE="${LOG_DIR}/build_${TIMESTAMP}.log"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Logging function
log() {
    echo -e "${GREEN}[$(date +'%Y-%m-%d %H:%M:%S')]${NC} $1" | tee -a "$LOG_FILE"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" | tee -a "$LOG_FILE"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1" | tee -a "$LOG_FILE"
}

# Create log directory
mkdir -p "$LOG_DIR"

log "===== Frosted Beignet Build Script ====="
log "Project root: $PROJECT_ROOT"
log "Build directory: $BUILD_DIR"
log "Log file: $LOG_FILE"

# Check dependencies
log "Checking build dependencies..."

check_command() {
    if ! command -v "$1" &> /dev/null; then
        error "$1 not found. Please install it."
        return 1
    else
        log "  ✓ $1 found: $(command -v "$1")"
        return 0
    fi
}

DEPS_OK=true
check_command cmake || DEPS_OK=false
check_command make || DEPS_OK=false
check_command pkg-config || DEPS_OK=false
check_command llvm-config || DEPS_OK=false

if [ "$DEPS_OK" = false ]; then
    error "Missing required dependencies. Please install them and try again."
    exit 1
fi

# Check LLVM version
LLVM_VERSION=$(llvm-config --version 2>/dev/null || echo "unknown")
log "LLVM version: $LLVM_VERSION"

# Check compiler
if [ -z "$CXX" ]; then
    if command -v g++ &> /dev/null; then
        export CXX=g++
        export CC=gcc
        log "Using GCC compiler"
    elif command -v clang++ &> /dev/null; then
        export CXX=clang++
        export CC=clang
        log "Using Clang compiler"
    else
        error "No suitable C++ compiler found"
        exit 1
    fi
fi

COMPILER_VERSION=$($CXX --version | head -n1)
log "Compiler: $COMPILER_VERSION"

# Clean old build if requested
if [ "$1" = "clean" ]; then
    log "Cleaning old build directory..."
    rm -rf "$BUILD_DIR"
    log "Clean complete"
    exit 0
fi

# Create build directory
log "Creating build directory..."
mkdir -p "$BUILD_DIR"

# Configure with CMake
log "Running CMake configuration..."
cd "$BUILD_DIR"

# Determine compiler type for CMake
COMPILER_TYPE="GCC"
if [[ "$CXX" == *"clang"* ]]; then
    COMPILER_TYPE="CLANG"
fi

CMAKE_ARGS=(
    -DCOMPILER="$COMPILER_TYPE"
    -DCMAKE_BUILD_TYPE=RelWithDebInfo
    -DBUILD_EXAMPLES=OFF
)

# Add extra arguments if provided
if [ -n "$2" ]; then
    CMAKE_ARGS+=("$2")
fi

log "CMake arguments: ${CMAKE_ARGS[*]}"

if ! cmake "${CMAKE_ARGS[@]}" .. 2>&1 | tee -a "$LOG_FILE"; then
    error "CMake configuration failed"
    exit 1
fi

# Build
log "Starting build..."
NPROC=$(nproc 2>/dev/null || echo 4)
log "Using $NPROC parallel jobs"

if ! make -j"$NPROC" 2>&1 | tee -a "$LOG_FILE"; then
    error "Build failed"
    exit 1
fi

log "Build successful!"

# Build tests
if [ "$1" = "test" ]; then
    log "Building unit tests..."
    if ! make utest 2>&1 | tee -a "$LOG_FILE"; then
        error "Test build failed"
        exit 1
    fi
    log "Tests built successfully"
fi

# Summary
log "===== Build Summary ====="
log "Build completed successfully at $(date)"
log "Build artifacts location: $BUILD_DIR"
log "Full log saved to: $LOG_FILE"

exit 0
