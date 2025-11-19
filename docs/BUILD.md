# Building Beignet (Frosted Beignet Edition)

**Comprehensive build instructions for the modernized Beignet OpenCL stack**

**Last Updated:** 2025-11-19
**Branch:** `claude/intel-gpu-opencl-support-01Y4TL9FKKTA7kmLwr4LuLAt`

## Quick Start

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake llvm-dev clang libdrm-dev libxext-dev libxfixes-dev \
                     pkg-config ocl-icd-libopencl1 ocl-icd-dev ocl-icd-opencl-dev

# Clone repository
git clone https://github.com/Oichkatzelesfrettschen/frosted_beignet.git
cd frosted_beignet

# Build
mkdir build && cd build
cmake ..
make -j$(nproc)

# Install
sudo make install

# Test
cd utests
./utest_run
```

## System Requirements

### Hardware

**Supported Intel GPU Generations:**

| Generation | Codename | Released | OpenCL | Status |
|------------|----------|----------|--------|--------|
| Gen6 | Sandy Bridge | 2011 | 1.1 | ✅ Implemented (Phase 2-3) |
| Gen7 | Ivy Bridge | 2012 | 1.2 | ✅ Supported (Legacy) |
| Gen7.5 | Haswell | 2013 | 1.2 | ✅ Supported (Legacy) |
| Gen8 | Broadwell | 2014 | 2.0 | ✅ Supported (Legacy) |
| Gen9 | Skylake | 2015 | 2.1 | ✅ Supported (Legacy) |

**Note:** This fork focuses on Gen6-7.5 modernization. Gen8+ support inherited from upstream.

### Software

**Required:**
- Linux kernel 4.2+ (for Haswell), 3.13+ (for Ivy Bridge/Sandy Bridge)
- CMake 2.6.0+ (3.10+ recommended)
- LLVM/Clang 16.x, 17.x, or **18.x** (latest validated)
- GCC 9.0+ or Clang 10.0+
- libdrm 2.4.39+
- pkg-config
- OpenCL ICD Loader

**Optional:**
- Python 3.6+ (for test generation)
- Doxygen (for documentation generation)
- Mesa DRI drivers (for testing)

## Detailed Build Instructions

### Step 1: Install Build Dependencies

#### Ubuntu/Debian 22.04+

```bash
# Core build tools
sudo apt-get update
sudo apt-get install build-essential cmake pkg-config

# LLVM/Clang (choose one version)
# LLVM 16 (stable)
sudo apt-get install llvm-16-dev clang-16 libclang-16-dev

# LLVM 17 (recent)
sudo apt-get install llvm-17-dev clang-17 libclang-17-dev

# LLVM 18 (latest - fully validated in Phase 4A)
sudo apt-get install llvm-18-dev clang-18 libclang-18-dev

# Graphics libraries
sudo apt-get install libdrm-dev libxext-dev libxfixes-dev libgl1-mesa-dev

# OpenCL ICD
sudo apt-get install ocl-icd-libopencl1 ocl-icd-dev ocl-icd-opencl-dev

# Optional: Python for test generation
sudo apt-get install python3 python3-numpy
```

#### Fedora/RHEL/CentOS

```bash
# Core build tools
sudo dnf groupinstall "Development Tools"
sudo dnf install cmake pkg-config

# LLVM/Clang
sudo dnf install llvm-devel clang-devel

# Graphics libraries
sudo dnf install libdrm-devel libXext-devel libXfixes-devel mesa-libGL-devel

# OpenCL ICD
sudo dnf install ocl-icd-devel

# Optional
sudo dnf install python3 python3-numpy
```

#### Arch Linux

```bash
# Core build tools
sudo pacman -S base-devel cmake pkg-config

# LLVM/Clang
sudo pacman -S llvm clang

# Graphics libraries
sudo pacman -S libdrm libxext libxfixes mesa

# OpenCL ICD
sudo pacman -S ocl-icd

