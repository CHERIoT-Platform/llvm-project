//===-- RISCVCheriCleanupOptPassInsts.cpp - Expand atomic pseudo instrs.
//---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains a pass that expands atomic pseudo instructions into
// target instructions. This pass should be run at the last possible moment,
// avoiding the possibility for other passes to break the requirements for
// forward progress in the LR/SC block.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "MCTargetDesc/RISCVMatInt.h"
#include "RISCV.h"
#include "RISCVInstrInfo.h"
#include "RISCVSubtarget.h"
#include "RISCVTargetMachine.h"

#include "llvm/CodeGen/LivePhysRegs.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/GlobalVariable.h"

using namespace llvm;

#define RISCV_CHERI_CLEANUP_NAME "RISCV CHERIoT early bounds check elision pass"

namespace {

class RISCVCheriCleanupOpt : public MachineFunctionPass {
public:
  const RISCVInstrInfo *TII;
  inline static char ID;

  RISCVCheriCleanupOpt() : MachineFunctionPass(ID) {
    initializeRISCVCheriCleanupOptPass(*PassRegistry::getPassRegistry());
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

  StringRef getPassName() const override { return RISCV_CHERI_CLEANUP_NAME; }
};

static bool rewriteMemoryReference(MachineOperand &Op,
                                   const MachineOperand &Src,
                                   MachineRegisterInfo &MRI,
                                   const TargetInstrInfo *TII) {
  // Update the opcode to an appropriate CLLC pseudo which will get expanded
  // post-RA.
  if (Op.getOperandNo() != 1)
    return false;
  bool IsStore = false;
  MachineInstr &II = *Op.getParent();
  switch (Op.getParent()->getOpcode()) {
  default:
  case RISCV::CLB:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLB));
    break;
  case RISCV::CLBU:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLBU));
    break;
  case RISCV::CSB:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CSB));
    IsStore = true;
    break;
  case RISCV::CLH:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLH));
    break;
  case RISCV::CLHU:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLHU));
    break;
  case RISCV::CSH:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CSH));
    IsStore = true;
    break;
  case RISCV::CLW:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLW));
    break;
  case RISCV::CLWU:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLWU));
    break;
  case RISCV::CSW:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CSW));
    IsStore = true;
    break;
  case RISCV::CLD:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLD));
    break;
  case RISCV::CSD:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CSD));
    IsStore = true;
    break;
  case RISCV::CLC_64:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLC_64));
    break;
  case RISCV::CSC_64:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CSC_64));
    IsStore = true;
    break;
  case RISCV::CLC_128:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CLC_128));
    break;
  case RISCV::CSC_128:
    II.setDesc(TII->get(RISCV::PseudoCLLCInbounds_CSC_128));
    IsStore = true;
    break;
  }

  // Replace the (reg, offset) destination with the global.
  II.removeOperand(2);
  II.removeOperand(1);

  if (IsStore) {
    // Add the temp register dest
    II.insert(II.operands_begin(),
              MachineOperand::CreateReg(
                  MRI.createVirtualRegister(&RISCV::YGPRRegClass),
                  /*IsDef=*/true, /*IsImp=*/false,
                  /*isKill=*/false, /*isDead=*/true, /*isUndef=*/true,
                  /*isEarlyClobber=*/true));
  }
  II.addOperand(MachineOperand::CreateGA(Src.getGlobal(), Src.getOffset(),
                                         Src.getTargetFlags()));

  return true;
}

