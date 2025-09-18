//===- RISCVCompressedCap.cpp - CHERI compression helpers ------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCVCompressedCap.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "llvm/CHERI/CapabilityFormat.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

namespace RISCVCompressedCap {

static inline CHERICapabilityFormat
GetCapabilityFormat(const MCSubtargetInfo &STI) {
  if (STI.hasFeature(RISCV::FeatureVendorXCheriot))
    return CHERICapabilityFormat::Cheriot64;

  bool IsRV64 = STI.hasFeature(RISCV::Feature64Bit);
  return IsRV64 ? CHERICapabilityFormat::Cheri128
                : CHERICapabilityFormat::Cheri64;
}

uint64_t getRepresentableLength(uint64_t Length, const MCSubtargetInfo &STI) {

  return GetCapabilityFormat(STI).getRepresentableLength(Length);
}

uint64_t getAlignmentMask(uint64_t Length, const MCSubtargetInfo &STI) {
  return GetCapabilityFormat(STI).getAlignmentMask(Length);
}

TailPaddingAmount getRequiredTailPadding(uint64_t Size,
                                         const MCSubtargetInfo &STI) {
  return GetCapabilityFormat(STI).getRequiredTailPadding(Size);
}

Align getRequiredAlignment(uint64_t Size, const MCSubtargetInfo &STI) {
  return GetCapabilityFormat(STI).getRequiredAlignment(Size);
}
} // namespace RISCVCompressedCap
} // namespace llvm
