//===-- RISCVExpandPseudoInsts.cpp - Expand pseudo instructions -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands pseudo instructions into target
// instructions. This pass should be run after register allocation but before
// the post-regalloc scheduling pass.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVBaseInfo.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "RISCV.h"
#include "RISCVInstrInfo.h"
#include "RISCVTargetMachine.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineOperand.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/GlobalAlias.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/MC/MCContext.h"
#include "llvm/Support/Compiler.h"
#include "llvm/Support/Debug.h"
#include <optional>

using namespace llvm;

#define RISCV_EXPAND_PSEUDO_NAME "RISC-V pseudo instruction expansion pass"
#define RISCV_PRERA_EXPAND_PSEUDO_NAME "RISC-V Pre-RA pseudo instruction expansion pass"

namespace {

class RISCVExpandPseudo : public MachineFunctionPass {
public:
  const RISCVSubtarget *STI;
  const RISCVInstrInfo *TII;
  static char ID;
  CHERIoTImportedObjectSet &ImportedObjects;

  RISCVExpandPseudo(CHERIoTImportedObjectSet &CHERIoTImports)
      : MachineFunctionPass(ID), ImportedObjects(CHERIoTImports) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return RISCV_EXPAND_PSEUDO_NAME; }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandAuipccInstPair(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            MachineBasicBlock::iterator &NextMBBI,
                            unsigned FlagsHi, unsigned SecondOpcode,
                            bool InBounds = false);
  bool expandAuicgpInstPair(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI,
                            MachineBasicBlock::iterator &NextMBBI,
                            unsigned SecondOpcode, bool InBounds = false);
  bool expandCapLoadLocalCap(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI,
                             MachineBasicBlock::iterator &NextMBBI,
                             bool InBounds);
  bool expandDerefCapLoadLocalCap(MachineBasicBlock &MBB,
                                  MachineBasicBlock::iterator MBBI,
                                  MachineBasicBlock::iterator &NextMBBI,
                                  unsigned DerefOpcode);
  bool expandCapLoadGlobalCap(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandCapLoadTLSIEAddress(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI);
  bool expandCapLoadTLSGDCap(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI,
                             MachineBasicBlock::iterator &NextMBBI);
  bool expandCGetAddr(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  bool expandCCOp(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                  MachineBasicBlock::iterator &NextMBBI);
  bool expandCCOpToCMov(MachineBasicBlock &MBB,
                        MachineBasicBlock::iterator MBBI);
  bool expandVMSET_VMCLR(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI, unsigned Opcode);
  bool expandMV_FPR16INX(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI);
  bool expandMV_FPR32INX(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI);
  bool expandRV32ZdinxStore(MachineBasicBlock &MBB,
                            MachineBasicBlock::iterator MBBI);
  bool expandRV32ZdinxLoad(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI);
  bool expandPseudoReadVLENBViaVSETVLIX0(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator MBBI);
  bool expandPseudoClearFPR64(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI);
  bool expandVSPILL(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  bool expandVRELOAD(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI);
  /// Expand a CHERIoT cross-compartment call into a call to the switcher using
  /// an import-table entry.
  bool expandCompartmentCall(MachineBasicBlock &MBB,
                             MachineBasicBlock::iterator MBBI,
                             MachineBasicBlock::iterator &NextMBBI);
  /// Expand a CHERIoT cross-library call into a call via an import-table entry.
  bool expandLibraryCall(MachineBasicBlock &MBB,
                         MachineBasicBlock::iterator MBBI,
                         MachineBasicBlock::iterator &NextMBBI);
  /**
   * Helper that inserts a load of the import table for `Fn` at `MBBI`.  This
   * inserts an AUIPCC and so will split `MBB`.  Calls the result if
   * `CallImportTarget` is true. `TreatAsLibrary` should be set if the exported
   * function is / may be exported from this compartment but, at this call site,
   * should be treated as a library call.
   */
  MachineBasicBlock *insertLoadOfImportTable(MachineBasicBlock &MBB,
                                             MachineBasicBlock::iterator MBBI,
                                             const Function *Fn,
                                             Register DestReg,
                                             bool TreatAsLibrary = false,
                                             bool CallImportTarget = false,
                                             const MachineInstr *OriginalCall = nullptr);
  /**
   * Helper that inserts a load from the import table identified by an import
   * and export table entry symbol.
   *
   * Calls the result if `CallImportTarget` is true.
   */
  MachineBasicBlock *
  insertLoadOfImportTable(MachineBasicBlock &MBB,
                          MachineBasicBlock::iterator MBBI,
                          MCSymbol *ImportSymbol, MCSymbol *ExportSymbol,
                          const StringRef ImportName, Register DestReg,
                          bool IsLibrary, bool IsPublic, bool CallImportTarget,
                          const MachineInstr* OriginalCall);

#ifndef NDEBUG
  unsigned getInstSizeInBytes(const MachineFunction &MF) const {
    unsigned Size = 0;
    for (auto &MBB : MF)
      for (auto &MI : MBB)
        Size += TII->getInstSizeInBytes(MI);
    return Size;
  }
#endif
};

char RISCVExpandPseudo::ID = 0;

bool RISCVExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  STI = &MF.getSubtarget<RISCVSubtarget>();
  TII = STI->getInstrInfo();

#ifndef NDEBUG
  const unsigned OldSize = getInstSizeInBytes(MF);
#endif

  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);

#ifndef NDEBUG
  const unsigned NewSize = getInstSizeInBytes(MF);
  assert(OldSize >= NewSize);
#endif
  return Modified;
}

bool RISCVExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool RISCVExpandPseudo::expandMI(MachineBasicBlock &MBB,
                                 MachineBasicBlock::iterator MBBI,
                                 MachineBasicBlock::iterator &NextMBBI) {
  bool InBounds = true;
  // RISCVInstrInfo::getInstSizeInBytes expects that the total size of the
  // expanded instructions for each pseudo is correct in the Size field of the
  // tablegen definition for the pseudo.
  switch (MBBI->getOpcode()) {
  case RISCV::PseudoMV_FPR16INX:
    return expandMV_FPR16INX(MBB, MBBI);
  case RISCV::PseudoMV_FPR32INX:
    return expandMV_FPR32INX(MBB, MBBI);
  case RISCV::PseudoCGetAddr:
    return expandCGetAddr(MBB, MBBI);
  case RISCV::PseudoCLLC:
    InBounds = false;
    LLVM_FALLTHROUGH;
  case RISCV::PseudoCLLCInbounds:
    return expandCapLoadLocalCap(MBB, MBBI, NextMBBI, InBounds);
  case RISCV::PseudoCLLCInbounds_CLB:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLB);
  case RISCV::PseudoCLLCInbounds_CLBU:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLBU);
  case RISCV::PseudoCLLCInbounds_CSB:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CSB);
  case RISCV::PseudoCLLCInbounds_CLH:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLH);
  case RISCV::PseudoCLLCInbounds_CLHU:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLHU);
  case RISCV::PseudoCLLCInbounds_CSH:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CSH);
  case RISCV::PseudoCLLCInbounds_CLW:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLW);
  case RISCV::PseudoCLLCInbounds_CLWU:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLWU);
  case RISCV::PseudoCLLCInbounds_CSW:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CSW);
  case RISCV::PseudoCLLCInbounds_CLD:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLD);
  case RISCV::PseudoCLLCInbounds_CSD:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CSD);
  case RISCV::PseudoCLLCInbounds_CLC_64:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLC_64);
  case RISCV::PseudoCLLCInbounds_CSC_64:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CSC_64);
  case RISCV::PseudoCLLCInbounds_CLC_128:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CLC_128);
  case RISCV::PseudoCLLCInbounds_CSC_128:
    return expandDerefCapLoadLocalCap(MBB, MBBI, NextMBBI, RISCV::CSC_128);
  case RISCV::PseudoCLGC:
    return expandCapLoadGlobalCap(MBB, MBBI, NextMBBI);
  case RISCV::PseudoCLA_TLS_IE:
    return expandCapLoadTLSIEAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoCLC_TLS_GD:
    return expandCapLoadTLSGDCap(MBB, MBBI, NextMBBI);
  case RISCV::PseudoRV32ZdinxSD:
    return expandRV32ZdinxStore(MBB, MBBI);
  case RISCV::PseudoRV32ZdinxLD:
    return expandRV32ZdinxLoad(MBB, MBBI);
  case RISCV::PseudoCCMOVGPRNoX0:
  case RISCV::PseudoCCMOVGPR:
  case RISCV::PseudoCCADD:
  case RISCV::PseudoCCSUB:
  case RISCV::PseudoCCAND:
  case RISCV::PseudoCCOR:
  case RISCV::PseudoCCXOR:
  case RISCV::PseudoCCMAX:
  case RISCV::PseudoCCMAXU:
  case RISCV::PseudoCCMIN:
  case RISCV::PseudoCCMINU:
  case RISCV::PseudoCCMUL:
  case RISCV::PseudoCCLUI:
  case RISCV::PseudoCCQC_E_LB:
  case RISCV::PseudoCCQC_E_LH:
  case RISCV::PseudoCCQC_E_LW:
  case RISCV::PseudoCCQC_E_LHU:
  case RISCV::PseudoCCQC_E_LBU:
  case RISCV::PseudoCCLB:
  case RISCV::PseudoCCLH:
  case RISCV::PseudoCCLW:
  case RISCV::PseudoCCLHU:
  case RISCV::PseudoCCLBU:
  case RISCV::PseudoCCLWU:
  case RISCV::PseudoCCLD:
  case RISCV::PseudoCCQC_LI:
  case RISCV::PseudoCCQC_E_LI:
  case RISCV::PseudoCCADDW:
  case RISCV::PseudoCCSUBW:
  case RISCV::PseudoCCSLL:
  case RISCV::PseudoCCSRL:
  case RISCV::PseudoCCSRA:
  case RISCV::PseudoCCADDI:
  case RISCV::PseudoCCSLLI:
  case RISCV::PseudoCCSRLI:
  case RISCV::PseudoCCSRAI:
  case RISCV::PseudoCCANDI:
  case RISCV::PseudoCCORI:
  case RISCV::PseudoCCXORI:
  case RISCV::PseudoCCSLLW:
  case RISCV::PseudoCCSRLW:
  case RISCV::PseudoCCSRAW:
  case RISCV::PseudoCCADDIW:
  case RISCV::PseudoCCSLLIW:
  case RISCV::PseudoCCSRLIW:
  case RISCV::PseudoCCSRAIW:
  case RISCV::PseudoCCANDN:
  case RISCV::PseudoCCORN:
  case RISCV::PseudoCCXNOR:
  case RISCV::PseudoCCNDS_BFOS:
  case RISCV::PseudoCCNDS_BFOZ:
    return expandCCOp(MBB, MBBI, NextMBBI);
  case RISCV::PseudoVMCLR_M_B1:
  case RISCV::PseudoVMCLR_M_B2:
  case RISCV::PseudoVMCLR_M_B4:
  case RISCV::PseudoVMCLR_M_B8:
  case RISCV::PseudoVMCLR_M_B16:
  case RISCV::PseudoVMCLR_M_B32:
  case RISCV::PseudoVMCLR_M_B64:
    // vmclr.m vd => vmxor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, RISCV::VMXOR_MM);
  case RISCV::PseudoVMSET_M_B1:
  case RISCV::PseudoVMSET_M_B2:
  case RISCV::PseudoVMSET_M_B4:
  case RISCV::PseudoVMSET_M_B8:
  case RISCV::PseudoVMSET_M_B16:
  case RISCV::PseudoVMSET_M_B32:
  case RISCV::PseudoVMSET_M_B64:
    // vmset.m vd => vmxnor.mm vd, vd, vd
    return expandVMSET_VMCLR(MBB, MBBI, RISCV::VMXNOR_MM);
  case RISCV::PseudoReadVLENBViaVSETVLIX0:
    return expandPseudoReadVLENBViaVSETVLIX0(MBB, MBBI);
  case RISCV::PseudoClearFPR64:
    return expandPseudoClearFPR64(MBB, MBBI);
  case RISCV::PseudoCompartmentCall:
    return expandCompartmentCall(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLibraryCall:
  case RISCV::PseudoCTAILLibrary:
    return expandLibraryCall(MBB, MBBI, NextMBBI);
  }

  return false;
}

