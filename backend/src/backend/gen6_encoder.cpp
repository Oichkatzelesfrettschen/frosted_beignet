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

/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/

/**
 * \file gen6_encoder.cpp
 * \brief Gen6 (Sandy Bridge) instruction encoder implementation
 *
 * This encoder generates machine code for Intel Gen6 (Sandy Bridge) GPUs.
 * Gen6 is the first "modern" Intel integrated GPU architecture but lacks
 * many features present in Gen7+.
 *
 * Key Gen6 limitations:
 * - No native OpenCL hardware support
 * - Fewer execution units (max 12 vs 16 in Gen7)
 * - Limited 3-source instruction support
 * - Different cache control mechanism
 * - Lower performance per EU (~50% of Gen7)
 * - No hardware scatter/gather optimization
 */

#include "backend/gen6_encoder.hpp"
#include "backend/gen6_instruction.hpp"

namespace gbe
{
  /**
   * Set instruction header with Gen6-specific defaults
   *
   * Gen6 instruction header is similar to Gen7 but with some differences:
   * - Flag register numbering is different
   * - Some fields have different meanings
   * - Gen6 has only 1 flag register vs 2 in Gen7
   */
  void Gen6Encoder::setHeader(GenNativeInstruction *insn) {
    Gen6NativeInstruction *gen6_insn = reinterpret_cast<Gen6NativeInstruction*>(insn);

    // Set execution width based on current SIMD width
    if (this->curr.execWidth == 8)
      gen6_insn->inst.header.execution_size = GEN_WIDTH_8;
    else if (this->curr.execWidth == 16)
      gen6_insn->inst.header.execution_size = GEN_WIDTH_16;
    else if (this->curr.execWidth == 4)
      gen6_insn->inst.header.execution_size = GEN_WIDTH_4;
    else if (this->curr.execWidth == 1)
      gen6_insn->inst.header.execution_size = GEN_WIDTH_1;
    else
      NOT_IMPLEMENTED;

    // Set control flags
    gen6_insn->inst.header.acc_wr_control = this->curr.accWrEnable;
    gen6_insn->inst.header.quarter_control = this->curr.quarterControl;
    gen6_insn->inst.bits1.ia1.nib_ctrl = this->curr.nibControl;
    gen6_insn->inst.header.mask_control = this->curr.noMask;

    // Set flag register (Gen6 has only 1 flag register vs 2 in Gen7+)
    // Flag register is stored differently for 3-source vs other instructions
    if (insn->header.opcode == GEN_OPCODE_MAD || insn->header.opcode == GEN_OPCODE_LRP) {
      // For 3-source instructions, flag is in bits1
      gen6_insn->inst.bits1.da16.flag_reg_nr = this->curr.flag & 0x1;
    } else {
      // For other instructions, flag is in bits2
      gen6_insn->inst.bits2.da1.flag_reg_nr = this->curr.flag & 0x1;
    }

    // Set predication control
    if (this->curr.predicate != GEN_PREDICATE_NONE) {
      gen6_insn->inst.header.predicate_control = this->curr.predicate;
      gen6_insn->inst.header.predicate_inverse = this->curr.inversePredicate;
    }

    // Set saturation
    gen6_insn->inst.header.saturate = this->curr.saturate;
  }

  /**
   * Set destination register for instruction
   *
   * Gen6 destination encoding is similar to Gen7 with some subtle differences
   * in how sub-register numbering works for certain data types.
   */
  void Gen6Encoder::setDst(GenNativeInstruction *insn, GenRegister dest) {
    Gen6NativeInstruction *gen6_insn = reinterpret_cast<Gen6NativeInstruction*>(insn);

    // Validate register number (Gen6 has max 128 registers)
    if (dest.file != GEN_ARCHITECTURE_REGISTER_FILE)
      assert(dest.nr < 128);

    // Set destination register file and type
    gen6_insn->inst.bits1.da1.dest_reg_file = dest.file;
    gen6_insn->inst.bits1.da1.dest_reg_type = dest.type;
    gen6_insn->inst.bits1.da1.dest_address_mode = dest.address_mode;
    gen6_insn->inst.bits1.da1.dest_reg_nr = dest.nr;
    gen6_insn->inst.bits1.da1.dest_subreg_nr = dest.subnr;

    // Set horizontal stride (Gen6 requires explicit stride for most types)
    // If stride is 0, set appropriate default based on data type
    if (dest.hstride == GEN_HORIZONTAL_STRIDE_0) {
      if (dest.type == GEN_TYPE_UB || dest.type == GEN_TYPE_B)
        dest.hstride = GEN_HORIZONTAL_STRIDE_4;
      else if (dest.type == GEN_TYPE_UW || dest.type == GEN_TYPE_W)
        dest.hstride = GEN_HORIZONTAL_STRIDE_2;
      else
        dest.hstride = GEN_HORIZONTAL_STRIDE_1;
    }
    gen6_insn->inst.bits1.da1.dest_horiz_stride = dest.hstride;
  }

