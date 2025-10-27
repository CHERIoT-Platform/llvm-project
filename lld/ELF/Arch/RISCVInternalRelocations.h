//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_ARCH_RISCVINTERNALRELOCATIONS_H
#define LLD_ELF_ARCH_RISCVINTERNALRELOCATIONS_H

namespace lld::elf {

// Bit 9 of RelType is used to indicate linker-internal relocations that are
// not vendor-specific.
// These are internal relocation numbers for GP/X0 relaxation. They aren't part
// of the psABI spec.
constexpr uint32_t INTERNAL_R_RISCV_GPREL_I = 256;
constexpr uint32_t INTERNAL_R_RISCV_GPREL_S = 257;
constexpr uint32_t INTERNAL_R_RISCV_X0REL_I = 258;
constexpr uint32_t INTERNAL_R_RISCV_X0REL_S = 259;

// Bits 10 -> 31 of RelType are used to indicate vendor-specific relocations.
constexpr uint32_t INTERNAL_RISCV_VENDOR_MASK = 0xFFFFFFFF << 9;
constexpr uint32_t INTERNAL_RISCV_VENDOR_QUALCOMM = 1 << 9;
constexpr uint32_t INTERNAL_RISCV_VENDOR_ANDES = 1 << 10;
constexpr uint32_t INTERNAL_RISCV_VENDOR_XCHERIOT1 = 1 << 11;

// CHERIoT Nonstandard Relocations
constexpr uint32_t INTERNAL_RISCV_XCHERIOT1_CHERIOT_COMPARTMENT_HI = INTERNAL_RISCV_VENDOR_XCHERIOT1 | llvm::ELF::R_RISCV_CHERIOT_COMPARTMENT_HI;
constexpr uint32_t INTERNAL_RISCV_XCHERIOT1_CHERIOT_COMPARTMENT_LO_I = INTERNAL_RISCV_VENDOR_XCHERIOT1 | llvm::ELF::R_RISCV_CHERIOT_COMPARTMENT_LO_I;
constexpr uint32_t INTERNAL_RISCV_XCHERIOT1_CHERIOT_COMPARTMENT_LO_S = INTERNAL_RISCV_VENDOR_XCHERIOT1 | llvm::ELF::R_RISCV_CHERIOT_COMPARTMENT_LO_S;
constexpr uint32_t INTERNAL_RISCV_XCHERIOT1_CHERIOT_COMPARTMENT_SIZE = INTERNAL_RISCV_VENDOR_XCHERIOT1 | llvm::ELF::R_RISCV_CHERIOT_COMPARTMENT_SIZE;

uint32_t getRISCVVendorRelType(StringRef rvVendor);
std::optional<StringRef> getRISCVVendorString(RelType ty);

class vendor_reloc_iterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = Relocation;
    using difference_type   = std::ptrdiff_t;
    using pointer           = Relocation*;
    using reference         = Relocation;  // returned by value

    vendor_reloc_iterator(MutableArrayRef<Relocation>::iterator i, MutableArrayRef<Relocation>::iterator e) : it(i), end(e) {}

    // Dereference
    Relocation operator*() const { 
      Relocation r = *it;
      r.type.v |= rvVendorFlag;
      return r;
    }

    struct vendor_reloc_proxy {
        Relocation r;
        const Relocation *operator->() const { return &r; }
    };

    vendor_reloc_proxy operator->() const { return vendor_reloc_proxy{this->operator*()}; }

    vendor_reloc_iterator& operator++() {
      ++it;
      if (it != end && it->type == llvm::ELF::R_RISCV_VENDOR) {
        rvVendorFlag = getRISCVVendorRelType(it->sym->getName());
        ++it;
      } else {
        rvVendorFlag = 0;
      }
      return *this;
    }

    vendor_reloc_iterator operator++(int) {
        vendor_reloc_iterator tmp(*this);
        ++(*this);
        return tmp;
    }

    bool operator==(const vendor_reloc_iterator& other) const { return it == other.it; }
    bool operator!=(const vendor_reloc_iterator& other) const { return it != other.it; }

    Relocation *getUnderlyingRelocation() const { return &*it; }

private:
  MutableArrayRef<Relocation>::iterator it;
  MutableArrayRef<Relocation>::iterator end;
  uint32_t rvVendorFlag = 0;
};

inline auto riscv_vendor_relocs(MutableArrayRef<Relocation> arr) {
  return llvm::make_range(vendor_reloc_iterator(arr.begin(), arr.end()),
                          vendor_reloc_iterator(arr.end(), arr.end()));
}

} // namespace lld::elf

#endif