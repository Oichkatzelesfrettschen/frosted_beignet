# Phase 4: LLVM 18 + OpenCL Modernization Strategy

**Status:** 🔄 **IN PROGRESS** - Phase 4A: LLVM 18 Type System Compatibility

**Objective:** Comprehensively modernize the Beignet OpenCL stack for LLVM 18 compatibility while ensuring complete OpenCL 1.1/1.2 implementation and full Gen 6/7/7.5 feature support.

## Problem Analysis

### Root Cause: LLVM 18 Type System Changes

**Issue Identified:** LLVM 18/Clang 18 pre-defines Intel AVC (Advanced Video Coding) motion estimation types as **opaque types** to better support `cl_intel_motion_estimation` extension. Beignet's legacy implementation attempts to redefine these as struct types, causing typedef redefinition conflicts.

**Build Errors (6 errors):**
```
ocl_misc.h:319: typedef redefinition: 'struct intel_sub_group_avc_ime_payload_t' vs 'intel_sub_group_avc_ime_payload_t'
ocl_misc.h:321: typedef redefinition: 'uint8' vs 'intel_sub_group_avc_ime_result_t'
ocl_misc.h:342: typedef redefinition: 'struct intel_sub_group_avc_ref_payload_t' vs 'intel_sub_group_avc_ref_payload_t'
ocl_misc.h:367: typedef redefinition: 'struct intel_sub_group_avc_sic_payload_t' vs 'intel_sub_group_avc_sic_payload_t'
ocl_misc.h:369: typedef redefinition: 'uint8' vs 'intel_sub_group_avc_ref_result_t'
ocl_misc.h:371: typedef redefinition: 'uint8' vs 'intel_sub_group_avc_sic_result_t'
```

**Affected Types:**
- `intel_sub_group_avc_ime_payload_t` - IME (Integer Motion Estimation) input payload
- `intel_sub_group_avc_ime_result_t` - IME output result
- `intel_sub_group_avc_ref_payload_t` - REF (Refinement) input payload
- `intel_sub_group_avc_ref_result_t` - REF output result
- `intel_sub_group_avc_sic_payload_t` - SIC (Skip/Intra Check) input payload
- `intel_sub_group_avc_sic_result_t` - SIC output result

**Affected Files:**
- `backend/src/libocl/include/ocl_misc.h` (lines 306-371)
- `backend/src/libocl/src/ocl_misc.cl` (lines 237-500+)

## Phase 4 Roadmap

### Phase 4A: LLVM 18 Type System Compatibility ⏳ (Current)

**Goal:** Fix all LLVM 18 type conflicts and restore build capability

**Tasks:**

1. **Fix Intel AVC Type Conflicts** `backend/src/libocl/include/ocl_misc.h`
   - Strategy: Conditional compilation based on LLVM version
   - Use `#ifdef __clang_major__` to detect LLVM 18+
   - For LLVM 18+: Use opaque types with internal struct wrappers
   - For LLVM <18: Use existing struct definitions
   - Maintain ABI compatibility

2. **Update Implementation Functions** `backend/src/libocl/src/ocl_misc.cl`
   - Adapt functions to work with both opaque and struct types
   - Use type-safe conversion macros
   - Preserve existing functionality

3. **Validate Build System**
   - Ensure CMake properly detects Clang/LLVM version
   - Test compilation with LLVM 18
   - Verify no warnings with `-Werror`

4. **Test Basic Functionality**
   - Compile simple OpenCL kernel
   - Validate motion estimation extension works
   - Check Gen6 pipeline integration

**Success Criteria:**
- ✅ Zero build errors
- ✅ Zero build warnings (with `-Werror`)
- ✅ Motion estimation functions compile
- ✅ Basic kernel compilation successful

### Phase 4B: OpenCL Implementation Completeness

**Goal:** Audit and complete OpenCL 1.1/1.2 feature implementation

**OpenCL 1.1 Features (Gen6 Target):**
- ✅ Vector data types (char2...double16)
- ✅ Shuffle/shuffle2 operations
- ⏳ Math functions (need audit)
- ⏳ Geometric functions (need audit)
- ⏳ Relational functions (need audit)
- ⏳ Image functions (need audit)
- ⏳ Atomic functions (need audit)
- ⏳ Work-item functions (need audit)
- ⏳ Synchronization functions (need audit)

**OpenCL 1.2 Features (Gen7/7.5 Target):**
- ⏳ Built-in image functions (read_imagef, write_imagef)
- ⏳ Image formats (need validation)
- ⏳ Device-side enqueue (OpenCL 2.0 features)
- ⏳ Pipes (OpenCL 2.0 features)

**Intel Extensions:**
- ✅ `cl_intel_subgroups` (implemented)
- ✅ `cl_intel_subgroups_short` (implemented)
- ✅ `cl_intel_motion_estimation` (needs LLVM 18 fix)
- ✅ `cl_intel_media_block_io` (implemented)
- ✅ `cl_intel_planar_yuv` (implemented)
- ⏳ `cl_intel_required_subgroup_size` (conditional on Clang 4.1+)

**Audit Tasks:**
1. Systematically check each OpenCL built-in category
2. Compare against OpenCL 1.1 and 1.2 specifications
3. Identify missing functions
4. Implement missing functionality
5. Add comprehensive test coverage

### Phase 4C: Gen 7/7.5 Feature Validation & Completion

**Goal:** Ensure all Gen7 (Ivy Bridge) and Gen7.5 (Haswell) features are fully implemented and tested