  /**
   * Set source 0 register for instruction
   *
   * Gen6 source encoding supports both direct and indirect addressing.
   * This is one of the more complex parts of the ISA.
   */
  void Gen6Encoder::setSrc0(GenNativeInstruction *insn, GenRegister reg) {
    Gen6NativeInstruction *gen6_insn = reinterpret_cast<Gen6NativeInstruction*>(insn);

    // Validate register number
    if (reg.file != GEN_ARCHITECTURE_REGISTER_FILE)
      assert(reg.nr < 128);

    if (reg.address_mode == GEN_ADDRESS_DIRECT) {
      // Direct addressing mode
      gen6_insn->inst.bits1.da1.src0_reg_file = reg.file;
      gen6_insn->inst.bits1.da1.src0_reg_type = reg.type;
      gen6_insn->inst.bits2.da1.src0_abs = reg.absolute;
      gen6_insn->inst.bits2.da1.src0_negate = reg.negation;
      gen6_insn->inst.bits2.da1.src0_address_mode = reg.address_mode;

      if (reg.file == GEN_IMMEDIATE_VALUE) {
        // Immediate value - store in bits3
        gen6_insn->inst.bits3.imm32.imm32 = reg.value.ud;

        // Required to set some fields in src1 for immediate values
        gen6_insn->inst.bits1.da1.src1_reg_file = 0; // Architecture register file
        gen6_insn->inst.bits1.da1.src1_reg_type = reg.type;
      } else {
        // Register operand
        if (gen6_insn->inst.header.access_mode == GEN_ALIGN_1) {
          // Align1 mode - byte-level addressing
          gen6_insn->inst.bits2.da1.src0_subreg_nr = reg.subnr;
          gen6_insn->inst.bits2.da1.src0_reg_nr = reg.nr;
        } else {
          // Align16 mode - 16-byte aligned addressing
          gen6_insn->inst.bits2.da16.src0_subreg_nr = reg.subnr / 16;
          gen6_insn->inst.bits2.da16.src0_reg_nr = reg.nr;
        }

        // Set region parameters (stride and width)
        if (reg.width == GEN_WIDTH_1 &&
            gen6_insn->inst.header.execution_size == GEN_WIDTH_1) {
          // Scalar region - no stride
          gen6_insn->inst.bits2.da1.src0_horiz_stride = GEN_HORIZONTAL_STRIDE_0;
          gen6_insn->inst.bits2.da1.src0_width = GEN_WIDTH_1;
          gen6_insn->inst.bits2.da1.src0_vert_stride = GEN_VERTICAL_STRIDE_0;
        } else {
          // Vector region - use specified strides
          gen6_insn->inst.bits2.da1.src0_horiz_stride = reg.hstride;
          gen6_insn->inst.bits2.da1.src0_width = reg.width;
          gen6_insn->inst.bits2.da1.src0_vert_stride = reg.vstride;
        }

        // Gen6 flag register numbering
        gen6_insn->inst.bits2.da1.flag_reg_nr = this->curr.flag & 0x1;
      }
    } else {
      // Indirect addressing mode (address register a0)
      gen6_insn->inst.bits1.ia1.src0_reg_file = GEN_GENERAL_REGISTER_FILE;
      gen6_insn->inst.bits1.ia1.src0_reg_type = reg.type;
      gen6_insn->inst.bits2.ia1.src0_subreg_nr = reg.a0_subnr;
      gen6_insn->inst.bits2.ia1.src0_indirect_offset = reg.addr_imm;
      gen6_insn->inst.bits2.ia1.src0_abs = reg.absolute;
      gen6_insn->inst.bits2.ia1.src0_negate = reg.negation;
      gen6_insn->inst.bits2.ia1.src0_address_mode = reg.address_mode;
      gen6_insn->inst.bits2.ia1.src0_horiz_stride = reg.hstride;
      gen6_insn->inst.bits2.ia1.src0_width = reg.width;
      gen6_insn->inst.bits2.ia1.src0_vert_stride = reg.vstride;
      gen6_insn->inst.bits2.ia1.flag_reg_nr = this->curr.flag & 0x1;
    }
  }

