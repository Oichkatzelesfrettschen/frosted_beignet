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
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Adapted for Gen6 (Sandy Bridge) by Frosted Beignet Team
  */

#ifndef __GEN6_INSTRUCTION_HPP__
#define __GEN6_INSTRUCTION_HPP__

#include <cstdint>

/**
 * Gen6 (Sandy Bridge) Native Instruction Format
 *
 * Sandy Bridge uses a 128-bit (16-byte) instruction format similar to Gen7
 * but with some architectural differences:
 * - Fewer execution units (max 12 vs 16)
 * - Different cache control options
 * - Limited atomics support
 * - No native OpenCL 1.2 features
 */

union Gen6NativeInstruction
{
  struct {
    // DWord 0 - Instruction header
    struct {
      uint32_t opcode:7;              // Instruction opcode
      uint32_t pad:1;                 // Reserved
      uint32_t access_mode:1;         // 0=align1, 1=align16
      uint32_t mask_control:1;        // Channel mask control
      uint32_t dependency_control:2;  // Dependency control
      uint32_t quarter_control:2;     // Quarter control for SIMD32
      uint32_t thread_control:2;      // Thread control
      uint32_t predicate_control:4;   // Predication control
      uint32_t predicate_inverse:1;   // Invert predicate
      uint32_t execution_size:3;      // SIMD execution size
      uint32_t destreg_or_condmod:4;  // Destination register or condition modifier
      uint32_t acc_wr_control:1;      // Accumulator write control
      uint32_t cmpt_control:1;        // Compaction control
      uint32_t debug_control:1;       // Debug control
      uint32_t saturate:1;            // Saturation enable
    } header;

    // DWord 1 - Destination and source register file/type info
    union {
      // Direct addressing mode, align1
      struct {
        uint32_t dest_reg_file:2;      // Destination register file
        uint32_t dest_reg_type:3;      // Destination type
        uint32_t src0_reg_file:2;      // Source 0 register file
        uint32_t src0_reg_type:3;      // Source 0 type
        uint32_t src1_reg_file:2;      // Source 1 register file
        uint32_t src1_reg_type:3;      // Source 1 type
        uint32_t nib_ctrl:1;           // Nibble control
        uint32_t dest_subreg_nr:5;     // Destination sub-register number
        uint32_t dest_reg_nr:8;        // Destination register number
        uint32_t dest_horiz_stride:2;  // Destination horizontal stride
        uint32_t dest_address_mode:1;  // 0=direct, 1=indirect
      } da1;

      // Indirect addressing mode, align1
      struct {
        uint32_t dest_reg_file:2;
        uint32_t dest_reg_type:3;
        uint32_t src0_reg_file:2;
        uint32_t src0_reg_type:3;
        uint32_t src1_reg_file:2;
        uint32_t src1_reg_type:3;
        uint32_t nib_ctrl:1;
        int dest_indirect_offset:10;   // Offset against dereferenced address
        uint32_t dest_subreg_nr:3;     // Sub-register for address register
        uint32_t dest_horiz_stride:2;
        uint32_t dest_address_mode:1;  // Must be 1 for indirect
      } ia1;

      // Direct addressing mode, align16
      struct {
        uint32_t dest_reg_file:2;
        uint32_t dest_reg_type:3;
        uint32_t src0_reg_file:2;
        uint32_t src0_reg_type:3;
        uint32_t src1_reg_file:2;
        uint32_t src1_reg_type:3;
        uint32_t nib_ctrl:1;
        uint32_t dest_subreg_nr:4;     // Destination sub-register
        uint32_t dest_reg_nr:8;
        uint32_t pad0:2;
        uint32_t dest_writemask:4;     // Write mask for align16
        uint32_t dest_address_mode:1;
      } da16;

      // Indirect addressing mode, align16
      struct {
        uint32_t dest_reg_file:2;
        uint32_t dest_reg_type:3;
        uint32_t src0_reg_file:2;
        uint32_t src0_reg_type:3;
        uint32_t src1_reg_file:2;
        uint32_t src1_reg_type:3;
        uint32_t nib_ctrl:1;
        uint32_t dest_subreg_nr:4;
        uint32_t dest_reg_nr:8;
        uint32_t pad0:2;
        uint32_t dest_writemask:4;
        uint32_t dest_address_mode:1;
      } ia16;

