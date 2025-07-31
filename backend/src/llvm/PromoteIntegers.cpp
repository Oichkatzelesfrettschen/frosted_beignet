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

// Copyright (c) 2003-2014 University of Illinois at Urbana-Champaign.
// All rights reserved.
//
// Developed by:
//
//    LLVM Team
//
//    University of Illinois at Urbana-Champaign
//
//    http://llvm.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal with
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimers.
//
//   * Redistributions in binary form must reproduce the above copyright notice,
//      this list of conditions and the following disclaimers in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the names of the LLVM Team, University of Illinois at
//      Urbana-Champaign, nor the names of its contributors may be used to
//      endorse or promote products derived from this Software without specific
//      prior written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
// SOFTWARE.

//===- PromoteIntegers.cpp - Promote illegal integers for PNaCl ABI -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License.
//
// A limited set of transformations to promote illegal-sized int types.
//
//===----------------------------------------------------------------------===//
//
// Legal sizes are currently 1, 8, 16, 32, 64 (and higher, see note below).
// Operations on illegal integers are changed to operate on the next-higher
// legal size.
// It maintains no invariants about the upper bits (above the size of the
// original type); therefore before operations which can be affected by the
// value of these bits (e.g. cmp, select, lshr), the upper bits of the operands
// are cleared.
//
// Limitations:
// 1) It can't change function signatures or global variables
// 2) It won't promote (and can't expand) types larger than i64
// 3) Doesn't support div operators
// 4) Doesn't handle arrays or structs with illegal types
// 5) Doesn't handle constant expressions (it also doesn't produce them, so it
//    can run after ExpandConstantExpr)
//
//===----------------------------------------------------------------------===//


#include "llvm_includes.hpp"

#include "llvm_gen_backend.hpp"

using namespace llvm;

namespace {
class PromoteIntegers : public FunctionPass {
 public:
  static char ID;
  PromoteIntegers() : FunctionPass(ID) {
  }
  virtual bool runOnFunction(Function &F);
};
}

char PromoteIntegers::ID = 0;

// Legal sizes are currently 1, 8, 16, 32, and 64.
// We can't yet expand types above 64 bit, so don't try to touch them for now.
// TODO(dschuff): expand >64bit types or disallow >64bit packed bitfields.
// There are currently none in our tests that use the ABI checker.
// See https://code.google.com/p/nativeclient/issues/detail?id=3360
static bool isLegalSize(unsigned Size) {
  if (Size > 64) return true;
  return Size == 1 || Size == 8 || Size == 16 || Size == 32 || Size == 64;
}

static Type *getPromotedIntType(IntegerType *Ty) {
  unsigned Width = Ty->getBitWidth();
  assert(Width <= 64 && "Don't know how to legalize >64 bit types yet");
  if (isLegalSize(Width))
    return Ty;
  return IntegerType::get(Ty->getContext(),
                          Width < 8 ? 8 : NextPowerOf2(Width));
}

// Return a legal integer type, promoting to a larger size if necessary.
static Type *getPromotedType(Type *Ty) {
  assert(isa<IntegerType>(Ty) && "Trying to convert a non-integer type");
  return getPromotedIntType(cast<IntegerType>(Ty));
}

// Return true if Val is an int which should be converted.
static bool shouldConvert(Value *Val) {
  Type *Ty = Val ? Val->getType() : NULL;
  if (IntegerType *ITy = dyn_cast<IntegerType>(Ty)) {
    if (!isLegalSize(ITy->getBitWidth())) {
      return true;
    }
  }
  return false;
}

// Return a constant which has been promoted to a legal size.
static Value *convertConstant(Constant *C, bool SignExt=false) {
  assert(shouldConvert(C));
  if (isa<UndefValue>(C)) {
    return UndefValue::get(getPromotedType(C->getType()));
  } else if (ConstantInt *CInt = dyn_cast<ConstantInt>(C)) {
    return ConstantInt::get(
        getPromotedType(C->getType()),
        SignExt ? CInt->getSExtValue() : CInt->getZExtValue(),
        /*isSigned=*/SignExt);
  } else {
    errs() << "Value: " << *C << "\n";
    report_fatal_error("Unexpected constant value");
    return NULL;
  }
}

