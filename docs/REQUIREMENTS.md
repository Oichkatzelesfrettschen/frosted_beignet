# Beignet Build Requirements

**Project:** Frosted Beignet - Modernized Intel GPU OpenCL Implementation
**Last Updated:** 2025-11-19
**Supported Architectures:** Intel Gen5, Gen6, Gen7, Gen7.5, Gen8, Gen9

---

## Overview

This document provides comprehensive build and runtime requirements for building and running Frosted Beignet, an extended and modernized version of Intel's Beignet OpenCL implementation with support for older Intel GPU generations.

---

## System Requirements

### Supported Operating Systems

- **Linux:** Primary development and testing platform
  - Ubuntu 20.04 LTS or later
  - Debian 11 (Bullseye) or later
  - Fedora 34 or later
  - Arch Linux (rolling release)
  - Other distributions with kernel 4.2+ and modern build tools

### Kernel Requirements

- **Minimum Kernel Version:** 4.2
  - Required for proper PPGTT (Per-Process Graphics Translation Table) support on Haswell
  - Older kernels may work for Gen7 (Ivy Bridge) and earlier with limitations

- **Recommended Kernel Version:** 5.4 or later
  - Better DRM (Direct Rendering Manager) support
  - Improved Intel i915 driver stability

### Hardware Requirements

#### Supported Intel GPU Generations

| Generation | Codename | Example Processors | OpenCL Version | Status |
|------------|----------|-------------------|----------------|--------|
| Gen5 | Ironlake | Core i3/i5/i7 1st gen (2010) | 1.1 subset | 🔄 In Development |
| Gen6 | Sandy Bridge | Core i3/i5/i7 2nd gen (2011) | 1.1 | 🔄 In Development |
| Gen7 | Ivy Bridge | Core i3/i5/i7 3rd gen (2012) | 1.2 | ✅ Supported |
| Gen7.5 | Haswell | Core i3/i5/i7 4th gen (2013) | 1.2 | ✅ Supported |
| Gen8 | Broadwell | Core i3/i5/i7 5th gen (2014) | 2.0 | ✅ Supported |
| Gen9 | Skylake/Kabylake | Core i3/i5/i7 6th/7th gen | 2.0 | ✅ Supported |

#### Minimum System RAM

- **2 GB RAM** (minimum for basic operation)
- **4 GB RAM** (recommended for development)
- **8 GB RAM** (recommended for compilation)

---

## Build Dependencies

### Core Build Tools

#### C/C++ Compiler

One of the following compiler toolchains:

**GCC (Recommended):**
```bash
- GCC 11 or later (for C++23 support)
- g++ 11 or later
```

**Clang:**
```bash
- Clang 14 or later (for C++23 support)
- clang++ 14 or later
```

**Intel Compiler (Optional):**
```bash
- ICC 2021 or later
- icpc 2021 or later
```

**Verification:**
```bash
# Check GCC version
gcc --version
g++ --version

# Check Clang version
clang --version
clang++ --version
```

#### CMake

- **Version:** 2.6.0 or later
- **Recommended:** 3.16 or later

```bash
# Install on Ubuntu/Debian
sudo apt-get install cmake

# Install on Fedora
sudo dnf install cmake

# Install on Arch
sudo pacman -S cmake
```

#### Python

- **Version:** 2.7 or 3.x
- **Purpose:** Build scripts and code generation

```bash
# Install on Ubuntu/Debian
sudo apt-get install python3

# Verify installation
python3 --version
```

### LLVM/Clang Framework

**Critical Dependency** - Beignet uses LLVM for OpenCL C compilation

#### LLVM Version

- **Supported:** LLVM 18.0 (currently configured)
- **Also Compatible:** LLVM 14.0, 15.0, 16.0, 17.0
- **Legacy Support:** LLVM 3.6-3.8 (for older Beignet versions)

#### Required LLVM Components

```bash
# Ubuntu/Debian installation
sudo apt-get install llvm-18 llvm-18-dev
sudo apt-get install clang-18 libclang-18-dev
sudo apt-get install libtinfo-dev
sudo apt-get install libedit-dev
sudo apt-get install zlib1g-dev

# Fedora installation
sudo dnf install llvm llvm-devel
sudo dnf install clang clang-devel
sudo dnf install ncurses-devel
sudo dnf install libedit-devel
sudo dnf install zlib-devel

# Arch installation
sudo pacman -S llvm clang
sudo pacman -S ncurses
sudo pacman -S libedit
sudo pacman -S zlib
```

