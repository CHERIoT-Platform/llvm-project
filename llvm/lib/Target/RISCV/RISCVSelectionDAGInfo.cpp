//===-- RISCVSelectionDAGInfo.cpp - RISCV SelectionDAG Info ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "RISCVSelectionDAGInfo.h"
#include "RISCVSubtarget.h"
#include "MCTargetDesc/RISCVBaseInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"

#define GET_SDNODE_DESC
#include "RISCVGenSDNodeInfo.inc"

using namespace llvm;

namespace {

/// Helper function to get the RISCV subclass of the subtarget
const RISCVSubtarget &getRISCVSubtarget(SelectionDAG &DAG) {
  return reinterpret_cast<const RISCVSubtarget&>(DAG.getSubtarget());
}

/// Helper function that generates a DAG node for calling a memory function.
SDValue callFunction(SelectionDAG &DAG, SDLoc dl, SDValue Chain, const char
    *fnName, SDValue Dst, SDValue Src, SDValue Size) {
  auto &Ctx = *DAG.getContext();
  auto &STI = getRISCVSubtarget(DAG);
  TargetLowering::ArgListTy Args;
  auto pushArg = [&](SDValue &Op) {
    Args.emplace_back(Op, Op.getValueType().getTypeForEVT(Ctx));
  };
  pushArg(Dst);
  pushArg(Src);
  pushArg(Size);

  SDValue memOpFn = DAG.getExternalFunctionSymbol(fnName);

  TargetLowering::CallLoweringInfo CLI(DAG);
  CLI.setDebugLoc(dl)
      .setChain(Chain)
      .setLibCallee(STI.getTargetABI() == RISCVABI::ABI_CHERIOT
                        ? CallingConv::CHERI_LibCall
                        : CallingConv::C,
                    Dst.getValueType().getTypeForEVT(Ctx), memOpFn,
                    std::move(Args))
      .setDiscardResult();

  const RISCVTargetLowering *TLI = STI.getTargetLowering();
  std::pair<SDValue,SDValue> CallResult = TLI->LowerCallTo(CLI);
  return CallResult.second;
}

/// Helper that emits the memcpy / memmove call, as required.
SDValue EmitTargetCodeForMemOp(SelectionDAG &DAG, const SDLoc &dl,
                               SDValue Chain, SDValue Dst, SDValue Src,
                               SDValue Size, Align Alignment, bool isVolatile,
                               bool AlwaysInline,
                               PreserveCheriTags PreserveTags,
                               MachinePointerInfo DstPtrInfo,
                               MachinePointerInfo SrcPtrInfo, bool isMemCpy) {
  // If AlwaysInline is set, let SelectionDAG expand this.
  if (AlwaysInline) {
    return SDValue();
  }
  // If we're copying AS0 to AS0, do the normal thing.
  unsigned DstAS = DstPtrInfo.getAddrSpace();
  unsigned SrcAS = SrcPtrInfo.getAddrSpace();
  if ((DstAS == 0) && (SrcAS == 0))
    return SDValue();
  auto &STI = getRISCVSubtarget(DAG);
  // If either argument is AS0, make it a capability.
  MVT CapType = STI.typeForCapabilities();
  if (DstAS == 0)
    Dst = DAG.getAddrSpaceCast(dl, CapType, Dst, 0, 200);
  if (SrcAS == 0)
    Src = DAG.getAddrSpaceCast(dl, CapType, Src, 0, 200);

  const char *memFnName = isMemCpy ?
    (RISCVABI::isCheriPureCapABI(STI.getTargetABI()) ?  "memcpy" : "memcpy_c") :
    (RISCVABI::isCheriPureCapABI(STI.getTargetABI()) ?  "memmove" : "memmove_c");
  return callFunction(DAG, dl, Chain, memFnName, Dst, Src, Size);
}
}

SDValue RISCVSelectionDAGInfo::EmitTargetCodeForMemcpy(
    SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool isVolatile, bool AlwaysInline,
    PreserveCheriTags PreserveTags, MachinePointerInfo DstPtrInfo,
    MachinePointerInfo SrcPtrInfo) const {
  return EmitTargetCodeForMemOp(DAG, dl, Chain, Dst, Src, Size, Alignment,
                                isVolatile, AlwaysInline, PreserveTags,
                                DstPtrInfo, SrcPtrInfo, true);
}