MachineBasicBlock *RISCVExpandPseudo::insertLoadOfImportTable(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    const Function *Fn, Register DestReg,
    bool TreatAsLibrary,
    bool CallImportTarget,
    const MachineInstr *OriginalCall) {
  auto *MF = MBB.getParent();
  const StringRef ImportName = Fn->getName();
  // We can hit this code path if we need to do a library-style import
  // for a local exported function.
  CallingConv::ID CC = Fn->getCallingConv();
  bool IsLibrary = CC == CallingConv::CHERIoT_LibraryCall;
  const StringRef CompartmentName =
      IsLibrary ? "libcalls"
                : Fn->getFnAttribute("cheri-compartment").getValueAsString();
  // Is this actually a compartment call that is locally imported?

  auto ImportEntryName = getImportExportTableName(
      CompartmentName, ImportName,
      TreatAsLibrary ? CallingConv::CHERIoT_LibraryCall : Fn->getCallingConv(),
      /*IsImport*/ true);
  // If this isn't really a library call then the export symbol will be
  // different.
  auto ExportEntryName = getImportExportTableName(CompartmentName, ImportName,
                                                  Fn->getCallingConv(),
                                                  /*IsImport*/ false);
  // Create the symbol for the import entry.  We don't use this symbol
  // directly (yet) but we need to allocate storage for the string where
  // it will remain valid for the duration of codegen.
  MCSymbol *ImportSymbol = MF->getContext().getOrCreateSymbol(ImportEntryName);
  MCSymbol *ExportSymbol = MF->getContext().getOrCreateSymbol(ExportEntryName);
  return insertLoadOfImportTable(
      MBB, MBBI, ImportSymbol, ExportSymbol, ImportName, DestReg,
      IsLibrary || TreatAsLibrary, Fn->hasExternalLinkage(), CallImportTarget, OriginalCall);
}

static const GlobalValue *resolveGlobalAlias(const GlobalValue *GV) {
  auto *GA = dyn_cast<GlobalAlias>(GV);
  if (GA)
    return GA->getAliaseeObject();
  return GV;
}

MachineBasicBlock *RISCVExpandPseudo::insertLoadOfImportTable(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MCSymbol *ImportSymbol, MCSymbol *ExportSymbol, const StringRef ImportName,
    Register DestReg,
    bool IsLibrary, bool IsPublic, bool CallImportTarget,
    const MachineInstr* OriginalCall) {
  auto *MF = MBB.getParent();
  const DebugLoc DL = MBBI->getDebugLoc();
  MachineBasicBlock *NewMBB =
      MBB.getParent()->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this
  // label to be emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), DestReg)
      .addExternalSymbol(ImportSymbol->getName().data(),
                         RISCVII::MO_CHERIOT1_COMPARTMENT_CODE_HI);
  BuildMI(NewMBB, DL, TII->get(RISCV::CLC_64), DestReg)
      .addReg(DestReg, RegState::Kill)
      .addMBB(NewMBB, RISCVII::MO_CHERIOT1_COMPARTMENT_LO_I);

  if (CallImportTarget) {
    bool ShouldTailCall =
        OriginalCall->getOpcode() == RISCV::PseudoCTAILLibrary;
    auto NewCallMI =
        BuildMI(NewMBB, DL,
                TII->get(ShouldTailCall ? RISCV::PseudoCTAILIndirect
                                        : RISCV::C_CJALR))
            .addReg(DestReg, RegState::Kill);
    if (OriginalCall && OriginalCall->shouldUpdateAdditionalCallInfo())
      MF->moveAdditionalCallInfo(OriginalCall, NewCallMI);
  }

  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  ImportedObjects.insert(
      {ImportSymbol->getName().str(), ExportSymbol->getName().str(),
       ImportName.str(),
       IsLibrary ? CHERIoTImportedObject::LibraryFlagValue::IsLibrary
                 : CHERIoTImportedObject::LibraryFlagValue::IsNotLibrary,
       IsPublic ? CHERIoTImportedObject::PublicFlagValue::IsPublic
                : CHERIoTImportedObject::PublicFlagValue::IsNotPublic,
       CHERIoTImportedObject::GlobalFlagValue::IsNotGlobal,
       IsPublic ? CHERIoTImportedObject::COMDATFlagValue::IsCOMDAT
                : CHERIoTImportedObject::COMDATFlagValue::IsNotCOMDAT,
       IsPublic ? CHERIoTImportedObject::WeakFlagValue::IsWeak
                : CHERIoTImportedObject::WeakFlagValue::IsNotWeak,
       IsPublic ? CHERIoTImportedObject::GroupedFlagValue::IsGrouped
                : CHERIoTImportedObject::GroupedFlagValue::IsNotGrouped,
       CHERIoTImportedObject::WritableFlagValue::IsNotWritable,
       CHERIoTImportedObject::SecondWordKind::EmptySecondWord, 0});

  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);
  return NewMBB;
}

