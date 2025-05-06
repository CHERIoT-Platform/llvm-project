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

#include "RISCVMCExpr.h"
#include "MCTargetDesc/RISCVAsmBackend.h"
#include "RISCVFixupKinds.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSymbolELF.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "riscvmcexpr"

const RISCVMCExpr *RISCVMCExpr::create(const MCExpr *Expr, Specifier S,
                                       MCContext &Ctx) {
  return new (Ctx) RISCVMCExpr(Expr, S);
}

void RISCVMCExpr::printImpl(raw_ostream &OS, const MCAsmInfo *MAI) const {
  Specifier S = getSpecifier();
  bool HasVariant = ((S != VK_None) && (S != VK_CALL) && (S != VK_CALL_PLT) && (S != VK_CCALL));

  if (HasVariant)
    OS << '%' << getSpecifierName(S) << '(';
  Expr->print(OS, MAI);
  if (HasVariant)
    OS << ')';
}

const MCFixup *RISCVMCExpr::getPCRelHiFixup(const MCFragment **DFOut) const {
  MCValue AUIPCLoc;
  if (!getSubExpr()->evaluateAsRelocatable(AUIPCLoc, nullptr))
    return nullptr;

  const MCSymbolRefExpr *AUIPCSRE = AUIPCLoc.getSymA();
  if (!AUIPCSRE)
    return nullptr;

  const MCSymbol *AUIPCSymbol = &AUIPCSRE->getSymbol();
  const auto *DF = dyn_cast_or_null<MCDataFragment>(AUIPCSymbol->getFragment());

  if (!DF)
    return nullptr;

  uint64_t Offset = AUIPCSymbol->getOffset();
  if (DF->getContents().size() == Offset) {
    DF = dyn_cast_or_null<MCDataFragment>(DF->getNext());
    if (!DF)
      return nullptr;
    Offset = 0;
  }

  for (const MCFixup &F : DF->getFixups()) {
    if (F.getOffset() != Offset)
      continue;

    switch ((unsigned)F.getKind()) {
    default:
      continue;
    case RISCV::fixup_riscv_got_hi20:
    case RISCV::fixup_riscv_tls_got_hi20:
    case RISCV::fixup_riscv_tls_gd_hi20:
    case RISCV::fixup_riscv_pcrel_hi20:
    case RISCV::fixup_riscv_tlsdesc_hi20:
    case RISCV::fixup_riscv_captab_pcrel_hi20:
    case RISCV::fixup_riscv_tls_ie_captab_pcrel_hi20:
    case RISCV::fixup_riscv_tls_gd_captab_pcrel_hi20:
      if (DFOut)
        *DFOut = DF;
      return &F;
    }
  }

  return nullptr;
}

bool RISCVMCExpr::evaluateAsRelocatableImpl(MCValue &Res,
                                            const MCAssembler *Asm) const {
  if (!getSubExpr()->evaluateAsRelocatable(Res, Asm))
    return false;

  Res = MCValue::get(Res.getSymA(), Res.getSymB(), Res.getConstant(),
                     getSpecifier());
  // Custom fixup types are not valid with symbol difference expressions.
  return Res.getSymB() ? getSpecifier() == VK_None : true;
}

void RISCVMCExpr::visitUsedExpr(MCStreamer &Streamer) const {
  Streamer.visitUsedExpr(*getSubExpr());
}

