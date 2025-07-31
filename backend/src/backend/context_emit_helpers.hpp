// backend/src/backend/context_emit_helpers.hpp
#ifndef __GBE_CONTEXT_EMIT_HELPERS_HPP__
#define __GBE_CONTEXT_EMIT_HELPERS_HPP__

#include <cstdint> // For uint16_t, int

// Forward declarations to minimize header includes
namespace gbe {
    class GenContext;
    class SelectionInstruction;
    class GenRegister;
    class GenEncoder; // For p-> access
    class GenRegAllocator; // For ra-> access
    namespace ir { // For ir::ocl constants
        class Kernel;
    }
}

namespace gbe {

class ContextEmitHelpers {
public:
    static void emitI64MULInstruction_shared(GenContext& ctx, const SelectionInstruction &insn);
    static void setA0Content_shared(GenContext& ctx, uint16_t new_a0[16], uint16_t max_offset, int sz);
    // Note: The Gen8Context::calculateFullU64MUL is virtual and overridden.
    // ChvContext and BxtContext provide identical overrides.
    // This helper will implement that specific override logic.
    static void calculateFullU64MUL_chv_bxt(GenContext& ctx, GenRegister src0, GenRegister src1, GenRegister dst_h, GenRegister dst_l, GenRegister s0l_s1h, GenRegister s0h_s1l);
    static void emitStackPointer_chv_bxt(GenContext& ctx);
};

} // namespace gbe
#endif // __GBE_CONTEXT_EMIT_HELPERS_HPP__