bool RISCVExpandPseudo::expandCompartmentCall(MachineBasicBlock &MBB,
    MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {

  const MachineOperand Callee = MBBI->getOperand(0);
  MachineInstr &MI = *MBBI;
  const DebugLoc DL = MBBI->getDebugLoc();
  auto *MF = MBB.getParent();
  if (Callee.isGlobal()) {
    // If this is a global, check if it's in the same compartment.  If so, we
    // want to lower as a direct ccall.
    auto *Fn = cast<Function>(resolveGlobalAlias(Callee.getGlobal()));
    if (MF->getFunction().hasFnAttribute("cheri-compartment") &&
        (Fn->getFnAttribute("cheri-compartment").getValueAsString() ==
         MF->getFunction()
             .getFnAttribute("cheri-compartment")
             .getValueAsString())) {
      if (isSafeToDirectCall(MF->getFunction(), *Fn)) {
        MI.setDesc(TII->get(RISCV::PseudoCCALL));
        return true;
      }
      // If this is within the same compartment but must be called via an import
      // table entry, then expand it as a library call.
      return expandLibraryCall(MBB, MBBI, NextMBBI);
    }
  }
  // Expand a cross-compartment call into a auipcc + clc to get the compartment
  // switcher, followed by a compressed cjalr to invoke it.  This is running
  // post-RA, but the ABI guarantees that C7 is not required to be preserved
  // here and so we can use it.
  // FIXME: This copies and pastes a lot of code from expandAuipccInstPair.

  const MachineOperand Switcher =
      MachineOperand::CreateES(".compartment_switcher", 0);

  auto *NewMBB = MBB.getParent()->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), RISCV::X7_Y)
      .addDisp(Switcher, 0, RISCVII::MO_CHERIOT1_COMPARTMENT_CODE_HI);
  BuildMI(NewMBB, DL, TII->get(RISCV::CLC_64), RISCV::X7_Y)
      .addReg(RISCV::X7_Y, RegState::Kill)
      .addMBB(NewMBB, RISCVII::MO_CHERIOT1_COMPARTMENT_LO_I);
  auto NewCallMI = BuildMI(NewMBB, DL, TII->get(RISCV::C_CJALR))
      .addReg(RISCV::X7_Y, RegState::Kill);
  if (MI.shouldUpdateAdditionalCallInfo())
    MF->moveAdditionalCallInfo(&MI, NewCallMI);

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  if (Callee.isGlobal()) {
    auto *Fn = dyn_cast<Function>(resolveGlobalAlias(Callee.getGlobal()));
    insertLoadOfImportTable(MBB, MBBI, Fn, RISCV::X6_Y);
  } else {
    assert(Callee.isReg() && "Expected register operand");
    if (Callee.getReg() != RISCV::X6_Y) {
      BuildMI(&MBB, DL, TII->get(RISCV::CMove)).addDef(RISCV::X6_Y).add(Callee);
    }
  }

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandLibraryCall(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  // Expand a cross-library call into a auipcc + clc to get the import table
  // entry , followed by a compressed cjalr to invoke it.  This is running
  // post-RA, but the ABI guarantees that C7 is not required to be preserved
  // here and so we can use it.
  const MachineOperand Callee = MBBI->getOperand(0);
  MachineInstr &MI = *MBBI;
  auto *MF = MBB.getParent();
  bool IsTailCall = MI.getOpcode() == RISCV::PseudoCTAILLibrary;
  if (Callee.isGlobal()) {
    auto *Fn = cast<Function>(resolveGlobalAlias(Callee.getGlobal()));
    // If this is a global, check if it's defined in the same module and has a
    // compatible interrupt status.  If so, we want to lower as a direct ccall.
    if (!Fn->isDeclaration() && isSafeToDirectCall(MF->getFunction(), *Fn)) {
      MI.setDesc(
          TII->get(IsTailCall ? RISCV::PseudoCTAIL : RISCV::PseudoCCALL));
      return true;
    }
    insertLoadOfImportTable(MBB, MBBI, Fn, RISCV::X7_Y, true, true, &MI);

    NextMBBI = MBB.end();
  } else if (Callee.isSymbol()) {
    // We can see symbols here from calls to runtime functions that are
    // inserted by the back end, for example memcpy expanded from the LLVM
    // intrinsic.  These don't have accompanying LLVM functions and so we just
    // need to treat them as an external library call.
    const auto &STI = MF->getSubtarget<RISCVSubtarget>();
    if (STI.getTargetABI() == RISCVABI::ABI_CHERIOT_BAREMETAL) {
      // If baremetal just blindly use a direct call
      DEBUG_WITH_TYPE("baremetal", llvm::dbgs() <<
        "baremetal library call of " << Callee.getSymbolName() << "\n");
      MI.setDesc(
          TII->get(IsTailCall ? RISCV::PseudoCTAIL : RISCV::PseudoCCALL));
      return true;
    }
    auto ImportEntryName = getImportExportTableName(
        "libcalls", Callee.getSymbolName(), CallingConv::CHERIoT_LibraryCall,
        /*IsImport*/ true);
    auto ExportEntryName = getImportExportTableName(
        "libcalls", Callee.getSymbolName(), CallingConv::CHERIoT_LibraryCall,
        /*IsImport*/ false);
    // Create the symbol for the import entry.  We don't use this symbol
    // directly (yet) but we need to allocate storage for the string where
    // it will remain valid for the duration of codegen.
    MCSymbol *ImportSymbol =
        MF->getContext().getOrCreateSymbol(ImportEntryName);
    MCSymbol *ExportSymbol =
        MF->getContext().getOrCreateSymbol(ExportEntryName);
    insertLoadOfImportTable(MBB, MBBI, ImportSymbol, ExportSymbol,
                            Callee.getSymbolName(), RISCV::X7_Y, true, true,
                            true, &MI);

    NextMBBI = MBB.end();
  } else {
    assert(Callee.isReg() && "Expected register operand");
    // Indirect library calls are just cjalr instructions.
    auto NewCallMI = BuildMI(&MBB, MI.getDebugLoc(),
                             TII->get(IsTailCall ? RISCV::PseudoCTAILIndirect
                                                 : RISCV::C_CJALR))
                         .add(Callee);
    if (MI.shouldUpdateAdditionalCallInfo())
      MF->moveAdditionalCallInfo(NewCallMI, &MI);
  }
  
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandAuicgpInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned SecondOpcode,
    bool InBounds) {
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();
  auto *MF = MBB.getParent();

  assert(MI.getNumOperands() <= 3);
  bool IsStore = MI.getNumOperands() == 3;
  Register TmpReg = MI.getOperand(0).getReg();
  Register DestReg = MI.getOperand(IsStore ? 1 : 0).getReg();
  const MachineOperand &Symbol = MI.getOperand(IsStore ? 2 : 1);
  if (RISCV::GPRRegClass.contains(TmpReg)) {
    // When there is no explicit tmp register and the dest is a GPR,
    // then we need to get the matching cap super register for use
    // as a cap temporary.
    const TargetRegisterInfo *TRI = STI->getRegisterInfo();
    TmpReg = TRI->getMatchingSuperReg(TmpReg, RISCV::sub_cap_addr,
                                      &RISCV::YGPRRegClass);
  }

  auto *NewMBB = MBB.getParent()->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  BuildMI(NewMBB, DL, TII->get(RISCV::PseudoAUIPCCData), TmpReg)
      .addDisp(Symbol, 0, RISCVII::MO_CHERIOT1_COMPARTMENT_DATA_HI);

  unsigned SecondFlags = RISCVII::MO_CHERIOT1_COMPARTMENT_LO_I;
  if (IsStore)
    SecondFlags = RISCVII::MO_CHERIOT1_COMPARTMENT_LO_S;
  BuildMI(NewMBB, DL, TII->get(SecondOpcode))
      .addReg(DestReg, getRegState(MI.getOperand(IsStore ? 1 : 0)))
      .addReg(TmpReg, RegState::Kill)
      .addMBB(NewMBB, SecondFlags);

  if (!InBounds) {
    assert(!IsStore && "CLLC store pseudo ops must be inbounds!");
    BuildMI(NewMBB, DL, TII->get(RISCV::CSetBoundsImm), DestReg)
        .addReg(DestReg, RegState::Kill)
        .addDisp(Symbol, 0, RISCVII::MO_CHERIOT1_COMPARTMENT_SIZE);
  }

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandAuipccInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned FlagsHi,
    unsigned SecondOpcode, bool InBounds) {
  auto ABI = MBB.getParent()->getSubtarget<RISCVSubtarget>().getTargetABI();
  bool IsCheriot = ABI == RISCVABI::ABI_CHERIOT ||
                   ABI == RISCVABI::ABI_CHERIOT_BAREMETAL;
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  bool HasTmpReg = MI.getNumOperands() > 2;
  Register DestReg = MI.getOperand(0).getReg();
  Register TmpReg = MI.getOperand(HasTmpReg ? 1 : 0).getReg();
  if (RISCV::GPRRegClass.contains(TmpReg)) {
    // When there is no explicit tmp register and the dest is a GPR,
    // then we need to get the matching cap super register for use
    // as a cap temporary.
    const TargetRegisterInfo *TRI = STI->getRegisterInfo();
    TmpReg = TRI->getMatchingSuperReg(TmpReg, RISCV::sub_cap_addr,
                                      &RISCV::YGPRRegClass);
  }
  const MachineOperand &Symbol = MI.getOperand(HasTmpReg ? 2 : 1);
  if (Symbol.getTargetFlags() & RISCVII::MO_JUMP_TABLE_BASE)
    FlagsHi |= RISCVII::MO_JUMP_TABLE_BASE;

  MachineBasicBlock *NewMBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  // Tell AsmPrinter that we unconditionally want the symbol of this label to be
  // emitted.
  NewMBB->setLabelMustBeEmitted();

  MF->insert(++MBB.getIterator(), NewMBB);

  auto CheriotCapImportAttrName =
      llvm::CHERIoTGlobalCapabilityImportAttr::getAttrName();
  std::optional<Attribute> CheriotCapImportAttr = std::nullopt;
  auto CheriotSealedValueAttrName = llvm::CHERIoTSealedValueAttr::getAttrName();
  std::optional<Attribute> CheriotSealedValueAttr = std::nullopt;
  auto CheriotSealingKeyTypeAttrName =
      llvm::CHERIoTSealingKeyTypeAttr::getAttrName();
  std::optional<Attribute> CheriotSealingKeyTypeAttr = std::nullopt;

  if (Symbol.isGlobal()) {
    auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(Symbol.getGlobal());
    if (GV && GV->hasAttribute(CheriotCapImportAttrName)) {
      CheriotCapImportAttr.emplace(GV->getAttribute(CheriotCapImportAttrName));
    }
    if (GV && GV->hasAttribute(CheriotSealedValueAttrName)) {
      CheriotSealedValueAttr.emplace(
          GV->getAttribute(CheriotSealedValueAttrName));
    }
    if (GV && GV->hasAttribute(CheriotSealingKeyTypeAttrName)) {
      CheriotSealingKeyTypeAttr.emplace(
          GV->getAttribute(CheriotSealingKeyTypeAttrName));
    }
  }

  if (CheriotCapImportAttr.has_value()) {
    MCContext &Ctxt = MF->getContext();
    llvm::CHERIoTGlobalCapabilityImportAttr CapAttr(
        CheriotCapImportAttr->getValueAsString());

    // The prefixed name of the import without permissions, e.g. `mem_uart`.
    std::string PrefixedImportName =
        (CapAttr.Domain + "_" + CapAttr.ObjectName).str();

    // The prefixed name of the import with permissions, e.g. `mem_uart_RWcm`.
    std::string PrefixedImportNameWithPermissions =
        (PrefixedImportName + "_" + CapAttr.Permissions).str();

    // The name of the import without permissions, e.g. `uart`.
    auto ImportName = CapAttr.ObjectName.str();

    // The permissions of the import, e.g. `RWcm`.
    auto CapabilityPermissions = CapAttr.Permissions.str();

    std::string ExportPrefix = "";

    // The MMIO kind of imports needs an ad-hoc `export_` prefix.
    if (CapAttr.ImportKind == llvm::CHERIoTGlobalCapabilityImportAttr::MMIO) {
      ExportPrefix = "export_";
    }

    auto MangledExportName = ("__" + ExportPrefix + PrefixedImportName);
    auto MangledExportNameEnd = MangledExportName + "_end";
    auto MangledImportName = "__import_" + PrefixedImportNameWithPermissions;
    MCSymbol *MangledImportSymbol = Ctxt.getOrCreateSymbol(MangledImportName);

    BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), TmpReg)
        .addSym(MangledImportSymbol, FlagsHi);

    auto EncodedPermissions = CapAttr.encodePermissions();

    ImportedObjects.insert(
        {MangledImportName, MangledExportName, ImportName,
         CHERIoTImportedObject::LibraryFlagValue::IsNotLibrary,
         CHERIoTImportedObject::PublicFlagValue::IsPublic,
         CHERIoTImportedObject::GlobalFlagValue::IsGlobal,
         CHERIoTImportedObject::COMDATFlagValue::IsCOMDAT,
         CHERIoTImportedObject::WeakFlagValue::IsNotWeak,
         CHERIoTImportedObject::GroupedFlagValue::IsGrouped,
         CHERIoTImportedObject::WritableFlagValue::IsWritable,
         CHERIoTImportedObject::SecondWordKind::DiffAndPermsSecondWord,
         EncodedPermissions});
  } else if (CheriotSealedValueAttr.has_value()) {
    // We can safely assume, here, that GV is not null.
    auto *GV = llvm::dyn_cast<llvm::GlobalVariable>(Symbol.getGlobal());

    MCContext &Ctxt = MF->getContext();
    auto SealedObjectName = std::string(GV->getName());
    auto MangledImportName = "__import.sealed_object." + SealedObjectName;
    MCSymbol *MangledImportSymbol = Ctxt.getOrCreateSymbol(MangledImportName);

    BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), TmpReg)
        .addSym(MangledImportSymbol, FlagsHi);

    auto DL = MBB.getParent()->getDataLayout();

    assert(DL.getTypeStoreSize(GV->getValueType()) <
               std::numeric_limits<uint32_t>::max() &&
           "Size of type should be less than uint32_t::max()");

    uint32_t TypeSize = DL.getTypeStoreSize(GV->getValueType());
    ImportedObjects.insert(
        {MangledImportName, SealedObjectName, SealedObjectName,
         CHERIoTImportedObject::LibraryFlagValue::IsNotLibrary,
         CHERIoTImportedObject::PublicFlagValue::IsPublic,
         CHERIoTImportedObject::GlobalFlagValue::IsNotGlobal,
         CHERIoTImportedObject::COMDATFlagValue::IsNotCOMDAT,
         CHERIoTImportedObject::WeakFlagValue::IsWeak,
         CHERIoTImportedObject::GroupedFlagValue::IsGrouped,
         CHERIoTImportedObject::WritableFlagValue::IsWritable,
         CHERIoTImportedObject::SecondWordKind::SizeOfTypeSecondWord,
         TypeSize});
  } else if (CheriotSealingKeyTypeAttr.has_value()) {
    MCContext &Ctxt = MF->getContext();
    auto KeyTypeAttr = CheriotSealingKeyTypeAttr.value();
    auto SealingKeySymbol = KeyTypeAttr.getValueAsString();
    auto MangledImportName = "__import." + SealingKeySymbol.str();
    auto MangledExportName = "__export." + SealingKeySymbol.str();
    MCSymbol *MangledImportSymbol = Ctxt.getOrCreateSymbol(MangledImportName);

    BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), TmpReg)
        .addSym(MangledImportSymbol, FlagsHi);
    ImportedObjects.insert(
        {MangledImportName, MangledExportName, SealingKeySymbol.str(),
         CHERIoTImportedObject::LibraryFlagValue::IsNotLibrary,
         CHERIoTImportedObject::PublicFlagValue::IsPublic,
         CHERIoTImportedObject::GlobalFlagValue::IsGlobal,
         CHERIoTImportedObject::COMDATFlagValue::IsCOMDAT,
         CHERIoTImportedObject::WeakFlagValue::IsNotWeak,
         CHERIoTImportedObject::GroupedFlagValue::IsGrouped,
         CHERIoTImportedObject::WritableFlagValue::IsWritable,
         CHERIoTImportedObject::SecondWordKind::SizeOfTypeSecondWord, 0});
  } else {
    BuildMI(NewMBB, DL, TII->get(RISCV::AUIPCC), TmpReg)
        .addDisp(Symbol, 0, FlagsHi);
  }

  unsigned SecondFlags = RISCVII::MO_PCREL_LO;
  if (IsCheriot)
    SecondFlags = RISCVII::MO_CHERIOT1_COMPARTMENT_LO_I;
  BuildMI(NewMBB, DL, TII->get(SecondOpcode), DestReg)
      .addReg(TmpReg)
      .addMBB(NewMBB, SecondFlags);
  if (!CheriotSealedValueAttr.has_value() &&
      !CheriotCapImportAttr.has_value() &&
      !CheriotSealingKeyTypeAttr.has_value() && !InBounds &&
      MF->getSubtarget<RISCVSubtarget>().isRV32E() && Symbol.isGlobal() &&
      isa<GlobalVariable>(Symbol.getGlobal()) &&
      (cast<GlobalVariable>(Symbol.getGlobal())->getSection() !=
       ".compartment_imports"))
    BuildMI(NewMBB, DL, TII->get(RISCV::CSetBoundsImm), DestReg)
        .addReg(DestReg)
        .addDisp(Symbol, 0, RISCVII::MO_CHERIOT1_COMPARTMENT_SIZE);

  // Move all the rest of the instructions to NewMBB.
  NewMBB->splice(NewMBB->end(), &MBB, std::next(MBBI), MBB.end());
  // Update machine-CFG edges.
  NewMBB->transferSuccessorsAndUpdatePHIs(&MBB);
  // Make the original basic block fall-through to the new.
  MBB.addSuccessor(NewMBB);

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *NewMBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandCapLoadLocalCap(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, bool InBounds) {
  auto ABI = MBB.getParent()->getSubtarget<RISCVSubtarget>().getTargetABI();
  if (ABI == RISCVABI::ABI_CHERIOT || ABI == RISCVABI::ABI_CHERIOT_BAREMETAL) {
    const MachineOperand &Symbol = MBBI->getOperand(1);
    if (!Symbol.isGlobal() ||
        Symbol.getTargetFlags() == RISCVII::MO_JUMP_TABLE_BASE)
      return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                                  RISCVII::MO_CHERIOT1_COMPARTMENT_CODE_HI,
                                  RISCV::CIncOffsetImm);

    const GlobalValue *GV = Symbol.getGlobal();
    auto *GVar = llvm::dyn_cast<GlobalVariable>(GV);
    if (GVar &&
        (GVar->hasAttribute(
             llvm::CHERIoTGlobalCapabilityImportAttr::getAttrName()) ||
         GVar->hasAttribute(llvm::CHERIoTSealedValueAttr::getAttrName()) ||
         GVar->hasAttribute(llvm::CHERIoTSealingKeyTypeAttr::getAttrName()))) {
      return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                                  RISCVII::MO_CHERIOT1_COMPARTMENT_CODE_HI,
                                  RISCV::CLC_64);
    }

    if (!isa<Function>(GV) && !cast<GlobalVariable>(GV)->isConstant())
      return expandAuicgpInstPair(MBB, MBBI, NextMBBI, RISCV::CIncOffsetImm,
                                  InBounds);

    auto *Fn = dyn_cast<Function>(GV);
    if (!Fn)
      return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                            RISCVII::MO_CHERIOT1_COMPARTMENT_CODE_HI,
                            RISCV::CIncOffsetImm, InBounds);

    auto CC = Fn->getCallingConv();
    if ((getInterruptStatus(*Fn) != Interrupts::Inherit) ||
        (CC == CallingConv::CHERIoT_CompartmentCall) ||
        (CC == CallingConv::CHERIoT_CompartmentCallee)) {
      insertLoadOfImportTable(MBB, MBBI, Fn, MBBI->getOperand(0).getReg(),
        /*TreatAsLibrary*/CC == CallingConv::C);
      NextMBBI = MBB.end();
      MBBI->eraseFromParent();
      return true;
    }

    return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                                RISCVII::MO_CHERIOT1_COMPARTMENT_CODE_HI,
                                RISCV::CIncOffsetImm, InBounds);
  }

  return expandAuipccInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_PCREL_HI,
                              RISCV::CIncOffsetImm);
}

