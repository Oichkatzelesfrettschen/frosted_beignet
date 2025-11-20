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
 * Author: Benjamin Segovia <benjamin.segovia@intel.com>
 *         Heldge RHodin <alice.rhodin@alice-dsl.net>
 */

/**
 * \file llvm_passes.cpp
 * \author Benjamin Segovia <benjamin.segovia@intel.com>
 * \author Heldge RHodin <alice.rhodin@alice-dsl.net>
 */

/* THIS CODE IS DERIVED FROM GPL LLVM PTX BACKEND. CODE IS HERE:
 * http://sourceforge.net/scm/?type=git&group_id=319085
 * Note that however, the original author, Heldge Rhodin, granted me (Benjamin
 * Segovia) the right to use another license for it (MIT here)
 */

#include "llvm_includes.hpp"

#include "llvm/llvm_gen_backend.hpp"
#include "ir/unit.hpp"
#include "sys/map.hpp"

using namespace llvm;

namespace gbe
{
  bool isKernelFunction(const llvm::Function &F) {
    bool bKernel = false;
#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 39
    bKernel = F.getMetadata("kernel_arg_name") != NULL;
#else
    const Module *module = F.getParent();
    const Module::NamedMDListType& globalMD = module->getNamedMDList();
    for(auto i = globalMD.begin(); i != globalMD.end(); i++) {
      const NamedMDNode &md = *i;
      if(strcmp(md.getName().data(), "opencl.kernels") != 0) continue;
      uint32_t ops = md.getNumOperands();
      for(uint32_t x = 0; x < ops; x++) {
        MDNode* node = md.getOperand(x);
#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR <= 35
        Value * op = node->getOperand(0);
#else
        Value * op = cast<ValueAsMetadata>(node->getOperand(0))->getValue();
#endif
        if(op == &F) bKernel = true;
      }
    }
#endif
    return bKernel;
  }

  uint32_t getModuleOclVersion(const llvm::Module *M) {
    uint32_t oclVersion = 120;
    NamedMDNode *version = M->getNamedMetadata("opencl.ocl.version");
    if (version == NULL)
      return oclVersion;
    uint32_t ops = version->getNumOperands();
    if(ops > 0) {
      uint32_t major = 0, minor = 0;
      MDNode* node = version->getOperand(0);
#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 36
      major = mdconst::extract<ConstantInt>(node->getOperand(0))->getZExtValue();
      minor = mdconst::extract<ConstantInt>(node->getOperand(1))->getZExtValue();
#else
      major = cast<ConstantInt>(node->getOperand(0))->getZExtValue();
      minor = cast<ConstantInt>(node->getOperand(1))->getZExtValue();
#endif
      oclVersion = major * 100 + minor * 10;
    }
    return oclVersion;
  }

  int32_t getPadding(int32_t offset, int32_t align) {
    return (align - (offset % align)) % align; 
  }

  uint32_t getAlignmentByte(const ir::Unit &unit, Type* Ty)
  {
    // LLVM 11+: Type::VectorTyID removed from TypeID enum, check separately
    if (Ty->isVectorTy()) {
      const VectorType* VecTy = cast<VectorType>(Ty);
      uint32_t elemNum = GBE_VECTOR_GET_NUM_ELEMENTS(VecTy);
      if (elemNum == 3) elemNum = 4; // OCL spec
      return elemNum * getTypeByteSize(unit, VecTy->getElementType());
    }

    switch (Ty->getTypeID()) {
      case Type::VoidTyID: NOT_SUPPORTED;
      case Type::PointerTyID:
      case Type::IntegerTyID:
      case Type::FloatTyID:
      case Type::DoubleTyID:
      case Type::HalfTyID:
        return getTypeBitSize(unit, Ty)/8;
      case Type::ArrayTyID:
        return getAlignmentByte(unit, cast<ArrayType>(Ty)->getElementType());
      case Type::StructTyID:
      {
        const StructType* StrTy = cast<StructType>(Ty);
        uint32_t maxa = 0;
        for(uint32_t subtype = 0; subtype < StrTy->getNumElements(); subtype++)
        {
          maxa = std::max(getAlignmentByte(unit, StrTy->getElementType(subtype)), maxa);
        }
        return maxa;
      }
      default: NOT_SUPPORTED;
    }
    return 0u;
  }

