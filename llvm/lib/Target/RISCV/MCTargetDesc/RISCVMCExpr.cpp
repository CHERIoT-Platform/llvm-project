//===-- RISCVMCExpr.cpp - RISC-V specific MC expression classes -----------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of the assembly expression modifiers
// accepted by the RISC-V architecture (e.g. ":lo12:", ":gottprel_g1:", ...).
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/RISCVAsmBackend.h"
#include "MCTargetDesc/RISCVMCAsmInfo.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCObjectWriter.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "riscvmcexpr"

RISCV::Specifier RISCV::parseSpecifierName(StringRef name, bool IsPurecap) {
  bool IsTGOT = IsPurecap && MCTargetOptions::cheriTLSUseTGOT();
  return StringSwitch<RISCV::Specifier>(name)
      .Case("lo", RISCV::S_LO)
      .Case("hi", ELF::R_RISCV_HI20)
      .Case("pcrel_lo", RISCV::S_PCREL_LO)
      .Case("pcrel_hi", RISCV::S_PCREL_HI)
      .Case("got_pcrel_hi", RISCV::S_GOT_HI)
      .Case("tprel_lo", RISCV::S_TPREL_LO)
      .Case("tprel_hi", ELF::R_RISCV_TPREL_HI20)
      .Case("tprel_add", ELF::R_RISCV_TPREL_ADD)
      .Case("tlsdesc_hi", ELF::R_RISCV_TLSDESC_HI20)
      .Case("tlsdesc_load_lo", ELF::R_RISCV_TLSDESC_LOAD_LO12)
      .Case("tlsdesc_add_lo", ELF::R_RISCV_TLSDESC_ADD_LO12)
      .Case("tlsdesc_call", ELF::R_RISCV_TLSDESC_CALL)
      .Case("qc.abs20", RISCV::S_QC_ABS20)
      .Case("qc.access", RISCV::S_QC_ACCESS)
      // Used in data directives
      .Case("pltpcrel", ELF::R_RISCV_PLT32)
      .Case("gotpcrel", ELF::R_RISCV_GOT32_PCREL)
      .Case("cheriot_compartment_hi", RISCV::S_CHERIOT_COMPARTMENT_CODE_HI)
      .Case("cheriot_compartment_code_hi", RISCV::S_CHERIOT_COMPARTMENT_CODE_HI)
      .Case("cheriot_compartment_data_hi", RISCV::S_CHERIOT_COMPARTMENT_DATA_HI)
      .Case("cheriot_compartment_lo_i", RISCV::S_CHERIOT_COMPARTMENT_LO_I)
      .Case("cheriot_compartment_lo_s", RISCV::S_CHERIOT_COMPARTMENT_LO_S)
      .Case("cheriot_compartment_size", RISCV::S_CHERIOT_COMPARTMENT_SIZE)
      .Case("code", ELF::R_RISCV_CHERI_CAPABILITY_CODE)
      .Case("tls_ie_pcrel_hi", IsTGOT ? ELF::R_RISCV_CHERI_TLS_TGOT_GOT_HI20
                                      : ELF::R_RISCV_TLS_GOT_HI20)
      .Case("tls_gd_pcrel_hi", IsTGOT ? ELF::R_RISCV_CHERI_TLS_TGOT_GD_HI20
                                      : ELF::R_RISCV_TLS_GD_HI20)
      .Case("tgot_tprel_lo", ELF::R_RISCV_CHERI_TLS_TGOT_LO12_I)
      .Case("tgot_tprel_hi", ELF::R_RISCV_CHERI_TLS_TGOT_HI20)
      .Case("tgot_tprel_add", ELF::R_RISCV_CHERI_TLS_TGOT_ADD)
      .Default(0);
}

StringRef RISCV::getSpecifierName(Specifier S) {
  switch (S) {
  case RISCV::S_None:
    llvm_unreachable("not used as %specifier()");
  case RISCV::S_LO:
    return "lo";
  case ELF::R_RISCV_HI20:
    return "hi";
  case RISCV::S_PCREL_LO:
    return "pcrel_lo";
  case RISCV::S_PCREL_HI:
    return "pcrel_hi";
  case RISCV::S_GOT_HI:
    return "got_pcrel_hi";
  case RISCV::S_TPREL_LO:
    return "tprel_lo";
  case ELF::R_RISCV_TPREL_HI20:
    return "tprel_hi";
  case ELF::R_RISCV_TPREL_ADD:
    return "tprel_add";
  case ELF::R_RISCV_TLS_GOT_HI20:
  case ELF::R_RISCV_CHERI_TLS_TGOT_GOT_HI20:
    return "tls_ie_pcrel_hi";
  case ELF::R_RISCV_TLSDESC_HI20:
    return "tlsdesc_hi";
  case ELF::R_RISCV_TLSDESC_LOAD_LO12:
    return "tlsdesc_load_lo";
  case ELF::R_RISCV_TLSDESC_ADD_LO12:
    return "tlsdesc_add_lo";
  case ELF::R_RISCV_TLSDESC_CALL:
    return "tlsdesc_call";
  case ELF::R_RISCV_TLS_GD_HI20:
  case ELF::R_RISCV_CHERI_TLS_TGOT_GD_HI20:
    return "tls_gd_pcrel_hi";
  case ELF::R_RISCV_CHERI_TLS_TGOT_LO12_I:
    return "tgot_tprel_lo";
  case ELF::R_RISCV_CHERI_TLS_TGOT_HI20:
    return "tgot_tprel_hi";
  case ELF::R_RISCV_CHERI_TLS_TGOT_ADD:
    return "tgot_tprel_add";
  case RISCV::S_CALL_PLT:
    return "call_plt";
  case RISCV::S_CHERIOT_COMPARTMENT_CODE_HI:
    return "cheriot_compartment_code_hi";
  case RISCV::S_CHERIOT_COMPARTMENT_DATA_HI:
    return "cheriot_compartment_data_hi";
  case RISCV::S_CHERIOT_COMPARTMENT_LO_I:
    return "cheriot_compartment_lo_i";
  case RISCV::S_CHERIOT_COMPARTMENT_LO_S:
    return "cheriot_compartment_lo_s";
  case RISCV::S_CHERIOT_COMPARTMENT_SIZE:
    return "cheriot_compartment_size";
  case ELF::R_RISCV_32_PCREL:
    return "32_pcrel";
  case ELF::R_RISCV_GOT32_PCREL:
    return "gotpcrel";
  case ELF::R_RISCV_PLT32:
    return "pltpcrel";
  case RISCV::S_QC_ABS20:
    return "qc.abs20";
  case RISCV::S_QC_ACCESS:
    return "qc.access";
  case ELF::R_RISCV_CHERI_CAPABILITY_CODE:
    return "code";
  }
  llvm_unreachable("Invalid ELF symbol kind");
}