SDValue RISCVSelectionDAGInfo::EmitTargetCodeForMemmove(
    SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool isVolatile,
    PreserveCheriTags PreserveTags, MachinePointerInfo DstPtrInfo,
    MachinePointerInfo SrcPtrInfo) const {
  return EmitTargetCodeForMemOp(DAG, dl, Chain, Dst, Src, Size, Alignment,
                                isVolatile, false, PreserveTags, DstPtrInfo,
                                SrcPtrInfo, false);
}

RISCVSelectionDAGInfo::RISCVSelectionDAGInfo()
    : SelectionDAGGenTargetInfo(RISCVGenSDNodeInfo) {}

RISCVSelectionDAGInfo::~RISCVSelectionDAGInfo() = default;

void RISCVSelectionDAGInfo::verifyTargetNode(const SelectionDAG &DAG,
                                             const SDNode *N) const {
#ifndef NDEBUG
  switch (N->getOpcode()) {
  default:
    return SelectionDAGGenTargetInfo::verifyTargetNode(DAG, N);
  case RISCVISD::TUPLE_EXTRACT:
    assert(N->getNumOperands() == 2 && "Expected three operands!");
    assert(N->getOperand(1).getOpcode() == ISD::TargetConstant &&
           N->getOperand(1).getValueType() == MVT::i32 &&
           "Expected index to be an i32 target constant!");
    break;
  case RISCVISD::TUPLE_INSERT:
    assert(N->getNumOperands() == 3 && "Expected three operands!");
    assert(N->getOperand(2).getOpcode() == ISD::TargetConstant &&
           N->getOperand(2).getValueType() == MVT::i32 &&
           "Expected index to be an i32 target constant!");
    break;
  case RISCVISD::VQDOT_VL:
  case RISCVISD::VQDOTU_VL:
  case RISCVISD::VQDOTSU_VL: {
    assert(N->getNumValues() == 1 && "Expected one result!");
    assert(N->getNumOperands() == 5 && "Expected five operands!");
    EVT VT = N->getValueType(0);
    assert(VT.isScalableVector() && VT.getVectorElementType() == MVT::i32 &&
           "Expected result to be an i32 scalable vector");
    assert(N->getOperand(0).getValueType() == VT &&
           N->getOperand(1).getValueType() == VT &&
           N->getOperand(2).getValueType() == VT &&
           "Expected result and first 3 operands to have the same type!");
    EVT MaskVT = N->getOperand(3).getValueType();
    assert(MaskVT.isScalableVector() &&
           MaskVT.getVectorElementType() == MVT::i1 &&
           MaskVT.getVectorElementCount() == VT.getVectorElementCount() &&
           "Expected mask VT to be an i1 scalable vector with same number of "
           "elements as the result");
    assert((N->getOperand(4).getValueType() == MVT::i32 ||
            N->getOperand(4).getValueType() == MVT::i64) &&
           "Expect VL operand to be i32 or i64");
    break;
  }
  }
#endif
}

