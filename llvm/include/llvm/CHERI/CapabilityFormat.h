//===--- CHERICompressedCapability.h ----------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_COMPRESSED_CAPABILITY_H
#define LLVM_COMPRESSED_CAPABILITY_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/MC/MCTargetOptions.h"
#include "llvm/Support/Alignment.h"

#include <algorithm>
#include <cstdint>

namespace llvm {

class CHERICapabilityFormat {
  constexpr CHERICapabilityFormat(uint64_t AM,
                                  ArrayRef<std::pair<uint64_t, uint64_t>> L)
      : AddressMask(AM), LUT(L) {}

  uint64_t AddressMask;
  ArrayRef<std::pair<uint64_t, uint64_t>> LUT;

public:
  inline uint64_t getAddressMask() const { return AddressMask; }
  uint64_t getAlignmentMask(uint64_t Length) const {
    auto el = std::find_if(LUT.begin(), LUT.end(),
                           [=](const auto &p) { return Length <= p.first; });
    assert(el != LUT.end());
    return el->second;
  }

  inline uint64_t getRepresentableLength(uint64_t Length) const {
    uint64_t Mask = getAlignmentMask(Length);
    return (Length + ~Mask) & Mask;
  }

  inline Align getRequiredAlignment(uint64_t Length) const {
    return Align((~getAlignmentMask(Length) + 1) & getAddressMask());
  }

  inline TailPaddingAmount getRequiredTailPadding(uint64_t Length) const {
    return static_cast<TailPaddingAmount>(
        llvm::alignTo(Length, getRequiredAlignment(Length)) - Length);
  }

  static const CHERICapabilityFormat Cheriot64;
  static const CHERICapabilityFormat Cheri64;
  static const CHERICapabilityFormat Cheri128;
};

} // namespace llvm

#endif