**Verify LLVM installation:**
```bash
llvm-config --version
clang --version
```

### Graphics and DRM Libraries

#### libdrm

Direct Rendering Manager library for GPU access

- **Minimum Version:** 2.4.52
- **Recommended:** 2.4.66 or later (required for OpenCL 2.0)

```bash
# Ubuntu/Debian
sudo apt-get install libdrm-dev

# Fedora
sudo dnf install libdrm-devel

# Arch
sudo pacman -S libdrm
```

**Verify:**
```bash
pkg-config --modversion libdrm
```

#### libdrm_intel

Intel-specific DRM library

- **Minimum Version:** 2.4.52
- **Recommended:** 2.4.66+

**Required Features:**
- `drm_intel_bo_alloc_userptr` (userptr support)
- `drm_intel_get_eu_total` (EU query support)
- `drm_intel_get_subslice_total` (subslice query)
- `drm_intel_bo_set_softpin_offset` (required for OpenCL 2.0)

```bash
# Usually installed with libdrm
sudo apt-get install libdrm-intel1 libdrm-intel-dev  # Ubuntu/Debian
sudo dnf install libdrm-devel  # Fedora (includes intel)
```

**Verify:**
```bash
pkg-config --modversion libdrm_intel
```

### X11 Libraries (Optional, but Recommended)

Required for X11 buffer sharing and most desktop use cases

```bash
# Ubuntu/Debian
sudo apt-get install libx11-dev
sudo apt-get install libxext-dev
sudo apt-get install libxfixes-dev

# Fedora
sudo dnf install libX11-devel
sudo dnf install libXext-devel
sudo dnf install libXfixes-devel

# Arch
sudo pacman -S libx11
sudo pacman -S libxext
sudo pacman -S libxfixes
```

**Note:** If X11 is not needed (e.g., headless servers), run with `-DBUILD_EXAMPLES=OFF`

### OpenCL ICD Loader (Recommended)

For multi-vendor OpenCL support

```bash
# Ubuntu/Debian
sudo apt-get install ocl-icd-dev
sudo apt-get install ocl-icd-libopencl1

# Fedora
sudo dnf install ocl-icd-devel
sudo dnf install ocl-icd

# Arch
sudo pacman -S ocl-icd
```

To disable ICD support (not recommended):
```bash
cmake -DOCLICD_COMPAT=0 ..
```

### OpenGL/EGL (Optional)

Required for `cl_khr_gl_sharing` extension

```bash
# Ubuntu/Debian
sudo apt-get install libgl1-mesa-dev
sudo apt-get install libegl1-mesa-dev

# Fedora
sudo dnf install mesa-libGL-devel
sudo dnf install mesa-libEGL-devel

# Arch
sudo pacman -S mesa
```

**Minimum Versions:**
- OpenGL >= 13.0.0
- EGL >= 13.0.0

---

## Optional Dependencies

### For Building Examples

#### libva (Video Acceleration API)

```bash
# Ubuntu/Debian
sudo apt-get install libva-dev
sudo apt-get install libva-x11-2

# Fedora
sudo dnf install libva-devel

# Arch
sudo pacman -S libva
```

- **Minimum Version:** 0.36.0 (for libva_buffer_sharing example)

### For Testing

#### pkg-config

```bash
# Ubuntu/Debian
sudo apt-get install pkg-config

# Fedora
sudo dnf install pkgconfig

# Arch
sudo pacman -S pkgconf
```

### For Documentation Generation

#### Doxygen (Optional)

```bash
# Ubuntu/Debian
sudo apt-get install doxygen

# Fedora
sudo dnf install doxygen

# Arch
sudo pacman -S doxygen
```

---

## Building from Source

### Quick Start

```bash
# Clone the repository
git clone <repository-url> frosted_beignet
cd frosted_beignet

# Create build directory
mkdir -p build
cd build

# Configure with CMake
cmake ..

# Or with specific compiler:
cmake -DCOMPILER=GCC ..
# cmake -DCOMPILER=CLANG ..
# cmake -DCOMPILER=ICC ..

# Build (adjust -j for your CPU cores)
make -j$(nproc)

# Run tests
make utest
cd ../utests
. setenv.sh
./utest_run

# Install (optional)
sudo make install
```

