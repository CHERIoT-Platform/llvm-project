//===- RISCVCompressedCap.cpp - CHERI compression helpers ------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "RISCVCompressedCap.h"
#include "MCTargetDesc/RISCVMCTargetDesc.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/Support/CHERICapabilityFormat.h"
#include "llvm/Support/ErrorHandling.h"

namespace llvm {

namespace RISCVCompressedCap {

uint64_t getRepresentableLength(uint64_t Length, const MCSubtargetInfo &STI) {
  if (STI.hasFeature(RISCV::FeatureVendorXCheriot))
    return CHERIoTCapabilityFormat::getRepresentableLength(Length);

  bool IsRV64 = STI.hasFeature(RISCV::Feature64Bit);
  return IsRV64 ? RV64YCapabilityFormat::getRepresentableLength(Length)
                : RV32YCapabilityFormat::getRepresentableLength(Length);
}

TailPaddingAmount getRequiredTailPadding(uint64_t Size,
                                         const MCSubtargetInfo &STI) {
  if (STI.hasFeature(RISCV::FeatureVendorXCheriot))
    return CHERIoTCapabilityFormat::getRequiredTailPadding(Size);

  bool IsRV64 = STI.hasFeature(RISCV::Feature64Bit);
  return IsRV64 ? RV64YCapabilityFormat::getRequiredTailPadding(Size)
                : RV32YCapabilityFormat::getRequiredTailPadding(Size);
}

Align getRequiredAlignment(uint64_t Size, const MCSubtargetInfo &STI) {
  if (STI.hasFeature(RISCV::FeatureVendorXCheriot))
    return CHERIoTCapabilityFormat::getRequiredAlignment(Size);

  bool IsRV64 = STI.hasFeature(RISCV::Feature64Bit);
  return IsRV64 ? RV64YCapabilityFormat::getRequiredAlignment(Size)
                : RV32YCapabilityFormat::getRequiredAlignment(Size);
}

uint64_t getAlignmentMask(uint64_t Size, const MCSubtargetInfo &STI) {
  if (STI.hasFeature(RISCV::FeatureVendorXCheriot))
    return CHERIoTCapabilityFormat::getAlignmentMask(Size);

  bool IsRV64 = STI.hasFeature(RISCV::Feature64Bit);
  return IsRV64 ? RV64YCapabilityFormat::getAlignmentMask(Size)
                : RV32YCapabilityFormat::getAlignmentMask(Size);
}

} // namespace RISCVCompressedCap
} // namespace llvm
