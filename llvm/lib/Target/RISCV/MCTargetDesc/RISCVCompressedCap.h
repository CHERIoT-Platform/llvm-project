//===- RISCVCompressedCap.h - CHERI compression helpers --------*- C++ -*--===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_COMPRESSEDCAP_H
#define LLVM_LIB_TARGET_RISCV_COMPRESSEDCAP_H

#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/Alignment.h"
#include <cstdint>

namespace llvm {

class MCSubtargetInfo;

namespace RISCVCompressedCap {
uint64_t getRepresentableLength(uint64_t Length, const MCSubtargetInfo &);

uint64_t getAlignmentMask(uint64_t Length, const MCSubtargetInfo &);

TailPaddingAmount getRequiredTailPadding(uint64_t Size,
                                         const MCSubtargetInfo &);

Align getRequiredAlignment(uint64_t Size, const MCSubtargetInfo &);
} // namespace RISCVCompressedCap
} // namespace llvm
#endif