namespace {
// Holds the state for converting/replacing values. Conversion is done in one
// pass, with each value requiring conversion possibly having two stages. When
// an instruction needs to be replaced (i.e. it has illegal operands or result)
// a new instruction is created, and the pass calls getConverted to get its
// operands. If the original operand has already been converted, the new value
// is returned. Otherwise, a placeholder is created and used in the new
// instruction. After a new instruction is created to replace an illegal one,
// recordConverted is called to register the replacement. All users are updated,
// and if there is a placeholder, its users are also updated.
// recordConverted also queues the old value for deletion.
// This strategy avoids the need for recursion or worklists for conversion.
class ConversionState {
 public:
  // Return the promoted value for Val. If Val has not yet been converted,
  // return a placeholder, which will be converted later.
  Value *getConverted(Value *Val) {
    if (!shouldConvert(Val))
        return Val;
    if (isa<GlobalVariable>(Val))
      report_fatal_error("Can't convert illegal GlobalVariables");
    if (RewrittenMap.count(Val))
      return RewrittenMap[Val];

    // Directly convert constants.
    if (Constant *C = dyn_cast<Constant>(Val))
      return convertConstant(C, /*SignExt=*/false);

    // No converted value available yet, so create a placeholder.
    Value *P = new Argument(getPromotedType(Val->getType()));

    RewrittenMap[Val] = P;
    Placeholders[Val] = P;
    return P;
  }

  // Replace the uses of From with To, replace the uses of any
  // placeholders for From, and optionally give From's name to To.
  // Also mark To for deletion.
  void recordConverted(Instruction *From, Value *To, bool TakeName=true) {
    ToErase.push_back(From);
    if (!shouldConvert(From)) {
      // From does not produce an illegal value, update its users in place.
      From->replaceAllUsesWith(To);
    } else {
      // From produces an illegal value, so its users will be replaced. When
      // replacements are created they will use values returned by getConverted.
      if (Placeholders.count(From)) {
        // Users of the placeholder can be updated in place.
        Placeholders[From]->replaceAllUsesWith(To);
        Placeholders.erase(From);
      }
      RewrittenMap[From] = To;
    }
    if (TakeName) {
      To->takeName(From);
    }
  }

  void eraseReplacedInstructions() {
    for (SmallVectorImpl<Instruction *>::iterator I = ToErase.begin(),
             E = ToErase.end(); I != E; ++I) {
      if (!(*I)->use_empty()) {
        (*I)->replaceAllUsesWith(UndefValue::get((*I)->getType()));
      }
    }
    for (SmallVectorImpl<Instruction *>::iterator I = ToErase.begin(),
             E = ToErase.end(); I != E; ++I)
      (*I)->eraseFromParent();
  }

 private:
  // Maps illegal values to their new converted values (or placeholders
  // if no new value is available yet)
  DenseMap<Value *, Value *> RewrittenMap;
  // Maps illegal values with no conversion available yet to their placeholders
  DenseMap<Value *, Value *> Placeholders;
  // Illegal values which have already been converted, will be erased.
  SmallVector<Instruction *, 8> ToErase;
};
} // anonymous namespace

