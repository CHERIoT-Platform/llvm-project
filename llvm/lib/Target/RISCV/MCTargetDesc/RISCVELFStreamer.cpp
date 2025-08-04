//===-- RISCVELFStreamer.cpp - RISC-V ELF Target Streamer Methods ---------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// This file provides RISC-V specific target streamer methods.
//
//===----------------------------------------------------------------------===//

#include "RISCVELFStreamer.h"
#include "RISCVAsmBackend.h"
#include "RISCVBaseInfo.h"
#include "RISCVFixupKinds.h"
#include "RISCVMCTargetDesc.h"
#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCAsmBackend.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCCodeEmitter.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCSubtargetInfo.h"

using namespace llvm;

// This part is for ELF object output.
RISCVTargetELFStreamer::RISCVTargetELFStreamer(MCStreamer &S,
                                               const MCSubtargetInfo &STI)
    : RISCVTargetStreamer(S), CurrentVendor("riscv") {
  MCAssembler &MCA = getStreamer().getAssembler();
  const FeatureBitset &Features = STI.getFeatureBits();
  auto &MAB = static_cast<RISCVAsmBackend &>(MCA.getBackend());
  setTargetABI(RISCVABI::computeTargetABI(STI.getTargetTriple(), Features,
                                          MAB.getTargetOptions().getABIName()));
  setFlagsFromFeatures(STI);
}

RISCVELFStreamer &RISCVTargetELFStreamer::getStreamer() {
  return static_cast<RISCVELFStreamer &>(Streamer);
}

void RISCVTargetELFStreamer::emitDirectiveOptionExact() {}
void RISCVTargetELFStreamer::emitDirectiveOptionNoExact() {}
void RISCVTargetELFStreamer::emitDirectiveOptionPIC() {}
void RISCVTargetELFStreamer::emitDirectiveOptionNoPIC() {}
void RISCVTargetELFStreamer::emitDirectiveOptionPop() {}
void RISCVTargetELFStreamer::emitDirectiveOptionPush() {}
void RISCVTargetELFStreamer::emitDirectiveOptionRelax() {}
void RISCVTargetELFStreamer::emitDirectiveOptionNoRelax() {}
void RISCVTargetELFStreamer::emitDirectiveOptionRVC() {}
void RISCVTargetELFStreamer::emitDirectiveOptionNoRVC() {}
void RISCVTargetELFStreamer::emitDirectiveOptionCapMode() {}
void RISCVTargetELFStreamer::emitDirectiveOptionNoCapMode() {}

void RISCVTargetELFStreamer::emitAttribute(unsigned Attribute, unsigned Value) {
  getStreamer().setAttributeItem(Attribute, Value, /*OverwriteExisting=*/true);
}

void RISCVTargetELFStreamer::emitTextAttribute(unsigned Attribute,
                                               StringRef String) {
  getStreamer().setAttributeItem(Attribute, String, /*OverwriteExisting=*/true);
}

void RISCVTargetELFStreamer::emitIntTextAttribute(unsigned Attribute,
                                                  unsigned IntValue,
                                                  StringRef StringValue) {
  getStreamer().setAttributeItems(Attribute, IntValue, StringValue,
                                  /*OverwriteExisting=*/true);
}

void RISCVTargetELFStreamer::finishAttributeSection() {
  RISCVELFStreamer &S = getStreamer();
  if (S.Contents.empty())
    return;

  S.emitAttributesSection(CurrentVendor, ".riscv.attributes",
                          ELF::SHT_RISCV_ATTRIBUTES, AttributeSection);
}

void RISCVTargetELFStreamer::finish() {
  RISCVTargetStreamer::finish();
  ELFObjectWriter &W = getStreamer().getWriter();
  RISCVABI::ABI ABI = getTargetABI();

  unsigned EFlags = W.getELFHeaderEFlags();

  if (hasRVC())
    EFlags |= ELF::EF_RISCV_RVC;
  if (hasTSO())
    EFlags |= ELF::EF_RISCV_TSO;

  if (isCapMode())
    EFlags |= ELF::EF_RISCV_CAP_MODE;

  switch (ABI) {
  case RISCVABI::ABI_ILP32:
  case RISCVABI::ABI_LP64:
    break;
  case RISCVABI::ABI_CHERIOT:
  case RISCVABI::ABI_CHERIOT_BAREMETAL:
    EFlags |= ELF::EF_RISCV_RVE;
    EFlags |= ELF::EF_RISCV_CHERIABI;
    EFlags |= ELF::EF_RISCV_CHERIOT;
    break;
  case RISCVABI::ABI_IL32PC64:
  case RISCVABI::ABI_L64PC128:
    EFlags |= ELF::EF_RISCV_CHERIABI;
    break;
  case RISCVABI::ABI_ILP32F:
  case RISCVABI::ABI_LP64F:
    EFlags |= ELF::EF_RISCV_FLOAT_ABI_SINGLE;
    break;
  case RISCVABI::ABI_IL32PC64F:
  case RISCVABI::ABI_L64PC128F:
    EFlags |= ELF::EF_RISCV_FLOAT_ABI_SINGLE;
    EFlags |= ELF::EF_RISCV_CHERIABI;
    break;
  case RISCVABI::ABI_ILP32D:
  case RISCVABI::ABI_LP64D:
    EFlags |= ELF::EF_RISCV_FLOAT_ABI_DOUBLE;
    break;
  case RISCVABI::ABI_IL32PC64D:
  case RISCVABI::ABI_L64PC128D:
    EFlags |= ELF::EF_RISCV_FLOAT_ABI_DOUBLE;
    EFlags |= ELF::EF_RISCV_CHERIABI;
    break;
  case RISCVABI::ABI_ILP32E:
  case RISCVABI::ABI_LP64E:
    EFlags |= ELF::EF_RISCV_RVE;
    break;
  case RISCVABI::ABI_IL32PC64E:
    EFlags |= ELF::EF_RISCV_RVE;
    EFlags |= ELF::EF_RISCV_CHERIABI;
    break;
  case RISCVABI::ABI_Unknown:
    llvm_unreachable("Improperly initialised target ABI");
  }

  W.setELFHeaderEFlags(EFlags);
}

