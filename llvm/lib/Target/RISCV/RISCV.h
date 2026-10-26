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
#include "llvm/CodeGen/MachineFunctionAnalysisManager.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/CodeGen/SelectionDAGISel.h"
#include "llvm/IR/Analysis.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "llvm/Target/TargetMachine.h"
#include <variant>

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

class RISCVCodeGenPreparePass
    : public OptionalPassInfoMixin<RISCVCodeGenPreparePass> {
private:
  const RISCVTargetMachine *TM;

public:
  RISCVCodeGenPreparePass(const RISCVTargetMachine *TM) : TM(TM) {}
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
FunctionPass *createRISCVCodeGenPrepareLegacyPass();
void initializeRISCVCodeGenPrepareLegacyPass(PassRegistry &);

class RISCVDeadRegisterDefinitionsPass
    : public OptionalPassInfoMixin<RISCVDeadRegisterDefinitionsPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};
FunctionPass *createRISCVDeadRegisterDefinitionsLegacyPass();
void initializeRISCVDeadRegisterDefinitionsLegacyPass(PassRegistry &);

class RISCVIndirectBranchTrackingPass
    : public RequiredPassInfoMixin<RISCVIndirectBranchTrackingPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};
FunctionPass *createRISCVIndirectBranchTrackingLegacyPass();
void initializeRISCVIndirectBranchTrackingLegacyPass(PassRegistry &);

class RISCVLandingPadSetupPass
    : public RequiredPassInfoMixin<RISCVLandingPadSetupPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};
FunctionPass *createRISCVLandingPadSetupLegacyPass();
void initializeRISCVLandingPadSetupLegacyPass(PassRegistry &);

/// Information about imported functions.
struct CHERIoTImportedObject {
  /// The name of the import symbol.
  std::string ImportName;
  /// The name of the export symbol.
  std::string ExportName;
  /// The name of the used symbol.
  std::string Name;

  enum class LibraryFlagValue : bool { IsNotLibrary = 0, IsLibrary = 1 };

  /// Flag indicating whether this is a library or compartment import.
  LibraryFlagValue LibraryFlag;

  enum class PublicFlagValue : bool { IsNotPublic = 0, IsPublic = 1 };

  /// Flag indicating that the entry should be public.
  PublicFlagValue PublicFlag;

  enum class GlobalFlagValue : bool { IsNotGlobal = 0, IsGlobal = 1 };

  /// Flag indicating that the entry is a global symbol.
  GlobalFlagValue GlobalFlag;

  enum class COMDATFlagValue : bool { IsNotCOMDAT = 0, IsCOMDAT = 1 };

  /// Flag indicating that the symbol is a COMDAT.
  COMDATFlagValue COMDATFlag;

  enum class WeakFlagValue : bool { IsNotWeak = 0, IsWeak = 1 };

  /// Flag indicating that the symbol has weak linking.
  WeakFlagValue WeakFlag;

  enum class GroupedFlagValue : bool { IsNotGrouped = 0, IsGrouped = 1 };

  /// Flag indicating that the entry is grouped.
  GroupedFlagValue GroupedFlag;

  enum class WritableFlagValue : bool { IsNotWritable = 0, IsWritable = 1 };

  /// Flag indicating that the entry needs the write flag set.
  WritableFlagValue WritableFlag;

  enum class SecondWordKind {
    /// The second word is zero.
    EmptySecondWord,
    /// The second word uses the `SecondWordValue` interpreted as the encoded
    /// value of permissions of the import.
    DiffAndPermsSecondWord,
    /// The second word uses the `SecondWordValue` interpreted as the size of
    /// the imported type.
    SizeOfTypeSecondWord
  };

  /// The kind of the second word.
  SecondWordKind SecondWord;

  /// The value of the second word.
  uint32_t SecondWordValue;
};

/**
 * Helper class to allow CHERIoTImportedObject structures to be used in a
 * dense map.
 */
struct CHERIoTImportedObjectDenseMapInfo {
  /// Anything with an empty string is invalid, use a canonical zero value.
  static CHERIoTImportedObject getEmptyKey() {
    return {"",
            "",
            "",
            CHERIoTImportedObject::LibraryFlagValue::IsNotLibrary,
            CHERIoTImportedObject::PublicFlagValue::IsNotPublic,
            CHERIoTImportedObject::GlobalFlagValue::IsNotGlobal,
            CHERIoTImportedObject::COMDATFlagValue::IsNotCOMDAT,
            CHERIoTImportedObject::WeakFlagValue::IsNotWeak,
            CHERIoTImportedObject::GroupedFlagValue::IsNotGrouped,
            CHERIoTImportedObject::WritableFlagValue::IsNotWritable,
            CHERIoTImportedObject::SecondWordKind::EmptySecondWord,
            0};
  }