bool RISCVCheriCleanupOpt::runOnMachineFunction(MachineFunction &MF) {
  auto ABI = static_cast<const RISCVSubtarget &>(MF.getSubtarget()).getTargetABI();
  if (ABI != RISCVABI::ABI_CHERIOT && ABI != RISCVABI::ABI_CHERIOT_BAREMETAL)
    return false;

  auto &MRI = MF.getRegInfo();
  TII = static_cast<const RISCVInstrInfo *>(MF.getSubtarget().getInstrInfo());
  bool Modified = false;
  SmallVector<std::pair<MachineInstr *, size_t>> largeCLLCs;
  SmallVector<MachineInstr *> ToDelete;
  for (auto &MBB : MF)
    for (auto &MI : MBB)
      if (MI.getOpcode() == RISCV::PseudoCLLC) {
        // If this is not a load of a global then this is surprising.
        if (!MI.getOperand(1).isGlobal())
          continue;
        uint32_t SafeSize = 0;
        // If this is the definition of a global, then we know the size.  Allow
        // any loads in that size to be safe.
        bool HasCheriotImportAttr = false;
        if (auto GV = dyn_cast<GlobalVariable>(MI.getOperand(1).getGlobal())) {
          if (GV->hasInitializer())
            SafeSize = MF.getDataLayout().getTypeAllocSize(GV->getValueType());
          HasCheriotImportAttr =
              (GV->hasAttribute(
                   llvm::CHERIoTGlobalCapabilityImportAttr::getAttrName()) ||
               GV->hasAttribute(llvm::CHERIoTSealedValueAttr::getAttrName()) ||
               GV->hasAttribute(
                   llvm::CHERIoTSealingKeyTypeAttr::getAttrName()));
        }

        bool UnsafeUse = false;
        for (auto &UI : MRI.use_instructions(MI.getOperand(0).getReg())) {
          size_t OpSize = 0;
          switch (UI.getOpcode()) {
          default:
            UnsafeUse = true;
            continue;
          case RISCV::CLB:
          case RISCV::CLBU:
          case RISCV::CSB:
            OpSize = 1;
            break;
          case RISCV::CLH:
          case RISCV::CLHU:
          case RISCV::CSH:
            OpSize = 2;
            break;
          case RISCV::CLW:
          case RISCV::CLWU:
          case RISCV::CSW:
            OpSize = 4;
            break;
          case RISCV::CLD:
          case RISCV::CSD:
          case RISCV::CLC_64:
          case RISCV::CSC_64:
            OpSize = 8;
            break;
          case RISCV::CLC_128:
          case RISCV::CSC_128:
            OpSize = 16;
            break;
          }
          size_t Offset = UI.getOperand(2).getImm();
          if (Offset == 0)
            continue;
          if (Offset + OpSize <= SafeSize)
            continue;
          UnsafeUse = true;
          break;
        }
        if (!UnsafeUse) {
          MI.setDesc(TII->get(RISCV::PseudoCLLCInbounds));

          // If there is more than one use of the CLLC, don't attempt to fold
          // the low bits into the dereference. The presence of any Cheriot
          // import attribute indicates that the CLC will become some kind of
          // import table load, in which case there is no purpose to trying to
          // fold it into the dereference.
          if (!HasCheriotImportAttr &&
              MRI.hasOneUse(MI.getOperand(0).getReg())) {
            // If there is only a single inbounds use, then we can fold the low
            // bits of the address computation into the load/store itself.
            MachineOperand &UOp = *MRI.use_begin(MI.getOperand(0).getReg());
            if (rewriteMemoryReference(UOp, MI.getOperand(1), MRI, TII))
              ToDelete.push_back(&MI);
          }
          Modified = true;
        } else if (SafeSize >= 4096) {
          // If we know the size now and we know that it doesn't fit in a
          // relocation, fill it in with a longer instruction sequence.  We
          // don't always know the size.
          largeCLLCs.push_back({&MI, SafeSize});
          Modified = true;
        }
      }
  for (auto I : largeCLLCs) {
    auto &MI = *I.first;
    const uint32_t SafeSize = I.second;
    // If the size is known and can't be represented with a single
    // CSetBounds instruction, emit the sequence of instructions to generate
    // the size now and then pass the size along with a variant of the CLLC
    // pseudo for later expansion.  This is necessary because CLLC is expanded
    // *after* register allocation and so can't introduce a new virtual
    // register.
    Register SizeReg;
    const RISCVMatInt::InstSeq Seq = RISCVMatInt::generateInstSeq(
        SafeSize, MF.getSubtarget());
    for (auto &SeqMI : Seq) {
      const Register Out = MRI.createVirtualRegister(&RISCV::GPRRegClass);
      auto &MID = TII->get(SeqMI.getOpcode());
      auto MIB = BuildMI(*MI.getParent(), MI, MI.getDebugLoc(), MID, Out);
      if (MID.getNumOperands() == 3)
        MIB.addReg(SizeReg);
      else
        assert(MID.getNumOperands() == 2);
      MIB.addImm(SeqMI.getImm());
      SizeReg = Out;
    }
    const Register DstReg = MI.getOperand(0).getReg();
    const Register Unbounded = MRI.createVirtualRegister(&RISCV::YGPRRegClass);
    MI.getOperand(0).setReg(Unbounded);
    // Replace the pseudo with the version that doesn't need CSetBounds applied.
    MI.setDesc(TII->get(RISCV::PseudoCLLCInbounds));
    // Insert the CSetBounds ourself afterwards.
    BuildMI(*MI.getParent(), ++MachineBasicBlock::iterator{MI},
            MI.getDebugLoc(), TII->get(RISCV::CSetBounds), DstReg)
        .addReg(Unbounded)
        .addReg(SizeReg);
  }

  for (auto *II : ToDelete)
    II->eraseFromParent();

  return Modified;
}

} // end of anonymous namespace

INITIALIZE_PASS(RISCVCheriCleanupOpt, "riscv-cheriot-expand-cllc",
                RISCV_CHERI_CLEANUP_NAME, false, false)

namespace llvm {

FunctionPass *createRISCVCheriCleanupOptPass() {
  return new RISCVCheriCleanupOpt();
}

} // end of namespace llvm
