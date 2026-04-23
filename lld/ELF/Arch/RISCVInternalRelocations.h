//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_ARCH_RISCVINTERNALRELOCATIONS_H
#define LLD_ELF_ARCH_RISCVINTERNALRELOCATIONS_H

// CHERIoT Nonstandard Relocations
static constexpr uint32_t INTERNAL_RISCV_VENDOR_CHERIOT1 = 3 << 9;
static constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI =
    INTERNAL_RISCV_VENDOR_CHERIOT1 | llvm::ELF::R_RISCV_CHERIOT1_COMPARTMENT_CODE_HI;
static constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI =
    INTERNAL_RISCV_VENDOR_CHERIOT1 |
    llvm::ELF::R_RISCV_CHERIOT1_COMPARTMENT_DATA_HI;

static constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI =
    INTERNAL_RISCV_VENDOR_CHERIOT1 |  257;


#endif