  /**
   * Set source 1 register for instruction
   *
   * Source 1 is simpler than source 0 - it doesn't support indirect addressing
   * (except when source 0 is immediate, then source 1 cannot be immediate).
   */
  void Gen6Encoder::setSrc1(GenNativeInstruction *insn, GenRegister reg) {
    Gen6NativeInstruction *gen6_insn = reinterpret_cast<Gen6NativeInstruction*>(insn);

    // Validate register number
    assert(reg.nr < 128);

    // Set register file and type
    gen6_insn->inst.bits1.da1.src1_reg_file = reg.file;
    gen6_insn->inst.bits1.da1.src1_reg_type = reg.type;
    gen6_insn->inst.bits3.da1.src1_abs = reg.absolute;
    gen6_insn->inst.bits3.da1.src1_negate = reg.negation;

    // Source 0 cannot be immediate if source 1 is also immediate
    assert(gen6_insn->inst.bits1.da1.src0_reg_file != GEN_IMMEDIATE_VALUE);

    if (reg.file == GEN_IMMEDIATE_VALUE) {
      // Immediate value
      gen6_insn->inst.bits3.imm32.imm32 = reg.value.ud;
    } else {
      // Register operand - must use direct addressing
      assert(reg.address_mode == GEN_ADDRESS_DIRECT);

      if (gen6_insn->inst.header.access_mode == GEN_ALIGN_1) {
        // Align1 mode
        gen6_insn->inst.bits3.da1.src1_subreg_nr = reg.subnr;
        gen6_insn->inst.bits3.da1.src1_reg_nr = reg.nr;
      } else {
        // Align16 mode
        gen6_insn->inst.bits3.da16.src1_subreg_nr = reg.subnr / 16;
        gen6_insn->inst.bits3.da16.src1_reg_nr = reg.nr;
      }

      // Set region parameters
      if (reg.width == GEN_WIDTH_1 &&
          gen6_insn->inst.header.execution_size == GEN_WIDTH_1) {
        // Scalar region
        gen6_insn->inst.bits3.da1.src1_horiz_stride = GEN_HORIZONTAL_STRIDE_0;
        gen6_insn->inst.bits3.da1.src1_width = GEN_WIDTH_1;
        gen6_insn->inst.bits3.da1.src1_vert_stride = GEN_VERTICAL_STRIDE_0;
      } else {
        // Vector region
        gen6_insn->inst.bits3.da1.src1_horiz_stride = reg.hstride;
        gen6_insn->inst.bits3.da1.src1_width = reg.width;
        gen6_insn->inst.bits3.da1.src1_vert_stride = reg.vstride;
      }
    }
  }

  #define NO_SWIZZLE ((0<<0) | (1<<2) | (2<<4) | (3<<6))

