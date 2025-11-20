/*
 * Copyright © 2012-2025 Intel Corporation
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 */

/**
 * \file llvm_callsite_compat.hpp
 * \brief LLVM CallSite compatibility layer for LLVM 11+
 *
 * LLVM 11 removed CallSite.h and the CallSite wrapper class.
 * This provides a compatibility layer that works with both old and new LLVM.
 *
 * Migration:
 * - LLVM ≤10: Uses CallSite wrapper around CallInst/InvokeInst
 * - LLVM ≥11: Uses CallBase base class directly
 */

#ifndef __GBE_LLVM_CALLSITE_COMPAT_HPP__
#define __GBE_LLVM_CALLSITE_COMPAT_HPP__

#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"

#if LLVM_VERSION_MAJOR >= 11
  // LLVM 11+: CallSite is gone, use CallBase directly
  // CallBase is the base class of CallInst, InvokeInst, and CallBrInst
  namespace llvm {
    // Define CallSite as an alias to CallBase* for compatibility
    using CallSiteBase = CallBase;

    // Compatibility wrapper that mimics old CallSite behavior
    class CallSite {
    private:
      CallBase *CB;

    public:
      // Constructor from Instruction
      CallSite(Instruction *I) : CB(dyn_cast<CallBase>(I)) {}

      // Constructor from Value
      CallSite(Value *V) : CB(dyn_cast<CallBase>(V)) {}

      // Constructor from CallBase
      CallSite(CallBase *C) : CB(C) {}

      // Get the underlying instruction
      Instruction *getInstruction() const { return CB; }

      // Check if valid
      operator bool() const { return CB != nullptr; }

      // Argument iterators (compatible with old CallSite API)
      using arg_iterator = User::op_iterator;

      arg_iterator arg_begin() const { return CB ? CB->arg_begin() : arg_iterator(); }
      arg_iterator arg_end() const { return CB ? CB->arg_end() : arg_iterator(); }

      // Get called value/function
      Value *getCalledValue() const { return CB ? CB->getCalledOperand() : nullptr; }
      Function *getCalledFunction() const { return CB ? CB->getCalledFunction() : nullptr; }

      // Get number of arguments
      unsigned getNumArgOperands() const { return CB ? CB->arg_size() : 0; }
      unsigned arg_size() const { return CB ? CB->arg_size() : 0; }

      // Get specific argument
      Value *getArgOperand(unsigned i) const { return CB ? CB->getArgOperand(i) : nullptr; }

      // Dereference operator
      CallBase *operator->() const { return CB; }
      CallBase &operator*() const { return *CB; }
    };

    // ImmutableCallSite alias (was used for const call sites)
    using ImmutableCallSite = CallSite;
  }
#else
  // LLVM ≤10: Use original CallSite from llvm/IR/CallSite.h
  // (Already included via llvm_includes.hpp)
#endif

#endif /* __GBE_LLVM_CALLSITE_COMPAT_HPP__ */
