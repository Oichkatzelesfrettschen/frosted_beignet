// backend/src/backend/context_emit_helpers.cpp
#include "backend/context_emit_helpers.hpp"
#include "backend/gen_context.hpp" // For GenContext methods and members like p, ra, kernel, fn, simdWidth
#include "backend/gen8_context.hpp" // For Gen8Context::unpacked_ud specifically
#include "backend/gen_encoder.hpp"   // For GenEncoder (ctx.p) methods
#include "backend/gen_insn_selection.hpp" // For SelectionInstruction
#include "backend/gen_reg_allocation.hpp" // For GenRegAllocator (ctx.ra)
#include "backend/gen_register.hpp" // For GenRegister
#include "ir/function.hpp"      // For ir::Function (ctx.fn)
#include "ir/kernel.hpp"        // For ir::Kernel (ctx.kernel)
#include "ir/value.hpp"         // For ir::ocl constants if any are used directly

namespace gbe {

// Taken from ChvContext::emitI64MULInstruction in gen8_context.cpp
void ContextEmitHelpers::emitI64MULInstruction_shared(GenContext& ctx, const SelectionInstruction &insn) {
    GenRegAllocator *ra = ctx.ra; // Get reg allocator from context
    GenEncoder *p = ctx.p;       // Get encoder from context

    GenRegister src0 = ra->genReg(insn.src(0));
    GenRegister src1 = ra->genReg(insn.src(1));
    GenRegister dst = ra->genReg(insn.dst(0));
    GenRegister res = ra->genReg(insn.dst(1));

    src0.type = src1.type = GEN_TYPE_UD;
    dst.type = GEN_TYPE_UL;
    res.type = GEN_TYPE_UL;

    /* Low 32 bits X low 32 bits. */
    GenRegister s0l = Gen8Context::unpacked_ud(src0); // Use static method from Gen8Context
    GenRegister s1l = Gen8Context::unpacked_ud(src1); // Use static method from Gen8Context
    p->MUL(dst, s0l, s1l);

    /* Low 32 bits X high 32 bits. */
    GenRegister s1h = Gen8Context::unpacked_ud(res); // Use static method from Gen8Context
    p->MOV(s1h, Gen8Context::unpacked_ud(src1, 1)); // Use static method from Gen8Context

    p->MUL(res, s0l, s1h);
    p->SHL(res, res, GenRegister::immud(32));
    p->ADD(dst, dst, res);

    /* High 32 bits X low 32 bits. */
    GenRegister s0h = Gen8Context::unpacked_ud(res); // Use static method from Gen8Context
    p->MOV(s0h, Gen8Context::unpacked_ud(src0, 1)); // Use static method from Gen8Context

    p->MUL(res, s0h, s1l);
    p->SHL(res, res, GenRegister::immud(32));
    p->ADD(dst, dst, res);
}

// Taken from ChvContext::setA0Content in gen8_context.cpp
void ContextEmitHelpers::setA0Content_shared(GenContext& ctx, uint16_t new_a0[16], uint16_t max_offset, int sz) {
    GenEncoder *p = ctx.p; // Get encoder from context

    if (sz == 0)
      sz = 16;
    GBE_ASSERT(sz%4 == 0);
    GBE_ASSERT(new_a0[0] >= 0 && new_a0[0] < 4096);

    p->push();
    p->curr.execWidth = 1;
    p->curr.predicate = GEN_PREDICATE_NONE;
    p->curr.noMask = 1;
    for (int i = 0; i < sz/2; i++) {
      p->MOV(GenRegister::retype(GenRegister::addr1(i*2), GEN_TYPE_UD),
             GenRegister::immud(new_a0[i*2 + 1] << 16 | new_a0[i*2]));
    }
    p->pop();
}

// Taken from ChvContext::calculateFullU64MUL in gen8_context.cpp
void ContextEmitHelpers::calculateFullU64MUL_chv_bxt(GenContext& ctx, GenRegister src0, GenRegister src1, GenRegister dst_h, GenRegister dst_l, GenRegister s0l_s1h, GenRegister s0h_s1l) {
    GenEncoder *p = ctx.p; // Get encoder from context

    src0.type = src1.type = GEN_TYPE_UD;
    dst_h.type = dst_l.type = GEN_TYPE_UL;
    s0l_s1h.type = s0h_s1l.type = GEN_TYPE_UL;

    GenRegister s0l = Gen8Context::unpacked_ud(src0); // Use static method from Gen8Context
    GenRegister s1l = Gen8Context::unpacked_ud(src1); // Use static method from Gen8Context
    GenRegister s0h = Gen8Context::unpacked_ud(s0l_s1h); //s0h only used before s0l_s1h, reuse s0l_s1h
    GenRegister s1h = Gen8Context::unpacked_ud(dst_l);   //s1h only used before dst_l, reuse dst_l

    p->MOV(s0h, GenRegister::offset(s0l, 0, 4));
    p->MOV(s1h, GenRegister::offset(s1l, 0, 4));

    /* High 32 bits X High 32 bits. */
    p->MUL(dst_h, s0h, s1h);
    /* High 32 bits X low 32 bits. */
    p->MUL(s0h_s1l, s0h, s1l);
    /* Low 32 bits X high 32 bits. */
    p->MUL(s0l_s1h, s0l, s1h);
    /* Low 32 bits X low 32 bits. */
    p->MUL(dst_l, s0l, s1l);

    GenRegister s0l_s1h_l = Gen8Context::unpacked_ud(s0l_s1h); // Use static method from Gen8Context
    p->ADD(s0h_s1l, s0h_s1l, s0l_s1h_l);

    p->SHR(s0l_s1h, s0l_s1h, GenRegister::immud(32));
    GenRegister s0l_s1h_h = Gen8Context::unpacked_ud(s0l_s1h); // Use static method from Gen8Context
    p->ADD(dst_h, dst_h, s0l_s1h_h);

    GenRegister dst_l_h_temp = Gen8Context::unpacked_ud(s0l_s1h); // Reuse s0l_s1h as temp (naming it dst_l_h_temp to avoid confusion with dst_l_h from Gen8 original)
    p->MOV(dst_l_h_temp, Gen8Context::unpacked_ud(dst_l, 1)); // Use static method from Gen8Context
    p->ADD(s0h_s1l, s0h_s1l, dst_l_h_temp);

    GenRegister tmp = s0l_s1h; // Reuse s0l_s1h as temp

    p->SHL(tmp, s0h_s1l, GenRegister::immud(32));
    GenRegister tmp_unpacked = Gen8Context::unpacked_ud(tmp, 1); // Use static method from Gen8Context
    p->MOV(Gen8Context::unpacked_ud(dst_l, 1), tmp_unpacked); // Use static method from Gen8Context

    p->SHR(tmp, s0h_s1l, GenRegister::immud(32));
    p->ADD(dst_h, dst_h, tmp);
}

// Taken from ChvContext::emitStackPointer in gen8_context.cpp
void ContextEmitHelpers::emitStackPointer_chv_bxt(GenContext& ctx) {
    GenRegAllocator *ra = ctx.ra; // Get reg allocator from context
    GenEncoder *p = ctx.p;       // Get encoder from context
    const ir::Kernel* kernel = ctx.kernel; // Get kernel from context
    const ir::Function& fn = ctx.getFunction(); // Get function from context

    using namespace ir;

    // Only emit stack pointer computation if we use a stack
    if (kernel->getStackSize() == 0)
      return;

    // Check that everything is consistent in the kernel code
    const uint32_t perLaneSize = kernel->getStackSize();
    GBE_ASSERT(perLaneSize > 0);

    const GenRegister selStackPtr = ctx.simdWidth == 8 ? // Use ctx.simdWidth
      GenRegister::ud8grf(ir::ocl::stackptr) :
      GenRegister::ud16grf(ir::ocl::stackptr);
    const GenRegister stackptr_reg = ra->genReg(selStackPtr); // Renamed to avoid conflict with variable name 'stackptr' in original code if it was a type
    // borrow block ip as temporary register as we will
    // initialize block ip latter.
    const GenRegister tmpReg = GenRegister::retype(GenRegister::vec1(ctx.getBlockIP()), GEN_TYPE_UW); // Use ctx.getBlockIP()
    const GenRegister tmpReg_ud = GenRegister::retype(tmpReg, GEN_TYPE_UD);

    ctx.loadLaneID(stackptr_reg); // Use ctx.loadLaneID()

    p->push();
      p->curr.execWidth = 1;
      p->curr.predicate = GEN_PREDICATE_NONE;
      p->AND(tmpReg, GenRegister::ud1grf(0,5), GenRegister::immuw(0x1ff)); //threadId
      p->MUL(tmpReg, tmpReg, GenRegister::immuw(ctx.simdWidth));  //threadId * simdWidth
      p->curr.execWidth = ctx.simdWidth;
      p->ADD(stackptr_reg, GenRegister::unpacked_uw(stackptr_reg), tmpReg);  //threadId * simdWidth + laneId, must < 64K
      p->curr.execWidth = 1;
      p->MOV(tmpReg_ud, GenRegister::immud(perLaneSize));
      p->curr.execWidth = ctx.simdWidth;
      p->MUL(stackptr_reg, tmpReg_ud, GenRegister::unpacked_uw(stackptr_reg)); // (threadId * simdWidth + laneId)*perLaneSize
      if (fn.getPointerFamily() == ir::FAMILY_QWORD) {
        const GenRegister selStackPtr2 = ctx.simdWidth == 8 ? // Use ctx.simdWidth
          GenRegister::ul8grf(ir::ocl::stackptr) :
          GenRegister::ul16grf(ir::ocl::stackptr);
        GenRegister stackptr2_reg = ra->genReg(selStackPtr2); // Renamed
        GenRegister sp = GenRegister::unpacked_ud(stackptr2_reg.nr, stackptr2_reg.subnr);
        int currentSimdWidth = p->curr.execWidth; // Renamed from simdWidth to avoid conflict
        if (currentSimdWidth == 16) {
          p->curr.execWidth = 8;
          p->curr.quarterControl = GEN_COMPRESSION_Q2;
          p->MOV(GenRegister::Qn(sp, 1), GenRegister::Qn(stackptr_reg,1));
          p->MOV(GenRegister::Qn(stackptr2_reg, 1), GenRegister::Qn(sp,1));
        }
        p->curr.quarterControl = GEN_COMPRESSION_Q1;
        p->MOV(sp, stackptr_reg);
        p->MOV(stackptr2_reg, sp);
      }
    p->pop();
}

} // namespace gbe