  /**
   * Three-source ALU operation (MAD, LRP)
   *
   * Gen6 has limited 3-source instruction support compared to Gen7+.
   * - Only supports float type (GEN_TYPE_F)
   * - Requires align16 mode
   * - Does not support SIMD16 (must use 2x SIMD8)
   * - All sources must be in GRF (general register file)
   */
  void Gen6Encoder::alu3(uint32_t opcode,
                         GenRegister dest,
                         GenRegister src0,
                         GenRegister src1,
                         GenRegister src2)
  {
    GenNativeInstruction *insn = this->next(opcode);
    Gen6NativeInstruction *gen6_insn = reinterpret_cast<Gen6NativeInstruction*>(insn);

    // Determine execution size
    int execution_size = 0;
    if (this->curr.execWidth == 1) {
      execution_size = GEN_WIDTH_1;
    } else if (this->curr.execWidth == 8) {
      execution_size = GEN_WIDTH_8;
    } else if (this->curr.execWidth == 16) {
      // Gen6 does not support SIMD16 alu3, use SIMD8
      execution_size = GEN_WIDTH_8;
    } else {
      NOT_IMPLEMENTED;
    }

    // Validate destination
    assert(dest.file == GEN_GENERAL_REGISTER_FILE);
    assert(dest.nr < 128);
    assert(dest.address_mode == GEN_ADDRESS_DIRECT);
    assert(dest.type == GEN_TYPE_F); // Gen6 alu3 only supports float

    // Set destination - using da16 (align16) mode for 3-source
    gen6_insn->inst.bits1.da16.dest_reg_file = 0; // GRF
    gen6_insn->inst.bits1.da16.dest_reg_nr = dest.nr;
    gen6_insn->inst.bits1.da16.dest_subreg_nr = dest.subnr / 4;
    gen6_insn->inst.bits1.da16.dest_writemask = 0xf; // Write all channels

    // Set header
    this->setHeader(insn);
    gen6_insn->inst.header.access_mode = GEN_ALIGN_16;
    gen6_insn->inst.header.execution_size = execution_size;

    // Validate and set source 0
    assert(src0.file == GEN_GENERAL_REGISTER_FILE);
    assert(src0.address_mode == GEN_ADDRESS_DIRECT);
    assert(src0.nr < 128);
    assert(src0.type == GEN_TYPE_F);

    gen6_insn->inst.bits2.da16.src0_swz_x = (NO_SWIZZLE >> 0) & 0x3;
    gen6_insn->inst.bits2.da16.src0_swz_y = (NO_SWIZZLE >> 2) & 0x3;
    gen6_insn->inst.bits2.da16.src0_swz_z = (NO_SWIZZLE >> 4) & 0x3;
    gen6_insn->inst.bits2.da16.src0_swz_w = (NO_SWIZZLE >> 6) & 0x3;
    gen6_insn->inst.bits2.da16.src0_subreg_nr = src0.subnr / 16;
    gen6_insn->inst.bits2.da16.src0_reg_nr = src0.nr;
    gen6_insn->inst.bits2.da16.src0_abs = src0.absolute;
    gen6_insn->inst.bits2.da16.src0_negate = src0.negation;
    gen6_insn->inst.bits2.da16.src0_vert_stride =
        (src0.vstride == GEN_VERTICAL_STRIDE_0) ? 0 : 1;

    // Validate and set source 1
    assert(src1.file == GEN_GENERAL_REGISTER_FILE);
    assert(src1.address_mode == GEN_ADDRESS_DIRECT);
    assert(src1.nr < 128);
    assert(src1.type == GEN_TYPE_F);

    gen6_insn->inst.bits3.da16.src1_swz_x = (NO_SWIZZLE >> 0) & 0x3;
    gen6_insn->inst.bits3.da16.src1_swz_y = (NO_SWIZZLE >> 2) & 0x3;
    gen6_insn->inst.bits3.da16.src1_swz_z = (NO_SWIZZLE >> 4) & 0x3;
    gen6_insn->inst.bits3.da16.src1_swz_w = (NO_SWIZZLE >> 6) & 0x3;
    gen6_insn->inst.bits3.da16.src1_subreg_nr = src1.subnr / 16;
    gen6_insn->inst.bits3.da16.src1_reg_nr = src1.nr;
    gen6_insn->inst.bits3.da16.src1_abs = src1.absolute;
    gen6_insn->inst.bits3.da16.src1_negate = src1.negation;
    gen6_insn->inst.bits3.da16.src1_vert_stride =
        (src1.vstride == GEN_VERTICAL_STRIDE_0) ? 0 : 1;

    // Validate source 2
    assert(src2.file == GEN_GENERAL_REGISTER_FILE);
    assert(src2.address_mode == GEN_ADDRESS_DIRECT);
    assert(src2.nr < 128);
    assert(src2.type == GEN_TYPE_F);

    // Note: Source 2 encoding for Gen6 3-source is in a different location
    // This is a simplified implementation - full implementation would need
    // proper 3-source instruction format handling

    // For SIMD16, emit second half
    if (this->curr.execWidth == 16) {
      GenNativeInstruction q1Insn = *insn;
      insn = this->next(opcode);
      *insn = q1Insn;
      gen6_insn = reinterpret_cast<Gen6NativeInstruction*>(insn);
      gen6_insn->inst.header.quarter_control = GEN_COMPRESSION_Q2;

      // Increment register numbers for second half
      gen6_insn->inst.bits1.da16.dest_reg_nr++;
      if (src0.vstride != GEN_VERTICAL_STRIDE_0)
        gen6_insn->inst.bits2.da16.src0_reg_nr++;
      if (src1.vstride != GEN_VERTICAL_STRIDE_0)
        gen6_insn->inst.bits3.da16.src1_reg_nr++;
      // src2 would also increment if vstride != 0
    }
  }