**Gen7 (Ivy Bridge) Features:**
- ⏳ Dual flag registers (f0.0, f0.1)
- ⏳ 4-bit cache control encoding
- ⏳ Max 16 EUs support
- ⏳ SIMD16 3-source operations
- ⏳ Enhanced message gateway
- ⏳ Thread payload optimization
- ⏳ OpenCL 1.2 full support

**Gen7.5 (Haswell) Features:**
- ⏳ Gen75Context validation
- ⏳ Enhanced EU architecture (20 EUs)
- ⏳ Improved SIMD execution
- ⏳ Advanced sampler features
- ⏳ Enhanced video processing
- ⏳ L3 cache improvements

**Validation Tasks:**
1. Create generation-specific test kernels
2. Validate ISA generation for each feature
3. Test runtime execution on actual hardware (if available)
4. Validate binary format compatibility
5. Performance validation (SIMD8 vs SIMD16)

### Phase 4D: Legacy Stack Modernization

**Goal:** Modernize infrastructure both backward (Gen 6/7/7.5) and forward (LLVM 18+)

**Backward Compatibility (Gen 6/7/7.5):**
- ✅ Gen6 instruction encoder (complete)
- ✅ Gen6 context and runtime (complete)
- ✅ Gen6 binary format (complete)
- ⏳ Gen6 optimization passes
- ⏳ Gen7 validation and testing
- ⏳ Gen7.5 validation and testing

**Forward Compatibility (LLVM 18+):**
- ⏳ LLVM 18 type system (in progress)
- ⏳ LLVM 18 IR compatibility
- ⏳ Modern C++23 patterns
- ⏳ Deprecation warnings cleanup
- ⏳ Future-proof API design

**Infrastructure Improvements:**
- ⏳ CMake modernization (use imported targets)
- ⏳ Better version detection
- ⏳ Improved error messages
- ⏳ Enhanced logging and debugging
- ⏳ Continuous integration setup

## Technical Approach

### LLVM Version Detection Strategy

```cpp
// Detect LLVM 18+ for opaque type handling
#if __clang_major__ >= 18
  #define BEIGNET_LLVM18_OPAQUE_TYPES 1
  // Use compiler-provided opaque types
  // Provide conversion functions
#else
  #define BEIGNET_LLVM18_OPAQUE_TYPES 0
  // Use legacy struct definitions
#endif
```

### Type Compatibility Pattern

```cpp
#if BEIGNET_LLVM18_OPAQUE_TYPES
  // LLVM 18+: Opaque types with internal struct wrappers
  typedef struct {
    // Internal representation (implementation detail)
    ushort2 srcCoord;
    // ... fields
  } __beignet_avc_ime_payload_internal;

  // Conversion macros for opaque <-> internal
  #define BEIGNET_AVC_PAYLOAD_TO_INTERNAL(opaque) ((__beignet_avc_ime_payload_internal*)&(opaque))
#else
  // LLVM <18: Direct struct typedefs
  typedef struct {
    ushort2 srcCoord;
    // ... fields
  } intel_sub_group_avc_ime_payload_t;
#endif
```

### Build System Integration

```cmake
# Detect Clang/LLVM version
if (LLVM_VERSION_MAJOR GREATER_EQUAL 18)
  add_definitions(-DBEIGNET_LLVM18_OPAQUE_TYPES=1)
  message(STATUS "Using LLVM 18+ opaque type support")
else()
  add_definitions(-DBEIGNET_LLVM18_OPAQUE_TYPES=0)
  message(STATUS "Using legacy struct type definitions")
endif()
```

## Success Metrics

### Phase 4A Success Criteria:
- ✅ Build completes with zero errors
- ✅ Build completes with zero warnings (`-Werror`)
- ✅ All OpenCL built-in libraries compile
- ✅ Motion estimation extension functions compile
- ✅ Simple test kernel compiles for Gen6

### Phase 4B Success Criteria:
- ✅ 100% OpenCL 1.1 built-in coverage
- ✅ 100% OpenCL 1.2 built-in coverage (Gen7/7.5)
- ✅ All Intel extensions functional
- ✅ Comprehensive test suite passes

### Phase 4C Success Criteria:
- ✅ All Gen7 features validated
- ✅ All Gen7.5 features validated
- ✅ Generation-specific optimizations working
- ✅ Binary compatibility maintained

### Phase 4D Success Criteria:
- ✅ LLVM 16/17/18 compatibility
- ✅ Modern C++23 patterns adopted
- ✅ Clean build with all warnings enabled
- ✅ Future-proof architecture

## Timeline Estimate

- **Phase 4A:** 2-3 hours (immediate priority)
- **Phase 4B:** 4-6 hours (comprehensive audit)
- **Phase 4C:** 3-4 hours (validation & testing)
- **Phase 4D:** 2-3 hours (infrastructure)

**Total Phase 4:** ~12-16 hours of focused development

## Risk Assessment

**Low Risk:**
- ✅ Type system fix (well-understood problem)
- ✅ Build system integration (straightforward)

**Medium Risk:**
- ⚠️ OpenCL feature completeness (large surface area)
- ⚠️ Generation-specific validation (requires careful testing)

**High Risk:**
- 🔴 Hardware availability for testing (may need to rely on emulation)
- 🔴 Undocumented legacy behavior (may encounter surprises)

## Next Steps

1. ✅ **Immediate:** Fix intel_sub_group_avc type conflicts (ocl_misc.h)
2. Validate build completes successfully
3. Test basic kernel compilation
4. Begin OpenCL feature audit
5. Commit Phase 4A completion

---

**Document Version:** 1.0
**Last Updated:** 2025-11-19
**Status:** Phase 4A in progress
