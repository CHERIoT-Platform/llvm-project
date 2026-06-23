//===----- RISCVCodeGenPrepare.cpp ----------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCV.h"
#include "RISCVSubtarget.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/CodeGen/MachineFunctionPass.h"

using namespace llvm;

#define DEBUG_TYPE "riscv-cheriot-ret-zeroing"
#define RISCV_CHERIOT_RET_ZEROING_NAME                                         \
  "RISC-V Eliminate CHERIoT Return Register Zeroing"

namespace {

class RISCVCheriotRetZeroing : public MachineFunctionPass {
public:
  static char ID;

  RISCVCheriotRetZeroing() : MachineFunctionPass(ID) {}

  bool runOnMachineFunction(MachineFunction &MF) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  StringRef getPassName() const override {
    return RISCV_CHERIOT_RET_ZEROING_NAME;
  }
};

} // end anonymous namespace

char RISCVCheriotRetZeroing::ID = 0;
INITIALIZE_PASS(RISCVCheriotRetZeroing, DEBUG_TYPE,
                RISCV_CHERIOT_RET_ZEROING_NAME, false, false)

FunctionPass *llvm::createRISCVCheriotRetZeroingPass() {
  return new RISCVCheriotRetZeroing();
}

bool isRegZeroingOp(Register RegNo, const MachineInstr &MI) {
  if (MI.getOpcode() != RISCV::ADDI)
    return false;

  const auto &Dst = MI.getOperand(0);
  if (Dst.getReg() != RegNo)
    return false;

  const auto &Src1 = MI.getOperand(1);
  if (Src1.getReg() != RISCV::X0)
    return false;

  const auto &Src2 = MI.getOperand(2);
  if (Src2.getImm() != 0)
    return false;

  return true;
}

struct ReturnBlockInfo {
  MachineBasicBlock *MBB = nullptr;
  MachineInstr *ReturnMI = nullptr;
  MachineInstr *X10Zeroing = nullptr;
  MachineInstr *X11Zeroing = nullptr;
};

bool RISCVCheriotRetZeroing::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(MF.getFunction()))
    return false;

  if (MF.getSubtarget<RISCVSubtarget>().getTargetABI() != RISCVABI::ABI_CHERIOT)
    return false;

  if (MF.getFunction().getCallingConv() !=
      CallingConv::CHERIoT_CompartmentCallee)
    return false;

  const TargetRegisterInfo *TRI =
      MF.getSubtarget<RISCVSubtarget>().getRegisterInfo();

  // Gather candidates by finding return blocks containing final zeroings of X10
  // and/or X11, and pre-filter any that contain preceding writes to the
  // corresponding
  // register.
  SmallVector<ReturnBlockInfo, 1> Candidates;
  for (auto &MBB : MF) {
    if (!MBB.isReturnBlock())
      continue;

    ReturnBlockInfo Info;
    Info.MBB = &MBB;

    bool ReadX10 = false;
    bool ReadX11 = false;
    for (auto &MI : llvm::reverse(MBB)) {
      if (MI.isReturn()) {
        Info.ReturnMI = &MI;
        continue;
      }
      if (!ReadX10 && isRegZeroingOp(RISCV::X10, MI)) {
        Info.X10Zeroing = &MI;
        continue;
      }
      if (!ReadX11 && isRegZeroingOp(RISCV::X11, MI)) {
        Info.X11Zeroing = &MI;
        continue;
      }

      if (MI.readsRegister(RISCV::X10, TRI))
        ReadX10 = true;
      if (MI.readsRegister(RISCV::X11, TRI))
        ReadX11 = true;
      if (MI.modifiesRegister(RISCV::X10, TRI))
        Info.X10Zeroing = nullptr;
      if (MI.modifiesRegister(RISCV::X11, TRI))
        Info.X11Zeroing = nullptr;
    }

    if (!Info.X10Zeroing && !Info.X11Zeroing)
      continue;

    Candidates.push_back(Info);
  }

  // Bail out early if we didn't find any candidates.
  if (Candidates.empty())
    return false;

  // Check whether any non-return blocks contain writes to X10/X11.
  DenseMap<const MachineBasicBlock *, std::pair<bool, bool>> Writes;
  for (auto &MBB : MF) {
    if (MBB.isReturnBlock())
      continue;
    auto &[WritesX10, WritesX11] = Writes[&MBB];
    for (const auto &MI : MBB) {
      if (MI.modifiesRegister(RISCV::X10, TRI))
        WritesX10 = true;
      if (MI.modifiesRegister(RISCV::X11, TRI))
        WritesX11 = true;
    }
  }

  // Propagate WritesX10/WritesX11 forward to a fixed point. At the end of this,
  // return blocks will have WritesX10/WritesX11 set if any transitive
  // predecessor contained the corresponding write.
  ReversePostOrderTraversal<MachineFunction *> RPOT(&MF);
  bool Changed = true;
  while (Changed) {
    Changed = false;
    for (auto *MBB : RPOT) {
      auto &[WritesX10, WritesX11] = Writes[MBB];
      for (auto *Pred : MBB->predecessors()) {
        auto &[PWritesX10, PWritesX11] = Writes[Pred];
        if (!WritesX10 && PWritesX10) {
          WritesX10 = true;
          Changed = true;
        }
        if (!WritesX11 && PWritesX11) {
          WritesX11 = true;
          Changed = true;
        }
      }
    }
  }

  // Remove any candidates that did not end up getting marked in the predecessor
  // propagation.
  bool MadeChange = false;
  for (auto &Info : Candidates) {
    auto [PredsWritesX10, PredsWritesX11] = Writes[Info.MBB];
    if (!PredsWritesX10 && Info.X10Zeroing) {
      Info.X10Zeroing->eraseFromParent();
      Info.ReturnMI->findRegisterUseOperand(RISCV::X10, TRI)->setIsUndef();
      MadeChange = true;
    }

    if (!PredsWritesX11 && Info.X11Zeroing) {
      Info.X11Zeroing->eraseFromParent();
      Info.ReturnMI->findRegisterUseOperand(RISCV::X11, TRI)->setIsUndef();
      MadeChange = true;
    }
  }

  return MadeChange;
}