bool RISCVExpandPseudo::expandDerefCapLoadLocalCap(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned DerefOpcode) {
  auto ABI = MBB.getParent()->getSubtarget<RISCVSubtarget>().getTargetABI();
  if (ABI == RISCVABI::ABI_CHERIOT || ABI == RISCVABI::ABI_CHERIOT_BAREMETAL) {
    bool IsStore = MBBI->getNumOperands() == 3;
    const MachineOperand &Symbol = MBBI->getOperand(IsStore ? 2 : 1);
    const GlobalVariable *GV = cast<GlobalVariable>(Symbol.getGlobal());
    if (IsStore || !GV->isConstant())
      return expandAuicgpInstPair(MBB, MBBI, NextMBBI, DerefOpcode, true);
    return expandAuipccInstPair(MBB, MBBI, NextMBBI,
                                RISCVII::MO_CHERIOT1_COMPARTMENT_CODE_HI, DerefOpcode,
                                true);
  }

  return expandAuipccInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_PCREL_HI,
                              DerefOpcode);
}

bool RISCVExpandPseudo::expandCapLoadGlobalCap(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();

  const auto &STI = MF->getSubtarget<RISCVSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RISCV::CLC_128 : RISCV::CLC_64;
  return expandAuipccInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_GOT_HI,
                              SecondOpcode);
}

