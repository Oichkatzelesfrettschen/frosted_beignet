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
 *
 * Gen6 Architecture (Sandy Bridge - 2011):
 * - Maximum 12 execution units (GT2 configuration)
 * - 128-bit vector execution units
 * - Limited 3-source instruction support
 * - No native A64 addressing (uses legacy 32-bit addressing)
 * - DirectX 10.1, OpenGL 3.0 support
 * - No hardware OpenCL support (Beignet provides software implementation)
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
   * Key Gen6 Characteristics:
   * - 6 or 12 execution units (EU) depending on GT1/GT2 configuration
   * - 128-bit SIMD: 8x 16-bit or 4x 32-bit operations per clock
   * - Limited 3-source instruction support (no SIMD16 MAD)
   * - 32-bit addressing only (no A64)
   * - Lower scratch memory bandwidth
   * - Simplified surface state formats vs Gen7+
   *
   * Implementation Pattern:
   * Follows Gen75Context pattern (minimal overrides) rather than Gen8
   * (comprehensive overrides) since Gen6 is closer to base Gen7 design.
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
     * Applies Gen6 workarounds and limitations during code generation
     * @return true on success, false on failure
     */
    virtual bool emitCode(void) override;

    /**
     * Align scratch size to Gen6 requirements
     * Gen6 has different alignment requirements than Gen7+
     * @param size Unaligned scratch size
     * @return Aligned scratch size
     */
    virtual uint32_t alignScratchSize(uint32_t size) override;

    /**
     * Get scratch memory size for Gen6
     * @return Scratch size in bytes
     */
    virtual uint32_t getScratchSize(void) override;

    /**
     * Emit stack pointer setup for Gen6
     * Gen6 may have different stack setup requirements
     */
    virtual void emitStackPointer(void) override;

    /**
     * Emit barrier instruction for Gen6
     * Gen6 has different barrier implementation than Gen7+
     * @param insn Selection instruction containing barrier operation
     */
    virtual void emitBarrierInstruction(const SelectionInstruction &insn) override;

  protected:
    /**
     * Allocate a new Gen6-specific encoder
     * @return Pointer to Gen6Encoder instance
     */
    virtual GenEncoder* generateEncoder(void) override {
      return GBE_NEW(Gen6Encoder, this->simdWidth, 6, this->deviceID);
    }

    /**
     * Create new selection for Gen6
     * Apply Gen6-specific instruction selection rules
     */
    virtual void newSelection(void) override;

    /**
     * Emit SLM (Shared Local Memory) offset for Gen6
     * Gen6 has different SLM handling than Gen7+
     */
    virtual void emitSLMOffset(void) override;

  private:
    /**
     * Gen6-specific private helper methods
     * These are NOT virtual overrides, but internal implementation details
     */

    /**
     * Apply Gen6 cache workarounds
     * Gen6 has known cache coherency issues that need workarounds:
     * - Insert explicit flushes after certain operations
     * - Add memory barriers for SLM access
     */
    void applyGen6CacheWorkarounds(void);

    /**
     * Handle Gen6 3-source instruction limitations
     * Gen6 cannot execute SIMD16 for 3-source ops (MAD, etc.)
     * Split SIMD16 3-source instructions into 2x SIMD8
     */
    void handleThreeSourceLimitations(void);

    /**
     * Optimize for Gen6 performance characteristics
     * - Prefer SIMD8 over SIMD16 for complex operations
     * - Minimize 3-source instructions
     * - Optimize memory access patterns for Gen6 cache
     * - Avoid long dependency chains
     */
    void optimizeForGen6(void);

    /**
     * Check if instruction is a 3-source operation
     * @param insn Instruction to check
     * @return true if 3-source op (MAD, DP4, etc.)
     */
    bool isThreeSourceOp(const SelectionInstruction &insn) const;

    /**
     * Split SIMD16 instruction into 2x SIMD8 for Gen6
     * Required for 3-source ops and some other limitations
     * @param insn SIMD16 instruction to split
     */
    void splitToSIMD8(const SelectionInstruction &insn);
  };

} // namespace gbe

#endif /* __GBE_GEN6_CONTEXT_HPP__ */
