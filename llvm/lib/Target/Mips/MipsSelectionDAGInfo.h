//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_MIPS_MIPSSELECTIONDAGINFO_H
#define LLVM_LIB_TARGET_MIPS_MIPSSELECTIONDAGINFO_H

#include "llvm/CodeGen/SelectionDAGTargetInfo.h"

#define GET_SDNODE_ENUM
#include "MipsGenSDNodeInfo.inc"

namespace llvm {

class MipsTargetMachine;
namespace MipsISD {

enum NodeType : unsigned {
  // Floating point Abs
  FAbs = GENERATED_OPCODE_END,

  DynAlloc,

  // Double select nodes for machines without conditional-move.
  DOUBLE_SELECT_I,
  DOUBLE_SELECT_I64,
};

} // namespace MipsISD

class MipsSelectionDAGInfo : public SelectionDAGGenTargetInfo {
public:
  MipsSelectionDAGInfo();

  ~MipsSelectionDAGInfo() override;
  const char *getTargetNodeName(unsigned Opcode) const override;

  void verifyTargetNode(const SelectionDAG &DAG,
                        const SDNode *N) const override;
  SDValue EmitTargetCodeForMemcpy(SelectionDAG &DAG, const SDLoc &dl,
                                  SDValue Chain, SDValue Op1, SDValue Op2,
                                  SDValue Op3, Align DstAlign, Align SrcAlign,
                                  bool isVolatile,
                                  bool AlwaysInline,
                                  PreserveCheriTags PreserveTags,
                                  MachinePointerInfo DstPtrInfo,
                                  MachinePointerInfo SrcPtrInfo) const override;

  SDValue EmitTargetCodeForMemmove(
      SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Op1,
      SDValue Op2, SDValue Op3, Align DstAlign, Align SrcAlign, bool isVolatile,
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