bool RISCVExpandPseudo::expandCapLoadTLSIEAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();
  assert(!STI->hasVendorXCheriot() && "TLS is not supported on XCheriot!");

  const auto &STI = MF->getSubtarget<RISCVSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RISCV::CLD : RISCV::CLW;
  unsigned FlagsHi;
  if (MCTargetOptions::cheriTLSUseTGOT())
    FlagsHi = RISCVII::MO_TLS_TGOT_GOT_HI;
  else
    FlagsHi = RISCVII::MO_TLS_GOT_HI;
  return expandAuipccInstPair(MBB, MBBI, NextMBBI, FlagsHi, SecondOpcode);
}

bool RISCVExpandPseudo::expandCapLoadTLSGDCap(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  assert(!STI->hasVendorXCheriot() && "TLS is not supported on XCheriot!");
  unsigned FlagsHi;
  if (MCTargetOptions::cheriTLSUseTGOT())
    FlagsHi = RISCVII::MO_TLS_TGOT_GD_HI;
  else
    FlagsHi = RISCVII::MO_TLS_GD_HI;
  return expandAuipccInstPair(MBB, MBBI, NextMBBI, FlagsHi,
                              RISCV::CIncOffsetImm);
}

bool RISCVExpandPseudo::expandCGetAddr(MachineBasicBlock &MBB,
                                       MachineBasicBlock::iterator MBBI) {
  const auto &STI = MBB.getParent()->getSubtarget<RISCVSubtarget>();
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI.getRegisterInfo();
  // TODO: We could replace this pseudo with a subregister read if none of the
  // readers can end up leaking the capability privileges.
  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADDI), MBBI->getOperand(0).getReg())
      .addReg(TRI->getSubReg(MBBI->getOperand(1).getReg(), RISCV::sub_cap_addr),
              getRegState(MBBI->getOperand(1)))
      .addImm(0);
  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool RISCVExpandPseudo::expandCCOp(MachineBasicBlock &MBB,
                                   MachineBasicBlock::iterator MBBI,
                                   MachineBasicBlock::iterator &NextMBBI) {
  // First try expanding to a Conditional Move rather than a branch+mv
  if (expandCCOpToCMov(MBB, MBBI))
    return true;

  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  MachineBasicBlock *TrueBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());
  MachineBasicBlock *MergeBB = MF->CreateMachineBasicBlock(MBB.getBasicBlock());

  MF->insert(++MBB.getIterator(), TrueBB);
  MF->insert(++TrueBB->getIterator(), MergeBB);

  // We want to copy the "true" value only when the branch is executed.
  // The SDNodeXform is responsible for the inversion.
  unsigned BranchOpCode =
      MI.getOperand(MI.getNumExplicitOperands() - 3).getImm();

  // Insert branch instruction.
  BuildMI(MBB, MBBI, DL, TII->get(BranchOpCode))
      .add(MI.getOperand(MI.getNumExplicitOperands() - 2))
      .add(MI.getOperand(MI.getNumExplicitOperands() - 1))
      .addMBB(MergeBB);

  Register DestReg = MI.getOperand(0).getReg();
  assert(MI.getOperand(1).getReg() == DestReg);

  if (MI.getOpcode() == RISCV::PseudoCCMOVGPR ||
      MI.getOpcode() == RISCV::PseudoCCMOVGPRNoX0) {
    // Add MV.
    BuildMI(TrueBB, DL, TII->get(RISCV::ADDI), DestReg)
        .add(MI.getOperand(2))
        .addImm(0);
  } else {
    unsigned NewOpc;
    // clang-format off
    switch (MI.getOpcode()) {
    default:
      llvm_unreachable("Unexpected opcode!");
    case RISCV::PseudoCCADD:   NewOpc = RISCV::ADD;   break;
    case RISCV::PseudoCCSUB:   NewOpc = RISCV::SUB;   break;
    case RISCV::PseudoCCSLL:   NewOpc = RISCV::SLL;   break;
    case RISCV::PseudoCCSRL:   NewOpc = RISCV::SRL;   break;
    case RISCV::PseudoCCSRA:   NewOpc = RISCV::SRA;   break;
    case RISCV::PseudoCCAND:   NewOpc = RISCV::AND;   break;
    case RISCV::PseudoCCOR:    NewOpc = RISCV::OR;    break;
    case RISCV::PseudoCCXOR:   NewOpc = RISCV::XOR;   break;
    case RISCV::PseudoCCMAX:   NewOpc = RISCV::MAX;   break;
    case RISCV::PseudoCCMIN:   NewOpc = RISCV::MIN;   break;
    case RISCV::PseudoCCMAXU:  NewOpc = RISCV::MAXU;  break;
    case RISCV::PseudoCCMINU:  NewOpc = RISCV::MINU;  break;
    case RISCV::PseudoCCMUL:   NewOpc = RISCV::MUL;   break;
    case RISCV::PseudoCCLUI:   NewOpc = RISCV::LUI;   break;
    case RISCV::PseudoCCQC_E_LB:  NewOpc = RISCV::QC_E_LB;    break;
    case RISCV::PseudoCCQC_E_LH:  NewOpc = RISCV::QC_E_LH;    break;
    case RISCV::PseudoCCQC_E_LW:  NewOpc = RISCV::QC_E_LW;    break;
    case RISCV::PseudoCCQC_E_LHU: NewOpc = RISCV::QC_E_LHU;   break;
    case RISCV::PseudoCCQC_E_LBU: NewOpc = RISCV::QC_E_LBU;   break;
    case RISCV::PseudoCCLB:    NewOpc = RISCV::LB;    break;
    case RISCV::PseudoCCLH:    NewOpc = RISCV::LH;    break;
    case RISCV::PseudoCCLW:    NewOpc = RISCV::LW;    break;
    case RISCV::PseudoCCLHU:   NewOpc = RISCV::LHU;   break;
    case RISCV::PseudoCCLBU:   NewOpc = RISCV::LBU;   break;
    case RISCV::PseudoCCLWU:   NewOpc = RISCV::LWU;   break;
    case RISCV::PseudoCCLD:    NewOpc = RISCV::LD;    break;
    case RISCV::PseudoCCQC_LI:  NewOpc = RISCV::QC_LI;   break;
    case RISCV::PseudoCCQC_E_LI: NewOpc = RISCV::QC_E_LI;   break;
    case RISCV::PseudoCCADDI:  NewOpc = RISCV::ADDI;  break;
    case RISCV::PseudoCCSLLI:  NewOpc = RISCV::SLLI;  break;
    case RISCV::PseudoCCSRLI:  NewOpc = RISCV::SRLI;  break;
    case RISCV::PseudoCCSRAI:  NewOpc = RISCV::SRAI;  break;
    case RISCV::PseudoCCANDI:  NewOpc = RISCV::ANDI;  break;
    case RISCV::PseudoCCORI:   NewOpc = RISCV::ORI;   break;
    case RISCV::PseudoCCXORI:  NewOpc = RISCV::XORI;  break;
    case RISCV::PseudoCCADDW:  NewOpc = RISCV::ADDW;  break;
    case RISCV::PseudoCCSUBW:  NewOpc = RISCV::SUBW;  break;
    case RISCV::PseudoCCSLLW:  NewOpc = RISCV::SLLW;  break;
    case RISCV::PseudoCCSRLW:  NewOpc = RISCV::SRLW;  break;
    case RISCV::PseudoCCSRAW:  NewOpc = RISCV::SRAW;  break;
    case RISCV::PseudoCCADDIW: NewOpc = RISCV::ADDIW; break;
    case RISCV::PseudoCCSLLIW: NewOpc = RISCV::SLLIW; break;
    case RISCV::PseudoCCSRLIW: NewOpc = RISCV::SRLIW; break;
    case RISCV::PseudoCCSRAIW: NewOpc = RISCV::SRAIW; break;
    case RISCV::PseudoCCANDN:  NewOpc = RISCV::ANDN;  break;
    case RISCV::PseudoCCORN:   NewOpc = RISCV::ORN;   break;
    case RISCV::PseudoCCXNOR:  NewOpc = RISCV::XNOR;  break;
    case RISCV::PseudoCCNDS_BFOS: NewOpc = RISCV::NDS_BFOS; break;
    case RISCV::PseudoCCNDS_BFOZ: NewOpc = RISCV::NDS_BFOZ; break;
    }
    // clang-format on

    if (NewOpc == RISCV::NDS_BFOZ || NewOpc == RISCV::NDS_BFOS) {
      BuildMI(TrueBB, DL, TII->get(NewOpc), DestReg)
          .add(MI.getOperand(2))
          .add(MI.getOperand(3))
          .add(MI.getOperand(4));
    } else if (NewOpc == RISCV::LUI || NewOpc == RISCV::QC_LI ||
               NewOpc == RISCV::QC_E_LI) {
      BuildMI(TrueBB, DL, TII->get(NewOpc), DestReg).add(MI.getOperand(2));
    } else {
      BuildMI(TrueBB, DL, TII->get(NewOpc), DestReg)
          .add(MI.getOperand(2))
          .add(MI.getOperand(3));
    }
  }

  TrueBB->addSuccessor(MergeBB);

  MergeBB->splice(MergeBB->end(), &MBB, MI, MBB.end());
  MergeBB->transferSuccessors(&MBB);

  MBB.addSuccessor(TrueBB);
  MBB.addSuccessor(MergeBB);

  NextMBBI = MBB.end();
  MI.eraseFromParent();

  // Make sure live-ins are correctly attached to this new basic block.
  LivePhysRegs LiveRegs;
  computeAndAddLiveIns(LiveRegs, *TrueBB);
  computeAndAddLiveIns(LiveRegs, *MergeBB);

  return true;
}

