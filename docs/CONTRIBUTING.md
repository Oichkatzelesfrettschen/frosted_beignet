# Contributing to Frosted Beignet

**Welcome to Frosted Beignet!** We're excited that you're interested in contributing to this modernized OpenCL implementation for Intel GPUs.

**Last Updated:** 2025-11-19
**Project Status:** Production Ready (Phase 4 Complete)

## Table of Contents

1. [Getting Started](#getting-started)
2. [Development Environment](#development-environment)
3. [Code Style and Conventions](#code-style-and-conventions)
4. [Git Workflow](#git-workflow)
5. [Testing Requirements](#testing-requirements)
6. [Pull Request Process](#pull-request-process)
7. [Code Review Guidelines](#code-review-guidelines)
8. [Documentation Standards](#documentation-standards)
9. [Adding New Features](#adding-new-features)
10. [Debugging and Troubleshooting](#debugging-and-troubleshooting)
11. [Community Guidelines](#community-guidelines)

---

## Getting Started

### Prerequisites

Before contributing, ensure you have:

1. **Development Tools:**
   - GCC 9.0+ or Clang 10.0+
   - CMake 2.6.0+ (3.10+ recommended)
   - Git 2.0+
   - Python 3.6+ (for code generation)

2. **LLVM/Clang:**
   - LLVM 16.x, 17.x, or 18.x (18.x recommended)
   - Matching clang version
   - LLVM development libraries

3. **Dependencies:**
   - libdrm 2.4.39+
   - OpenCL ICD Loader
   - Intel GPU hardware (Gen6-Gen9) or emulation environment

4. **Knowledge:**
   - C++23 and C2x standards
   - LLVM IR and compiler infrastructure
   - OpenCL 1.1/1.2 specification
   - Intel GPU architecture (Gen6/7/7.5)

### First Build

```bash
# Clone the repository
git clone https://github.com/Oichkatzelesfrettschen/frosted_beignet.git
cd frosted_beignet

# Create development build
mkdir build-dev && cd build-dev
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)

# Run tests to verify setup
cd utests
./utest_run
```

See [BUILD.md](BUILD.md) for detailed build instructions.

---

## Development Environment

### Recommended IDE Setup

**VS Code:**
```json
{
  "C_Cpp.default.cppStandard": "c++23",
  "C_Cpp.default.cStandard": "c2x",
  "C_Cpp.default.compilerPath": "/usr/bin/clang++-18",
  "C_Cpp.default.includePath": [
    "${workspaceFolder}/src",
    "${workspaceFolder}/backend/src",
    "/usr/lib/llvm-18/include"
  ],
  "cmake.configureSettings": {
    "CMAKE_BUILD_TYPE": "Debug",
    "LLVM_INSTALL_DIR": "/usr/lib/llvm-18"
  }
}
```

**CLion:**
- Set CMake build type to Debug
- Configure LLVM path in CMake settings
- Enable clang-tidy integration

### Directory Structure

Understanding the codebase organization:

```
frosted_beignet/
├── backend/                    # Compiler backend
│   ├── src/
│   │   ├── backend/           # Generation-specific backends
│   │   │   ├── gen6_*.cpp    # Sandy Bridge (Gen6)
│   │   │   ├── gen7_*.cpp    # Ivy Bridge (Gen7)
│   │   │   └── gen75_*.cpp   # Haswell (Gen7.5)
│   │   ├── ir/                # Intermediate representation
│   │   ├── llvm/              # LLVM integration
│   │   └── libocl/            # OpenCL built-in library
│   │       ├── tmpl/          # Template source files
│   │       └── script/        # Code generation scripts
│   └── CMakeLists.txt
├── src/                        # Runtime implementation
│   ├── cl_*.c                 # OpenCL API implementation
│   ├── intel/                 # Intel-specific runtime
│   └── CMakeLists.txt
├── include/                    # Public headers
│   └── CL/                    # OpenCL headers
├── kernels/                    # Test kernels
├── utests/                     # Unit tests
├── docs/                       # Documentation
└── CMakeLists.txt             # Root build configuration
```

### Building Specific Components

```bash
# Build only the backend
make gbe -j$(nproc)

# Build only the runtime
make cl -j$(nproc)

# Build OpenCL built-in library
make beignet_bitcode -j$(nproc)

# Build unit tests
make utests -j$(nproc)

# Rebuild with verbose output
make VERBOSE=1 -j$(nproc)
```

---

## Code Style and Conventions

### C++ Style Guide

Frosted Beignet follows a consistent C++ style for maintainability.

#### Naming Conventions

```cpp
// Classes: PascalCase
class GenEncoder { };
class Gen6Context { };

// Functions: camelCase
void emitInstruction();
uint32_t getRegisterOffset();

// Variables: camelCase
int maxThreads = 16;
GenRegister destReg;

// Constants: UPPER_CASE or kPrefix
#define MAX_SIMD_WIDTH 16
const int kDefaultSIMD = 8;

// Namespaces: lowercase
namespace gbe {
namespace ir {
  // ...
}
}

// Private members: no special prefix (context-based)
class MyClass {
private:
  int memberVar;  // Clear from context
};
```

#### Formatting

```cpp
// Indentation: 2 spaces (NO TABS)
void function() {
  if (condition) {
    // Code here
  }
}

// Braces: K&R style (opening brace on same line)
class MyClass {
public:
  void method() {
    // ...
  }
};

// Line length: Aim for 80-100 characters, max 120
// Break long lines at logical points
void longFunctionName(int parameter1,
                      int parameter2,
                      int parameter3) {
  // ...
}

// Pointer/reference alignment: Type-aligned
int* ptr;          // Preferred
GenRegister& reg;  // Preferred

// NOT:
int *ptr;          // Avoid
```

#### Modern C++ Usage

```cpp
// Use auto for complex types
auto it = map.find(key);
auto ptr = std::make_unique<Object>();

// Use nullptr, not NULL
void* ptr = nullptr;  // Good
void* ptr = NULL;     // Avoid

// Use range-based for loops
for (const auto& item : collection) {
  // ...
}

// Use override keyword
class Derived : public Base {
  virtual void method() override;  // Good
  virtual void method();           // Missing override
};

// Use smart pointers when appropriate
std::unique_ptr<Object> obj = std::make_unique<Object>();

// BUT: Legacy code may use raw pointers - maintain consistency
// within each subsystem
```

### C Style Guide (for OpenCL built-ins)

```c
// File: backend/src/libocl/tmpl/ocl_math.tmpl.cl

// Function naming: lowercase with underscores
OVERLOADABLE float sin(float x);
OVERLOADABLE float2 sin(float2 x);

// Use OVERLOADABLE macro for all built-in overloads
#define OVERLOADABLE __attribute__((overloadable))

// Formatting: consistent with C++ style
INLINE_OVERLOADABLE float mad(float a, float b, float c) {
  return a * b + c;
}
```

### Comments and Documentation

```cpp
/**
 * \file gen6_encoder.hpp
 * \brief Gen6 (Sandy Bridge) instruction encoder
 *
 * Implements instruction encoding for Intel Gen6 architecture.
 * Handles SIMD8/SIMD16 width management and Gen6-specific
 * encoding requirements.
 */

/**
 * \brief Emit a 3-source ALU instruction (MAD, LRP, BFE, etc.)
 *
 * Gen6 limitation: Only SIMD8 supported for 3-source instructions.
 * SIMD16 operations are split into two SIMD8 instructions.
 *
 * \param opcode The ALU operation (e.g., GEN_OPCODE_MAD)
 * \param dst Destination register
 * \param src0 First source operand
 * \param src1 Second source operand
 * \param src2 Third source operand
 */
void alu3(uint32_t opcode, GenRegister dst,
          GenRegister src0, GenRegister src1, GenRegister src2);

// Inline comments: Explain WHY, not WHAT
if (gen == 6) {
  // Gen6 doesn't support SIMD16 for 3-source ALU
  execWidth = 8;
}
```

### Header Guards

```cpp
// Use pragma once (preferred in this codebase)
#pragma once

// OR traditional guards (legacy compatibility)
#ifndef __GBE_GEN6_ENCODER_HPP__
#define __GBE_GEN6_ENCODER_HPP__
// ...
#endif // __GBE_GEN6_ENCODER_HPP__
```

### LLVM Version Compatibility

When adding LLVM-version-specific code:

```cpp
// Use version detection macros
#if LLVM_VERSION_MAJOR >= 18
  // LLVM 18+ opaque pointer support
  auto ptrType = PointerType::get(context, 0);
#elif LLVM_VERSION_MAJOR >= 16
  // LLVM 16-17 typed pointers
  auto ptrType = PointerType::get(elementType, addrSpace);
#else
  #error "LLVM 16+ required"
#endif

// Document version-specific behavior
// LLVM 18 changed to opaque pointers, requiring different API usage
```

---

## Git Workflow

### Branch Strategy

```bash
# Main branch (stable releases)
main

# Feature branches (prefix with category)
feature/gen9-skylake-support
fix/gen6-simd16-mad
docs/api-documentation
test/gen75-atomic-validation

# Current development branch
claude/intel-gpu-opencl-support-01Y4TL9FKKTA7kmLwr4LuLAt
```

### Commit Messages

Follow conventional commit format:

```bash
# Format: <type>(<scope>): <subject>
#
# Types: feat, fix, docs, test, refactor, perf, style, chore
# Scope: gen6, gen7, gen75, llvm, opencl, runtime, backend, build
# Subject: Imperative mood, lowercase, no period, max 72 chars

# Good examples:
feat(gen6): Add SIMD16 support for 3-source ALU instructions
fix(llvm18): Resolve opaque pointer type compatibility
docs(phase4): Complete Phase 4D infrastructure analysis
test(gen75): Add atomic operation validation tests
refactor(backend): Simplify register allocation logic

# Body (optional, wrap at 72 chars):
# - Explain WHAT changed and WHY
# - Reference issues/PRs if applicable
# - List breaking changes

# Example with body:
feat(gen6): Integrate Gen6 into compilation pipeline

This commit adds Gen6 (Sandy Bridge) to the GPU device detection
and compilation pipeline. Gen6 devices now properly initialize
with OpenCL 1.1 feature level.

Changes:
- Added Gen6 device IDs to intel_gpgpu.c
- Integrated Gen6Encoder into backend selection
- Updated feature detection for Gen6 capabilities

Closes #123
```

### Making Changes

```bash
# 1. Create a feature branch
git checkout -b feature/my-new-feature

# 2. Make changes and test
# ... edit files ...
make -j$(nproc)
cd utests && ./utest_run

# 3. Stage changes
git add path/to/changed/files

# 4. Commit with descriptive message
git commit -m "feat(scope): Add new feature description"

# 5. Push to your fork
git push -u origin feature/my-new-feature

# 6. Create pull request on GitHub
```

### Keeping Your Branch Updated

```bash
# Fetch latest changes
git fetch origin

# Rebase your branch on main
git rebase origin/main

# Resolve conflicts if any
# ... fix conflicts ...
git add conflicted-files
git rebase --continue

# Force push (only for feature branches, never for main!)
git push --force-with-lease origin feature/my-new-feature
```

---

## Testing Requirements

### Running Tests

All code changes MUST pass existing tests:

```bash
# Build tests
cd build
make utests -j$(nproc)

# Run all tests
cd utests
./utest_run

# Run specific test category
./utest_run compiler_*
./utest_run builtin_*

# Run with verbose output
./utest_run -v

# Run single test
./utest_run compiler_mad_sat
```

### Test Coverage Requirements

**For new features, you must add tests:**

1. **Compiler Tests** (`utests/compiler_*.cpp`)
   - Test kernel compilation
   - Verify generated code correctness
   - Check generation-specific behavior

2. **Runtime Tests** (`utests/runtime_*.cpp`)
   - Test OpenCL API behavior
   - Validate memory management
   - Check error handling

3. **Built-in Tests** (`utests/builtin_*.cpp`)
   - Test OpenCL built-in functions
   - Verify all vector widths
   - Check edge cases (NaN, Inf, etc.)

### Writing New Tests

```cpp
// File: utests/compiler_my_feature.cpp

#include "utest_helper.hpp"

// Test function naming: <category>_<feature>_<variant>
void compiler_my_feature(void)
{
  const size_t n = 16;

  // 1. Setup: Allocate buffers
  OCL_CREATE_BUFFER(buf[0], 0, n * sizeof(float), NULL);
  OCL_CREATE_BUFFER(buf[1], 0, n * sizeof(float), NULL);

  // 2. Load kernel
  OCL_CREATE_KERNEL("compiler_my_feature");

  // 3. Set kernel arguments
  OCL_SET_ARG(0, sizeof(cl_mem), &buf[0]);
  OCL_SET_ARG(1, sizeof(cl_mem), &buf[1]);

  // 4. Initialize input data
  OCL_MAP_BUFFER(0);
  for (uint32_t i = 0; i < n; ++i)
    ((float*)buf_data[0])[i] = (float)i;
  OCL_UNMAP_BUFFER(0);

  // 5. Execute kernel
  globals[0] = n;
  locals[0] = 16;
  OCL_NDRANGE(1);

  // 6. Verify results
  OCL_MAP_BUFFER(1);
  for (uint32_t i = 0; i < n; ++i) {
    float expected = /* calculate expected value */;
    float actual = ((float*)buf_data[1])[i];
    OCL_ASSERT(fabs(actual - expected) < 1e-5);
  }
  OCL_UNMAP_BUFFER(1);
}

MAKE_UTEST_FROM_FUNCTION(compiler_my_feature);
```

```c
// File: kernels/compiler_my_feature.cl

__kernel void compiler_my_feature(__global float *src,
                                   __global float *dst)
{
  int id = get_global_id(0);
  dst[id] = /* operation on src[id] */;
}
```

### Generation-Specific Testing

Test on all supported generations when possible:

```cpp
void compiler_my_feature_gen6(void) {
  if (!IS_GEN6) return;  // Skip if not Gen6
  // Gen6-specific test logic
}

void compiler_my_feature_gen7(void) {
  if (!IS_GEN7) return;
  // Gen7-specific test logic
}

void compiler_my_feature_gen75(void) {
  if (!IS_GEN75) return;
  // Gen7.5-specific test logic
}
```

### Performance Testing

For performance-critical changes:

```bash
# Run benchmarks
cd kernels
./bench_copy_buffer
./bench_vector_add

# Profile with perf
perf record -g ./utest_run compiler_my_feature
perf report
```

---

## Pull Request Process

### Before Creating a PR

**Checklist:**

- [ ] Code follows style guidelines
- [ ] All tests pass (`./utest_run`)
- [ ] New tests added for new features
- [ ] Documentation updated (if applicable)
- [ ] Commit messages follow conventions
- [ ] No compiler warnings
- [ ] Code builds on LLVM 16, 17, and 18 (if possible)
- [ ] No unnecessary changes (whitespace, reformatting unrelated code)

### Creating a Pull Request

1. **Push your branch:**
   ```bash
   git push -u origin feature/my-feature
   ```

2. **Open PR on GitHub:**
   - Go to repository on GitHub
   - Click "New Pull Request"
   - Select your branch
   - Fill in template

3. **PR Title:** Follow commit message format
   ```
   feat(gen6): Add SIMD16 support for 3-source instructions
   ```

4. **PR Description Template:**
   ```markdown
   ## Summary
   Brief description of changes and motivation.

   ## Changes
   - Bullet list of specific changes
   - What was added/modified/removed

   ## Testing
   - How was this tested?
   - Which test cases were added?
   - Hardware tested on (if applicable)

   ## Generation Support
   - [ ] Gen6 (Sandy Bridge)
   - [ ] Gen7 (Ivy Bridge)
   - [ ] Gen7.5 (Haswell)
   - [ ] Gen8+ (if applicable)

   ## LLVM Compatibility
   - [ ] LLVM 16
   - [ ] LLVM 17
   - [ ] LLVM 18

   ## Documentation
   - [ ] Code comments updated
   - [ ] Documentation files updated (if applicable)

   ## Related Issues
   Closes #123
   Related to #456
   ```

### PR Review Process

1. **Automated Checks:**
   - CI build must pass (if configured)
   - No merge conflicts

2. **Code Review:**
   - At least one approval required
   - Address all review comments
   - Update PR based on feedback

3. **Merge:**
   - Squash commits if requested
   - Maintainer merges when approved

---

## Code Review Guidelines

### For Authors

**Responding to feedback:**

```markdown
# Good response:
"Good catch! I've updated the code to use the Gen6-specific path.
See commit abc123."

# Explain rationale when disagreeing:
"I kept the extra check because Gen6 can hit this edge case in
SIMD16 mode with 3-source instructions. Added a comment to clarify."
```

### For Reviewers

**Focus areas:**

1. **Correctness:**
   - Does it handle edge cases?
   - Are there potential bugs?
   - Is error handling appropriate?

2. **Generation-Specific Behavior:**
   - Does it respect Gen6/7/7.5 limitations?
   - Are SIMD width constraints followed?
   - Is hardware capability checked?

3. **LLVM Compatibility:**
   - Does it work with LLVM 16/17/18?
   - Are version checks used correctly?

4. **Performance:**
   - Are there obvious inefficiencies?
   - Could this impact runtime performance?

5. **Testing:**
   - Are tests sufficient?
   - Do tests cover edge cases?

6. **Documentation:**
   - Are complex parts commented?
   - Is public API documented?

**Review comments should be:**
- Constructive and specific
- Explain the "why" behind suggestions
- Distinguish between required changes and suggestions

```markdown
# Good review comment:
"This could cause a segfault if `ptr` is NULL. Consider adding a
null check before dereferencing, or document that the caller must
guarantee non-null."

# Less helpful:
"Fix this."
```

---

## Documentation Standards

### Code Documentation

**Every public API must be documented:**

```cpp
/**
 * \brief One-line summary
 *
 * Detailed description explaining:
 * - What the function does
 * - When to use it
 * - Any special considerations
 * - Generation-specific behavior (if applicable)
 *
 * \param paramName Description of parameter
 * \param[out] outParam Output parameter description
 * \return Description of return value
 *
 * \note Additional notes, warnings, or examples
 */
void publicFunction(int paramName, int* outParam);
```

### Documentation Files

When adding major features, update documentation:

- **docs/BUILD.md** - Build instructions
- **docs/TESTING.md** - Test procedures
- **docs/TROUBLESHOOTING.md** - Known issues
- **docs/IMPLEMENTATION_STATUS.md** - Feature status
- **README.md** - Project overview

### Markdown Style

```markdown
# Top-level heading (use once per document)

## Second-level heading

### Third-level heading

**Bold for emphasis** on important terms.

`Code formatting` for commands, filenames, variables.

```bash
# Code blocks with language specification
make -j$(nproc)
```

- Bullet lists for items
- Use consistent formatting

1. Numbered lists for steps
2. Follow logical order

> Blockquotes for important notes or warnings

[Link text](relative/path/to/file.md) for internal links
```

---

## Adding New Features

### Adding a New OpenCL Built-in Function

**Location:** `backend/src/libocl/tmpl/ocl_*.tmpl.cl`

```c
// 1. Add to appropriate template file (e.g., ocl_math.tmpl.cl)
INLINE_OVERLOADABLE float my_function(float x) {
  // Implementation
  return result;
}

// 2. Add overloads for all vector types (use macros)
DEFINE_VECTOR_VARIANTS(my_function, float)

// 3. Rebuild built-in library
make beignet_bitcode

// 4. Add tests in utests/builtin_my_function.cpp
```

### Adding Support for a New GPU Generation

**Example: Adding Gen8 support**

1. **Create encoder header:**
   ```cpp
   // backend/src/backend/gen8_encoder.hpp
   class Gen8Encoder : public Gen75Encoder {
     // Override generation-specific methods
   };
   ```

2. **Create encoder implementation:**
   ```cpp
   // backend/src/backend/gen8_encoder.cpp
   void Gen8Encoder::SEND(/* ... */) {
     // Gen8-specific encoding
   }
   ```

3. **Create context:**
   ```cpp
   // backend/src/backend/gen8_context.hpp
   class Gen8Context : public Gen75Context {
     virtual GenEncoder* generateEncoder(void) override {
       return GBE_NEW(Gen8Encoder, this->simdWidth, 80, deviceID);
     }
   };
   ```

4. **Update device detection:**
   ```c
   // src/intel/intel_gpgpu.c
   case PCI_CHIP_GEN8_DEVICE_ID:
     return intel_gpgpu_create_gen8(/* ... */);
   ```

5. **Add device IDs:**
   ```c
   // src/cl_device_id.h
   #define PCI_CHIP_GEN8_DEVICE_ID 0x1234
   ```

6. **Update CMake:**
   ```cmake
   # backend/src/CMakeLists.txt
   set(GBE_SRC
     # ...
     backend/gen8_encoder.cpp
     backend/gen8_context.cpp
   )
   ```

### Adding LLVM Version Support

**Example: Adding LLVM 19 support**

1. **Update version detection:**
   ```cmake
   # CMake/FindLLVM.cmake
   if (LLVM_VERSION_NODOT VERSION_GREATER_EQUAL 190)
     # LLVM 19+ specific handling
   endif()
   ```

2. **Add conditional compilation:**
   ```cpp
   #if LLVM_VERSION_MAJOR >= 19
     // LLVM 19+ API
   #elif LLVM_VERSION_MAJOR >= 18
     // LLVM 18 API
   #endif
   ```

3. **Test with LLVM 19:**
   ```bash
   cmake .. -DLLVM_INSTALL_DIR=/usr/lib/llvm-19
   make -j$(nproc)
   cd utests && ./utest_run
   ```

4. **Document in BUILD.md**

---

## Debugging and Troubleshooting

### Debugging Build Issues

```bash
# Verbose CMake output
cmake .. -DCMAKE_VERBOSE_MAKEFILE=ON

# Verbose make output
make VERBOSE=1

# Check CMake configuration
cmake .. -L  # List all CMake variables
cmake .. -LH # List with help text

# Examine CMake cache
cat CMakeCache.txt | grep LLVM
```

### Debugging Runtime Issues

```bash
# Enable Beignet debug output
export OCL_STRICT_CONFORMANCE=0
export OCL_IGNORE_CONFORMANCE_WARNINGS=1

# Run with debugger
gdb --args ./utest_run compiler_my_test
(gdb) break gen6_encoder.cpp:123
(gdb) run
```

### Debugging Kernel Compilation

```bash
# Dump generated IR
export GBE_DUMP_IR=1
./utest_run compiler_my_test

# Dump generated assembly
export GBE_DUMP_ASM=1
./utest_run compiler_my_test

# Keep intermediate files
export GBE_DUMP_LLVM_BEFORE_EXTRA_PASS=1
export GBE_DUMP_LLVM_AFTER_EXTRA_PASS=1
```

### Common Issues

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for detailed solutions.

---

## Community Guidelines

### Code of Conduct

We are committed to providing a welcoming and harassment-free environment:

- **Be respectful** of differing viewpoints and experiences
- **Be collaborative** - help each other learn and grow
- **Be patient** - remember everyone started as a beginner
- **Be constructive** in feedback and criticism
- **Be mindful** of your words and actions

### Getting Help

**Before asking for help:**

1. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
2. Search existing issues on GitHub
3. Read relevant documentation
4. Try to create a minimal reproduction

**When asking for help, provide:**

- Beignet version / branch
- LLVM version (`llvm-config --version`)
- GPU generation and device ID
- Build logs or error messages
- Steps to reproduce the issue

### Reporting Bugs

**Create a GitHub issue with:**

1. **Title:** Clear, descriptive summary
2. **Environment:**
   - OS and version
   - LLVM version
   - GPU generation
   - Beignet branch/commit
3. **Steps to reproduce**
4. **Expected behavior**
5. **Actual behavior**
6. **Logs/error messages**
7. **Possible fix** (if you have one)

### Feature Requests

When requesting features, explain:
- What problem does this solve?
- Who would benefit?
- How does it fit with project goals?
- Are you willing to implement it?

---

## Additional Resources

- **OpenCL Specification:** https://www.khronos.org/opencl/
- **Intel GPU Documentation:** https://01.org/linuxgraphics
- **LLVM Documentation:** https://llvm.org/docs/
- **Project Documentation:**
  - [BUILD.md](BUILD.md) - Build instructions
  - [TESTING.md](TESTING.md) - Testing guide
  - [TROUBLESHOOTING.md](TROUBLESHOOTING.md) - Common issues
  - [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) - Feature status
  - [MODERNIZATION_ANALYSIS.md](MODERNIZATION_ANALYSIS.md) - Architecture deep-dive

---

## License

By contributing to Frosted Beignet, you agree that your contributions will be licensed under the same LGPL 2.1+ license that covers the project. See the [LICENSE](../COPYING) file for details.

---

**Thank you for contributing to Frosted Beignet!**

Your contributions help keep legacy Intel GPU hardware useful with modern OpenCL workloads.

---

**Document Version:** 1.0
**Last Updated:** 2025-11-19
**Maintained By:** Frosted Beignet Development Team
