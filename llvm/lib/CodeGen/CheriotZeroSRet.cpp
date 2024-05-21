//===- llvm/CodeGen/CherotZeroSRet.cpp - Zero sret pointers ---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file implements a pass that zeroes objects passed as sret pointers for
// CHERIoT cross-compartment calls.  Normally, sret pointers point to memory
// that is uninitialised.  It is the responsibility of the callee to initialise
// them.  For cross-compartment calls, this can cause information leakage and
// so we must zero them before the call.
//
//===----------------------------------------------------------------------===//

#include "llvm/Analysis/Utils/Local.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Instructions.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"
#include "llvm/Target/TargetMachine.h"

#define DEBUG_TYPE "cheriot-zero-sret"
using namespace llvm;

namespace {

class CheriotZeroSRet : public FunctionPass,
                        public InstVisitor<CheriotZeroSRet> {
  llvm::SmallVector<CallBase *, 16> Calls;

public:
  static char ID;
  CheriotZeroSRet() : FunctionPass(ID) {
    initializeCheriotZeroSRetPass(*PassRegistry::getPassRegistry());
  }
  StringRef getPassName() const override { return "CHERIoT zero sret "; }
  void visitCallBase(CallBase &Call) {
    auto *Type = Call.getFunctionType();
    if (Type->getNumParams() == 0)
      return;
    if (Call.getCallingConv() != CallingConv::CHERI_CCall)
      return;
    if (Call.paramHasAttr(0, Attribute::StructRet)) {
      Calls.push_back(&Call);
    }
  }
  bool runOnFunction(Function &Fn) override {
    if (auto *ABIStr = dyn_cast_or_null<MDString>(
            Fn.getParent()->getModuleFlag("target-abi")))
      if (ABIStr->getString() != "cheriot")
        return false;
    Calls.clear();
    visit(Fn);
    if (Calls.empty())
      return false;
    auto &DL = Fn.getParent()->getDataLayout();
    Value *Zero = ConstantInt::get(Type::getInt8Ty(Fn.getContext()), 0);
    for (auto *Call : Calls) {
      IRBuilder<> Builder(Call);
      unsigned Align = 0;
      if (auto *Alloca = dyn_cast<AllocaInst>(
              Call->getOperand(0)->stripPointerCastsAndAliases()))
        Align = Alloca->getAlignment();
      else if (auto *Arg = dyn_cast<Argument>(
                   Call->getOperand(0)->stripPointerCastsAndAliases()))
        Align = Arg->getParamAlignment();
      Builder.CreateMemSet(
          Call->getArgOperand(0), Zero,
          DL.getTypeStoreSize(
              Call->getParamAttr(0, Attribute::StructRet).getValueAsType()),
          MaybeAlign(Align));
    }
    return true;
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<TargetPassConfig>();
    AU.setPreservesCFG();
  }
};

} // anonymous namespace

char CheriotZeroSRet::ID;
INITIALIZE_PASS(CheriotZeroSRet, DEBUG_TYPE,
                "CHERI add bounds to alloca instructions", false, false)

FunctionPass *llvm::createCheriotZeroSRetPass(void) {
  return new CheriotZeroSRet();
}