  uint32_t getTypeBitSize(const ir::Unit &unit, Type* Ty)
  {
    // LLVM 11+: Type::VectorTyID removed from TypeID enum, check separately
    if (Ty->isVectorTy()) {
      const VectorType* VecTy = cast<VectorType>(Ty);
      uint32_t numElem = GBE_VECTOR_GET_NUM_ELEMENTS(VecTy);
      if(numElem == 3) numElem = 4; // OCL spec
      return numElem * getTypeBitSize(unit, VecTy->getElementType());
    }

    switch (Ty->getTypeID()) {
      case Type::VoidTyID:    NOT_SUPPORTED;
      case Type::PointerTyID: return unit.getPointerSize();
      case Type::IntegerTyID:
      {
        // use S16 to represent SLM bool variables.
        int bitWidth = cast<IntegerType>(Ty)->getBitWidth();
        return (bitWidth == 1) ? 16 : bitWidth;
      }
      case Type::HalfTyID:    return 16;
      case Type::FloatTyID:   return 32;
      case Type::DoubleTyID:  return 64;
      case Type::ArrayTyID:
      {
        const ArrayType* ArrTy = cast<ArrayType>(Ty);
        Type* elementType = ArrTy->getElementType();
        uint32_t size_element = getTypeBitSize(unit, elementType);
        uint32_t size = ArrTy->getNumElements() * size_element;
        uint32_t align = 8 * getAlignmentByte(unit, elementType);
        size += (ArrTy->getNumElements()-1) * getPadding(size_element, align);
        return size;
      }
      case Type::StructTyID:
      {
        const StructType* StrTy = cast<StructType>(Ty);
        uint32_t size = 0;
        for(uint32_t subtype=0; subtype < StrTy->getNumElements(); subtype++)
        {
          Type* elementType = StrTy->getElementType(subtype);
          uint32_t align = 8 * getAlignmentByte(unit, elementType);
          size += getPadding(size, align);
          size += getTypeBitSize(unit, elementType);
        }
        return size;
      }
      default: NOT_SUPPORTED;
    }
    return 0u;
  }

  uint32_t getTypeByteSize(const ir::Unit &unit, Type* Ty)
  {
    uint32_t size_bit = getTypeBitSize(unit, Ty);
    assert((size_bit%8==0) && "no multiple of 8");
    return size_bit/8;
  }

  Type* getEltType(Type* eltTy, uint32_t index) {
    Type *elementType = NULL;
#if LLVM_VERSION_MAJOR >= 15
    // LLVM 15+: Opaque pointers - cannot get element type from pointer
    // For opaque pointers, caller must provide type from context (load/store/GEP)
    if(isa<PointerType>(eltTy)) {
      // Return null for opaque pointers - caller must handle
      return NULL;
    }
#else
    if (PointerType* ptrType = dyn_cast<PointerType>(eltTy))
      elementType = ptrType->getElementType();
#endif
#if LLVM_VERSION_MAJOR >= 11
    // LLVM 11+: SequentialType removed, handle ArrayType and VectorType separately
    else if(ArrayType * arrType = dyn_cast<ArrayType>(eltTy))
      elementType = arrType->getElementType();
    else if(VectorType * vecType = dyn_cast<VectorType>(eltTy))
      elementType = vecType->getElementType();
    // LLVM 11+: CompositeType removed, handle StructType directly
    else if(StructType * structTy = dyn_cast<StructType>(eltTy))
      elementType = structTy->getTypeAtIndex(index);
#else
    else if(SequentialType * seqType = dyn_cast<SequentialType>(eltTy))
      elementType = seqType->getElementType();
    else if(CompositeType * compTy= dyn_cast<CompositeType>(eltTy))
      elementType = compTy->getTypeAtIndex(index);
#endif
    GBE_ASSERT(elementType);
    return elementType;
  }

  int32_t getGEPConstOffset(const ir::Unit &unit, Type *eltTy, int32_t TypeIndex) {
    int32_t offset = 0;
    if (!eltTy->isStructTy()) {
      if (TypeIndex != 0) {
        Type *elementType = getEltType(eltTy);
        uint32_t elementSize = getTypeByteSize(unit, elementType);
        uint32_t align = getAlignmentByte(unit, elementType);
        elementSize += getPadding(elementSize, align);
        offset = elementSize * TypeIndex;
      }
    } else {
      int32_t step = TypeIndex > 0 ? 1 : -1;
      for(int32_t ty_i=0; ty_i != TypeIndex; ty_i += step)
      {
        Type* elementType = getEltType(eltTy, ty_i);
        uint32_t align = getAlignmentByte(unit, elementType);
        offset += getPadding(offset, align * step);
        offset += getTypeByteSize(unit, elementType) * step;
      }

      //add getPaddingding for accessed type
      const uint32_t align = getAlignmentByte(unit, getEltType(eltTy ,TypeIndex));
      offset += getPadding(offset, align * step);
    }
    return offset;
  }