      // Generic branch instruction format
      struct {
        uint32_t dest_reg_file:2;
        uint32_t flag_reg_nr:1;        // Flag register number for Gen6
        uint32_t pad0:1;
        uint32_t src0_abs:1;           // Source 0 absolute value
        uint32_t src0_negate:1;        // Source 0 negate
        uint32_t src1_reg_file:2;
        uint32_t src1_reg_type:3;
        uint32_t pad1:1;
        uint32_t dest_subreg_nr:5;
        uint32_t dest_reg_nr:8;
        uint32_t dest_horiz_stride:2;
        uint32_t dest_address_mode:1;
      } branch_gen6;
    } bits1;

    // DWord 2 - Source 0 operand
    union {
      // Direct register addressing
      struct {
        uint32_t src0_subreg_nr:5;
        uint32_t src0_reg_nr:8;
        uint32_t src0_abs:1;
        uint32_t src0_negate:1;
        uint32_t src0_address_mode:1;
        uint32_t src0_horiz_stride:2;
        uint32_t src0_width:3;
        uint32_t src0_vert_stride:4;
        uint32_t flag_reg_nr:1;        // Gen6 flag register
        uint32_t pad0:6;
      } da1;

      // Indirect register addressing
      struct {
        int src0_indirect_offset:10;
        uint32_t src0_subreg_nr:3;
        uint32_t src0_abs:1;
        uint32_t src0_negate:1;
        uint32_t src0_address_mode:1;  // Must be 1
        uint32_t src0_horiz_stride:2;
        uint32_t src0_width:3;
        uint32_t src0_vert_stride:4;
        uint32_t flag_reg_nr:1;
        uint32_t pad0:6;
      } ia1;

      // Immediate value (32-bit)
      struct {
        int imm32:32;
      } imm32;

      // Float immediate
      struct {
        float f;
      } immf;

      // Align16 direct addressing
      struct {
        uint32_t src0_swz_x:2;
        uint32_t src0_swz_y:2;
        uint32_t src0_swz_z:2;
        uint32_t src0_swz_w:2;
        uint32_t src0_writemask:4;
        uint32_t src0_subreg_nr:1;
        uint32_t src0_reg_nr:8;
        uint32_t src0_abs:1;
        uint32_t src0_negate:1;
        uint32_t src0_address_mode:1;
        uint32_t src0_vert_stride:4;   // Replicated for swizzle control
        uint32_t flag_reg_nr:1;
        uint32_t pad0:2;
      } da16;
    } bits2;

    // DWord 3 - Source 1 operand
    union {
      // Direct register addressing
      struct {
        uint32_t src1_subreg_nr:5;
        uint32_t src1_reg_nr:8;
        uint32_t src1_abs:1;
        uint32_t src1_negate:1;
        uint32_t src1_address_mode:1;
        uint32_t src1_horiz_stride:2;
        uint32_t src1_width:3;
        uint32_t src1_vert_stride:4;
        uint32_t pad0:7;
      } da1;

      // Indirect register addressing
      struct {
        int src1_indirect_offset:10;
        uint32_t src1_subreg_nr:3;
        uint32_t src1_abs:1;
        uint32_t src1_negate:1;
        uint32_t src1_address_mode:1;
        uint32_t src1_horiz_stride:2;
        uint32_t src1_width:3;
        uint32_t src1_vert_stride:4;
        uint32_t pad0:7;
      } ia1;

      // Immediate value (32-bit)
      struct {
        int imm32:32;
      } imm32;

      // Float immediate
      struct {
        float f;
      } immf;

      // Align16 direct addressing
      struct {
        uint32_t src1_swz_x:2;
        uint32_t src1_swz_y:2;
        uint32_t src1_swz_z:2;
        uint32_t src1_swz_w:2;
        uint32_t src1_writemask:4;
        uint32_t src1_subreg_nr:1;
        uint32_t src1_reg_nr:8;
        uint32_t src1_abs:1;
        uint32_t src1_negate:1;
        uint32_t src1_address_mode:1;
        uint32_t src1_vert_stride:4;
        uint32_t pad0:3;
      } da16;

      // Branch instructions
      struct {
        uint32_t jip:32;               // Jump instruction pointer
      } branch;

      // Gen6 specific send instruction
      struct {
        uint32_t pad:19;
        uint32_t end_of_thread:1;
        uint32_t pad1:12;
      } send_gen6;
    } bits3;
  } inst;

  uint32_t ud[4];                      // Access as array of uint32_t
  int32_t  d[4];                       // Access as array of int32_t
  float    f[4];                       // Access as array of float
};

// Compile-time size check
static_assert(sizeof(Gen6NativeInstruction) == 16,
              "Gen6NativeInstruction must be exactly 128 bits (16 bytes)");

#endif /* __GEN6_INSTRUCTION_HPP__ */
