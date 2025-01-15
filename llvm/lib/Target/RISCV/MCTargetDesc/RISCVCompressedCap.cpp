//===- RISCVCompressedCap.cpp - CHERI compression helpers ------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCVCompressedCap.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "llvm/CHERI/CompressedCapability.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

namespace RISCVCompressedCap {

static inline CompressedCapability::CapabilityFormat
GetCapabilitySize(const MCSubtargetInfo &STI) {
  if (STI.getCPU() == "cheriot")
    return CompressedCapability::Cheriot64;

  bool IsRV64 = STI.hasFeature(RISCV::Feature64Bit);
  return IsRV64 ? CompressedCapability::Cheri128
                : CompressedCapability::Cheri64;
}

uint64_t getRepresentableLength(uint64_t Length, const MCSubtargetInfo &STI) {

  return CompressedCapability::GetRepresentableLength(Length,
                                                      GetCapabilitySize(STI));
}

uint64_t getAlignmentMask(uint64_t Length, const MCSubtargetInfo &STI) {
  return CompressedCapability::GetAlignmentMask(Length, GetCapabilitySize(STI));
}

TailPaddingAmount getRequiredTailPadding(uint64_t Size,
                                         const MCSubtargetInfo &STI) {
  return CompressedCapability::GetRequiredTailPadding(Size,
                                                      GetCapabilitySize(STI));
}

Align getRequiredAlignment(uint64_t Size, const MCSubtargetInfo &STI) {
  return CompressedCapability::GetRequiredAlignment(Size,
                                                    GetCapabilitySize(STI));
}
} // namespace RISCVCompressedCap
} // namespace llvm
