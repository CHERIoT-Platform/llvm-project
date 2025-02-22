//===-- RISCV.h - Top-level interface for RISC-V ----------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the entry points for global functions defined in the LLVM
// RISC-V back-end.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCV_H
#define LLVM_LIB_TARGET_RISCV_RISCV_H

#include "MCTargetDesc/RISCVBaseInfo.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Function.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {
class FunctionPass;
class InstructionSelector;
class MCInst;
class MCOperand;
class MachineInstr;
class MachineOperand;
class ModulePass;
class PassRegistry;
class RISCVRegisterBankInfo;
class RISCVSubtarget;
class RISCVTargetMachine;

FunctionPass *createRISCVCodeGenPreparePass();
void initializeRISCVCodeGenPreparePass(PassRegistry &);

FunctionPass *createRISCVDeadRegisterDefinitionsPass();
void initializeRISCVDeadRegisterDefinitionsPass(PassRegistry &);

FunctionPass *createRISCVDeadRegisterDefinitionsPass();
void initializeRISCVDeadRegisterDefinitionsPass(PassRegistry &);

/// Information about imported functions.
struct CHERIoTImportedFunction {
  /// The name of the import symbol.
  StringRef ImportName;
  /// The name of the import symbol.
  StringRef ExportName;
  /// Flag indicating whether this is a library or compartment import.
  bool IsLibrary;
  /// Flag indicating that the entry should be public and a COMDAT.
  bool IsPublic;
};

/**
 * Helper class to allow CHERIoTImportedFunction structures to be used in a
 * dense map.
 */
struct CHERIoTImportedFunctionDenseMapInfo {
  /// Anything with an empty string is invalid, use a canonical zero value.
  static CHERIoTImportedFunction getEmptyKey() {
    return {"", "", false, false};
  }

  /// Anything with an empty string is invalid, use the IsPublic field to
  /// differentiate from the canonical zero value.
  static CHERIoTImportedFunction getTombstoneKey() {
    return {"", "", true, false};
  }

  /// The import name is unique within a compilation unit, use it for the hash.
  static unsigned getHashValue(const CHERIoTImportedFunction &Val) {
    return llvm::hash_value(Val.ImportName);
  }

  /// Compare for equality.
  static bool isEqual(const CHERIoTImportedFunction &LHS,
                      const CHERIoTImportedFunction &RHS) {
    // Don't bother comparing export names.  It's an error to have two imports
    // with mismatched export names (two different imports referring to the
    // same export may be permitted).  Similarly, IsPublic depends on the
    // export and so may not differ.
    return (LHS.IsLibrary == RHS.IsLibrary) &&
           (LHS.ImportName == RHS.ImportName);
  }
};

/// The set of functions imported from this compilation unit.
using CHERIoTImportedFunctionSet = SetVector<
    CHERIoTImportedFunction, std::vector<CHERIoTImportedFunction>,
    DenseSet<CHERIoTImportedFunction, CHERIoTImportedFunctionDenseMapInfo>>;

FunctionPass *createRISCVISelDag(RISCVTargetMachine &TM,
                                 CodeGenOptLevel OptLevel);

FunctionPass *createRISCVMakeCompressibleOptPass();
void initializeRISCVMakeCompressibleOptPass(PassRegistry &);

FunctionPass *createRISCVGatherScatterLoweringPass();
void initializeRISCVGatherScatterLoweringPass(PassRegistry &);

FunctionPass *createRISCVVectorPeepholePass();
void initializeRISCVVectorPeepholePass(PassRegistry &);

FunctionPass *createRISCVOptWInstrsPass();
void initializeRISCVOptWInstrsPass(PassRegistry &);

FunctionPass *createRISCVMergeBaseOffsetOptPass();
void initializeRISCVMergeBaseOffsetOptPass(PassRegistry &);