bool RISCVExpandPseudo::expandCCOpToCMov(MachineBasicBlock &MBB,
                                         MachineBasicBlock::iterator MBBI) {
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  if (MI.getOpcode() != RISCV::PseudoCCMOVGPR &&
      MI.getOpcode() != RISCV::PseudoCCMOVGPRNoX0)
    return false;

  if (!STI->hasVendorXqcicm())
    return false;

  MachineOperand &LHS = MI.getOperand(MI.getNumExplicitOperands() - 2);
  MachineOperand &RHS = MI.getOperand(MI.getNumExplicitOperands() - 1);

  // FIXME: Would be wonderful to support LHS=X0, but not very easy.
  if (LHS.getReg() == RISCV::X0 || MI.getOperand(1).getReg() == RISCV::X0 ||
      MI.getOperand(2).getReg() == RISCV::X0)
    return false;

  // Use branch opcode to select appropriate Xqcicm instruction
  unsigned BCC = MI.getOperand(MI.getNumExplicitOperands() - 3).getImm();
  std::optional<unsigned> CMovRegOpcode;
  unsigned CMovImmOpcode;
  switch (BCC) {
  default:
    return false; // Unhandled branch opcodes
  case RISCV::BNE:
    CMovRegOpcode = RISCV::QC_MVEQ;
    CMovImmOpcode = RISCV::QC_MVEQI;
    break;
  case RISCV::BEQ:
    CMovRegOpcode = RISCV::QC_MVNE;
    CMovImmOpcode = RISCV::QC_MVNEI;
    break;
  case RISCV::BGE:
    CMovRegOpcode = RISCV::QC_MVLT;
    CMovImmOpcode = RISCV::QC_MVLTI;
    break;
  case RISCV::BLT:
    CMovRegOpcode = RISCV::QC_MVGE;
    CMovImmOpcode = RISCV::QC_MVGEI;
    break;
  case RISCV::BGEU:
    CMovRegOpcode = RISCV::QC_MVLTU;
    CMovImmOpcode = RISCV::QC_MVLTUI;
    break;
  case RISCV::BLTU:
    CMovRegOpcode = RISCV::QC_MVGEU;
    CMovImmOpcode = RISCV::QC_MVGEUI;
    break;
  case RISCV::QC_BEQI:
    CMovImmOpcode = RISCV::QC_MVNEI;
    break;
  case RISCV::QC_BNEI:
    CMovImmOpcode = RISCV::QC_MVEQI;
    break;
  case RISCV::QC_BLTI:
    CMovImmOpcode = RISCV::QC_MVGEI;
    break;
  case RISCV::QC_BGEI:
    CMovImmOpcode = RISCV::QC_MVLTI;
    break;
  case RISCV::QC_BLTUI:
    CMovImmOpcode = RISCV::QC_MVGEUI;
    break;
  case RISCV::QC_BGEUI:
    CMovImmOpcode = RISCV::QC_MVLTUI;
    break;
  }

  if (RHS.isImm() && isInt<5>(RHS.getImm())) {
    // $dst = PseudoCCMOVGPR $falsev(=$dst), $truev, $opcode, $lhs, $rhs_imm
    // $dst = PseudoCCMOVGPRNoX0 $falsev(=$dst), $truev, $opcode, $lhs, $rhs_imm
    // =>
    // $dst = QC_MVccI $falsev (=$dst), $lhs, $rhs_imm, $truev
    BuildMI(MBB, MBBI, DL, TII->get(CMovImmOpcode))
        .addDef(MI.getOperand(0).getReg())
        .addReg(MI.getOperand(1).getReg())
        .addReg(LHS.getReg())
        .add(RHS)
        .addReg(MI.getOperand(2).getReg());

    MI.eraseFromParent();
    return true;
  }

  if (RHS.getReg() == RISCV::X0) {
    // $dst = PseudoCCMOVGPR $falsev (=$dst), $truev, $opcode, $lhs, X0
    // $dst = PseudoCCMOVGPRNoX0 $falsev (=$dst), $truev, $opcode, $lhs, X0
    // =>
    // $dst = QC_MVccI $falsev (=$dst), $lhs, 0, $truev
    BuildMI(MBB, MBBI, DL, TII->get(CMovImmOpcode))
        .addDef(MI.getOperand(0).getReg())
        .addReg(MI.getOperand(1).getReg())
        .addReg(LHS.getReg())
        .addImm(0)
        .addReg(MI.getOperand(2).getReg());

    MI.eraseFromParent();
    return true;
  }

  if (!CMovRegOpcode)
    return false;

  // $dst = PseudoCCMOVGPR $falsev (=$dst), $truev, $opcode, $lhs, $rhs
  // $dst = PseudoCCMOVGPRNoX0 $falsev (=$dst), $truev, $opcode, $lhs, $rhs
  // =>
  // $dst = QC_MVcc $falsev (=$dst), $lhs, $rhs, $truev
  BuildMI(MBB, MBBI, DL, TII->get(*CMovRegOpcode))
      .addDef(MI.getOperand(0).getReg())
      .addReg(MI.getOperand(1).getReg())
      .addReg(LHS.getReg())
      .addReg(RHS.getReg())
      .addReg(MI.getOperand(2).getReg());
  MI.eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandVMSET_VMCLR(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI,
                                          unsigned Opcode) {
  DebugLoc DL = MBBI->getDebugLoc();
  Register DstReg = MBBI->getOperand(0).getReg();
  const MCInstrDesc &Desc = TII->get(Opcode);
  BuildMI(MBB, MBBI, DL, Desc, DstReg)
      .addReg(DstReg, RegState::Undef)
      .addReg(DstReg, RegState::Undef);
  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool RISCVExpandPseudo::expandMV_FPR16INX(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register DstReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(0).getReg(), RISCV::sub_16, &RISCV::GPRRegClass);
  Register SrcReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(1).getReg(), RISCV::sub_16, &RISCV::GPRRegClass);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADDI), DstReg)
      .addReg(SrcReg, getKillRegState(MBBI->getOperand(1).isKill()))
      .addImm(0);

  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