RISCVMCExpr::Specifier RISCVMCExpr::getSpecifierForName(StringRef name) {
  return StringSwitch<RISCVMCExpr::Specifier>(name)
      .Case("lo", VK_LO)
      .Case("hi", VK_HI)
      .Case("pcrel_lo", VK_PCREL_LO)
      .Case("pcrel_hi", VK_PCREL_HI)
      .Case("got_pcrel_hi", VK_GOT_HI)
      .Case("tprel_lo", VK_TPREL_LO)
      .Case("tprel_hi", VK_TPREL_HI)
      .Case("tprel_add", VK_TPREL_ADD)
      .Case("tls_ie_pcrel_hi", VK_TLS_GOT_HI)
      .Case("tls_gd_pcrel_hi", VK_TLS_GD_HI)
      .Case("tlsdesc_hi", VK_TLSDESC_HI)
      .Case("tlsdesc_load_lo", VK_TLSDESC_LOAD_LO)
      .Case("tlsdesc_add_lo", VK_TLSDESC_ADD_LO)
      .Case("tlsdesc_call", VK_TLSDESC_CALL)
      .Case("captab_pcrel_hi", VK_CAPTAB_PCREL_HI)
      .Case("tprel_cincoffset", VK_TPREL_CINCOFFSET)
      .Case("tls_ie_captab_pcrel_hi", VK_TLS_IE_CAPTAB_PCREL_HI)
      .Case("tls_gd_captab_pcrel_hi", VK_TLS_GD_CAPTAB_PCREL_HI)
      .Case("cheriot_compartment_hi", VK_CHERIOT_COMPARTMENT_HI)
      .Case("cheriot_compartment_lo_i", VK_CHERIOT_COMPARTMENT_LO_I)
      .Case("cheriot_compartment_lo_s", VK_CHERIOT_COMPARTMENT_LO_S)
      .Case("cheriot_compartment_size", VK_CHERIOT_COMPARTMENT_SIZE)
      .Default(VK_Invalid);
}

StringRef RISCVMCExpr::getSpecifierName(Specifier S) {
  switch (S) {
  case VK_Invalid:
  case VK_None:
    llvm_unreachable("Invalid ELF symbol kind");
  case VK_LO:
    return "lo";
  case VK_HI:
    return "hi";
  case VK_PCREL_LO:
    return "pcrel_lo";
  case VK_PCREL_HI:
    return "pcrel_hi";
  case VK_GOT_HI:
    return "got_pcrel_hi";
  case VK_TPREL_LO:
    return "tprel_lo";
  case VK_TPREL_HI:
    return "tprel_hi";
  case VK_TPREL_ADD:
    return "tprel_add";
  case VK_TLS_GOT_HI:
    return "tls_ie_pcrel_hi";
  case VK_TLSDESC_HI:
    return "tlsdesc_hi";
  case VK_TLSDESC_LOAD_LO:
    return "tlsdesc_load_lo";
  case VK_TLSDESC_ADD_LO:
    return "tlsdesc_add_lo";
  case VK_TLSDESC_CALL:
    return "tlsdesc_call";
  case VK_TLS_GD_HI:
    return "tls_gd_pcrel_hi";
  case VK_CAPTAB_PCREL_HI:
    return "captab_pcrel_hi";
  case VK_TPREL_CINCOFFSET:
    return "tprel_cincoffset";
  case VK_TLS_IE_CAPTAB_PCREL_HI:
    return "tls_ie_captab_pcrel_hi";
  case VK_TLS_GD_CAPTAB_PCREL_HI:
    return "tls_gd_captab_pcrel_hi";
  case VK_CALL:
    return "call";
  case VK_CALL_PLT:
    return "call_plt";
  case VK_CCALL:
    return "ccall";
  case VK_CHERIOT_COMPARTMENT_HI:
    return "cheriot_compartment_hi";
  case VK_CHERIOT_COMPARTMENT_LO_I:
    return "cheriot_compartment_lo_i";
  case VK_CHERIOT_COMPARTMENT_LO_S:
    return "cheriot_compartment_lo_s";
  case VK_CHERIOT_COMPARTMENT_SIZE:
    return "cheriot_compartment_size";
  case VK_32_PCREL:
    return "32_pcrel";
  }
  llvm_unreachable("Invalid ELF symbol kind");
}

bool RISCVMCExpr::evaluateAsConstant(int64_t &Res) const {
  MCValue Value;
  if (specifier != VK_LO && specifier != VK_HI)
    return false;

  if (!getSubExpr()->evaluateAsRelocatable(Value, nullptr))
    return false;

  if (!Value.isAbsolute())
    return false;

  Res = evaluateAsInt64(Value.getConstant());
  return true;
}

int64_t RISCVMCExpr::evaluateAsInt64(int64_t Value) const {
  switch (specifier) {
  default:
    llvm_unreachable("Invalid kind");
  case VK_LO:
    return SignExtend64<12>(Value);
  case VK_HI:
    // Add 1 if bit 11 is 1, to compensate for low 12 bits being negative.
    return ((Value + 0x800) >> 12) & 0xfffff;
  }
}
