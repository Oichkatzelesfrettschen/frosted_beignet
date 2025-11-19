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
 * \file gen6_context.hpp
 * \brief Gen6 (Sandy Bridge) code generation context
 *
 * This context manages the code generation process for Gen6 GPUs,
 * including instruction emission, register allocation, and kernel
 * compilation.
 */

#ifndef __GBE_GEN6_CONTEXT_HPP__
#define __GBE_GEN6_CONTEXT_HPP__

#include "backend/gen_context.hpp"
#include "backend/gen6_encoder.hpp"

namespace gbe
{
  /**
   * Gen6Context - Code generation context for Sandy Bridge (Gen6)
   *
   * This class extends GenContext to provide Gen6-specific code generation.
   * It handles the unique constraints and capabilities of the Gen6 architecture.
   *
   * Key Gen6 characteristics:
   * - Maximum 12 execution units (vs 16 in Gen7)
   * - Limited 3-source instruction support
   * - Different cache control mechanism
   * - No native OpenCL hardware features
   * - Lower performance per EU
   * - Simplified surface state formats
   */
  class Gen6Context : public GenContext
  {
  public:
    /**
     * Constructor
     * @param unit Compilation unit containing IR
     * @param name Kernel name
     * @param deviceID PCI device ID
     * @param relaxMath Allow relaxed math optimizations
     */
    Gen6Context(const ir::Unit &unit,
                const std::string &name,
                uint32_t deviceID,
                bool relaxMath = false);

    /**
     * Destructor
     */
    virtual ~Gen6Context(void);

    /**
     * Emit Gen6-specific code for the kernel
     * @return true on success, false on failure
     */
    virtual bool emitCode(void) override;

  protected:
    /**
     * Allocate a new Gen6 encoder
     * @return Pointer to Gen6-specific encoder
     */
    virtual GenEncoder* generateEncoder(void) override {
      return GBE_NEW(Gen6Encoder, this->simdWidth, 6, this->deviceID);
    }

    /**
     * Get maximum number of execution units for Gen6
     * Gen6 has at most 12 EUs (GT2 configuration)
     * @return Maximum EU count
     */
    virtual uint32_t getMaxExecutionUnits(void) const override {
      // Gen6 GT1 has 6 EUs, GT2 has 12 EUs
      // Return maximum for GT2
      return 12;
    }

    /**
     * Get SIMD width for Gen6 kernels
     * Gen6 works best with SIMD8, SIMD16 has limitations
     * @return Preferred SIMD width
     */
    virtual uint32_t getPreferredSIMDWidth(void) const override {
      // Gen6 performs best with SIMD8
      // SIMD16 is supported but with reduced throughput
      return 8;
    }

    /**
     * Check if Gen6 supports specific feature
     * @param feature Feature to check
     * @return true if supported, false otherwise
     */
    virtual bool supportsFeature(GenFeature feature) const override;

    /**
     * Get Gen6-specific cache control settings
     * @return Cache control bits
     */
    virtual uint32_t getCacheControl(void) const override;

    /**
     * Emit Gen6-specific prologue code
     * Sets up registers and state for kernel execution
     */
    virtual void emitPrologue(void) override;

    /**
     * Emit Gen6-specific epilogue code
     * Cleanup and finalization before kernel return
     */
    virtual void emitEpilogue(void) override;

    /**
     * Emit Gen6-specific atomic operation
     * Gen6 has limited atomic support compared to Gen7+
     * @param atomic Atomic instruction from IR
     */
    virtual void emitAtomic(const ir::AtomicInstruction &atomic) override;

    /**
     * Emit Gen6-specific barrier instruction
     * @param barrier Barrier instruction from IR
     */
    virtual void emitBarrier(const ir::BarrierInstruction &barrier) override;

    /**
     * Allocate registers with Gen6 constraints
     * Gen6 has different register pressure characteristics
     * @return true on success, false if allocation fails
     */
    virtual bool allocateRegisters(void) override;

  private:
    /**
     * Gen6-specific workarounds and fixes
     */

    /**
     * Apply Gen6 cache workarounds
     * Gen6 has known cache coherency issues that need workarounds
     */
    void applyGen6CacheWorkarounds(void);

    /**
     * Handle Gen6 3-source instruction limitations
     * Gen6 cannot do SIMD16 for 3-source ops, needs splitting
     */
    void handleThreeSourceLimitations(void);

    /**
     * Optimize for Gen6 performance characteristics
     * - Prefer SIMD8 over SIMD16
     * - Minimize 3-source instructions
     * - Optimize memory access patterns
     */
    void optimizeForGen6(void);
  };

} // namespace gbe

#endif /* __GBE_GEN6_CONTEXT_HPP__ */