bool RISCVExpandPseudo::expandMV_FPR32INX(MachineBasicBlock &MBB,
                                          MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register DstReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(0).getReg(), RISCV::sub_32, &RISCV::GPRRegClass);
  Register SrcReg = TRI->getMatchingSuperReg(
      MBBI->getOperand(1).getReg(), RISCV::sub_32, &RISCV::GPRRegClass);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADDI), DstReg)
      .addReg(SrcReg, getKillRegState(MBBI->getOperand(1).isKill()))
      .addImm(0);

  MBBI->eraseFromParent(); // The pseudo instruction is gone now.
  return true;
}

// This function expands the PseudoRV32ZdinxSD for storing a double-precision
// floating-point value into memory by generating an equivalent instruction
// sequence for RV32.
bool RISCVExpandPseudo::expandRV32ZdinxStore(MachineBasicBlock &MBB,
                                             MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register Lo =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_even);
  Register Hi =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_odd);
  if (Hi == RISCV::DUMMY_REG_PAIR_WITH_X0)
    Hi = RISCV::X0;

  auto MIBLo = BuildMI(MBB, MBBI, DL, TII->get(RISCV::SW))
                   .addReg(Lo, getKillRegState(MBBI->getOperand(0).isKill()))
                   .addReg(MBBI->getOperand(1).getReg())
                   .add(MBBI->getOperand(2));

  MachineInstrBuilder MIBHi;
  if (MBBI->getOperand(2).isGlobal() || MBBI->getOperand(2).isCPI()) {
    assert(MBBI->getOperand(2).getOffset() % 8 == 0);
    MBBI->getOperand(2).setOffset(MBBI->getOperand(2).getOffset() + 4);
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::SW))
                .addReg(Hi, getKillRegState(MBBI->getOperand(0).isKill()))
                .add(MBBI->getOperand(1))
                .add(MBBI->getOperand(2));
  } else {
    assert(isInt<12>(MBBI->getOperand(2).getImm() + 4));
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::SW))
                .addReg(Hi, getKillRegState(MBBI->getOperand(0).isKill()))
                .add(MBBI->getOperand(1))
                .addImm(MBBI->getOperand(2).getImm() + 4);
  }

  MachineFunction *MF = MBB.getParent();
  SmallVector<MachineMemOperand *> NewLoMMOs;
  SmallVector<MachineMemOperand *> NewHiMMOs;
  for (const MachineMemOperand *MMO : MBBI->memoperands()) {
    NewLoMMOs.push_back(MF->getMachineMemOperand(MMO, 0, 4));
    NewHiMMOs.push_back(MF->getMachineMemOperand(MMO, 4, 4));
  }
  MIBLo.setMemRefs(NewLoMMOs);
  MIBHi.setMemRefs(NewHiMMOs);

  MBBI->eraseFromParent();
  return true;
}

// This function expands PseudoRV32ZdinxLoad for loading a double-precision
// floating-point value from memory into an equivalent instruction sequence for
// RV32.
bool RISCVExpandPseudo::expandRV32ZdinxLoad(MachineBasicBlock &MBB,
                                            MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  const TargetRegisterInfo *TRI = STI->getRegisterInfo();
  Register Lo =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_even);
  Register Hi =
      TRI->getSubReg(MBBI->getOperand(0).getReg(), RISCV::sub_gpr_odd);
  assert(Hi != RISCV::DUMMY_REG_PAIR_WITH_X0 && "Cannot write to X0_Pair");

  MachineInstrBuilder MIBLo, MIBHi;

  // If the register of operand 1 is equal to the Lo register, then swap the
  // order of loading the Lo and Hi statements.
  bool IsOp1EqualToLo = Lo == MBBI->getOperand(1).getReg();
  // Order: Lo, Hi
  if (!IsOp1EqualToLo) {
    MIBLo = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Lo)
                .addReg(MBBI->getOperand(1).getReg())
                .add(MBBI->getOperand(2));
  }

  if (MBBI->getOperand(2).isGlobal() || MBBI->getOperand(2).isCPI()) {
    auto Offset = MBBI->getOperand(2).getOffset();
    assert(Offset % 8 == 0);
    MBBI->getOperand(2).setOffset(Offset + 4);
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Hi)
                .addReg(MBBI->getOperand(1).getReg())
                .add(MBBI->getOperand(2));
    MBBI->getOperand(2).setOffset(Offset);
  } else {
    assert(isInt<12>(MBBI->getOperand(2).getImm() + 4));
    MIBHi = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Hi)
                .addReg(MBBI->getOperand(1).getReg())
                .addImm(MBBI->getOperand(2).getImm() + 4);
  }

  // Order: Hi, Lo
  if (IsOp1EqualToLo) {
    MIBLo = BuildMI(MBB, MBBI, DL, TII->get(RISCV::LW), Lo)
                .addReg(MBBI->getOperand(1).getReg())
                .add(MBBI->getOperand(2));
  }

  MachineFunction *MF = MBB.getParent();
  SmallVector<MachineMemOperand *> NewLoMMOs;
  SmallVector<MachineMemOperand *> NewHiMMOs;
  for (const MachineMemOperand *MMO : MBBI->memoperands()) {
    NewLoMMOs.push_back(MF->getMachineMemOperand(MMO, 0, 4));
    NewHiMMOs.push_back(MF->getMachineMemOperand(MMO, 4, 4));
  }
  MIBLo.setMemRefs(NewLoMMOs);
  MIBHi.setMemRefs(NewHiMMOs);

  MBBI->eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandPseudoReadVLENBViaVSETVLIX0(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI) {
  DebugLoc DL = MBBI->getDebugLoc();
  Register Dst = MBBI->getOperand(0).getReg();
  unsigned Mul = MBBI->getOperand(1).getImm();
  RISCVVType::VLMUL VLMUL = RISCVVType::encodeLMUL(Mul, /*Fractional=*/false);
  unsigned VTypeImm = RISCVVType::encodeVTYPE(
      VLMUL, /*SEW=*/8, /*TailAgnostic=*/true, /*MaskAgnostic=*/true);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::PseudoVSETVLIX0))
      .addReg(Dst, RegState::Define)
      .addReg(RISCV::X0, RegState::Kill)
      .addImm(VTypeImm);

  MBBI->eraseFromParent();
  return true;
}

bool RISCVExpandPseudo::expandPseudoClearFPR64(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI) {
  const DebugLoc &DL = MBBI->getDebugLoc();
  Register Dst = MBBI->getOperand(0).getReg();

  if (STI->is64Bit()) {
    BuildMI(MBB, MBBI, DL, TII->get(RISCV::FMV_D_X), Dst).addReg(RISCV::X0);
  } else {
    BuildMI(MBB, MBBI, DL, TII->get(RISCV::FCVT_D_W), Dst)
        .addReg(RISCV::X0)
        .addImm(RISCVFPRndMode::RNE);
  }

  MBBI->eraseFromParent();
  return true;
}

