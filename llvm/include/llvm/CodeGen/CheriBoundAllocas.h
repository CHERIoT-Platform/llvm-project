//===- CheriBoundAllocas.h -------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
/// \file
///
/// Defines an IR pass for bounding allocas on CHERI targets
///
//===----------------------------------------------------------------------===//

#ifndef LLVM_CODEGEN_CHERI_BOUND_ALLOCAS_H
#define LLVM_CODEGEN_CHERI_BOUND_ALLOCAS_H

#include "llvm/IR/PassManager.h"

namespace llvm {

class Module;
class TargetMachine;

class CheriBoundAllocasPass : public PassInfoMixin<CheriBoundAllocasPass> {
    private:
      const TargetMachine *TM;
    
    public:
    CheriBoundAllocasPass(const TargetMachine *TM) : TM(TM) {}
    PreservedAnalyses run(Function &F, FunctionAnalysisManager &FM);
    };
} // end namespace llvm

#endif // LLVM_CODEGEN_CHERI_BOUND_ALLOCAS_H