void RISCVTargetELFStreamer::reset() {
  AttributeSection = nullptr;
}

void RISCVTargetELFStreamer::emitDirectiveVariantCC(MCSymbol &Symbol) {
  getStreamer().getAssembler().registerSymbol(Symbol);
  static_cast<MCSymbolELF &>(Symbol).setOther(ELF::STO_RISCV_VARIANT_CC);
}

void RISCVELFStreamer::reset() {
  static_cast<RISCVTargetStreamer *>(getTargetStreamer())->reset();
  MCELFStreamer::reset();
  LastMappingSymbols.clear();
  LastEMS = EMS_None;
}

void RISCVELFStreamer::emitDataMappingSymbol() {
  if (LastEMS == EMS_Data)
    return;
  emitMappingSymbol("$d");
  LastEMS = EMS_Data;
}

void RISCVELFStreamer::emitInstructionsMappingSymbol() {
  if (LastEMS == EMS_Instructions)
    return;
  emitMappingSymbol("$x");
  LastEMS = EMS_Instructions;
}

void RISCVELFStreamer::emitMappingSymbol(StringRef Name) {
  auto *Symbol =
      static_cast<MCSymbolELF *>(getContext().createLocalSymbol(Name));
  emitLabel(Symbol);
  Symbol->setType(ELF::STT_NOTYPE);
  Symbol->setBinding(ELF::STB_LOCAL);
}

void RISCVELFStreamer::changeSection(MCSection *Section, uint32_t Subsection) {
  // We have to keep track of the mapping symbol state of any sections we
  // use. Each one should start off as EMS_None, which is provided as the
  // default constructor by DenseMap::lookup.
  LastMappingSymbols[getPreviousSection().first] = LastEMS;
  LastEMS = LastMappingSymbols.lookup(Section);

  MCELFStreamer::changeSection(Section, Subsection);
}

void RISCVELFStreamer::emitInstruction(const MCInst &Inst,
                                       const MCSubtargetInfo &STI) {
  emitInstructionsMappingSymbol();
  MCELFStreamer::emitInstruction(Inst, STI);
}

void RISCVELFStreamer::emitBytes(StringRef Data) {
  emitDataMappingSymbol();
  MCELFStreamer::emitBytes(Data);
}

void RISCVELFStreamer::emitFill(const MCExpr &NumBytes, uint64_t FillValue,
                                SMLoc Loc) {
  emitDataMappingSymbol();
  MCELFStreamer::emitFill(NumBytes, FillValue, Loc);
}

void RISCVELFStreamer::emitValueImpl(const MCExpr *Value, unsigned Size,
                                     SMLoc Loc) {
  emitDataMappingSymbol();
  MCELFStreamer::emitValueImpl(Value, Size, Loc);
}

void RISCVELFStreamer::emitCheriIntcap(const MCExpr *Expr, unsigned CapSize,
                                       SMLoc Loc) {
  assert(CapSize == (getContext().getTargetTriple().isArch64Bit() ? 16 : 8));
  emitCheriIntcapGeneric(Expr, CapSize, Loc);
}

void RISCVELFStreamer::EmitCheriCapabilityImpl(const MCSymbol *Symbol,
                                               const MCExpr *Addend,
                                               unsigned CapSize, SMLoc Loc) {
  assert(Addend && "Should have received a MCConstExpr(0) instead of nullptr");
  visitUsedSymbol(*Symbol);
  MCContext &Context = getContext();

  const MCSymbolRefExpr *SRE = MCSymbolRefExpr::create(Symbol, 0, Context, Loc);
  const MCBinaryExpr *CapExpr = MCBinaryExpr::createAdd(SRE, Addend, Context);

  // Pad to ensure that the capability is aligned
  emitValueToAlignment(Align(CapSize), 0, 1, 0);
  addFixup(CapExpr, MCFixupKind(RISCV::fixup_riscv_capability));
  appendContents(CapSize, '\xca');
}

MCStreamer *llvm::createRISCVELFStreamer(const Triple &, MCContext &C,
                                         std::unique_ptr<MCAsmBackend> &&MAB,
                                         std::unique_ptr<MCObjectWriter> &&MOW,
                                         std::unique_ptr<MCCodeEmitter> &&MCE) {
  return new RISCVELFStreamer(C, std::move(MAB), std::move(MOW),
                              std::move(MCE));
}