class RISCVPreRAExpandPseudo : public MachineFunctionPass {
public:
  const RISCVSubtarget *STI;
  const RISCVInstrInfo *TII;
  static char ID;

  RISCVPreRAExpandPseudo() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }
  StringRef getPassName() const override {
    return RISCV_PRERA_EXPAND_PSEUDO_NAME;
  }

private:
  bool expandMBB(MachineBasicBlock &MBB);
  bool expandMI(MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
                MachineBasicBlock::iterator &NextMBBI);
  bool expandAuipcInstPair(MachineBasicBlock &MBB,
                           MachineBasicBlock::iterator MBBI,
                           MachineBasicBlock::iterator &NextMBBI,
                           unsigned FlagsHi, unsigned SecondOpcode);
  bool expandLoadLocalAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadGlobalAddress(MachineBasicBlock &MBB,
                               MachineBasicBlock::iterator MBBI,
                               MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSIEAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSGDAddress(MachineBasicBlock &MBB,
                              MachineBasicBlock::iterator MBBI,
                              MachineBasicBlock::iterator &NextMBBI);
  bool expandLoadTLSDescAddress(MachineBasicBlock &MBB,
                                MachineBasicBlock::iterator MBBI,
                                MachineBasicBlock::iterator &NextMBBI);

#ifndef NDEBUG
  unsigned getInstSizeInBytes(const MachineFunction &MF) const {
    unsigned Size = 0;
    for (auto &MBB : MF)
      for (auto &MI : MBB)
        Size += TII->getInstSizeInBytes(MI);
    return Size;
  }
#endif
};

char RISCVPreRAExpandPseudo::ID = 0;

bool RISCVPreRAExpandPseudo::runOnMachineFunction(MachineFunction &MF) {
  STI = &MF.getSubtarget<RISCVSubtarget>();
  TII = STI->getInstrInfo();

#ifndef NDEBUG
  const unsigned OldSize = getInstSizeInBytes(MF);
#endif

  bool Modified = false;
  for (auto &MBB : MF)
    Modified |= expandMBB(MBB);

#ifndef NDEBUG
  const unsigned NewSize = getInstSizeInBytes(MF);
  assert(OldSize >= NewSize);
#endif
  return Modified;
}

bool RISCVPreRAExpandPseudo::expandMBB(MachineBasicBlock &MBB) {
  bool Modified = false;

  MachineBasicBlock::iterator MBBI = MBB.begin(), E = MBB.end();
  while (MBBI != E) {
    MachineBasicBlock::iterator NMBBI = std::next(MBBI);
    Modified |= expandMI(MBB, MBBI, NMBBI);
    MBBI = NMBBI;
  }

  return Modified;
}

bool RISCVPreRAExpandPseudo::expandMI(MachineBasicBlock &MBB,
                                      MachineBasicBlock::iterator MBBI,
                                      MachineBasicBlock::iterator &NextMBBI) {

  switch (MBBI->getOpcode()) {
  case RISCV::PseudoLLA:
    return expandLoadLocalAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLGA:
    return expandLoadGlobalAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLS_IE:
    return expandLoadTLSIEAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLS_GD:
    return expandLoadTLSGDAddress(MBB, MBBI, NextMBBI);
  case RISCV::PseudoLA_TLSDESC:
    return expandLoadTLSDescAddress(MBB, MBBI, NextMBBI);
  }
  return false;
}

bool RISCVPreRAExpandPseudo::expandAuipcInstPair(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI, unsigned FlagsHi,
    unsigned SecondOpcode) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  Register DestReg = MI.getOperand(0).getReg();
  Register ScratchReg =
      MF->getRegInfo().createVirtualRegister(&RISCV::GPRRegClass);

  MachineOperand &Symbol = MI.getOperand(1);
  Symbol.setTargetFlags(FlagsHi);
  MCSymbol *AUIPCSymbol = MF->getContext().createNamedTempSymbol("pcrel_hi");

  MachineInstr *MIAUIPC =
      BuildMI(MBB, MBBI, DL, TII->get(RISCV::AUIPC), ScratchReg).add(Symbol);
  MIAUIPC->setPreInstrSymbol(*MF, AUIPCSymbol);

  MachineInstr *SecondMI =
      BuildMI(MBB, MBBI, DL, TII->get(SecondOpcode), DestReg)
          .addReg(ScratchReg)
          .addSym(AUIPCSymbol, RISCVII::MO_PCREL_LO);

  if (MI.hasOneMemOperand())
    SecondMI->addMemOperand(*MF, *MI.memoperands_begin());

  MI.eraseFromParent();
  return true;
}

bool RISCVPreRAExpandPseudo::expandLoadLocalAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_PCREL_HI,
                             RISCV::ADDI);
}

bool RISCVPreRAExpandPseudo::expandLoadGlobalAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  unsigned SecondOpcode = STI->is64Bit() ? RISCV::LD : RISCV::LW;
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_GOT_HI,
                             SecondOpcode);
}

bool RISCVPreRAExpandPseudo::expandLoadTLSIEAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  unsigned SecondOpcode = STI->is64Bit() ? RISCV::LD : RISCV::LW;
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_TLS_GOT_HI,
                             SecondOpcode);
}

bool RISCVPreRAExpandPseudo::expandLoadTLSGDAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  return expandAuipcInstPair(MBB, MBBI, NextMBBI, RISCVII::MO_TLS_GD_HI,
                             RISCV::ADDI);
}

bool RISCVPreRAExpandPseudo::expandLoadTLSDescAddress(
    MachineBasicBlock &MBB, MachineBasicBlock::iterator MBBI,
    MachineBasicBlock::iterator &NextMBBI) {
  MachineFunction *MF = MBB.getParent();
  MachineInstr &MI = *MBBI;
  DebugLoc DL = MI.getDebugLoc();

  const auto &STI = MF->getSubtarget<RISCVSubtarget>();
  unsigned SecondOpcode = STI.is64Bit() ? RISCV::LD : RISCV::LW;

  Register FinalReg = MI.getOperand(0).getReg();
  Register DestReg =
      MF->getRegInfo().createVirtualRegister(&RISCV::GPRRegClass);
  Register ScratchReg =
      MF->getRegInfo().createVirtualRegister(&RISCV::GPRRegClass);

  MachineOperand &Symbol = MI.getOperand(1);
  Symbol.setTargetFlags(RISCVII::MO_TLSDESC_HI);
  MCSymbol *AUIPCSymbol = MF->getContext().createNamedTempSymbol("tlsdesc_hi");

  MachineInstr *MIAUIPC =
      BuildMI(MBB, MBBI, DL, TII->get(RISCV::AUIPC), ScratchReg).add(Symbol);
  MIAUIPC->setPreInstrSymbol(*MF, AUIPCSymbol);

  BuildMI(MBB, MBBI, DL, TII->get(SecondOpcode), DestReg)
      .addReg(ScratchReg)
      .addSym(AUIPCSymbol, RISCVII::MO_TLSDESC_LOAD_LO);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADDI), RISCV::X10)
      .addReg(ScratchReg)
      .addSym(AUIPCSymbol, RISCVII::MO_TLSDESC_ADD_LO);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::PseudoTLSDESCCall), RISCV::X5)
      .addReg(DestReg)
      .addImm(0)
      .addSym(AUIPCSymbol, RISCVII::MO_TLSDESC_CALL);

  BuildMI(MBB, MBBI, DL, TII->get(RISCV::ADD), FinalReg)
      .addReg(RISCV::X10)
      .addReg(RISCV::X4);

  MI.eraseFromParent();
  return true;
}

} // end of anonymous namespace

namespace llvm {

template <> Pass *callDefaultCtor<RISCVExpandPseudo>() {
  static CHERIoTImportedObjectSet CHERIoTImports;
  return new RISCVExpandPseudo(CHERIoTImports);
}

FunctionPass *
createRISCVExpandPseudoPass(CHERIoTImportedObjectSet &CHERIoTImports) {
  return new RISCVExpandPseudo(CHERIoTImports);
}

FunctionPass *createRISCVPreRAExpandPseudoPass() { return new RISCVPreRAExpandPseudo(); }

} // end of namespace llvm

INITIALIZE_PASS(RISCVExpandPseudo, "riscv-expand-pseudo",
                RISCV_EXPAND_PSEUDO_NAME, false, false)

INITIALIZE_PASS(RISCVPreRAExpandPseudo, "riscv-prera-expand-pseudo",
                RISCV_PRERA_EXPAND_PSEUDO_NAME, false, false)