// Split an illegal load into multiple legal loads and return the resulting
// promoted value. The size of the load is assumed to be a multiple of 8.
static Value *splitLoad(LoadInst *Inst, ConversionState &State) {
  if (Inst->isVolatile() || Inst->isAtomic())
    report_fatal_error("Can't split volatile/atomic loads");
  if (cast<IntegerType>(Inst->getType())->getBitWidth() % 8 != 0)
    report_fatal_error("Loads must be a multiple of 8 bits");

  unsigned AddrSpace = Inst->getPointerAddressSpace();
  Value *OrigPtr = State.getConverted(Inst->getPointerOperand());
  // OrigPtr is a placeholder in recursive calls, and so has no name
  if (OrigPtr->getName().empty())
    OrigPtr->setName(Inst->getPointerOperand()->getName());
  unsigned Width = cast<IntegerType>(Inst->getType())->getBitWidth();
  Type *NewType = getPromotedType(Inst->getType());
  unsigned LoWidth = Width;

  while (!isLegalSize(LoWidth)) LoWidth -= 8;
  IntegerType *LoType = IntegerType::get(Inst->getContext(), LoWidth);
  IntegerType *HiType = IntegerType::get(Inst->getContext(), Width - LoWidth);
  IRBuilder<> IRB(Inst);

  Value *BCLo = IRB.CreateBitCast(
      OrigPtr,
      LoType->getPointerTo(AddrSpace),
      OrigPtr->getName() + ".loty");
  Value *LoadLo = IRB.CreateAlignedLoad(
      BCLo, Inst->getAlignment(), Inst->getName() + ".lo");
  Value *LoExt = IRB.CreateZExt(LoadLo, NewType, LoadLo->getName() + ".ext");
  Value *GEPHi = IRB.CreateConstGEP1_32(BCLo, 1, OrigPtr->getName() + ".hi");
  Value *BCHi = IRB.CreateBitCast(
        GEPHi,
        HiType->getPointerTo(AddrSpace),
        OrigPtr->getName() + ".hity");

  Value *LoadHi = IRB.CreateLoad(BCHi, Inst->getName() + ".hi");
  if (!isLegalSize(Width - LoWidth)) {
    LoadHi = splitLoad(cast<LoadInst>(LoadHi), State);
  }

  Value *HiExt = IRB.CreateZExt(LoadHi, NewType, LoadHi->getName() + ".ext");
  Value *HiShift = IRB.CreateShl(HiExt, LoWidth, HiExt->getName() + ".sh");
  Value *Result = IRB.CreateOr(LoExt, HiShift);

  State.recordConverted(Inst, Result);

  return Result;
}

static Value *splitStore(StoreInst *Inst, ConversionState &State) {
  if (Inst->isVolatile() || Inst->isAtomic())
    report_fatal_error("Can't split volatile/atomic stores");
  if (cast<IntegerType>(Inst->getValueOperand()->getType())->getBitWidth() % 8
      != 0)
    report_fatal_error("Stores must be a multiple of 8 bits");

  unsigned AddrSpace = Inst->getPointerAddressSpace();
  Value *OrigPtr = State.getConverted(Inst->getPointerOperand());
  // OrigPtr is now a placeholder in recursive calls, and so has no name.
  if (OrigPtr->getName().empty())
    OrigPtr->setName(Inst->getPointerOperand()->getName());
  Value *OrigVal = State.getConverted(Inst->getValueOperand());
  unsigned Width = cast<IntegerType>(
      Inst->getValueOperand()->getType())->getBitWidth();
  unsigned LoWidth = Width;

  while (!isLegalSize(LoWidth)) LoWidth -= 8;
  IntegerType *LoType = IntegerType::get(Inst->getContext(), LoWidth);
  IntegerType *HiType = IntegerType::get(Inst->getContext(), Width - LoWidth);
  IRBuilder<> IRB(Inst);

  Value *BCLo = IRB.CreateBitCast(
      OrigPtr,
      LoType->getPointerTo(AddrSpace),
      OrigPtr->getName() + ".loty");
  Value *LoTrunc = IRB.CreateTrunc(
      OrigVal, LoType, OrigVal->getName() + ".lo");
  IRB.CreateAlignedStore(LoTrunc, BCLo, Inst->getAlignment());

  Value *HiLShr = IRB.CreateLShr(
      OrigVal, LoWidth, OrigVal->getName() + ".hi.sh");
  Value *GEPHi = IRB.CreateConstGEP1_32(BCLo, 1, OrigPtr->getName() + ".hi");
  Value *HiTrunc = IRB.CreateTrunc(
      HiLShr, HiType, OrigVal->getName() + ".hi");
  Value *BCHi = IRB.CreateBitCast(
        GEPHi,
        HiType->getPointerTo(AddrSpace),
        OrigPtr->getName() + ".hity");

  Value *StoreHi = IRB.CreateStore(HiTrunc, BCHi);

  if (!isLegalSize(Width - LoWidth)) {
    // HiTrunc is still illegal, and is redundant with the truncate in the
    // recursive call, so just get rid of it.
    State.recordConverted(cast<Instruction>(HiTrunc), HiLShr,
                          /*TakeName=*/false);
    StoreHi = splitStore(cast<StoreInst>(StoreHi), State);
  }
  State.recordConverted(Inst, StoreHi, /*TakeName=*/false);
  return StoreHi;
}