  /// Anything with an empty string is invalid, use the IsPublic field to
  /// differentiate from the canonical zero value.
  static CHERIoTImportedObject getTombstoneKey() {
    return {"",
            "",
            "",
            CHERIoTImportedObject::LibraryFlagValue::IsLibrary,
            CHERIoTImportedObject::PublicFlagValue::IsNotPublic,
            CHERIoTImportedObject::GlobalFlagValue::IsNotGlobal,
            CHERIoTImportedObject::COMDATFlagValue::IsNotCOMDAT,
            CHERIoTImportedObject::WeakFlagValue::IsNotWeak,
            CHERIoTImportedObject::GroupedFlagValue::IsNotGrouped,
            CHERIoTImportedObject::WritableFlagValue::IsNotWritable,
            CHERIoTImportedObject::SecondWordKind::EmptySecondWord,
            0};
  }

  /// The import name is unique within a compilation unit, use it for the hash.
  static unsigned getHashValue(const CHERIoTImportedObject &Val) {
    return llvm::hash_value(Val.ImportName);
  }

  /// Compare for equality.
  static bool isEqual(const CHERIoTImportedObject &LHS,
                      const CHERIoTImportedObject &RHS) {
    // Don't bother comparing export names.  It's an error to have two imports
    // with mismatched export names (two different imports referring to the
    // same export may be permitted).  Similarly, IsPublic depends on the
    // export and so may not differ.
    return (LHS.LibraryFlag == RHS.LibraryFlag) &&
           (LHS.ImportName == RHS.ImportName);
  }
};

/// The set of functions imported from this compilation unit.
using CHERIoTImportedObjectSet = SetVector<
    CHERIoTImportedObject, std::vector<CHERIoTImportedObject>,
    DenseSet<CHERIoTImportedObject, CHERIoTImportedObjectDenseMapInfo>>;

FunctionPass *createRISCVISelDagLegacyPass(RISCVTargetMachine &TM,
                                           CodeGenOptLevel OptLevel);

class RISCVISelDAGToDAGPass : public SelectionDAGISelPass {
public:
  RISCVISelDAGToDAGPass(RISCVTargetMachine &TM, CodeGenOptLevel OptLevel);
};

class RISCVLateBranchOptPass
    : public OptionalPassInfoMixin<RISCVLateBranchOptPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};
FunctionPass *createRISCVLateBranchOptLegacyPass();
void initializeRISCVLateBranchOptLegacyPass(PassRegistry &);

class RISCVMakeCompressiblePass
    : public OptionalPassInfoMixin<RISCVMakeCompressiblePass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};
FunctionPass *createRISCVMakeCompressibleLegacyPass();
void initializeRISCVMakeCompressibleLegacyPass(PassRegistry &);

class RISCVGatherScatterLoweringPass
    : public OptionalPassInfoMixin<RISCVGatherScatterLoweringPass> {
private:
  const RISCVTargetMachine *TM;

public:
  RISCVGatherScatterLoweringPass(const RISCVTargetMachine *TM) : TM(TM) {}
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};

FunctionPass *createRISCVGatherScatterLoweringLegacyPass();
void initializeRISCVGatherScatterLoweringLegacyPass(PassRegistry &);

class RISCVVectorPeepholePass
    : public OptionalPassInfoMixin<RISCVVectorPeepholePass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
  MachineFunctionProperties getRequiredProperties() const {
    return MachineFunctionProperties().setIsSSA();
  }
};

FunctionPass *createRISCVVectorPeepholeLegacyPass();
void initializeRISCVVectorPeepholeLegacyPass(PassRegistry &);

class RISCVOptWInstrsPass : public OptionalPassInfoMixin<RISCVOptWInstrsPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVOptWInstrsLegacyPass();
void initializeRISCVOptWInstrsLegacyPass(PassRegistry &);

class RISCVFoldMemOffsetPass
    : public OptionalPassInfoMixin<RISCVFoldMemOffsetPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVFoldMemOffsetLegacyPass();
void initializeRISCVFoldMemOffsetLegacyPass(PassRegistry &);

class RISCVMergeBaseOffsetPass
    : public OptionalPassInfoMixin<RISCVMergeBaseOffsetPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVMergeBaseOffsetLegacyPass();
void initializeRISCVMergeBaseOffsetLegacyPass(PassRegistry &);

class RISCVExpandPseudoPreRAPass
    : public RequiredPassInfoMixin<RISCVExpandPseudoPreRAPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
  MachineFunctionProperties getRequiredProperties() const {
    return MachineFunctionProperties().setIsSSA();
  }
};

FunctionPass *createRISCVExpandPseudoPreRALegacyPass();
void initializeRISCVExpandPseudoPreRALegacyPass(PassRegistry &);

class RISCVExpandPseudoPostRAPass
    : public RequiredPassInfoMixin<RISCVExpandPseudoPostRAPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVExpandPseudoPostRALegacyPass();
void initializeRISCVExpandPseudoPostRALegacyPass(PassRegistry &);

class RISCVExpandPseudoPreEmitPass
    : public RequiredPassInfoMixin<RISCVExpandPseudoPreEmitPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVExpandPseudoPreEmitLegacyPass();
void initializeRISCVExpandPseudoPreEmitLegacyPass(PassRegistry &);

class RISCVExpandPseudoAtomicsPass
    : public RequiredPassInfoMixin<RISCVExpandPseudoAtomicsPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVExpandPseudoAtomicsLegacyPass();
void initializeRISCVExpandPseudoAtomicsLegacyPass(PassRegistry &);

class RISCVInsertVSETVLIPass
    : public RequiredPassInfoMixin<RISCVInsertVSETVLIPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVInsertVSETVLILegacyPass();
void initializeRISCVInsertVSETVLILegacyPass(PassRegistry &);
extern char &RISCVInsertVSETVLILegacyID;

class RISCVInsertReadWriteCSRPass
    : public RequiredPassInfoMixin<RISCVInsertReadWriteCSRPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVInsertReadWriteCSRLegacyPass();
void initializeRISCVInsertReadWriteCSRLegacyPass(PassRegistry &);

class RISCVInsertWriteVXRMPass
    : public RequiredPassInfoMixin<RISCVInsertWriteVXRMPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVInsertWriteVXRMLegacyPass();
void initializeRISCVInsertWriteVXRMLegacyPass(PassRegistry &);

class RISCVRedundantCopyEliminationPass
    : public OptionalPassInfoMixin<RISCVRedundantCopyEliminationPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);

  MachineFunctionProperties getRequiredProperties() const {
    return MachineFunctionProperties().setNoVRegs();
  }
};

FunctionPass *createRISCVRedundantCopyEliminationLegacyPass();
void initializeRISCVRedundantCopyEliminationLegacyPass(PassRegistry &);

class RISCVMoveMergePass : public OptionalPassInfoMixin<RISCVMoveMergePass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVMoveMergeLegacyPass();
void initializeRISCVMoveMergeLegacyPass(PassRegistry &);

class RISCVPushPopOptPass : public OptionalPassInfoMixin<RISCVPushPopOptPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVPushPopOptLegacyPass();
void initializeRISCVPushPopOptLegacyPass(PassRegistry &);

class RISCVLoadStoreOptPass
    : public OptionalPassInfoMixin<RISCVLoadStoreOptPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVLoadStoreOptLegacyPass();
void initializeRISCVLoadStoreOptLegacyPass(PassRegistry &);

class RISCVPreAllocZilsdOptPass
    : public OptionalPassInfoMixin<RISCVPreAllocZilsdOptPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVPreAllocZilsdOptLegacyPass();
void initializeRISCVPreAllocZilsdOptLegacyPass(PassRegistry &);

class RISCVZacasABIFixPass
    : public RequiredPassInfoMixin<RISCVZacasABIFixPass> {
private:
  const RISCVTargetMachine *TM;

public:
  RISCVZacasABIFixPass(const RISCVTargetMachine *TM) : TM(TM) {}
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &FAM);
};
FunctionPass *createRISCVZacasABIFixLegacyPass();
void initializeRISCVZacasABIFixLegacyPass(PassRegistry &);

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

class RISCVPromoteConstantPass
    : public OptionalPassInfoMixin<RISCVPromoteConstantPass> {
private:
  const RISCVTargetMachine *TM;

public:
  RISCVPromoteConstantPass(const RISCVTargetMachine *TM) : TM(TM) {}
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

ModulePass *createRISCVPromoteConstantLegacyPass();
void initializeRISCVPromoteConstantLegacyPass(PassRegistry &);

class RISCVVLOptimizerPass
    : public OptionalPassInfoMixin<RISCVVLOptimizerPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVVLOptimizerLegacyPass();
void initializeRISCVVLOptimizerLegacyPass(PassRegistry &);

class RISCVVMV0EliminationPass
    : public RequiredPassInfoMixin<RISCVVMV0EliminationPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);

  MachineFunctionProperties getRequiredProperties() const {
    return MachineFunctionProperties().setIsSSA();
  }
};

FunctionPass *createRISCVVMV0EliminationLegacyPass();
void initializeRISCVVMV0EliminationLegacyPass(PassRegistry &);

class RISCVQCRelaxMarkingPass
    : public OptionalPassInfoMixin<RISCVQCRelaxMarkingPass> {
public:
  PreservedAnalyses run(MachineFunction &MF,
                        MachineFunctionAnalysisManager &MFAM);
};

FunctionPass *createRISCVQCRelaxMarkingLegacyPass();
void initializeRISCVQCRelaxMarkingLegacyPass(PassRegistry &);

void initializeRISCVAsmPrinterPass(PassRegistry &);

/// Returns the symbol name for either an import or export table entry.
inline std::string getImportExportTableName(StringRef Compartment,
                                            StringRef FnName, int CC,
                                            bool IsImport) {
  bool IsCCall = (CC == CallingConv::CHERIoT_CompartmentCall) ||
                 (CC == CallingConv::CHERIoT_CompartmentCallee);
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
