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
 * Author: Yang Rong <rong.r.yang@intel.com>
 */

/**
 * \file llvm_includes.hpp
 * \author Yang Rong <rong.r.yang@intel.com>
 */
#ifndef __GBE_IR_LLVM_INCLUDES_HPP__
#define __GBE_IR_LLVM_INCLUDES_HPP__

#ifdef GBE_COMPILER_AVAILABLE
#include "llvm/Config/llvm-config.h"

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/LLVMContext.h"

#include "llvm_includes.hpp"

#include "llvm/Pass.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/SmallString.h"

#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Analysis/Passes.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FormattedStream.h"
// LLVM-18 fix: TargetRegistry moved to MC subdirectory
#include "llvm/MC/TargetRegistry.h"
#if LLVM_VERSION_MAJOR >= 18
#include "llvm/TargetParser/Host.h"
#else
#include "llvm/Support/Host.h"
#endif
#include "llvm/Support/ToolOutputFile.h"

#include "llvm-c/Linker.h"
#include "llvm/IRReader/IRReader.h"
#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 40
#include <llvm/Bitcode/BitcodeWriter.h>
//#include <llvm/Bitcode/BitcodeReader.h>
#else
#include "llvm/Bitcode/ReaderWriter.h"
#endif
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Utils/Cloning.h"

#include "llvm/CodeGen/Passes.h"
#include "llvm/CodeGen/IntrinsicLowering.h"

#include "llvm/Transforms/Scalar.h"
#if LLVM_VERSION_MAJOR >= 7
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#endif
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCObjectFileInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"

#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 35
#include "llvm/IR/Mangler.h"
// LLVM-11+ fix: CallSite.h was removed, use CallBase and AbstractCallSite instead
// CallSite was a wrapper around CallInst and InvokeInst
// Now we use CallBase directly (base class of CallInst, InvokeInst, CallBrInst)
#if LLVM_VERSION_MAJOR >= 11
  // For LLVM 11+, CallBase is in Instructions.h (already included above)
  // #include "llvm/IR/Instructions.h"  // Already included
  // No CallSite.h needed
  // Include compatibility layer that provides CallSite wrapper around CallBase
  #include "llvm/llvm_callsite_compat.hpp"

  // LLVM-11+ also renamed getCalledValue() to getCalledOperand()
  // Provide inline compatibility helper
  namespace llvm {
    inline Value *getCalledValueCompat(CallInst *CI) {
      return CI->getCalledOperand();
    }
    inline Value *getCalledValueCompat(const CallInst *CI) {
      return CI->getCalledOperand();
    }
  }
  #define GBE_GET_CALLED_VALUE(CI) (CI)->getCalledOperand()
#else
  // For LLVM 10 and earlier
  #include "llvm/IR/CallSite.h"
  #define GBE_GET_CALLED_VALUE(CI) (CI)->getCalledValue()
#endif

// LLVM-12+ fix: VectorType::getNumElements() removed for scalable vector support
// OpenCL uses fixed-width vectors only (vec2, vec3, vec4, etc.)
#if LLVM_VERSION_MAJOR >= 12
  #include "llvm/IR/DerivedTypes.h"
  // For fixed-width vectors, cast to FixedVectorType
  #define GBE_VECTOR_GET_NUM_ELEMENTS(VT) (cast<llvm::FixedVectorType>(VT)->getNumElements())
#else
  // For LLVM 11 and earlier
  #define GBE_VECTOR_GET_NUM_ELEMENTS(VT) ((VT)->getNumElements())
#endif

// LLVM-11+ fix: LoadInst/StoreInst::getAlignment() → getAlign().value()
// getAlignment() deprecated in LLVM 10, removed in LLVM 11+
#if LLVM_VERSION_MAJOR >= 11
  #define GBE_GET_ALIGNMENT(I) ((I).getAlign().value())
#else
  #define GBE_GET_ALIGNMENT(I) ((I).getAlignment())
#endif

#include "llvm/IR/CFG.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Dominators.h"
#else
#include "llvm/Support/CallSite.h"
#include "llvm/Support/CFG.h"
#include "llvm/Support/InstIterator.h"
#include "llvm/InstVisitor.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Analysis/Dominators.h"
#include "llvm/Assembly/PrintModulePass.h"
#include "llvm/Target/Mangler.h"
#endif

#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 37
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#else
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/PassManager.h"
#endif
// LLVM-16+ fix: Triple.h moved from ADT to TargetParser
#if LLVM_VERSION_MAJOR >= 16
#include "llvm/TargetParser/Triple.h"
#else
#include "llvm/ADT/Triple.h"
#endif

#include <clang/CodeGen/CodeGenAction.h>

#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 38
#include "llvm/Analysis/BasicAliasAnalysis.h"
#include "llvm/Analysis/TypeBasedAliasAnalysis.h"
#endif

#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 39
#include "llvm/Transforms/IPO/FunctionAttrs.h"
#include "llvm/Transforms/Scalar/GVN.h"
#endif

#if LLVM_VERSION_MAJOR * 10 + LLVM_VERSION_MINOR >= 39
#include "llvm/Support/Error.h"
#endif

#endif /*GBE_COMPILER_AVAILABLE */

#endif /* __GBE_IR_LLVM_INCLUDES_HPP__ */