// Return a converted value with the bits of the operand above the size of the
// original type cleared.
static Value *getClearConverted(Value *OriginalOperand, Type *OrigType, IRBuilder<> &IRB, ConversionState &State) {
  if(!OriginalOperand)
    return OriginalOperand;
  Instruction *OrigInst = dyn_cast<Instruction>(OriginalOperand);
  Value *ConvertedOperand = State.getConverted(OriginalOperand); // This is the potentially promoted operand

  // If the operand is a constant, it will have been created by
  // ConversionState.getConverted (via convertConstant), which zero-extends by default.
  if (isa<Constant>(ConvertedOperand))
    return ConvertedOperand;

  Value *Mask = ConstantInt::get(
          getPromotedType(OrigType),
          APInt::getLowBitsSet(getPromotedType(OrigType)->getIntegerBitWidth(),
                               OrigType->getIntegerBitWidth()));
  Value *NewInst = IRB.CreateAnd(ConvertedOperand, Mask, ConvertedOperand->getName() + ".clear");

  if (OrigInst) {
    if (Instruction *CreatedInst = dyn_cast<Instruction>(NewInst)) {
        CopyDebug(CreatedInst, OrigInst);
    }
  }
  return NewInst;
}

// Return a value with the bits of the operand above the size of the original
// type equal to the sign bit of the original operand. The new operand is
// assumed to have been legalized already.
// This is done by shifting the sign bit of the smaller value up to the MSB
// position in the larger size, and then arithmetic-shifting it back down.
static Value *getSignExtend(Value *ConvertedOperand, Value *OriginalOperand, Type *OrigType, IRBuilder<> &IRB) {
  // If OriginalOperand was a constant, ConvertedOperand will have been created by
  // ConversionState.getConverted (via convertConstant), which zero-extends by default.
  // But that is wrong here, so replace it with a sign-extended constant.
  if (Constant *C = dyn_cast<Constant>(OriginalOperand))
    return convertConstant(C, /*SignExt=*/true);

  Type *PromotedType = getPromotedType(OrigType);

  ConstantInt *ShiftAmt = ConstantInt::getSigned(
      cast<IntegerType>(PromotedType),
      PromotedType->getIntegerBitWidth() - OrigType->getIntegerBitWidth());

  Value *Shl = IRB.CreateShl(ConvertedOperand, ShiftAmt, ConvertedOperand->getName() + ".getsign");
  if (Instruction *OrigInst = dyn_cast<Instruction>(OriginalOperand)) {
    if (Instruction *CreatedInst = dyn_cast<Instruction>(Shl)) CopyDebug(CreatedInst, OrigInst);
  }

  Value *AShr = IRB.CreateAShr(Shl, ShiftAmt, ConvertedOperand->getName() + ".signed");
  if (Instruction *OrigInst = dyn_cast<Instruction>(OriginalOperand)) {
     if (Instruction *CreatedInst = dyn_cast<Instruction>(AShr)) CopyDebug(CreatedInst, OrigInst);
  }
  return AShr;
}