# Optional
sudo pacman -S python python-numpy
```

### Step 2: Configure Build

#### Basic Configuration

```bash
mkdir build && cd build
cmake ..
```

#### Advanced Configuration Options

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \          # Release, Debug, RelWithDebInfo
  -DCMAKE_INSTALL_PREFIX=/usr/local \   # Installation prefix
  -DLLVM_INSTALL_DIR=/usr/lib/llvm-18 \ # Specific LLVM version
  -DENABLE_OPENCL_20=ON \                # Enable OpenCL 2.0 features
  -DGEN_PCI_ID=0x0162                    # Specific GPU device ID (optional)
```

**Common CMake Variables:**

| Variable | Default | Description |
|----------|---------|-------------|
| `CMAKE_BUILD_TYPE` | Release | Build configuration |
| `CMAKE_INSTALL_PREFIX` | /usr/local | Installation directory |
| `LLVM_INSTALL_DIR` | Auto-detected | LLVM installation path |
| `ENABLE_OPENCL_20` | ON | OpenCL 2.0 feature support |
| `EXPERIMENTAL_DOUBLE` | OFF | FP64 support (Gen7.5+) |

#### Configuration for Specific LLVM Versions

```bash
# Force LLVM 18
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-18

# Force LLVM 17
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-17

# Force LLVM 16
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-16
```

### Step 3: Build

#### Standard Build

```bash
make -j$(nproc)
```

#### Specific Targets

```bash
# Build OpenCL runtime only
make cl -j$(nproc)

# Build OpenCL bitcode library only
make beignet_bitcode -j$(nproc)

# Build unit tests
make utests -j$(nproc)

# Build everything
make all -j$(nproc)
```

#### Build with Verbose Output

```bash
make VERBOSE=1 -j$(nproc)
```

### Step 4: Install

#### System-Wide Installation (requires root)

```bash
sudo make install
```

**Installed Components:**
- `/usr/local/lib/beignet/` - Runtime libraries
- `/usr/local/lib/beignet/include/` - OpenCL built-in headers
- `/usr/local/lib/libcl.so` - OpenCL ICD library
- `/usr/local/lib/beignet/beignet.bc` - OpenCL built-in bitcode
- `/etc/OpenCL/vendors/intel-beignet.icd` - ICD configuration

#### Local Installation (no root)

```bash
cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/.local
make install

# Add to OpenCL ICD search path
mkdir -p ~/.local/etc/OpenCL/vendors
echo "$HOME/.local/lib/beignet/libcl.so" > ~/.local/etc/OpenCL/vendors/beignet.icd
export OCL_ICD_VENDORS=$HOME/.local/etc/OpenCL/vendors
```

### Step 5: Verify Installation

```bash
# List OpenCL platforms
clinfo

# Expected output should include:
# Platform Name: Intel Gen OCL Driver
# Device Name: Intel(R) HD Graphics [Your GPU Model]
# OpenCL Version: OpenCL 1.1/1.2/2.0 (depending on hardware)
```

## Build Configurations

### Development Build (with debugging)

```bash
mkdir build-debug && cd build-debug
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Debugging symbols included, optimizations disabled
# Useful for: debugging, development, tracing issues
```

### Release Build (optimized)

```bash
mkdir build-release && cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Optimizations enabled, debugging symbols stripped
# Useful for: production, performance testing
```

### RelWithDebInfo Build (best of both)

```bash
mkdir build-relwithdebinfo && cd build-relwithdebinfo
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
make -j$(nproc)

# Optimizations enabled, debugging symbols included
# Useful for: performance profiling, production debugging
```

## Troubleshooting Build Issues

### Issue: LLVM Not Found

**Error:**
```
CMake Error: Could not find LLVM installation
```

**Solution:**
```bash
# Manually specify LLVM location
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-18

# Or install LLVM dev packages
sudo apt-get install llvm-dev clang
```

### Issue: OpenCL Headers Not Found

**Error:**
```
fatal error: CL/cl.h: No such file or directory
```

**Solution:**
```bash
# Install OpenCL headers
sudo apt-get install ocl-icd-opencl-dev

# Or manually specify include path
cmake .. -DOPENCL_INCLUDE_DIR=/usr/include
```

### Issue: libdrm Not Found

**Error:**
```
Could NOT find DRM (missing: DRM_INCLUDE_DIR DRM_LIBRARY)
```

