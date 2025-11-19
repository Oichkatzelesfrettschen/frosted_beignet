# Phase 5A Stage 2 - Build Environment Requirements

**Date:** 2025-11-19
**Branch:** `claude/llvm-18-phase-5a-01FB6a5Wy3Lh1722MZcU9hjr`
**Status:** Documentation for environment setup

---

## Overview

Stage 2 of Phase 5A requires a properly configured LLVM-18 development environment to build and test the RegisterMap and IntervalStore optimizations.

---

## Required Packages

### Ubuntu/Debian

```bash
# Update package lists
sudo apt-get update

# Install LLVM-18 development tools and libraries
sudo apt-get install -y \
    llvm-18 \
    llvm-18-dev \
    clang-18 \
    libclang-18-dev \
    llvm-18-runtime \
    llvm-18-tools

# Install build dependencies
sudo apt-get install -y \
    cmake \
    build-essential \
    pkg-config \
    libdrm-dev \
    libx11-dev \
    libxext-dev \
    libxfixes-dev \
    zlib1g-dev
```

### Verification

```bash
# Verify LLVM-18 installation
llvm-config-18 --version
# Expected: 18.1.3 or higher

# Check for development libraries
ls /usr/lib/llvm-18/lib/libLLVM*.a | head -5
# Expected: Multiple .a static library files

# Check for headers
ls /usr/lib/llvm-18/include/llvm/ | head -5
# Expected: Directory listing of LLVM headers
```

---

## Current Environment Issue

### Symptom

```bash
llvm-config: error: missing: /usr/lib/llvm-18/lib/libLLVMDemangle.a
llvm-config: error: missing: /usr/lib/llvm-18/lib/libLLVMSupport.a
...
```

### Root Cause

The `llvm-18-dev` package is not installed. This package provides:
- LLVM static libraries (.a files)
- LLVM header files
- Development utilities

### Solution

Install the development package:

```bash
sudo apt-get install llvm-18-dev clang-18 libclang-18-dev
```

---

## Build Process (After Environment Setup)

### Step 1: Configure Build

```bash
cd /home/user/frosted_beignet
mkdir -p build && cd build

# Configure with CMake
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DLLVM_CONFIG_EXECUTABLE=/usr/bin/llvm-config-18 \
    -DENABLE_PHASE5A=ON
```

**Expected output:**
```
-- LLVM llvm-config found at: /usr/bin/llvm-config-18
-- LLVM version: 18.1.3
-- LLVM libraries: <list of libraries>
-- Configuring done
-- Generating done
```

### Step 2: Build

```bash
# Clean build (recommended for Phase 5A testing)
make clean

# Build with all cores
make -j$(nproc)
```

**Expected duration:** 5-10 minutes

**Expected output:**
```
[  1%] Building CXX object backend/src/CMakeFiles/gbe.dir/backend/gen_reg_allocation.cpp.o
[  2%] Building CXX object backend/src/CMakeFiles/gbe.dir/backend/...
...
[100%] Built target beignet
```

### Step 3: Verify Build

```bash
# Check for built libraries
ls -lh /home/user/frosted_beignet/build/backend/src/libgbe.so
# Expected: Shared library ~2-3 MB

# Check for OpenCL bitcode
ls -lh /home/user/frosted_beignet/build/backend/src/libocl/beignet.bc
# Expected: Bitcode file ~800 KB
```

---

## Phase 5A Build Flags

### CMake Options

```cmake
# Enable Phase 5A optimizations (default: ON)
-DENABLE_PHASE5A=ON

# Disable Phase 5A (for comparison testing)
-DENABLE_PHASE5A=OFF
```

### C++ Preprocessor Flags

Phase 5A code uses conditional compilation:

```cpp
#if USE_PHASE5A_OPTIMIZATIONS
    // New O(1) code
#else
    // Old O(log n) code
#endif
```

**Toggle at build time:**
```cpp
// In backend/src/backend/gen_reg_allocation.cpp
#ifndef USE_PHASE5A_OPTIMIZATIONS
#define USE_PHASE5A_OPTIMIZATIONS 1  // Change to 0 to disable
#endif
```

---

## Testing Phase 5A

### Step 1: Run Test Suite

```bash
cd /home/user/frosted_beignet/build/utests

# Run all tests
./utest_run
```

