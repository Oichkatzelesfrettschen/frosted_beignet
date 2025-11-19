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
 * \file gen6_encoder.hpp
 * \brief Gen6 (Sandy Bridge) instruction encoder
 *
 * This encoder handles instruction generation for Intel Gen6 GPUs (Sandy Bridge).
 * Gen6 has limited OpenCL support compared to Gen7+, with fewer execution units
 * and different cache control mechanisms.
 */

#ifndef __GBE_GEN6_ENCODER_HPP__
#define __GBE_GEN6_ENCODER_HPP__

#include "backend/gen_encoder.hpp"

namespace gbe
{
  /**
   * Gen6Encoder - Instruction encoder for Sandy Bridge (Gen6) architecture
   *
   * Key Gen6 architectural limitations:
   * - Maximum 12 execution units (vs 16 in Gen7)
   * - No native OpenCL support (software implementation required)
   * - Limited atomic operations
   * - Different cache control options
   * - Simplified surface state formats
   */
  class Gen6Encoder : public GenEncoder
  {
  public:
    virtual ~Gen6Encoder(void) { }

    /**
     * Constructor
     * @param simdWidth SIMD width (8, 16, or 32)
     * @param gen Generation number (should be 6)
     * @param deviceID PCI device ID
     */
    Gen6Encoder(uint32_t simdWidth, uint32_t gen, uint32_t deviceID)
         : GenEncoder(simdWidth, gen, deviceID) { }

    /**
     * Set instruction header with Gen6-specific defaults
     * @param insn Pointer to instruction to modify
     */
    virtual void setHeader(GenNativeInstruction *insn) override;

    /**
     * Set destination register for instruction
     * @param insn Pointer to instruction to modify
     * @param dest Destination register
     */
    virtual void setDst(GenNativeInstruction *insn, GenRegister dest) override;

    /**
     * Set source 0 register for instruction
     * @param insn Pointer to instruction to modify
     * @param reg Source 0 register
     */
    virtual void setSrc0(GenNativeInstruction *insn, GenRegister reg) override;

    /**
     * Set source 1 register for instruction
     * @param insn Pointer to instruction to modify
     * @param reg Source 1 register
     */
    virtual void setSrc1(GenNativeInstruction *insn, GenRegister reg) override;

    /**
     * Three-source ALU operation
     * Note: Gen6 has limited 3-source instruction support compared to Gen7+
     * @param opcode Operation code
     * @param dst Destination register
     * @param src0 Source 0 register
     * @param src1 Source 1 register
     * @param src2 Source 2 register
     */
    virtual void alu3(uint32_t opcode, GenRegister dst,
                      GenRegister src0, GenRegister src1, GenRegister src2) override;

    /**
     * Media block read (MBlock read)
     * Gen6 has different message formats than Gen7+
     * @param dst Destination register
     * @param header Message header register
     * @param bti Binding table index
     * @param elemSize Element size in bytes
     */
    virtual void MBREAD(GenRegister dst, GenRegister header,
                        uint32_t bti, uint32_t elemSize) override;

    /**
     * Media block write (MBlock write)
     * Gen6 uses different message descriptors than Gen7+
     * @param header Message header register
     * @param data Data register
     * @param bti Binding table index
     * @param elemSize Element size in bytes
     * @param useSends Use split send instruction (Gen6 may not support this)
     */
    virtual void MBWRITE(GenRegister header, GenRegister data,
                         uint32_t bti, uint32_t elemSize, bool useSends) override;

  protected:
    /**
     * Get Gen6-specific cache control bits
     * Gen6 has different cache control options than Gen7
     * @return Cache control bits
     */
    uint32_t getCacheControlGen6() const;

    /**
     * Check if instruction is supported on Gen6
     * @param opcode Instruction opcode
     * @return true if supported, false otherwise
     */
    bool isSupportedOnGen6(uint32_t opcode) const;
  };
}

#endif /* __GBE_GEN6_ENCODER_HPP__ */
