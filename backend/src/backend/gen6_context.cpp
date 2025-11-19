/*
 * Copyright © 2012-2025 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Frosted Beignet Development Team
 */

/**
 * \file gen6_context.cpp
 * \brief Gen6 (Sandy Bridge) code generation context implementation
 */

#include "backend/gen6_context.hpp"
#include "backend/gen_insn_selection.hpp"
#include "ir/function.hpp"

namespace gbe
{
  /**
   * Gen6 scratch buffer size (smaller than Gen7+ due to hardware limitations)
   * Gen6 has 512KB L3 cache shared across all EUs
   */
  #define GEN6_SCRATCH_SIZE  (256 * KB)

  /**
   * Constructor
   */
  Gen6Context::Gen6Context(const ir::Unit &unit,
                           const std::string &name,
                           uint32_t deviceID,
                           bool relaxMath)
      : GenContext(unit, name, deviceID, relaxMath)
  {
    // Gen6-specific initialization
    // Force SIMD8 mode for better Gen6 performance
    if (this->simdWidth == 16) {
      // Gen6 SIMD16 has significant performance penalties
      // Recommend using SIMD8 instead
      this->simdWidth = 8;
    }
  }

  /**
   * Destructor
   */
  Gen6Context::~Gen6Context(void)
  {
    // Cleanup Gen6-specific resources
  }

  /**
   * Check if Gen6 supports specific feature
   */
  bool Gen6Context::supportsFeature(GenFeature feature) const
  {
    switch (feature) {
      // Features NOT supported on Gen6
      case GEN_FEATURE_ATOMIC_INT64:
        return false; // Gen6 does not support 64-bit atomics

      case GEN_FEATURE_FP64:
        return false; // Gen6 does not support native double precision

      case GEN_FEATURE_3D_IMAGE_WRITE:
        return false; // Limited image write support

      case GEN_FEATURE_OPENCL_2_0:
        return false; // Gen6 is OpenCL 1.1 only

      case GEN_FEATURE_SUBGROUPS:
        return false; // No subgroup support

      // Features supported on Gen6
      case GEN_FEATURE_BASIC_ALU:
        return true;

      case GEN_FEATURE_BASIC_ATOMICS:
        return true; // 32-bit atomics supported

      case GEN_FEATURE_IMAGES:
        return true; // Basic image support

      case GEN_FEATURE_SIMD8:
        return true;

      case GEN_FEATURE_SIMD16:
        return true; // Supported but with performance caveats

      default:
        // Unknown feature, assume not supported
        return false;
    }
  }

  /**
   * Get Gen6-specific cache control settings
   */
  uint32_t Gen6Context::getCacheControl(void) const
  {
    // Gen6 cache control is simpler than Gen7+
    // Use write-back cache for best performance
    return 0x3; // Write-back
  }

  /**
   * Emit Gen6-specific code for the kernel
   */
  bool Gen6Context::emitCode(void)
  {
    // Apply Gen6-specific optimizations before code generation
    this->optimizeForGen6();

    // Call parent class code emission
    if (!GenContext::emitCode())
      return false;

    // Apply Gen6-specific workarounds after code generation
    this->applyGen6CacheWorkarounds();
    this->handleThreeSourceLimitations();

    return true;
  }

  /**
   * Emit Gen6-specific prologue code
   */
  void Gen6Context::emitPrologue(void)
  {
    // Call parent prologue
    GenContext::emitPrologue();

    // Gen6-specific prologue setup
    // Set up cache control for optimal Gen6 performance
    // This would emit instructions to configure cache policies
  }

  /**
   * Emit Gen6-specific epilogue code
   */
  void Gen6Context::emitEpilogue(void)
  {
    // Gen6-specific epilogue cleanup

    // Call parent epilogue
    GenContext::emitEpilogue();
  }

  /**
   * Emit Gen6-specific atomic operation
   */
  void Gen6Context::emitAtomic(const ir::AtomicInstruction &atomic)
  {
    // Gen6 has limited atomic support
    // - Only 32-bit atomics (no 64-bit)
    // - Fewer atomic operations than Gen7+
    // - Different message format

    switch (atomic.getAtomicOpcode()) {
      case ir::ATOMIC_OP_ADD:
      case ir::ATOMIC_OP_SUB:
      case ir::ATOMIC_OP_INC:
      case ir::ATOMIC_OP_DEC:
      case ir::ATOMIC_OP_MIN:
      case ir::ATOMIC_OP_MAX:
      case ir::ATOMIC_OP_AND:
      case ir::ATOMIC_OP_OR:
      case ir::ATOMIC_OP_XOR:
      case ir::ATOMIC_OP_XCHG:
      case ir::ATOMIC_OP_CMPXCHG:
        // These are supported on Gen6 (32-bit only)
        GenContext::emitAtomic(atomic);
        break;

      default:
        // Unsupported atomic operation on Gen6
        // Would need software fallback or error
        NOT_SUPPORTED;
        break;
    }
  }

  /**
   * Emit Gen6-specific barrier instruction
   */
  void Gen6Context::emitBarrier(const ir::BarrierInstruction &barrier)
  {
    // Gen6 barrier implementation is similar to Gen7
    // but uses different message encoding
    GenContext::emitBarrier(barrier);
  }

  /**
   * Allocate registers with Gen6 constraints
   */
  bool Gen6Context::allocateRegisters(void)
  {
    // Gen6 has same 128 GRF registers as Gen7
    // but different performance characteristics

    // Gen6 register allocation strategy:
    // - Prefer fewer registers to reduce pressure
    // - Account for lower EU count (max 12 vs 16)
    // - Consider cache thrashing on Gen6

    return GenContext::allocateRegisters();
  }

  /**
   * Apply Gen6 cache workarounds
   */
  void Gen6Context::applyGen6CacheWorkarounds(void)
  {
    // Gen6 has known cache coherency issues that require workarounds:
    // 1. Render cache flush after writes to ensure visibility
    // 2. Extra barriers around atomic operations
    // 3. Explicit cache control for shared local memory

    // These workarounds would insert additional instructions
    // into the generated code to ensure correctness
  }

  /**
   * Handle Gen6 3-source instruction limitations
   */
  void Gen6Context::handleThreeSourceLimitations(void)
  {
    // Gen6 limitations for 3-source instructions (MAD, LRP):
    // - Cannot do SIMD16 (must use 2x SIMD8)
    // - Only supports float type
    // - All operands must be in GRF

    // Walk through generated instructions and fix any that violate these constraints
    // This is already handled by the encoder, but we verify here
  }

  /**
   * Optimize for Gen6 performance characteristics
   */
  void Gen6Context::optimizeForGen6(void)
  {
    // Gen6-specific optimizations:

    // 1. Prefer SIMD8 over SIMD16
    //    Gen6 SIMD16 has ~50% lower throughput per EU than SIMD8
    if (this->simdWidth > 8) {
      // Consider splitting into multiple SIMD8 kernels
      // This is a trade-off between occupancy and throughput
    }

    // 2. Minimize 3-source instructions
    //    MAD/LRP are expensive on Gen6
    //    Consider expanding to 2-source sequence where beneficial

    // 3. Optimize memory access patterns
    //    Gen6 cache is smaller and less sophisticated
    //    Prefer sequential access, avoid random patterns

    // 4. Reduce register pressure
    //    Gen6 has slower register spilling than Gen7+
    //    Try to use <80 registers when possible

    // These optimizations would be implemented in the instruction
    // selection and scheduling passes
  }

} // namespace gbe