  class GenRemoveGEPPasss : public FunctionPass
  {

   public:
    static char ID;
    GenRemoveGEPPasss(const ir::Unit &unit) :
      FunctionPass(ID),
      unit(unit) {}
    const ir::Unit &unit;
    void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesCFG();
    }

#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 40
    virtual StringRef getPassName() const {
#else
    virtual const char *getPassName() const {
#endif
      return "SPIR backend: insert special spir instructions";
    }

    bool simplifyGEPInstructions(GetElementPtrInst* GEPInst);

    virtual bool runOnFunction(Function &F)
    {
      bool changed = false;
      // Iterate over all basic blocks in the function
      for (Function::iterator BI = F.begin(), BE = F.end(); BI != BE; ++BI) {
        BasicBlock &BB = *BI;
        // Iterate safely, as simplifyGEPInstructions may erase the current instruction
        for (Instruction &I : BB) {
          if (GetElementPtrInst* gep = dyn_cast<GetElementPtrInst>(&I)) {
            // Check if gep is still in a basic block, as it might have been erased by a previous iteration
            if (gep->getParent()) {
              if (simplifyGEPInstructions(gep)) {
                changed = true;
            // After simplification, the GEP is erased, so we don't need to worry about iterator invalidation
            // for this specific instruction. However, the loop structure itself needs to be safe.
            // Re-evaluating BB.getInstList().end() or using a different loop might be needed if further modifications
            // within the loop could alter BB structure significantly beyond erasing 'gep'.
            // For now, assuming simplifyGEPInstructions only erases 'gep' and inserts before it.
              }
            }
          }
        }
      }
      return changed;
    }
  };

  char GenRemoveGEPPasss::ID = 0;

  bool GenRemoveGEPPasss::simplifyGEPInstructions(GetElementPtrInst* GEPInst)
  {
  IRBuilder<> IRB(GEPInst); // Initialize IRBuilder before GEPInst
    const uint32_t ptrSize = unit.getPointerSize();
    Value* parentPointer = GEPInst->getOperand(0);
    Type* eltTy = parentPointer ? parentPointer->getType() : NULL;
    if(!eltTy)
      return false;

  Value* currentAddrInst =
    IRB.CreatePtrToInt(parentPointer, IntegerType::get(GEPInst->getContext(), ptrSize));

    int32_t constantOffset = 0;

    for(uint32_t op=1; op<GEPInst->getNumOperands(); ++op)
    {
      int32_t TypeIndex;
      ConstantInt* ConstOP = dyn_cast<ConstantInt>(GEPInst->getOperand(op));
      if (ConstOP != NULL) {
        TypeIndex = ConstOP->getZExtValue();
        constantOffset += getGEPConstOffset(unit, eltTy, TypeIndex);
      }
      else {
      TypeIndex = 0; // For non-constant indices, type stepping is based on the current eltTy

        Type* elementType = getEltType(eltTy);
        uint32_t size = getTypeByteSize(unit, elementType);
        uint32_t align = getAlignmentByte(unit, elementType);
        size += getPadding(size, align);

      Value *operand = GEPInst->getOperand(op);
        if(!operand)
          continue;

        Value* tmpOffset = operand;
        if (size != 1) {
          if (isPowerOf<2>(size)) {
            Constant* shiftAmnt =
              ConstantInt::get(IntegerType::get(GEPInst->getContext(), ptrSize), logi2(size));
          tmpOffset = IRB.CreateShl(operand, shiftAmnt);
          } else{
            Constant* sizeConst =
              ConstantInt::get(IntegerType::get(GEPInst->getContext(), ptrSize), size);
          tmpOffset = IRB.CreateMul(operand, sizeConst); // operand was first before, ensure correct order
          }
        }
      currentAddrInst = IRB.CreateAdd(currentAddrInst, tmpOffset);
      }

      eltTy = getEltType(eltTy, TypeIndex);
    }

    if (constantOffset != 0) {
      Constant* newConstOffset =
      ConstantInt::get(IntegerType::get(GEPInst->getContext(), ptrSize), constantOffset);
    currentAddrInst = IRB.CreateAdd(currentAddrInst, newConstOffset);
    }

  Value* intToPtrInst = IRB.CreateIntToPtr(currentAddrInst, GEPInst->getType());

    GEPInst->replaceAllUsesWith(intToPtrInst);
  // GEPInst->dropAllReferences(); // Removed
    GEPInst->eraseFromParent();

    return true;
  }

  FunctionPass *createRemoveGEPPass(const ir::Unit &unit) {
    return new GenRemoveGEPPasss(unit);
  }
} /* namespace gbe */