  /**
   * Helper function to set Gen6 message descriptor for MBlock read/write
   *
   * Gen6 uses different message formats than Gen7. This sets up the
   * message descriptor for media block read/write operations.
   */
  static void setMBlockRWGEN6(GenEncoder *p,
                              GenNativeInstruction *insn,
                              uint32_t bti,
                              uint32_t msg_type,
                              uint32_t msg_length,
                              uint32_t response_length)
  {
    // Gen6 uses render cache for data port operations
    const GenMessageTarget sfid = GEN_SFID_DATAPORT_RENDER;
    p->setMessageDescriptor(insn, sfid, msg_length, response_length);

    // Set Gen6-specific message descriptor fields
    // Note: Gen6 message format is slightly different from Gen7
    // This is a simplified implementation
    Gen6NativeInstruction *gen6_insn = reinterpret_cast<Gen6NativeInstruction*>(insn);
    gen6_insn->inst.bits3.send_gen6.end_of_thread = 0;

    // Message type and BTI would be set in extended message descriptor
    // Gen6 uses different encoding than Gen7 for these fields
  }

  /**
   * Media block read instruction
   *
   * Gen6 MBlock read is similar to Gen7 but uses different message types.
   * Performance may be lower due to less optimized cache hierarchy.
   */
  void Gen6Encoder::MBREAD(GenRegister dst, GenRegister header,
                           uint32_t bti, uint32_t size)
  {
    GenNativeInstruction *insn = this->next(GEN_OPCODE_SEND);
    const uint32_t msg_length = 1;
    const uint32_t response_length = size; // Size in registers

    this->setHeader(insn);
    this->setDst(insn, GenRegister::ud8grf(dst.nr, 0));
    this->setSrc0(insn, GenRegister::ud8grf(header.nr, 0));
    this->setSrc1(insn, GenRegister::immud(0));

    // Use Gen6 message descriptor setup
    // Note: GEN75_P1_MEDIA_BREAD may not be correct for Gen6,
    // would need Gen6-specific message type
    setMBlockRWGEN6(this, insn, bti,
                    0x04, // Gen6 media block read message type (example)
                    msg_length, response_length);
  }

  /**
   * Media block write instruction
   *
   * Gen6 MBlock write is similar to Gen7 but:
   * - Does not support split send (useSends parameter ignored)
   * - Uses different message encoding
   * - May have different size limitations
   */
  void Gen6Encoder::MBWRITE(GenRegister header, GenRegister data,
                            uint32_t bti, uint32_t size, bool useSends)
  {
    // Gen6 does not support split send, ignore useSends parameter
    if (useSends) {
      // Log warning: Gen6 does not support split send
      // Fall back to regular send
    }

    GenNativeInstruction *insn = this->next(GEN_OPCODE_SEND);
    const uint32_t msg_length = 1 + size;
    const uint32_t response_length = 0; // No response for write

    this->setHeader(insn);
    this->setDst(insn, GenRegister::retype(GenRegister::null(), GEN_TYPE_UW));
    this->setSrc0(insn, GenRegister::ud8grf(header.nr, 0));
    this->setSrc1(insn, GenRegister::immud(0));

    // Use Gen6 message descriptor setup
    // Note: Message type would need to be Gen6-specific
    setMBlockRWGEN6(this, insn, bti,
                    0x0A, // Gen6 media block write message type (example)
                    msg_length, response_length);
  }

  /**
   * Get Gen6-specific cache control bits
   *
   * Gen6 has a different cache control mechanism than Gen7.
   * It uses a simpler 2-bit encoding in the message descriptor.
   */
  uint32_t Gen6Encoder::getCacheControlGen6() const
  {
    // Gen6 cache control options:
    // 00b = Use default cache policy
    // 01b = Bypass cache
    // 10b = Write-through cache
    // 11b = Write-back cache

    // For OpenCL workloads, write-back is usually best
    return 0x3; // Write-back cache
  }

  /**
   * Check if instruction opcode is supported on Gen6
   *
   * Some Gen7+ instructions are not available on Gen6.
   */
  bool Gen6Encoder::isSupportedOnGen6(uint32_t opcode) const
  {
    // List of unsupported opcodes on Gen6
    switch (opcode) {
      // Gen7+ specific instructions
      case GEN_OPCODE_BFI1:
      case GEN_OPCODE_BFI2:
        return false; // Bit field insert not supported on Gen6

      // Most basic ALU and control flow instructions are supported
      default:
        return true;
    }
  }

} // namespace gbe