### Build Options

#### Compiler Selection

```bash
# GCC (default)
cmake -DCOMPILER=GCC ..

# Clang
cmake -DCOMPILER=CLANG ..

# Intel Compiler
cmake -DCOMPILER=ICC ..
```

#### Build Types

```bash
# Debug build (default: -O0 -g)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release with debug info (recommended: -O2 -g)
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..

# Minimal size (-Os)
cmake -DCMAKE_BUILD_TYPE=MinSizeRel ..

# Release (-O2, no debug)
cmake -DCMAKE_BUILD_TYPE=Release ..
```

#### Feature Flags

```bash
# Enable experimental double precision (FP64)
cmake -DEXPERIMENTAL_DOUBLE=ON ..

# Enable OpenCL 2.0 (requires LLVM 3.9+, 64-bit, libdrm 2.4.66+)
cmake -DENABLE_OPENCL_20=ON ..

# Build examples
cmake -DBUILD_EXAMPLES=ON ..

# Disable OCL ICD support
cmake -DOCLICD_COMPAT=0 ..
```

#### Advanced Options

```bash
# Use standalone GBE compiler
cmake -DUSE_STANDALONE_GBE_COMPILER=true ..

# Custom install prefix
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..

# Custom library install directory
cmake -DLIB_INSTALL_DIR=/usr/local/lib64 ..

# Custom ICD vendor file location
cmake -DOCL_ICD_INSTALL_PREFIX=/etc/OpenCL/vendors ..
```

### Build with Warnings as Errors

For development and quality assurance:

```bash
# Modify CMakeLists.txt or use:
cmake -DCMAKE_CXX_FLAGS="-Werror" -DCMAKE_C_FLAGS="-Werror" ..
```

---

## Installation

### System-wide Installation

```bash
cd build
sudo make install
```

**Installed Files:**
```
/usr/local/lib/beignet/
├── libcl.so                  # Main OpenCL library
├── libgbe.so                 # GBE compiler library
├── libgbeinterp.so           # GBE interpreter
├── ocl_stdlib.h              # OpenCL standard library header
├── ocl_stdlib.h.pch          # Precompiled header
└── beignet.bc                # Bitcode library

/etc/OpenCL/vendors/
└── intel-beignet.icd         # ICD loader configuration
```

### User-local Installation

```bash
cmake -DCMAKE_INSTALL_PREFIX=$HOME/.local ..
make install

# Add to OpenCL ICD manually
mkdir -p $HOME/.local/etc/OpenCL/vendors
echo "$HOME/.local/lib/beignet/libcl.so" > $HOME/.local/etc/OpenCL/vendors/beignet.icd
```

---

## Runtime Requirements

### Kernel Modules

Ensure Intel i915 DRM driver is loaded:

```bash
# Check if loaded
lsmod | grep i915

# If not loaded
sudo modprobe i915

# Enable at boot (usually automatic)
```

### Permissions

#### Run as root (not recommended)

```bash
sudo ./your_opencl_app
```

#### Run as normal user (recommended)

**Option 1:** Enable DRM render nodes
```bash
# Add to kernel boot parameters
drm.rnodes=1

# Verify
ls -l /dev/dri/render*
```

**Option 2:** Add user to video group
```bash
sudo usermod -a -G video $USER
# Log out and back in
```

### Environment Variables

#### Required for Unit Tests

```bash
cd utests
. setenv.sh
./utest_run
```

#### Optional Runtime Variables

```bash
# Ignore self-test (not recommended unless __local broken)
export OCL_IGNORE_SELF_TEST=1

# Disable high-precision math (better performance, less accuracy)
export OCL_STRICT_CONFORMANCE=0

# Debugging
export OCL_KERNEL_PATH=/path/to/kernels
export OCL_BITCODE_LIB_PATH=/path/to/beignet.bc
```

---

## Troubleshooting

### Common Build Issues

#### Issue: "LLVM not found"

```bash
# Solution: Install LLVM development files
sudo apt-get install llvm-18-dev clang-18

# Or specify LLVM path
cmake -DLLVM_INSTALL_DIR=/usr/lib/llvm-18 ..
```