static void convertInstruction(Instruction *Inst, ConversionState &State) {
  IRBuilder<> IRB(Inst); // Default IRB pointing at the instruction itself.
                         // Will be repositioned if needed.

  if (SExtInst *Sext = dyn_cast<SExtInst>(Inst)) {
    Value *Op = Sext->getOperand(0);
    Value *NewInst = NULL;
    if (shouldConvert(Op)) {
      NewInst = getSignExtend(State.getConverted(Op), Op, Op->getType(), IRB);
    }
    if (getPromotedType(Op->getType()) != getPromotedType(Sext->getType())) {
      IRB.SetInsertPoint(Sext);
      NewInst = IRB.CreateSExt(NewInst ? NewInst : State.getConverted(Op),
                               getPromotedType(cast<IntegerType>(Sext->getType())),
                               Sext->getName() + ".sext");
      CopyDebug(cast<Instruction>(NewInst), Sext);
    }
    assert(NewInst && "Failed to convert sign extension");
    State.recordConverted(Sext, NewInst);
  } else if (ZExtInst *Zext = dyn_cast<ZExtInst>(Inst)) {
    Value *Op = Zext->getOperand(0);
    Value *NewInst = NULL;
    if (shouldConvert(Op)) {
      NewInst = getClearConverted(Op, Op->getType(), IRB, State);
    }
    if (getPromotedType(Op->getType()) != getPromotedType(Zext->getType())) {
      IRB.SetInsertPoint(Zext);
      NewInst = IRB.CreateZExtOrBitCast(NewInst ? NewInst : State.getConverted(Op),
                                        getPromotedType(cast<IntegerType>(Zext->getType())));
      if (!Zext->getName().empty()) NewInst->setName(Zext->getName() + ".zext");
      CopyDebug(cast<Instruction>(NewInst), Zext);
    }
    assert(NewInst);
    State.recordConverted(Zext, NewInst);
  } else if (TruncInst *Trunc = dyn_cast<TruncInst>(Inst)) {
    Value *Op = Trunc->getOperand(0);
    Value *NewInst;
    if (getPromotedType(Op->getType()) != getPromotedType(Trunc->getType())) {
      IRB.SetInsertPoint(Trunc);
      NewInst = IRB.CreateTrunc(State.getConverted(Op),
                                getPromotedType(cast<IntegerType>(Trunc->getType())),
                                State.getConverted(Op)->getName() + ".trunc");
      CopyDebug(cast<Instruction>(NewInst), Trunc);
    } else {
      NewInst = State.getConverted(Op);
    }
    State.recordConverted(Trunc, NewInst);
  } else if (LoadInst *Load = dyn_cast<LoadInst>(Inst)) {
    if (shouldConvert(Load)) {
      splitLoad(Load, State);
    }
  } else if (StoreInst *Store = dyn_cast<StoreInst>(Inst)) {
    if (shouldConvert(Store->getValueOperand())) {
      splitStore(Store, State);
    }
  } else if (isa<CallInst>(Inst)) {
    report_fatal_error("can't convert calls with illegal types");
  } else if (BinaryOperator *Binop = dyn_cast<BinaryOperator>(Inst)) {
    Value *NewInst = NULL;
    switch (Binop->getOpcode()) {
      case Instruction::AShr: {
        // The AShr operand needs to be sign-extended to the promoted size
        // before shifting. Because the sign-extension is implemented with
        // with AShr, it can be combined with the original operation.
        Value *Operand = Binop->getOperand(0);
        Value *ShiftAmountOperand = Binop->getOperand(1);
        Value *ConvertedLHS = State.getConverted(Operand);
        Value *SignExtendedLHS = getSignExtend(ConvertedLHS, Operand, Operand->getType(), IRB);

        Value *ShiftAmount = NULL;
        APInt SignShiftVal = APInt(getPromotedType(Operand->getType())->getIntegerBitWidth(),
                                   getPromotedType(Operand->getType())->getIntegerBitWidth() -
                                   Operand->getType()->getIntegerBitWidth());

        if (ConstantInt *C = dyn_cast<ConstantInt>(State.getConverted(ShiftAmountOperand))) {
          ShiftAmount = ConstantInt::get(getPromotedType(Operand->getType()), SignShiftVal + C->getValue());
        } else {
          Value *ClearedShiftAmount = getClearConverted(ShiftAmountOperand, ShiftAmountOperand->getType(), IRB, State);
          IRB.SetInsertPoint(Binop);
          ShiftAmount = IRB.CreateAdd(ClearedShiftAmount,
                                      ConstantInt::get(getPromotedType(ShiftAmountOperand->getType()), SignShiftVal),
                                      State.getConverted(Operand)->getName() + ".shamt");
        }
        IRB.SetInsertPoint(Binop);
        NewInst = IRB.CreateAShr(SignExtendedLHS, ShiftAmount, Binop->getName() + ".result");
        CopyDebug(cast<Instruction>(NewInst), Binop);
        break;
      }

      case Instruction::LShr:
      case Instruction::Shl: {
        Value *LHS_Operand = Binop->getOperand(0);
        Value *RHS_Operand = Binop->getOperand(1);
        Value *ConvertedLHS = (Binop->getOpcode() == Instruction::LShr)
                            ? getClearConverted(LHS_Operand, LHS_Operand->getType(), IRB, State)
                            : State.getConverted(LHS_Operand);
        Value *ConvertedRHS = getClearConverted(RHS_Operand, RHS_Operand->getType(), IRB, State);

        IRB.SetInsertPoint(Binop);
        NewInst = IRB.CreateBinOp(Binop->getOpcode(), ConvertedLHS, ConvertedRHS,
                                  Binop->getName() + ".result");
        break;
      }
      case Instruction::Add:
      case Instruction::Sub:
      case Instruction::Mul:
      case Instruction::And:
      case Instruction::Or:
      case Instruction::Xor:
        IRB.SetInsertPoint(Binop);
        NewInst = IRB.CreateBinOp(Binop->getOpcode(),
                                  State.getConverted(Binop->getOperand(0)),
                                  State.getConverted(Binop->getOperand(1)),
                                  Binop->getName() + ".result");
        CopyDebug(cast<Instruction>(NewInst), Binop);
        break;
      case Instruction::FAdd:
      case Instruction::FSub:
      case Instruction::FMul:
      case Instruction::UDiv:
      case Instruction::SDiv:
      case Instruction::FDiv:
      case Instruction::URem:
      case Instruction::SRem:
      case Instruction::FRem:
      case Instruction::BinaryOpsEnd:
        // We should not see FP operators here.
        // We don't handle div.
        errs() << *Inst << "\n";
        llvm_unreachable("Cannot handle binary operator");
        break;
    }

    if (isa<OverflowingBinaryOperator>(NewInst)) {
      cast<BinaryOperator>(NewInst)->setHasNoUnsignedWrap(
          Binop->hasNoUnsignedWrap());
      cast<BinaryOperator>(NewInst)->setHasNoSignedWrap(
          Binop->hasNoSignedWrap());
    }
    State.recordConverted(Binop, NewInst);
  } else if (ICmpInst *Cmp = dyn_cast<ICmpInst>(Inst)) {
    Value *Op0, *Op1;
    // For signed compares, operands are sign-extended to their
    // promoted type. For unsigned or equality compares, the upper bits are
    // cleared.
    if (Cmp->isSigned()) {
      Op0 = getSignExtend(State.getConverted(Cmp->getOperand(0)), Cmp->getOperand(0), Cmp->getOperand(0)->getType(), IRB);
      Op1 = getSignExtend(State.getConverted(Cmp->getOperand(1)), Cmp->getOperand(1), Cmp->getOperand(1)->getType(), IRB);
    } else {
      Op0 = getClearConverted(Cmp->getOperand(0), Cmp->getOperand(0)->getType(), IRB, State);
      Op1 = getClearConverted(Cmp->getOperand(1), Cmp->getOperand(1)->getType(), IRB, State);
    }
    IRB.SetInsertPoint(Cmp);
    Instruction *NewInst = IRB.CreateICmp(Cmp->getPredicate(), Op0, Op1, Cmp->getName());
    CopyDebug(NewInst, Cmp);
    State.recordConverted(Cmp, NewInst);
  } else if (SelectInst *Select = dyn_cast<SelectInst>(Inst)) {
    IRB.SetInsertPoint(Select);
    Instruction *NewInst = IRB.CreateSelect(Select->getCondition(),
                                          State.getConverted(Select->getTrueValue()),
                                          State.getConverted(Select->getFalseValue()),
                                          Select->getName());
    CopyDebug(NewInst, Select);
    State.recordConverted(Select, NewInst);
  } else if (PHINode *Phi = dyn_cast<PHINode>(Inst)) {
    // PHINodes are typically not created with IRBuilder in modification passes like this.
    // Their structure (incoming blocks) is essential.
    PHINode *NewPhi = PHINode::Create(
        getPromotedType(Phi->getType()),
        Phi->getNumIncomingValues(),
        "", Phi);
    CopyDebug(NewPhi, Phi);
    for (unsigned I = 0, E = Phi->getNumIncomingValues(); I < E; ++I) {
      NewPhi->addIncoming(State.getConverted(Phi->getIncomingValue(I)),
                          Phi->getIncomingBlock(I));
    }
    State.recordConverted(Phi, NewPhi);
  } else if (SwitchInst *Switch = dyn_cast<SwitchInst>(Inst)) {
    Value *Condition = getClearConverted(Switch->getCondition(), Switch->getCondition()->getType(), IRB, State);
    // SwitchInst::Create still takes InsertBefore instruction.
    SwitchInst *NewInst = SwitchInst::Create(
        Condition,
        Switch->getDefaultDest(),
        Switch->getNumCases(),
        Switch); // Switch will be replaced, so this is okay.
    CopyDebug(NewInst, Switch);
    for (SwitchInst::CaseIt I = Switch->case_begin(),
             E = Switch->case_end();
         I != E; ++I) {
#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 50
      NewInst->addCase(cast<ConstantInt>(convertConstant(I->getCaseValue())),
                       I->getCaseSuccessor());
#else
      NewInst->addCase(cast<ConstantInt>(convertConstant(I.getCaseValue())),
                       I.getCaseSuccessor());
#endif
    }
    Switch->eraseFromParent();
  } else {
    errs() << *Inst<<"\n";
    llvm_unreachable("unhandled instruction");
  }
}