**Expected:**
- **615 tests** should run
- **All tests PASS**
- **Zero failures** or validation errors

### Step 2: Check Phase 5A Output

```bash
# Run tests and capture Phase 5A messages
./utest_run 2>&1 | tee phase5a_test_log.txt

# Look for Phase 5A validation messages
grep "Phase 5A" phase5a_test_log.txt
```

**Expected output:**
```
[Phase 5A] Optimizations enabled (validation mode: ON)
[Phase 5A] Final stats - RegisterMap size: 1234 entries, memory: 8 KB
```

**Should NOT see:**
```
[Phase 5A] MISMATCH: ...
GBE_ASSERT failed: ...
```

### Step 3: Performance Validation

```bash
# Disable Phase 5A
cd /home/user/frosted_beignet/build
cmake .. -DENABLE_PHASE5A=OFF
make clean && make -j$(nproc)

# Run performance test
time ./utests/utest_run builtin_kernel_max_global_size

# Re-enable Phase 5A
cmake .. -DENABLE_PHASE5A=ON
make clean && make -j$(nproc)

# Run same test
time ./utests/utest_run builtin_kernel_max_global_size

# Compare times
```

**Expected:** Phase 5A should be **3-10% faster**

---

## Troubleshooting

### Issue: "missing libLLVM*.a"

**Cause:** llvm-18-dev not installed

**Solution:**
```bash
sudo apt-get install llvm-18-dev
```

### Issue: "Cannot find llvm-config-18"

**Cause:** LLVM-18 not in PATH

**Solution:**
```bash
export PATH=/usr/lib/llvm-18/bin:$PATH
hash -r  # Refresh PATH cache
```

### Issue: Build fails with "undefined reference to LLVM::..."

**Cause:** LLVM library linking issue

**Solution:**
```bash
# Reconfigure with explicit LLVM config
cd build
rm CMakeCache.txt
cmake .. -DLLVM_CONFIG_EXECUTABLE=/usr/bin/llvm-config-18
```

### Issue: Tests fail with GBE_ASSERT

**Cause:** Phase 5A validation detected mismatch

**Solution:**
```bash
# Disable Phase 5A temporarily
cmake .. -DENABLE_PHASE5A=OFF
make clean && make -j$(nproc)
./utests/utest_run

# If tests pass without Phase 5A, there's a bug in Phase 5A code
# Report issue with:
./utests/utest_run 2>&1 | tee phase5a_bug_report.txt
```

---

## Environment Status Checklist

Before proceeding with Stage 2 testing:

- [ ] LLVM-18 installed (`llvm-18 --version` works)
- [ ] LLVM-18-dev installed (`ls /usr/lib/llvm-18/lib/*.a` shows files)
- [ ] Clang-18 installed (`clang-18 --version` works)
- [ ] Build dependencies installed (cmake, build-essential, etc.)
- [ ] Build directory clean (`rm -rf build && mkdir build`)
- [ ] CMake configuration successful (no errors)
- [ ] Build completes successfully (no errors)
- [ ] Test suite runs (./utest_run works)

---

## Next Steps

Once environment is configured:

1. ✅ **Build with Phase 5A** (Step 1-3 above)
2. ✅ **Run RegisterMap tests** (validate existing integration)
3. ✅ **Complete IntervalStore integration** (code changes prepared)
4. ✅ **Run full validation** (all 615 tests)
5. ✅ **Measure performance** (before/after comparison)
6. ✅ **Document results** (PHASE5A_RESULTS.md)

---

## Alternative: Docker Environment

If system package installation is restricted, use Docker:

```dockerfile
# Dockerfile for Phase 5A testing
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    llvm-18 llvm-18-dev clang-18 libclang-18-dev \
    cmake build-essential pkg-config \
    libdrm-dev libx11-dev libxext-dev libxfixes-dev zlib1g-dev \
    git

WORKDIR /workspace
COPY . /workspace

RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DENABLE_PHASE5A=ON && \
    make -j$(nproc)

CMD ["./build/utests/utest_run"]
```

**Usage:**
```bash
docker build -t frosted-beignet-phase5a .
docker run --rm frosted-beignet-phase5a
```

---

**Document Version:** 1.0
**Created:** 2025-11-19
**Purpose:** Environment setup for Phase 5A Stage 2 testing
**Status:** Ready for use