SDValue RISCVSelectionDAGInfo::EmitTargetCodeForMemsetCHERI(
    SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool isVolatile, bool AlwaysInline,
    MachinePointerInfo DstPtrInfo) const {
  // If we're setting via an AS0 pointer, do the normal thing.
  unsigned DstAS = DstPtrInfo.getAddrSpace();
  if (DstAS == 0)
    return SDValue();

  auto &STI = getRISCVSubtarget(DAG);
  MVT CapType = STI.typeForCapabilities();
  unsigned CLen = CapType.getSizeInBits();
  unsigned CLenInBytes = CLen / 8;
  // If this is capability aligned, but not a multiple of capability size, we
  // might have given up too early trying to emit capability instructions.
  if (Alignment >= CLenInBytes) {
    if (auto ConstantSize = dyn_cast<ConstantSDNode>(Size)) {
      uint64_t SizeVal = ConstantSize->getZExtValue();
      // If this size is a small constant, and the value we're writing is zero,
      // then let's emit some stores instead.
      if (SizeVal < (CLenInBytes * 8))
        if (isa<ConstantSDNode>(Src) &&
            cast<ConstantSDNode>(Src)->isZero()) {
          SmallVector<SDValue, 8> OutChains;
          SDValue ZeroCap = DAG.getNullCapability(dl);
          for (uint64_t i = 0; i < (SizeVal / CLenInBytes); i++) {
            uint64_t DstOff = i * CLenInBytes;
            SDValue Store = DAG.getStore(
                Chain, dl, ZeroCap,
                DAG.getMemBasePlusOffset(Dst, TypeSize::getFixed(DstOff), dl),
                DstPtrInfo.getWithOffset(DstOff), Alignment,
                isVolatile ? MachineMemOperand::MOVolatile
                           : MachineMemOperand::MONone);
            OutChains.push_back(Store);
          }
          MVT XLenVT = STI.getXLenVT();
          unsigned XLenInBytes = STI.getXLen() / 8;
          unsigned Remainder = SizeVal % CLenInBytes;
          SDValue ZeroXLen = DAG.getConstant(0, dl, XLenVT);
          uint64_t Done = (SizeVal / CLenInBytes) * CLenInBytes;
          // Write zero or one XLen words.
          while (Remainder >= XLenInBytes) {
            SDValue Store = DAG.getStore(
                Chain, dl, ZeroXLen,
                DAG.getMemBasePlusOffset(Dst, TypeSize::getFixed(Done), dl),
                DstPtrInfo.getWithOffset(Done), Alignment,
                isVolatile ? MachineMemOperand::MOVolatile
                           : MachineMemOperand::MONone);
            OutChains.push_back(Store);
            Done += XLenInBytes;
            Remainder -= XLenInBytes;
          }
          // We can always do the remaining 1 to (XLen-1) bytes in at most two
          // instructions, either two adjacent stores or an unaligned
          // overlapping store.
          // We prefer the two-store version, because it reduces dependencies
          // between instructions.
          while (Remainder > 0) {
            SDValue Zero;
            uint64_t DstOff;
            switch (Remainder) {
            default:
              llvm_unreachable("Remainder must be < 8");
            case 7:
            case 6:
            case 5:
            case 4:
              assert(XLenInBytes >= 8);
              Zero = DAG.getConstant(0, dl, MVT::i32);
              ;
              DstOff = SizeVal - Remainder;
              Remainder -= 4;
              break;
            case 3:
            case 2:
              Zero = DAG.getConstant(0, dl, MVT::i16);
              ;
              DstOff = SizeVal - Remainder;
              Remainder -= 2;
              break;
            case 1:
              Zero = DAG.getConstant(0, dl, MVT::i8);
              ;
              DstOff = SizeVal - Remainder;
              Remainder -= 1;
              break;
            }
            SDValue Store = DAG.getStore(
                Chain, dl, Zero,
                DAG.getMemBasePlusOffset(Dst, TypeSize::getFixed(DstOff), dl),
                DstPtrInfo.getWithOffset(DstOff), Alignment,
                isVolatile ? MachineMemOperand::MOVolatile
                           : MachineMemOperand::MONone);
            OutChains.push_back(Store);
          }
          return DAG.getNode(ISD::TokenFactor, dl, MVT::Other, OutChains);
        }
    }
  }

  const char *memFnName =
      RISCVABI::isCheriPureCapABI(STI.getTargetABI()) ? "memset" : "memset_c";
  return callFunction(DAG, dl, Chain, memFnName, Dst, Src, Size);
}