bool PromoteIntegers::runOnFunction(Function &F) {
  // Don't support changing the function arguments. This should not be
  // generated by clang.
  for (Function::arg_iterator I = F.arg_begin(), E = F.arg_end(); I != E; ++I) {
    Value *Arg = &*I;
    if (shouldConvert(Arg)) {
      errs() << "Function " << F.getName() << ": " << *Arg << "\n";
      llvm_unreachable("Function has illegal integer/pointer argument");
    }
  }

  ConversionState State;
  bool Modified = false;
  for (Function::iterator FI = F.begin(), FE = F.end(); FI != FE; ++FI) {
    for (BasicBlock::iterator BBI = FI->begin(), BBE = FI->end(); BBI != BBE;) {
      Instruction *Inst = &*BBI++;
      // Only attempt to convert an instruction if its result or any of its
      // operands are illegal.
      bool ShouldConvert = shouldConvert(Inst);
      for (User::op_iterator OI = Inst->op_begin(), OE = Inst->op_end();
           OI != OE; ++OI)
        ShouldConvert |= shouldConvert(cast<Value>(OI));

      if (ShouldConvert) {
        convertInstruction(Inst, State);
        Modified = true;
      }
    }
  }
  State.eraseReplacedInstructions();
  return Modified;
}

FunctionPass *llvm::createPromoteIntegersPass() {
  return new PromoteIntegers();
}