FunctionPass *createRISCVExpandPseudoPass(CHERIoTImportedFunctionSet &);
void initializeRISCVExpandPseudoPass(PassRegistry &);

FunctionPass *createRISCVPreRAExpandPseudoPass();
void initializeRISCVPreRAExpandPseudoPass(PassRegistry &);

FunctionPass *createRISCVExpandAtomicPseudoPass();
void initializeRISCVExpandAtomicPseudoPass(PassRegistry &);

FunctionPass *createRISCVInsertVSETVLIPass();
void initializeRISCVInsertVSETVLIPass(PassRegistry &);
extern char &RISCVInsertVSETVLIID;

FunctionPass *createRISCVPostRAExpandPseudoPass();
void initializeRISCVPostRAExpandPseudoPass(PassRegistry &);
FunctionPass *createRISCVInsertReadWriteCSRPass();
void initializeRISCVInsertReadWriteCSRPass(PassRegistry &);

FunctionPass *createRISCVInsertWriteVXRMPass();
void initializeRISCVInsertWriteVXRMPass(PassRegistry &);

FunctionPass *createRISCVRedundantCopyEliminationPass();
void initializeRISCVRedundantCopyEliminationPass(PassRegistry &);

FunctionPass *createRISCVMoveMergePass();
void initializeRISCVMoveMergePass(PassRegistry &);

FunctionPass *createRISCVPushPopOptimizationPass();
void initializeRISCVPushPopOptPass(PassRegistry &);

FunctionPass *createRISCVCheriCleanupOptPass();
void initializeRISCVCheriCleanupOptPass(PassRegistry &);

InstructionSelector *
createRISCVInstructionSelector(const RISCVTargetMachine &,
                               const RISCVSubtarget &,
                               const RISCVRegisterBankInfo &);
void initializeRISCVDAGToDAGISelLegacyPass(PassRegistry &);

FunctionPass *createRISCVPostLegalizerCombiner();
void initializeRISCVPostLegalizerCombinerPass(PassRegistry &);

FunctionPass *createRISCVO0PreLegalizerCombiner();
void initializeRISCVO0PreLegalizerCombinerPass(PassRegistry &);

FunctionPass *createRISCVPreLegalizerCombiner();
void initializeRISCVPreLegalizerCombinerPass(PassRegistry &);

/// Returns the symbol name for either an import or export table entry.
inline std::string getImportExportTableName(StringRef Compartment,
                                            StringRef FnName, int CC,
                                            bool IsImport) {
  bool IsCCall =
      (CC == CallingConv::CHERI_CCall) || (CC == CallingConv::CHERI_CCallee);
  Twine TargetPrefix = !IsCCall ? "__library" : "_";
  Twine KindPrefix = TargetPrefix + (IsImport ? "_import_" : "_export_");
  return (KindPrefix + Compartment + "_" + FnName).str();
}

/**
 * Type for interrupt status.
 */
enum Interrupts { Disabled, Enabled, Inherit };

/**
 * Returns the interrupt status associated with the specified function.
 */
inline Interrupts getInterruptStatus(const Function &fn) {
  // If the interrupt posture attribute is not present then the function
  // inherits interrupt posture.
  if (fn.hasFnAttribute("interrupt-state")) {
    return StringSwitch<Interrupts>(
               fn.getFnAttribute("interrupt-state").getValueAsString())
        .Case("disabled", Disabled)
        .Case("enabled", Enabled)
        .Case("inherit", Inherit)
        .Default(Inherit);
  }
  return Inherit;
}

/**
 * Returns true if calls from function 'from' to function 'to' may be replaced
 * with direct calls without accidentally altering interrupt status.
 */
inline bool isSafeToDirectCall(const Function &from, const Function &to) {
  auto toStatus = getInterruptStatus(to);
  if (toStatus == Inherit)
    return true;
  auto fromStatus = getInterruptStatus(from);
  return fromStatus == toStatus;
}
} // namespace llvm

#endif