SDValue RISCVSelectionDAGInfo::EmitTargetCodeForMemset(
    SelectionDAG &DAG, const SDLoc &dl, SDValue Chain, SDValue Dst, SDValue Src,
    SDValue Size, Align Alignment, bool isVolatile, bool AlwaysInline,
    MachinePointerInfo DstPtrInfo) const {
  const auto &Subtarget = DAG.getSubtarget<RISCVSubtarget>();

  if (Subtarget.hasVendorXCheri() || Subtarget.hasVendorXCheriot())
    return EmitTargetCodeForMemsetCHERI(DAG, dl, Chain, Dst, Src, Size,
                                        Alignment, isVolatile, AlwaysInline,
                                        DstPtrInfo);

  // We currently do this only for Xqcilsm
  if (!Subtarget.hasVendorXqcilsm())
    return SDValue();

  // Do this only if we know the size at compile time.
  ConstantSDNode *ConstantSize = dyn_cast<ConstantSDNode>(Size);
  if (!ConstantSize)
    return SDValue();

  uint64_t NumberOfBytesToWrite = ConstantSize->getZExtValue();

  // Do this only if it is word aligned and we write a multiple of 4 bytes.
  if (!(Alignment >= 4) || !((NumberOfBytesToWrite & 3) == 0))
    return SDValue();

  SmallVector<SDValue, 8> OutChains;
  SDValue SrcValueReplicated = DAG.getNode(ISD::ZERO_EXTEND, dl, MVT::i32, Src);
  int NumberOfWords = NumberOfBytesToWrite / 4;
  MachineFunction &MF = DAG.getMachineFunction();
  auto Volatile =
      isVolatile ? MachineMemOperand::MOVolatile : MachineMemOperand::MONone;

  // Helper for constructing the QC_SETWMI instruction
  auto getSetwmiNode = [&](uint8_t SizeWords, uint8_t OffsetSetwmi) -> SDValue {
    SDValue Ops[] = {Chain, SrcValueReplicated, Dst,
                     DAG.getTargetConstant(SizeWords, dl, MVT::i32),
                     DAG.getTargetConstant(OffsetSetwmi, dl, MVT::i32)};
    MachineMemOperand *BaseMemOperand = MF.getMachineMemOperand(
        DstPtrInfo.getWithOffset(OffsetSetwmi),
        MachineMemOperand::MOStore | Volatile, SizeWords * 4, Align(4));
    return DAG.getMemIntrinsicNode(RISCVISD::QC_SETWMI, dl,
                                   DAG.getVTList(MVT::Other), Ops, MVT::i32,
                                   BaseMemOperand);
  };

  // If i8 type and constant non-zero value.
  if ((Src.getValueType() == MVT::i8) && !isNullConstant(Src))
    // Replicate byte to word by multiplication with 0x01010101.
    SrcValueReplicated =
        DAG.getNode(ISD::MUL, dl, MVT::i32, SrcValueReplicated,
                    DAG.getConstant(0x01010101ul, dl, MVT::i32));

  // We limit a QC_SETWMI to 16 words or less to improve interruptibility.
  // So for 1-16 words we use a single QC_SETWMI:
  //
  // QC_SETWMI reg1, N, 0(reg2)
  //
  // For 17-32 words we use two QC_SETWMI's with the first as 16 words and the
  // second for the remainder:
  //
  // QC_SETWMI reg1, 16, 0(reg2)
  // QC_SETWMI reg1, N, 64(reg2)
  //
  // For 33-48 words, we would like to use (16, 16, n), but that means the last
  // QC_SETWMI needs an offset of 128 which the instruction doesn't support.
  // So in this case we use a length of 15 for the second instruction and we do
  // the rest with the third instruction.
  // This means the maximum inlined number of words is 47 (for now):
  //
  // QC_SETWMI R2, R0, 16, 0
  // QC_SETWMI R2, R0, 15, 64
  // QC_SETWMI R2, R0, N, 124
  //
  // For 48 words or more, call the target independent memset
  if (NumberOfWords >= 48)
    return SDValue();

  if (NumberOfWords <= 16) {
    // 1 - 16 words
    return getSetwmiNode(NumberOfWords, 0);
  }

  if (NumberOfWords <= 32) {
    // 17 - 32 words
    OutChains.push_back(getSetwmiNode(NumberOfWords - 16, 64));
    OutChains.push_back(getSetwmiNode(16, 0));
  } else {
    // 33 - 47 words
    OutChains.push_back(getSetwmiNode(NumberOfWords - 31, 124));
    OutChains.push_back(getSetwmiNode(15, 64));
    OutChains.push_back(getSetwmiNode(16, 0));
  }

  return DAG.getNode(ISD::TokenFactor, dl, MVT::Other, OutChains);
}