**Solution:**
```bash
# Install libdrm development files
sudo apt-get install libdrm-dev

# Or manually specify location
cmake .. -DDRM_INCLUDE_DIR=/usr/include/libdrm -DDRM_LIBRARY=/usr/lib/x86_64-linux-gnu/libdrm.so
```

### Issue: Compilation Errors with LLVM 18

**Error:**
```
error: typedef redefinition with different types
  ('intel_sub_group_avc_ime_payload_t' vs 'struct intel_sub_group_avc_ime_payload_t')
```

**Solution:**
This was fixed in Phase 4A. Ensure you're on the latest branch:
```bash
git checkout claude/intel-gpu-opencl-support-01Y4TL9FKKTA7kmLwr4LuLAt
git pull origin claude/intel-gpu-opencl-support-01Y4TL9FKKTA7kmLwr4LuLAt
```

### Issue: Python Module Not Found

**Error:**
```
ImportError: No module named 'numpy'
```

**Solution:**
```bash
# Install Python dependencies
sudo apt-get install python3-numpy

# Or skip Python-based generation (uses pre-generated files)
cmake .. -DPYTHON_EXECUTABLE=""
```

## Cross-Compilation

### For Different Architectures

```bash
# Example: Cross-compile for ARM64
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../cmake/arm64-toolchain.cmake \
  -DCMAKE_INSTALL_PREFIX=/opt/beignet-arm64

make -j$(nproc)
```

**Note:** Cross-compilation for OpenCL is complex and requires matching target LLVM.

## Advanced Build Options

### Building with AddressSanitizer (ASan)

```bash
cmake .. \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" \
  -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer"

make -j$(nproc)
```

### Building with Clang Instead of GCC

```bash
export CC=clang-18
export CXX=clang++-18

cmake .. -DCOMPILER=CLANG
make -j$(nproc)
```

### Building Static Libraries

```bash
cmake .. -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
```

## Performance Considerations

### Compiler Optimizations

The default build uses:
- `-O2` for Release builds
- `-O0 -g` for Debug builds
- Aggressive loop unrolling (`-funroll-loops`)
- Strict aliasing (`-fstrict-aliasing`)
- SSE 4.1 vectorization (`-msse4.1`)

### CPU-Specific Optimizations

```bash
# For modern CPUs (Haswell+)
cmake .. -DCMAKE_CXX_FLAGS="-march=haswell"

# For native CPU
cmake .. -DCMAKE_CXX_FLAGS="-march=native"
```

**Warning:** CPU-specific builds may not be portable to other systems.

## Build Artifacts

After a successful build, you'll find:

```
build/
├── src/
│   └── libcl.so                 # Main OpenCL runtime library
├── backend/
│   ├── src/
│   │   ├── libgbe.so            # Gen Backend Engine
│   │   └── libocl/
│   │       ├── beignet.bc       # OpenCL built-in library bitcode
│   │       └── beignet.pch      # Precompiled headers
│   └── libgbeinterp.so          # IR interpreter
├── utests/
│   └── utest_run                # Unit test executable
└── kernels/
    ├── builtin_*.cl             # Test kernels (265 files)
    └── compiler_*.cl            # Compiler test kernels
```

## Next Steps

After successfully building:

1. **Run Tests:** See [TESTING.md](TESTING.md) for test execution
2. **Verify GPU:** Use `clinfo` to check OpenCL device detection
3. **Run Benchmarks:** Execute performance benchmarks in `kernels/bench_*.cl`
4. **Develop:** Write and compile your own OpenCL kernels

## Additional Resources

- **Project Documentation:** [docs/README.md](README.md)
- **Implementation Status:** [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md)
- **Architecture Analysis:** [MODERNIZATION_ANALYSIS.md](MODERNIZATION_ANALYSIS.md)
- **Testing Guide:** [TESTING.md](TESTING.md)
- **Phase 4 Completion:** [PROJECT_COMPLETION_SUMMARY.md](PROJECT_COMPLETION_SUMMARY.md)

## Getting Help

If you encounter issues:

1. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common problems
2. Review build logs in `build/CMakeFiles/CMakeOutput.log`
3. Search existing issues on GitHub
4. Open a new issue with build logs and system information

---

**Build Guide Version:** 1.0
**Last Updated:** 2025-11-19
**Maintained By:** Frosted Beignet Development Team
