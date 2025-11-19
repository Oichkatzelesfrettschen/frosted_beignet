# Troubleshooting Guide - Frosted Beignet

**Common issues and solutions for the modernized Beignet OpenCL stack**

**Last Updated:** 2025-11-19
**Version:** 1.0

---

## Table of Contents

1. [Build Issues](#build-issues)
2. [Runtime Issues](#runtime-issues)
3. [Kernel Compilation Issues](#kernel-compilation-issues)
4. [Performance Issues](#performance-issues)
5. [Hardware Detection Issues](#hardware-detection-issues)
6. [LLVM Version Issues](#llvm-version-issues)
7. [Generation-Specific Issues](#generation-specific-issues)
8. [Debugging Techniques](#debugging-techniques)

---

## Build Issues

### Issue: CMake Cannot Find LLVM

**Symptoms:**
```
CMake Error: Could not find LLVM installation
LLVM_INSTALL_DIR not found
```

**Solutions:**

1. **Install LLVM development packages:**
```bash
# Ubuntu/Debian
sudo apt-get install llvm-18-dev clang-18 libclang-18-dev

# Fedora/RHEL
sudo dnf install llvm-devel clang-devel

# Arch Linux
sudo pacman -S llvm clang
```

2. **Manually specify LLVM location:**
```bash
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-18
```

3. **Check LLVM installation:**
```bash
llvm-config --version  # Should output LLVM version
which clang            # Should show clang path
```

### Issue: libdrm Not Found

**Symptoms:**
```
Could NOT find DRM (missing: DRM_INCLUDE_DIR DRM_LIBRARY)
```

**Solutions:**

1. **Install libdrm development files:**
```bash
# Ubuntu/Debian
sudo apt-get install libdrm-dev

# Fedora/RHEL
sudo dnf install libdrm-devel

# Arch Linux
sudo pacman -S libdrm
```

2. **Manually specify libdrm location:**
```bash
cmake .. \
  -DDRM_INCLUDE_DIR=/usr/include/libdrm \
  -DDRM_LIBRARY=/usr/lib/x86_64-linux-gnu/libdrm.so
```

### Issue: OpenCL Headers Not Found

**Symptoms:**
```
fatal error: CL/cl.h: No such file or directory
```

**Solutions:**

1. **Install OpenCL ICD development files:**
```bash
# Ubuntu/Debian
sudo apt-get install ocl-icd-opencl-dev

# Fedora/RHEL
sudo dnf install ocl-icd-devel

# Arch Linux
sudo pacman -S ocl-icd
```

2. **Verify OpenCL headers exist:**
```bash
ls /usr/include/CL/cl.h  # Should exist
```

### Issue: Compilation Errors with LLVM 18

**Symptoms:**
```
error: typedef redefinition with different types
  ('intel_sub_group_avc_ime_payload_t' vs 'struct intel_sub_group_avc_ime_payload_t')
```

**Solutions:**

1. **Ensure you're on the correct branch with Phase 4A fixes:**
```bash
git checkout claude/intel-gpu-opencl-support-01Y4TL9FKKTA7kmLwr4LuLAt
git pull origin claude/intel-gpu-opencl-support-01Y4TL9FKKTA7kmLwr4LuLAt
```

2. **Verify Phase 4A commit is present:**
```bash
git log --oneline | grep "Phase 4A"
# Should show: dca9264 feat: LLVM 18 OpenCL Built-in Library Compatibility - Phase 4A
```

3. **Clean and rebuild:**
```bash
rm -rf build
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Issue: Python Module Not Found During Build

**Symptoms:**
```
ImportError: No module named 'numpy'
ModuleNotFoundError: No module named 'mako'
```

**Solutions:**

1. **Install Python dependencies:**
```bash
# Ubuntu/Debian
sudo apt-get install python3-numpy python3-mako

# Fedora/RHEL
sudo dnf install python3-numpy python3-mako

# Arch Linux
sudo pacman -S python-numpy python-mako
```

2. **Skip Python-based generation (use pre-generated files):**
```bash
cmake .. -DPYTHON_EXECUTABLE=""
```

---

## Runtime Issues

### Issue: No OpenCL Platforms Found

**Symptoms:**
```bash
$ clinfo
Number of platforms: 0
```

**Solutions:**

1. **Verify Beignet is installed:**
```bash
ls /usr/local/lib/beignet/libcl.so  # Should exist
```

2. **Check ICD registration:**
```bash
cat /etc/OpenCL/vendors/intel-beignet.icd
# Should contain: /usr/local/lib/beignet/libcl.so
```

3. **Manually register ICD:**
```bash
sudo mkdir -p /etc/OpenCL/vendors
echo "/usr/local/lib/beignet/libcl.so" | sudo tee /etc/OpenCL/vendors/intel-beignet.icd
```

4. **Set OCL_ICD_VENDORS environment variable:**
```bash
export OCL_ICD_VENDORS=/etc/OpenCL/vendors
clinfo
```

### Issue: GPU Not Detected

**Symptoms:**
```
Number of devices: 0
Platform Name: Intel Gen OCL Driver
```

**Solutions:**

1. **Check if Intel GPU exists:**
```bash
lspci | grep VGA
# Should show Intel Graphics

# More detailed:
lspci -v | grep -A 10 VGA
```

2. **Verify DRM device:**
```bash
ls -l /dev/dri/
# Should show: card0, renderD128, etc.

# Check permissions:
groups  # Should include 'video' or 'render' group
```

3. **Add user to video group:**
```bash
sudo usermod -a -G video $USER
sudo usermod -a -G render $USER
# Log out and log back in
```

4. **Check kernel module:**
```bash
lsmod | grep i915
# Should show i915 module loaded

# If not loaded:
sudo modprobe i915
```

### Issue: Kernel Execution Fails

**Symptoms:**
```
clEnqueueNDRangeKernel failed: CL_OUT_OF_RESOURCES
clEnqueueNDRangeKernel failed: CL_INVALID_WORK_GROUP_SIZE
```

**Solutions:**

1. **Check work-group size limits:**
```bash
clinfo | grep "Max work group size"
# Typical limits:
# Gen6: 512
# Gen7/7.5: 512-1024
```

2. **Reduce work-group size in kernel:**
```c
// Instead of:
clEnqueueNDRangeKernel(..., global_size, NULL, ...);  // NULL = driver chooses

// Use explicit local size:
size_t local_size = 64;  // Conservative size
clEnqueueNDRangeKernel(..., global_size, &local_size, ...);
```

3. **Check local memory usage:**
```c
// Query max local memory:
cl_ulong local_mem_size;
clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, NULL);
printf("Max local memory: %lu KB\n", local_mem_size / 1024);

// Gen6/7/7.5 typically: 64 KB
```

---

## Kernel Compilation Issues

### Issue: Kernel Compilation Failed

**Symptoms:**
```
clBuildProgram failed: CL_BUILD_PROGRAM_FAILURE
Build log: error: use of undeclared identifier 'foo'
```

**Solutions:**

1. **Get detailed build log:**
```c
cl_int status = clBuildProgram(program, ...);
if (status != CL_SUCCESS) {
    size_t log_size;
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
    char *log = malloc(log_size);
    clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
    printf("Build log:\n%s\n", log);
    free(log);
}
```

2. **Check OpenCL C version:**
```c
// For OpenCL 1.1 (Gen6):
const char *options = "-cl-std=CL1.1";
clBuildProgram(program, 1, &device, options, NULL, NULL);

// For OpenCL 1.2 (Gen7/7.5):
const char *options = "-cl-std=CL1.2";
clBuildProgram(program, 1, &device, options, NULL, NULL);
```

3. **Common syntax errors:**
```c
// WRONG: Using C++ features
__kernel void test() {
    auto x = 5;  // ERROR: auto not supported
}

// CORRECT: Use C99 syntax
__kernel void test() {
    int x = 5;
}
```

### Issue: Unsupported Built-in Function

**Symptoms:**
```
Build log: error: use of undeclared identifier 'atomiccompareexchange'
```

**Solutions:**

1. **Check function availability per generation:**

**Gen6 (OpenCL 1.1):**
- ✅ Basic atomics: `atomic_add`, `atomic_sub`, `atomic_xchg`
- ❌ No atomic compare-exchange
- ❌ No FP64 (double precision)

**Gen7/7.5 (OpenCL 1.2):**
- ✅ All OpenCL 1.2 atomics
- ✅ `atomic_cmpxchg`
- ⚠️ FP64 limited (Gen7.5 with `-DEXPERIMENTAL_DOUBLE`)

2. **Use appropriate functions:**
```c
// Gen6 workaround for cmpxchg:
int old_val = atomic_xchg(&dest, new_val);  // Use exchange instead

// Gen7/7.5:
int old_val = atomic_cmpxchg(&dest, expected, new_val);  // Full support
```

---

## Performance Issues

### Issue: Slow Kernel Execution on Gen6

**Symptoms:**
- Kernel runs but performance is 50% slower than expected

**Solutions:**

1. **Use SIMD8 instead of SIMD16 on Gen6:**
```c
// Force SIMD8 with work-group size:
size_t local_size = 8;  // Not 16
clEnqueueNDRangeKernel(..., global_size, &local_size, ...);
```

2. **Avoid 3-source operations (MAD, LRP):**
```c
// SLOW on Gen6 (uses 2×SIMD8):
result = mad(a, b, c);  // Multiply-add

// FASTER on Gen6:
result = a * b + c;  // Compiler may still use MAD, but has more options
```

3. **Check SIMD width in use:**
```c
// Query preferred work-group size:
size_t preferred_wg_size;
clGetKernelWorkGroupInfo(kernel, device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                         sizeof(preferred_wg_size), &preferred_wg_size, NULL);
printf("Preferred work-group size multiple: %zu\n", preferred_wg_size);
// Gen6 should prefer: 8
// Gen7/7.5 may prefer: 16
```

### Issue: High Register Pressure / Spilling

**Symptoms:**
```
Warning: Kernel uses too many registers, performance may be degraded
```

**Solutions:**

1. **Reduce local variable usage:**
```c
// BAD: Many intermediate variables
float temp1 = sin(x);
float temp2 = cos(x);
float temp3 = temp1 * temp2;
result = temp3 + 1.0f;

// BETTER: Fewer variables
result = sin(x) * cos(x) + 1.0f;
```

2. **Use `restrict` keyword:**
```c
// Helps compiler optimize pointer aliasing
__kernel void foo(__global float * restrict out,
                   __global const float * restrict in) {
    // ...
}
```

3. **Reduce work-group size:**
```c
// Smaller work-groups = more registers per work-item
size_t local_size = 32;  // Instead of 256
```

---

## Hardware Detection Issues

### Issue: Wrong GPU Generation Detected

**Symptoms:**
```bash
$ clinfo | grep "Device Name"
Device Name: Intel(R) HD Graphics [Unknown]
```

**Solutions:**

1. **Check PCI device ID:**
```bash
lspci -nn | grep VGA
# Look for [8086:XXXX] - the XXXX is the device ID

# Example outputs:
# Sandy Bridge (Gen6): [8086:0102], [8086:0112], [8086:0122]
# Ivy Bridge (Gen7): [8086:0152], [8086:0162]
# Haswell (Gen7.5): [8086:0402], [8086:0412], [8086:0422]
```

2. **Verify device ID in code:**
Check `src/cl_device_data.h` to ensure your device ID is mapped correctly.

3. **Force specific device ID (testing only):**
```bash
# Set environment variable:
export CL_DEVICE_ID=0x0162  # Example: Force Ivy Bridge GT2
```

### Issue: Multiple GPUs, Wrong One Selected

**Symptoms:**
- System has integrated + discrete GPU
- Wrong GPU being used

**Solutions:**

1. **List all OpenCL devices:**
```c
cl_uint num_platforms;
clGetPlatformIDs(0, NULL, &num_platforms);
// Enumerate and select correct platform/device
```

2. **Select specific GPU by index:**
```c
cl_device_id devices[10];
cl_uint num_devices;
clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 10, devices, &num_devices);

// Print all devices:
for (int i = 0; i < num_devices; i++) {
    char name[128];
    clGetDeviceInfo(devices[i], CL_DEVICE_NAME, sizeof(name), name, NULL);
    printf("Device %d: %s\n", i, name);
}

// Select the one you want:
cl_device_id device = devices[desired_index];
```

---

## LLVM Version Issues

### Issue: LLVM Version Mismatch

**Symptoms:**
```
CMake Error: LLVM version 15.0.0 required, but found 18.0.0
```

**Solutions:**

1. **This error should NOT occur with Frosted Beignet (Phase 4A fixes applied)**
   - Our version supports LLVM 16, 17, and 18

2. **If you see this, verify Phase 4A fixes:**
```bash
git log --oneline | grep "LLVM 18"
# Should show Phase 4A commit
```

3. **Force specific LLVM version:**
```bash
# Use LLVM 18:
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-18

# Use LLVM 17:
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-17

# Use LLVM 16:
cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-16
```

### Issue: Clang Not Found

**Symptoms:**
```
Could not find clang executable
```

**Solutions:**

1. **Install clang matching your LLVM version:**
```bash
# For LLVM 18:
sudo apt-get install clang-18

# Verify:
clang-18 --version
```

2. **Symlink clang to versioned clang:**
```bash
sudo ln -s /usr/bin/clang-18 /usr/bin/clang
sudo ln -s /usr/bin/clang++-18 /usr/bin/clang++
```

---

## Generation-Specific Issues

### Gen6 (Sandy Bridge) Specific

**Issue: 3-Source Instructions Slow**

**Symptom:** MAD/LRP operations have poor performance

**Solution:** This is expected - Gen6 splits SIMD16 → 2×SIMD8 for 3-source ops.
Workaround: Use SIMD8 kernels, or avoid MAD/LRP if possible.

**Issue: Atomic Operations Limited**

**Symptom:** Some atomic operations don't work

**Solution:** Gen6 has limited atomic support. Stick to basic operations:
- ✅ `atomic_add`, `atomic_sub`
- ✅ `atomic_xchg`
- ❌ `atomic_cmpxchg` may not work reliably

### Gen7 (Ivy Bridge) Specific

**Issue: Shared Local Memory Not Working**

**Symptom:** `__local` memory access causes crashes

**Solution:** Known issue on some Gen7 hardware/kernel combinations.
Workaround: Use private memory or small local memory allocations (<16KB).

### Gen7.5 (Haswell) Specific

**Issue: PPGTT (Per-Process GTT) Not Enabled**

**Symptom:** Context creation fails or GPU hangs

**Solution:** Enable PPGTT in kernel boot parameters:
```bash
# Edit /etc/default/grub:
GRUB_CMDLINE_LINUX="... i915.enable_ppgtt=2"

# Update grub and reboot:
sudo update-grub
sudo reboot
```

---

## Debugging Techniques

### Enable Debug Logging

**1. Environment Variables:**
```bash
# Enable all debug output:
export CL_LOG_ERRORS=stdout

# Enable verbose OpenCL ICD loader:
export OCL_ICD_DEBUG=15

# Run your program:
./my_opencl_app
```

**2. Beignet-Specific Debug:**
```bash
# Enable Beignet debug output:
export GBE_DEBUG=1

# Enable kernel dump:
export GBE_DUMP_KERNEL_SOURCE=1
export GBE_DUMP_KERNEL_LLVM=1
export GBE_DUMP_KERNEL_ASM=1
```

### Using GDB for Debugging

```bash
# Compile with debug symbols:
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run with GDB:
gdb --args ./my_opencl_app

# GDB commands:
(gdb) break clBuildProgram  # Break on kernel compilation
(gdb) run                   # Run program
(gdb) bt                    # Backtrace on crash
(gdb) print variable_name   # Inspect variables
```

### Kernel Debugging

**1. Printf Debugging:**
```c
__kernel void my_kernel(__global float *data) {
    int gid = get_global_id(0);
    printf("gid=%d, data[gid]=%f\n", gid, data[gid]);  // OpenCL 1.2+
}
```

**2. Dump Intermediate Representations:**
```bash
export GBE_DUMP_KERNEL_LLVM=1  # Dump LLVM IR
export GBE_DUMP_KERNEL_ASM=1   # Dump GPU assembly

# Output will be in:
# - kernel.ll (LLVM IR)
# - kernel.asm (Gen ISA)
```

**3. Analyze Generated Assembly:**
```bash
# After running with GBE_DUMP_KERNEL_ASM=1:
cat /tmp/beignet-kernel-*.asm

# Look for:
# - Instruction count (too many = slow)
# - Register usage
# - Memory access patterns
```

---

## Getting More Help

### Check Documentation

1. **Build Issues:** [BUILD.md](BUILD.md)
2. **Testing Issues:** [TESTING.md](TESTING.md)
3. **Architecture Questions:** [docs/MODERNIZATION_ANALYSIS.md](MODERNIZATION_ANALYSIS.md)
4. **Generation-Specific:** [docs/PHASE4C_GENERATION_VALIDATION.md](PHASE4C_GENERATION_VALIDATION.md)

### Report Issues

If you've tried everything and still have problems:

1. **Gather system information:**
```bash
# Create bug report:
cat > bug_report.txt <<EOF
GPU: $(lspci | grep VGA)
Kernel: $(uname -r)
LLVM: $(llvm-config --version)
Beignet Version: $(git describe --tags)
Build Type: $(cat build/CMakeCache.txt | grep CMAKE_BUILD_TYPE)

Error Message:
[paste error here]

Build Log:
[paste relevant build output]

Runtime Log:
[paste runtime errors]
EOF
```

2. **Include debug output:**
```bash
export GBE_DEBUG=1
export OCL_ICD_DEBUG=15
./my_program 2>&1 | tee runtime_log.txt
```

3. **Open an issue on GitHub** with the collected information

---

**Troubleshooting Guide Version:** 1.0
**Last Updated:** 2025-11-19
**Covers:** Build, Runtime, Performance, Hardware Detection, LLVM 16/17/18, Gen6/7/7.5
