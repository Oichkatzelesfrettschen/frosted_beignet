/*
 * Copyright © 2012 Intel Corporation
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
 */

/**
 * \file gen9_context.cpp
 */

#include "backend/gen9_context.hpp"
#include "backend/context_emit_helpers.hpp" // Added for helpers
#include "backend/gen_insn_selection.hpp"
#include "backend/gen_program.hpp"

namespace gbe
{
  void Gen9Context::newSelection(void) {
    this->sel = GBE_NEW(Selection9, *this);
  }

  void Gen9Context::emitBarrierInstruction(const SelectionInstruction &insn) {
    const GenRegister src = ra->genReg(insn.src(0));
    const GenRegister fenceDst = ra->genReg(insn.dst(0));
    uint32_t barrierType = insn.extra.barrierType;
    const GenRegister barrierId = ra->genReg(GenRegister::ud1grf(ir::ocl::barrierid));
    bool imageFence = barrierType & ir::SYNC_IMAGE_FENCE;

    if (barrierType & ir::SYNC_GLOBAL_READ_FENCE) {
      p->FENCE(fenceDst, imageFence);
      p->MOV(fenceDst, fenceDst);
    }
    p->push();
      // As only the payload.2 is used and all the other regions are ignored
      // SIMD8 mode here is safe.
      p->curr.execWidth = 8;
      p->curr.physicalFlag = 0;
      p->curr.noMask = 1;
      // Copy barrier id from r0.
      p->AND(src, barrierId, GenRegister::immud(0x8f000000));
      // A barrier is OK to start the thread synchronization *and* SLM fence
      p->BARRIER(src);
      p->curr.execWidth = 1;
      // Now we wait for the other threads
      p->curr.predicate = GEN_PREDICATE_NONE;
      p->WAIT();
    p->pop();
    if (imageFence) {
      p->FLUSH_SAMPLERCACHE(fenceDst);
      p->MOV(fenceDst, fenceDst);
    }
  }

