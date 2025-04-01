//===-- RISCVSelectionDAGInfo.h - RISCV SelectionDAG Info -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVSELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_RISCV_RISCVSELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

namespace llvm {

class RISCVSelectionDAGInfo : public SelectionDAGTargetInfo {
public:
~RISCVSelectionDAGInfo() override;

bool isTargetMemoryOpcode(unsigned Opcode) const override;

bool isTargetStrictFPOpcode(unsigned Opcode) const override;
  SDValue EmitTargetCodeForMemcpy(SelectionDAG &DAG, const SDLoc &dl,
                                  SDValue Chain, SDValue Op1, SDValue Op2,
                                  SDValue Op3, Align Alignment, bool isVolatile,
                                  bool AlwaysInline,
                                  PreserveCheriTags PreserveTags,
                                  MachinePointerInfo DstPtrInfo,
                                  MachinePointerInfo SrcPtrInfo) const override;

  SDValue EmitTargetCodeForMemmove(
      SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Op1,
      SDValue Op2, SDValue Op3, Align Alignment, bool isVolatile,
      PreserveCheriTags PreserveTags, MachinePointerInfo DstPtrInfo,
      MachinePointerInfo SrcPtrInfo) const override;

  SDValue EmitTargetCodeForMemset(SelectionDAG &DAG, const SDLoc &dl,
                                  SDValue Chain, SDValue Op1, SDValue Op2,
                                  SDValue Op3, Align Alignment, bool isVolatile,
                                  bool AlwaysInline,
                                  MachinePointerInfo DstPtrInfo) const override;
};

}

#endif
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_RISCVSELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_RISCV_RISCVSELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

namespace llvm {

class RISCVSelectionDAGInfo : public SelectionDAGTargetInfo {
public:
  ~RISCVSelectionDAGInfo() override;

  bool isTargetMemoryOpcode(unsigned Opcode) const override;

  bool isTargetStrictFPOpcode(unsigned Opcode) const override;
};

} // namespace llvm

#endif // LLVM_LIB_TARGET_RISCV_RISCVSELECTIONDAGINFO_H