#### Issue: "libdrm_intel too old"

```bash
# Solution: Upgrade libdrm
sudo apt-get update
sudo apt-get install libdrm-dev libdrm-intel-dev

# Check version
pkg-config --modversion libdrm_intel
```

#### Issue: C++23 compiler errors

```bash
# Solution: Use newer compiler
sudo apt-get install g++-11  # Ubuntu 20.04
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-11 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-11 100
```

### Common Runtime Issues

#### Issue: "Beignet: self-test failed"

**Cause:** Kernel permission or compatibility issues

**Solutions:**
1. Update kernel to 4.2+
2. Enable render nodes: Add `drm.rnodes=1` to kernel boot params
3. Disable CMD parser: `echo 0 | sudo tee /sys/module/i915/parameters/enable_cmd_parser`
4. (Haswell) Enable PPGTT: Add `i915.enable_ppgtt=2` to kernel boot params

#### Issue: GPU hang / timeout

**Check:**
```bash
dmesg | grep -i "hangcheck"
```

**Solution (risky):**
```bash
echo -n 0 | sudo tee /sys/module/i915/parameters/enable_hangcheck
```

**Warning:** This disables hang detection. GPU lockup requires reboot.

#### Issue: "No OpenCL platforms found"

**Diagnosis:**
```bash
# Check ICD
ls -l /etc/OpenCL/vendors/

# Check library
ldd /usr/local/lib/beignet/libcl.so

# Test directly
LD_LIBRARY_PATH=/usr/local/lib/beignet clinfo
```

**Solution:**
1. Verify installation: `sudo make install`
2. Check ICD file exists
3. Run `ldconfig`: `sudo ldconfig`

---

## Platform-Specific Notes

### Ubuntu 20.04/22.04

```bash
# Full dependency install
sudo apt-get update
sudo apt-get install -y \
    cmake pkg-config python3 \
    llvm-18 llvm-18-dev clang-18 libclang-18-dev \
    libdrm-dev libdrm-intel1 \
    ocl-icd-dev ocl-icd-libopencl1 \
    libx11-dev libxext-dev libxfixes-dev \
    libegl1-mesa-dev \
    libtinfo-dev libedit-dev zlib1g-dev \
    build-essential
```

### Fedora 38+

```bash
# Full dependency install
sudo dnf install -y \
    cmake pkgconfig python3 \
    llvm llvm-devel clang clang-devel \
    libdrm-devel \
    ocl-icd ocl-icd-devel \
    libX11-devel libXext-devel libXfixes-devel \
    mesa-libEGL-devel \
    ncurses-devel libedit-devel zlib-devel \
    gcc gcc-c++
```

### Arch Linux

```bash
# Full dependency install
sudo pacman -S \
    cmake pkgconfig python \
    llvm clang \
    libdrm \
    ocl-icd \
    libx11 libxext libxfixes \
    mesa \
    ncurses libedit zlib \
    base-devel
```

---

## Development Tools (Optional)

### Code Formatting

```bash
sudo apt-get install clang-format
```

### Static Analysis

```bash
sudo apt-get install cppcheck
sudo apt-get install clang-tidy
```

### Debugging

```bash
sudo apt-get install gdb
sudo apt-get install valgrind
```

### Intel GPU Tools

```bash
sudo apt-get install intel-gpu-tools

# Useful commands
intel_gpu_top        # GPU monitoring
intel_error_decode   # Decode GPU errors
```

---

## Verification

### Verify Build

```bash
cd build
./utests/utest_run
```

### Verify Installation

```bash
# List OpenCL platforms
clinfo

# Should show:
#   Platform Name: Intel Gen OCL Driver
#   Platform Vendor: Intel
```

### Verify Device

```bash
# Run simple OpenCL test
cd examples
./your_example_program
```

---

## References

- **Beignet Project:** https://github.com/intel/beignet
- **LLVM Project:** https://llvm.org
- **Intel Graphics:** https://01.org/linuxgraphics
- **OpenCL Specification:** https://www.khronos.org/opencl/
- **DRM/i915 Documentation:** https://www.kernel.org/doc/html/latest/gpu/i915.html

---

**Document Version:** 1.0
**Last Updated:** 2025-11-19
**Maintainer:** Frosted Beignet Development Team