  void Gen9Context::emitImeInstruction(const SelectionInstruction &insn) {
    const GenRegister dst = ra->genReg(insn.dst(0));
    const unsigned int msg_type = insn.extra.ime_msg_type;

    GBE_ASSERT(msg_type == 1 || msg_type == 2 || msg_type == 3);
    uint32_t execWidth_org = p->curr.execWidth;
    int virt_pld_len;
    int phi_pld_len = 0;
    int virt_rsp_len;

#define PHI_SIC_PAYLOAD_LEN 8
#define PHI_IME_PAYLOAD_LEN 6
#define PHI_VME_WRITEBACK_LEN 7

    if(msg_type == 1 || msg_type == 2 || msg_type == 3)
      virt_rsp_len = PHI_VME_WRITEBACK_LEN;
    if(msg_type == 1 || msg_type == 3)
      phi_pld_len = PHI_SIC_PAYLOAD_LEN;
    else if(msg_type == 2)
      phi_pld_len = PHI_IME_PAYLOAD_LEN;
    if(execWidth_org == 8)
      virt_pld_len = phi_pld_len;
    else if(execWidth_org == 16)
      virt_pld_len = (phi_pld_len + 1) / 2;
    p->push();
    p->curr.predicate = GEN_PREDICATE_NONE;
    p->curr.noMask = 1;
    p->curr.execWidth = 1;
    /* Now cl_intel_device_side_avc_motion_estimation is impelemented based on simd16 mode.
     * So fall back to simd8 is not acceptable now.
     * */
    GBE_ASSERT(execWidth_org == 16);
    /* Use MOV to Setup bits of payload: mov payload value stored in insn.src(x) to
     * consecutive payload grf.
     * In simd8 mode, one virtual grf register map to one physical grf register. But
     * in simd16 mode, one virtual grf register map to two physical grf registers.
     * So we should treat them differently.
     * */
    if(execWidth_org == 8){
      for(int i=0; i < virt_pld_len; i++){
        GenRegister payload_grf = ra->genReg(insn.dst(virt_rsp_len+i));
        payload_grf.vstride = GEN_VERTICAL_STRIDE_0;
        payload_grf.width = GEN_WIDTH_1;
        payload_grf.hstride = GEN_HORIZONTAL_STRIDE_0;
        payload_grf.subphysical = 1;
        for(int j=0; j < 8; j++){
          payload_grf.subnr = (7 - j) * typeSize(GEN_TYPE_UD);
          GenRegister payload_val = ra->genReg(insn.src(i*8+j));
          payload_val.vstride = GEN_VERTICAL_STRIDE_0;
          payload_val.width = GEN_WIDTH_1;
          payload_val.hstride = GEN_HORIZONTAL_STRIDE_0;

          p->MOV(payload_grf, payload_val);
        }
      }
    }
    else if(execWidth_org == 16){
      for(int i=0; i < virt_pld_len; i++){
        int nr_num = 2;
        if( (i == virt_pld_len-1) && (phi_pld_len%2 == 1) )
          nr_num = 1;
        for(int k = 0; k < nr_num; k++){
          GenRegister payload_grf = ra->genReg(insn.dst(virt_rsp_len+i));
          payload_grf.nr += k;
          payload_grf.vstride = GEN_VERTICAL_STRIDE_0;
          payload_grf.width = GEN_WIDTH_1;
          payload_grf.hstride = GEN_HORIZONTAL_STRIDE_0;
          payload_grf.subphysical = 1;
          for(int j=0; j < 8; j++){
            payload_grf.subnr = (7 - j) * typeSize(GEN_TYPE_UD);
            GenRegister payload_val = ra->genReg(insn.src(i*16+k*8+j));
            payload_val.vstride = GEN_VERTICAL_STRIDE_0;
            payload_val.width = GEN_WIDTH_1;
            payload_val.hstride = GEN_HORIZONTAL_STRIDE_0;

            p->MOV(payload_grf, payload_val);
          }
        }
      }
    }
    p->pop();

#undef PHI_SIC_PAYLOAD_LEN
#undef PHI_IME_PAYLOAD_LEN
#undef PHI_VME_WRITEBACK_LEN

    p->push();
    p->curr.predicate = GEN_PREDICATE_NONE;
    p->curr.noMask = 1;
    p->curr.execWidth = 1;
    GenRegister payload_did = GenRegister::retype(ra->genReg(insn.dst(virt_rsp_len)), GEN_TYPE_UB);
    payload_did.vstride = GEN_VERTICAL_STRIDE_0;
    payload_did.width = GEN_WIDTH_1;
    payload_did.hstride = GEN_HORIZONTAL_STRIDE_0;
    payload_did.subphysical = 1;
    payload_did.subnr = 20 * typeSize(GEN_TYPE_UB);
    GenRegister grf0 = GenRegister::ub1grf(0, 20);
    p->MOV(payload_did, grf0);
    p->pop();

    const GenRegister msgPayload = ra->genReg(insn.dst(virt_rsp_len));
    const unsigned char bti = insn.getbti();
    p->IME(bti, dst, msgPayload, msg_type);
  }

  void BxtContext::newSelection(void) {
    this->sel = GBE_NEW(SelectionBxt, *this);
  }

  void BxtContext::calculateFullU64MUL(GenRegister src0, GenRegister src1, GenRegister dst_h,
                                             GenRegister dst_l, GenRegister s0l_s1h, GenRegister s0h_s1l)
  {
    // Note: BxtContext inherits from Gen9Context, which inherits from Gen8Context.
    // The call to unpacked_ud inside the helper will correctly resolve to Gen8Context::unpacked_ud.
    ContextEmitHelpers::calculateFullU64MUL_chv_bxt(*this, src0, src1, dst_h, dst_l, s0l_s1h, s0h_s1l);
  }

  void BxtContext::emitI64MULInstruction(const SelectionInstruction &insn)
  {
    ContextEmitHelpers::emitI64MULInstruction_shared(*this, insn);
  }

  void BxtContext::setA0Content(uint16_t new_a0[16], uint16_t max_offset, int sz) {
    ContextEmitHelpers::setA0Content_shared(*this, new_a0, max_offset, sz);
  }

  void BxtContext::emitStackPointer(void) {
    ContextEmitHelpers::emitStackPointer_chv_bxt(*this);
  }

  void KblContext::newSelection(void) {
    this->sel = GBE_NEW(SelectionKbl, *this);
  }

  void GlkContext::newSelection(void) {
    this->sel = GBE_NEW(SelectionGlk, *this);
  }

}
