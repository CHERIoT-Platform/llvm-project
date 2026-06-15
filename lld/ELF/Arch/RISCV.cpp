//===- RISCV.cpp ----------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Cheri.h"
#include "InputFiles.h"
#include "OutputSections.h"
#include "Relocations.h"
#include "RelocScan.h"
#include "Symbols.h"
#include "SyntheticSections.h"
#include "Target.h"
#include "llvm/Support/CHERICapabilityFormat.h"
#include "llvm/Support/ELFAttributes.h"
#include "llvm/Support/LEB128.h"
#include "llvm/Support/RISCVAttributeParser.h"
#include "llvm/Support/RISCVAttributes.h"
#include "llvm/Support/TimeProfiler.h"
#include "llvm/TargetParser/RISCVISAInfo.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::support::endian;
using namespace llvm::ELF;
using namespace lld;
using namespace lld::elf;

namespace {

class RISCV final : public TargetInfo {
public:
  RISCV(Ctx &);
  uint32_t calcEFlags() const override;
  int getCapabilitySize() const override;
  uint64_t getCheriRequiredAlignment(uint64_t len) const override;
  int64_t getImplicitAddend(const uint8_t *buf, RelType type) const override;
  void writeGotHeader(uint8_t *buf) const override;
  void writeGotPlt(uint8_t *buf, const Symbol &s) const override;
  void writeIgotPlt(uint8_t *buf, const Symbol &s) const override;
  void writePltHeader(uint8_t *buf) const override;
  void writePlt(uint8_t *buf, const Symbol &sym,
                uint64_t pltEntryAddr) const override;
  template <class ELFT, class RelTy>
  void scanSectionImpl(InputSectionBase &, Relocs<RelTy>);
  void scanSection(InputSectionBase &) override;
  RelType getDynRel(RelType type) const override;
  RelExpr getRelExpr(RelType type, const Symbol &s,
                     const uint8_t *loc) const override;
  void relocate(uint8_t *loc, const Relocation &rel,
                uint64_t val) const override;
  void relocateAlloc(InputSection &sec, uint8_t *buf) const override;
  bool relaxOnce(int pass) const override;
  template <class ELFT, class RelTy>
  bool synthesizeAlignForInput(uint64_t &dot, InputSection *sec,
                               Relocs<RelTy> rels);
  template <class ELFT, class RelTy>
  void finalizeSynthesizeAligns(uint64_t &dot, InputSection *sec,
                                Relocs<RelTy> rels);
  template <class ELFT>
  bool synthesizeAlignAux(uint64_t &dot, InputSection *sec);
  bool synthesizeAlign(uint64_t &dot, InputSection *sec) override;
  void finalizeRelax(int passes) const override;
  uint64_t cheriRequiredAlignment(uint64_t) const override;

  // The following two variables are used by synthesized ALIGN relocations.
  InputSection *baseSec = nullptr;
  // r_offset and r_addend pairs.
  SmallVector<std::pair<uint64_t, uint64_t>, 0> synthesizedAligns;
};

} // end anonymous namespace

// Bit 8 of RelType is used to indicate linker-internal relocations that are
// not vendor-specific.
// These are internal relocation numbers for GP/X0 relaxation. They aren't part
// of the psABI spec.
constexpr uint32_t INTERNAL_R_RISCV_GPREL_I = 256;
constexpr uint32_t INTERNAL_R_RISCV_GPREL_S = 257;
constexpr uint32_t INTERNAL_R_RISCV_X0REL_I = 258;
constexpr uint32_t INTERNAL_R_RISCV_X0REL_S = 259;

// Bits 9 -> 31 of RelType are used to indicate vendor-specific relocations.
constexpr uint32_t INTERNAL_RISCV_VENDOR_MASK = 0xFFFFFFFF << 9;
constexpr uint32_t INTERNAL_RISCV_VENDOR_QUALCOMM = 1 << 9;
constexpr uint32_t INTERNAL_RISCV_VENDOR_ANDES = 2 << 9;
constexpr uint32_t INTERNAL_RISCV_VENDOR_CHERIOT1 = 3 << 9;

// CHERIoT Nonstandard Relocations
constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI =
    INTERNAL_RISCV_VENDOR_CHERIOT1 | llvm::ELF::R_RISCV_CHERIOT1_COMPARTMENT_CODE_HI;
constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I =
    INTERNAL_RISCV_VENDOR_CHERIOT1 | llvm::ELF::R_RISCV_CHERIOT1_COMPARTMENT_LO_I;
constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S =
    INTERNAL_RISCV_VENDOR_CHERIOT1 | llvm::ELF::R_RISCV_CHERIOT1_COMPARTMENT_LO_S;
constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_SIZE =
    INTERNAL_RISCV_VENDOR_CHERIOT1 | llvm::ELF::R_RISCV_CHERIOT1_COMPARTMENT_SIZE;
constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI =
    INTERNAL_RISCV_VENDOR_CHERIOT1 |
    llvm::ELF::R_RISCV_CHERIOT1_COMPARTMENT_DATA_HI;

constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I =
    INTERNAL_RISCV_VENDOR_CHERIOT1 |  256;
constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI =
    INTERNAL_RISCV_VENDOR_CHERIOT1 |  257;

// Used internally to mark the post-auicgp/auipcc NOP.
constexpr uint32_t INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI_NOP =
    INTERNAL_RISCV_VENDOR_CHERIOT1 | 258;

static uint32_t getRISCVVendorRelMarker(StringRef rvVendor) {
  return StringSwitch<uint32_t>(rvVendor)
      .Case("QUALCOMM", INTERNAL_RISCV_VENDOR_QUALCOMM)
      .Case("ANDES", INTERNAL_RISCV_VENDOR_ANDES)
      .Case("CHERIOT1", INTERNAL_RISCV_VENDOR_CHERIOT1)
      .Default(0);
}

const uint64_t dtpOffset = 0x800;

namespace {
enum Op {
  ADDI = 0x13,
  AUIPC = 0x17,
  JALR = 0x67,
  LD = 0x3003,
  LUI = 0x37,
  LW = 0x2003,
  SRLI = 0x5013,
  SUB = 0x40000033,

  CIncOffsetImm = 0x105b,
  CLC_64 = 0x3003,
  CLC_128 = 0x200f,

  AUIPCC = 0x17,
  AUICGP = 0x7b,
};

enum Reg {
  X_X0 = 0,
  X_RA = 1,
  X_GP = 3,
  X_TP = 4,
  X_T0 = 5,
  X_T1 = 6,
  X_T2 = 7,
  X_A0 = 10,
  X_T3 = 28,
};
} // namespace

static uint32_t hi20(uint32_t val) { return (val + 0x800) >> 12; }
static uint32_t lo12(uint32_t val) { return val & 4095; }

static uint32_t itype(uint32_t op, uint32_t rd, uint32_t rs1, uint32_t imm) {
  return op | (rd << 7) | (rs1 << 15) | (imm << 20);
}
static uint32_t rtype(uint32_t op, uint32_t rd, uint32_t rs1, uint32_t rs2) {
  return op | (rd << 7) | (rs1 << 15) | (rs2 << 20);
}
static uint32_t utype(uint32_t op, uint32_t rd, uint32_t imm) {
  return op | (rd << 7) | (imm << 12);
}

// Extract bits v[begin:end], where range is inclusive, and begin must be < 63.
static uint32_t extractBits(uint64_t v, uint32_t begin, uint32_t end) {
  return (v & ((1ULL << (begin + 1)) - 1)) >> end;
}

static uint32_t setLO12_I(uint32_t insn, uint32_t imm) {
  return (insn & 0xfffff) | (imm << 20);
}
static uint32_t setLO12_S(uint32_t insn, uint32_t imm) {
  return (insn & 0x1fff07f) | (extractBits(imm, 11, 5) << 25) |
         (extractBits(imm, 4, 0) << 7);
}

RISCV::RISCV(Ctx &ctx) : TargetInfo(ctx) {
  copyRel = R_RISCV_COPY;
  pltRel = R_RISCV_JUMP_SLOT;
  relativeRel = R_RISCV_RELATIVE;
  if (ctx.arg.isCheriAbi)
    relativeFuncRel = R_RISCV_FUNC_RELATIVE;
  iRelativeRel = R_RISCV_IRELATIVE;
  symbolicCapRel = R_RISCV_CHERI_CAPABILITY;
  symbolicCodeCapRel = R_RISCV_CHERI_CAPABILITY_CODE;
  tgotRel = R_RISCV_CHERI_TLS_TGOT_SLOT;
  tgotGotRel = R_RISCV_CHERI_TLS_TGOTREL;
  if (ctx.arg.is64) {
    symbolicRel = R_RISCV_64;
    tlsModuleIndexRel = R_RISCV_TLS_DTPMOD64;
    tlsOffsetRel = R_RISCV_TLS_DTPREL64;
    tlsGotRel = R_RISCV_TLS_TPREL64;
  } else {
    symbolicRel = R_RISCV_32;
    tlsModuleIndexRel = R_RISCV_TLS_DTPMOD32;
    tlsOffsetRel = R_RISCV_TLS_DTPREL32;
    tlsGotRel = R_RISCV_TLS_TPREL32;
  }
  if (ctx.arg.isCheriAbi)
    gotRel = *symbolicCapRel;
  else
    gotRel = symbolicRel;
  tlsDescRel = R_RISCV_TLSDESC;

  // .got[0] = _DYNAMIC
  gotHeaderEntriesNum = 1;
  if (ctx.arg.isCheriAbi)
    gotEntrySize = getCapabilitySize();

  // CHERIoT may use a GOT for CGP-relative relocations that end up out side of
  // the 12-bit displacement.
  if (ctx.arg.isCheriot)
    gotHeaderEntriesNum = 0;

  // .got.plt[0] = _dl_runtime_resolve, .got.plt[1] = link_map
  gotPltHeaderEntriesNum = 2;

  pltHeaderSize = 32;
  pltEntrySize = 16;
  ipltEntrySize = 16;
}

static uint32_t getEFlags(Ctx &ctx, InputFile *f) {
  if (ctx.arg.is64)
    return cast<ObjFile<ELF64LE>>(f)->getObj().getHeader().e_flags;
  return cast<ObjFile<ELF32LE>>(f)->getObj().getHeader().e_flags;
}

int RISCV::getCapabilitySize() const {
  return ctx.arg.is64 ? 16 : 8;
}

uint64_t RISCV::getCheriRequiredAlignment(uint64_t len) const {
  if (ctx.arg.isCheriot)
    return llvm::CHERIoTCapabilityFormat::getRequiredAlignment(len).value();
  else if (!ctx.arg.is64)
    return llvm::RV32YCapabilityFormat::getRequiredAlignment(len).value();
  return llvm::RV64YCapabilityFormat::getRequiredAlignment(len).value();
}

uint32_t RISCV::calcEFlags() const {
  // If there are only binary input files (from -b binary), use a
  // value of 0 for the ELF header flags.
  if (ctx.objectFiles.empty())
    return 0;

  uint32_t target = getEFlags(ctx, ctx.objectFiles.front());
  for (InputFile *f : ctx.objectFiles) {
    uint32_t eflags = getEFlags(ctx, f);
    if (eflags & EF_RISCV_RVC)
      target |= EF_RISCV_RVC;

    if ((eflags & EF_RISCV_FLOAT_ABI) != (target & EF_RISCV_FLOAT_ABI))
      Err(ctx) << f
               << ": cannot link object files with different "
                  "floating-point ABI from "
               << ctx.objectFiles[0];

    if ((eflags & EF_RISCV_RVE) != (target & EF_RISCV_RVE))
      Err(ctx) << f << ": cannot link object files with different EF_RISCV_RVE";

    if ((eflags & EF_RISCV_CHERIABI) != (target & EF_RISCV_CHERIABI))
      Err(ctx) << f
               << ": cannot link object files with different EF_RISCV_CHERIABI";

    if ((eflags & EF_RISCV_CAP_MODE) != (target & EF_RISCV_CAP_MODE))
      Err(ctx) << f
               << ": cannot link object files with different EF_RISCV_CAP_MODE";

    if ((eflags & EF_RISCV_CHERIOT) != (target & EF_RISCV_CHERIOT))
      Err(ctx) << f
               << ": cannot link object files with different EF_RISCV_CHERIOT";
  }

  return target;
}

int64_t RISCV::getImplicitAddend(const uint8_t *buf, RelType type) const {
  switch (type) {
  default:
    InternalErr(ctx, buf) << "cannot read addend for relocation " << type;
    return 0;
  case R_RISCV_32:
  case R_RISCV_TLS_DTPMOD32:
  case R_RISCV_TLS_DTPREL32:
  case R_RISCV_TLS_TPREL32:
    return SignExtend64<32>(read32le(buf));
  case R_RISCV_64:
  case R_RISCV_TLS_DTPMOD64:
  case R_RISCV_TLS_DTPREL64:
  case R_RISCV_TLS_TPREL64:
    return read64le(buf);
  case R_RISCV_RELATIVE:
  case R_RISCV_IRELATIVE:
  case R_RISCV_FUNC_RELATIVE:
    return ctx.arg.is64 ? read64le(buf) : read32le(buf);
  case R_RISCV_NONE:
  case R_RISCV_JUMP_SLOT:
    // These relocations are defined as not having an implicit addend.
    return 0;
  case R_RISCV_TLSDESC:
    return ctx.arg.is64 ? read64le(buf + 8) : read32le(buf + 4);
  }
}

void RISCV::writeGotHeader(uint8_t *buf) const {
  if (ctx.arg.is64)
    write64le(buf, ctx.in.dynamic->getVA());
  else
    write32le(buf, ctx.in.dynamic->getVA());
}

void RISCV::writeGotPlt(uint8_t *buf, const Symbol &s) const {
  // Initialised by __cap_relocs for CHERI
  if (ctx.arg.isCheriAbi)
    return;

  if (ctx.arg.is64)
    write64le(buf, ctx.in.plt->getVA());
  else
    write32le(buf, ctx.in.plt->getVA());
}

void RISCV::writeIgotPlt(uint8_t *buf, const Symbol &s) const {
  if (ctx.arg.writeAddends) {
    if (ctx.arg.is64)
      write64le(buf, s.getVA(ctx));
    else
      write32le(buf, s.getVA(ctx));
  }
}

void RISCV::writePltHeader(uint8_t *buf) const {
  // 1: auipc(c) (c)t2, %pcrel_hi(.got.plt)
  // (c)sub t1, (c)t1, (c)t3
  // l[wdc] (c)t3, %pcrel_lo(1b)((c)t2); (c)t3 = _dl_runtime_resolve
  // addi t1, t1, -pltHeaderSize-12; t1 = &.plt[i] - &.plt[0]
  // addi/cincoffset (c)t0, (c)t2, %pcrel_lo(1b)
  // (if shift != 0): srli t1, t1, shift; t1 = &.got.plt[i] - &.got.plt[0]
  // l[wdc] (c)t0, Ptrsize((c)t0); (c)t0 = link_map
  // (c)jr (c)t3
  // (if shift == 0): nop
  uint32_t offset = ctx.in.gotPlt->getVA() - ctx.in.plt->getVA();
  uint32_t ptrload = ctx.arg.isCheriAbi ? ctx.arg.is64 ? CLC_128 : CLC_64
                     : ctx.arg.is64     ? LD
                                        : LW;
  uint32_t ptraddi = ctx.arg.isCheriAbi ? CIncOffsetImm : ADDI;
  // Shift is log2(pltsize / ptrsize), which is 0 for CHERI-128 so skipped
  uint32_t shift = 2 - ctx.arg.is64 - ctx.arg.isCheriAbi;
  uint32_t ptrsize =
      ctx.arg.isCheriAbi ? ctx.arg.capabilitySize : ctx.arg.wordsize;
  write32le(buf + 0, utype(AUIPC, X_T2, hi20(offset)));
  write32le(buf + 4, rtype(SUB, X_T1, X_T1, X_T3));
  write32le(buf + 8, itype(ptrload, X_T3, X_T2, lo12(offset)));
  write32le(buf + 12, itype(ADDI, X_T1, X_T1, -ctx.target->pltHeaderSize - 12));
  write32le(buf + 16, itype(ptraddi, X_T0, X_T2, lo12(offset)));
  if (shift != 0)
    write32le(buf + 20, itype(SRLI, X_T1, X_T1, shift));
  write32le(buf + 24 - 4 * (shift == 0), itype(ptrload, X_T0, X_T0, ptrsize));
  write32le(buf + 28 - 4 * (shift == 0), itype(JALR, 0, X_T3, 0));
  if (shift == 0)
    write32le(buf + 28, itype(ADDI, 0, 0, 0));
}

void RISCV::writePlt(uint8_t *buf, const Symbol &sym,
                     uint64_t pltEntryAddr) const {
  // 1: auipc(c) (c)t3, %pcrel_hi(f@[.got.plt|.got])
  // l[wdc] (c)t3, %pcrel_lo(1b)((c)t3)
  // (c)jalr (c)t1, (c)t3
  // nop
  uint32_t ptrload = ctx.arg.isCheriAbi ? ctx.arg.is64 ? CLC_128 : CLC_64
                                        : ctx.arg.is64 ? LD : LW;
  uint32_t entryva = sym.getGotPltVA(ctx);
  uint32_t offset = entryva - pltEntryAddr;
  write32le(buf + 0, utype(AUIPC, X_T3, hi20(offset)));
  write32le(buf + 4, itype(ptrload, X_T3, X_T3, lo12(offset)));
  write32le(buf + 8, itype(JALR, X_T1, X_T3, 0));
  write32le(buf + 12, itype(ADDI, 0, 0, 0));
}

RelType RISCV::getDynRel(RelType type) const {
  return type == symbolicRel || type == symbolicCapRel
             ? type
             : static_cast<RelType>(R_RISCV_NONE);
}

// Only needed to support relocations used by relocateNonAlloc and
// preprocessRelocs.
RelExpr RISCV::getRelExpr(const RelType type, const Symbol &s,
                          const uint8_t *loc) const {
  switch (type) {
  case R_RISCV_NONE:
    return R_NONE;
  case R_RISCV_32:
  case R_RISCV_64:
    return R_ABS;
  case R_RISCV_ADD8:
  case R_RISCV_ADD16:
  case R_RISCV_ADD32:
  case R_RISCV_ADD64:
  case R_RISCV_SET6:
  case R_RISCV_SET8:
  case R_RISCV_SET16:
  case R_RISCV_SET32:
  case R_RISCV_SUB6:
  case R_RISCV_SUB8:
  case R_RISCV_SUB16:
  case R_RISCV_SUB32:
  case R_RISCV_SUB64:
    return RE_RISCV_ADD;
  case R_RISCV_32_PCREL:
    return R_PC;
  case R_RISCV_SET_ULEB128:
  case R_RISCV_SUB_ULEB128:
    return RE_RISCV_LEB128;
  case R_RISCV_CHERI_CAPABILITY:
  case R_RISCV_CHERI_CAPABILITY_CODE:
    return R_ABS_CAP;
  case R_RISCV_CHERI_TLS_TGOT_HI20:
  case R_RISCV_CHERI_TLS_TGOT_LO12_I:
    return R_TGOT_TP;
  case R_RISCV_CHERI_TLS_TGOT_GOT_HI20:
    return R_TGOT_GOT_PC;
  case R_RISCV_CHERI_TLS_TGOT_GD_HI20:
    return R_TGOT_TLSGD_PC;
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI:
    return R_PC;
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI:
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI:
    return RE_CHERIOT1_COMPARTMENT_CGPREL_HI;
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I:
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S:
    return RE_CHERIOT1_COMPARTMENT_CGPREL_LO;
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I:
    return RE_RISCV_PC_INDIRECT;
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_SIZE:
    return RE_CHERIOT1_COMPARTMENT_SIZE;
  default:
    if (type.v & INTERNAL_RISCV_VENDOR_MASK) {
      Err(ctx) << getErrorLoc(ctx, loc)
               << "unsupported vendor-specific relocation " << type
               << " against symbol " << &s;
      return R_NONE;
    }
    Err(ctx) << getErrorLoc(ctx, loc) << "unknown relocation ("
             << (type.v & ~INTERNAL_RISCV_VENDOR_MASK) << ") against symbol "
             << &s;
    return R_NONE;
  }
}


/**
 * Perform a substantial pre-pass of Cheriot relocations ahead of relaxation
 * and relocation. This pre-pass has two goals:
 *  - Separate PCC-relative and CGP-relative relocations. This reduces
 *    complexity in relaxation and relocation by making the decision once
 * upfront.
 *  - Resolve the target address for the second in a pair of relocations. This
 *    is required because relaxation may eliminate the first in a pair of
 *    relocations, which would leave the second one unable to be relocated.
 *
 * The specific post-conditions are:
 *  - All PCC-relative CHERIOT1_COMPARTMENT_CODE_HI relocations are represented as
 *    INTERNAL_R_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI.
 *  - All CGP-relative CHERIOT1_COMPARTMENT_CODE_HI relocations are represented as
 *    R_RISCV_CHERIOT1_COMPARTMENT_CODE_HI.
 *  - All PCC-relative CHERIOT1_COMPARTMENT_LO_{I|CINCOFFSET} relocations are represented as
 *    INTERNAL_R_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I.
 *  - All CGP-relative CHERIOT1_COMPARTMENT_LO_{I|CINCOFFSET} relocations are represented as
 *    R_RISCV_CHERIOT1_COMPARTMENT_LO_{I|CINCOFFSET}.
 *  - The targets of all CHERIOT1_COMPARTMENT_LO_{I|CINCOFFSET} relocations are resolved to
 *    the target of their paired CHERIOT1_COMPARTMENT_CODE_HI relocation.
 *  - PCC-relative CHERIOT1_COMPARTMENT_LO_S relocations do not exist.
 *  - All CGP-relative CHERIOT1_COMPARTMENT_LO_S relocations are represented as
 *    R_RISCV_CHERIOT1_COMPARTMENT_LO_S.
 */
static bool rewriteCheriotLowRelocs(Ctx &ctx, InputSectionBase &sec) {
  bool modified = false;
  for (Relocation &r :sec.relocations ) {
    if ((r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI ||
         r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI) &&
        isPCCRelative(ctx, nullptr, r.sym)) {
      modified = true;
      r.type = INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI;
      r.expr = R_PC;
    } else if (r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I ||
               r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S) {
      // If this is PCC-relative, then the relocation points to the auicgp /
      // auipcc instruction and we need to look there to find the real target.
      if (!isPCCRelative(ctx, nullptr, r.sym))
        fatal("R_RISCV_CHERIOT1_COMPARTMENT_LO_[I/S] must point to "
              "R_RISCV_COMPARTMENT_CODE_HI");

      const Defined *d = cast<Defined>(r.sym);
      if (!d->section)
        error("R_RISCV_CHERIOT1_COMPARTMENT_LO_[I/S] relocation points to an "
              "absolute symbol: " +
              r.sym->getName());
      InputSection *isec = cast<InputSection>(d->section);

      // Relocations are sorted by offset, so we can use std::equal_range to
      // do binary search.
      Relocation targetReloc;
      targetReloc.offset = d->value;
      auto range = std::equal_range(
          isec->relocations.begin(), isec->relocations.end(), targetReloc,
          [](const Relocation &lhs, const Relocation &rhs) {
            return lhs.offset < rhs.offset;
          });

      std::optional<Relocation> target;
      for (auto it = range.first; it != range.second; ++it)
        if (it->type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI ||
            it->type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI ||
            it->type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI) {
          target = *it;
          break;
        }
      if (!target) {
        error(
            "Could not find R_RISCV_CHERIOT1_COMPARTMENT_CODE_HI relocation for " +
            toStr(ctx, *r.sym));
      }
      modified = true;
      // If the target is PCC-relative then the auipcc can't be erased and so
      // skip the rewriting.
      if (isPCCRelative(ctx, nullptr, target->sym)) {
        assert(r.type != INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S &&
               "Malformed R_RISCV_CHERIOT1_COMPARTMENT_LO_S relocation!");
        r.type = INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I;
        r.expr = RE_RISCV_PC_INDIRECT;
        continue;
      }
      // Update our relocation to point to the target thing.
      r.sym = target->sym;
      r.addend = target->addend;
    }
  }
  return modified;
}

template <class ELFT, class RelTy>
void RISCV::scanSectionImpl(InputSectionBase &sec, Relocs<RelTy> rels) {
  RelocScan rs(ctx, &sec);
  // Many relocations end up in sec.relocations.
  sec.relocations.reserve(rels.size());

  StringRef vendor;
  for (auto it = rels.begin(); it != rels.end(); ++it) {
    RelType type = it->getType(false);
    uint32_t symIndex = it->getSymbol(false);
    Symbol &sym = sec.getFile<ELFT>()->getSymbol(symIndex);
    uint64_t offset = it->r_offset;
    if (sym.isUndefined() && symIndex != 0 &&
        rs.maybeReportUndefined(cast<Undefined>(sym), offset))
      continue;
    if (!vendor.empty()) {
      uint32_t VendorFlag = getRISCVVendorRelMarker(vendor);
      if (!VendorFlag) {
        Err(ctx) << getErrorLoc(ctx, sec.content().data() + offset)
                 << "unknown vendor-specific relocation (" << type.v
                 << ") in namespace '" << vendor << "' against symbol '" << &sym
                 << "'";
        continue;
      }
      type.v |= VendorFlag;
      vendor = "";
    }
    int64_t addend = rs.getAddend<ELFT>(*it, type);
    RelExpr expr;
    // Relocation types that only need a RelExpr set `expr` and break out of
    // the switch to reach rs.process(). Types that need special handling
    // (fast-path helpers, TLS) call a handler and use `continue`.
    switch (type) {
    case R_RISCV_NONE:
      continue;

    // Absolute relocations:
    case R_RISCV_32:
    case R_RISCV_64:
    case R_RISCV_HI20:
    case R_RISCV_LO12_I:
    case R_RISCV_LO12_S:
      expr = R_ABS;
      break;

    // PC-relative relocations:
    case R_RISCV_JAL:
    case R_RISCV_BRANCH:
    case R_RISCV_PCREL_HI20:
    case R_RISCV_RVC_BRANCH:
    case R_RISCV_RVC_JUMP:
    case R_RISCV_32_PCREL:
      rs.processR_PC(type, offset, addend, sym);
      continue;
    case R_RISCV_PCREL_LO12_I:
    case R_RISCV_PCREL_LO12_S:
      expr = RE_RISCV_PC_INDIRECT;
      break;

    // PLT-generating relocations:
    case R_RISCV_CALL:
    case R_RISCV_CALL_PLT:
    case R_RISCV_PLT32:
      rs.processR_PLT_PC(type, offset, addend, sym);
      continue;

    // GOT-generating relocations:
    case R_RISCV_GOT_HI20:
    case R_RISCV_GOT32_PCREL:
      expr = R_GOT_PC;
      break;

    // TLS relocations:
    case R_RISCV_TPREL_HI20:
    case R_RISCV_TPREL_LO12_I:
    case R_RISCV_TPREL_LO12_S:
      if (rs.checkTlsLe(offset, sym, type))
        continue;
      expr = R_TPREL;
      break;
    case R_RISCV_TLS_GOT_HI20:
      // There is no IE to LE optimization.
      rs.handleTlsIe<false>(R_GOT_PC, type, offset, addend, sym);
      continue;
    case R_RISCV_TLS_GD_HI20:
      // There is no GD to IE/LE optimization.
      rs.handleTlsGd(R_TLSGD_PC, R_NONE, R_NONE, type, offset, addend, sym);
      continue;

    // TLSDESC relocations:
    case R_RISCV_TLSDESC_HI20:
      rs.handleTlsDesc(R_TLSDESC_PC, R_GOT_PC, type, offset, addend, sym);
      continue;
    case R_RISCV_TLSDESC_LOAD_LO12:
    case R_RISCV_TLSDESC_ADD_LO12:
      // R_RISCV_TLSDESC_{LOAD_LO12,ADD_LO12,CALL} reference a label, not the
      // TLS symbol, so we cannot use handleTlsDesc (which sets NEEDS_TLSDESC).
      // For TLSDESC->IE, use R_TPREL as well, but relocateAlloc uses isToLe
      // (from HI20) to select the correct transform.
      sec.addReloc(ctx, {ctx.arg.shared ? R_TLSDESC_PC : R_TPREL, type, offset,
                    addend, &sym});
      continue;
    case R_RISCV_TLSDESC_CALL:
      if (!ctx.arg.shared)
        sec.addReloc(ctx, {R_TPREL, type, offset, addend, &sym});
      continue;

    // Relaxation hints:
    case R_RISCV_ALIGN:
      sec.addReloc(ctx,{R_RELAX_HINT, type, offset, addend, &sym});
      continue;
    case R_RISCV_TPREL_ADD:
    case R_RISCV_RELAX:
      if (ctx.arg.relax)
        sec.addReloc(ctx, {R_RELAX_HINT, type, offset, addend, &sym});
      continue;

    // Misc relocations:
    case R_RISCV_ADD8:
    case R_RISCV_ADD16:
    case R_RISCV_ADD32:
    case R_RISCV_ADD64:
    case R_RISCV_SET6:
    case R_RISCV_SET8:
    case R_RISCV_SET16:
    case R_RISCV_SET32:
    case R_RISCV_SUB6:
    case R_RISCV_SUB8:
    case R_RISCV_SUB16:
    case R_RISCV_SUB32:
    case R_RISCV_SUB64:
      expr = RE_RISCV_ADD;
      break;
    case R_RISCV_SET_ULEB128:
    case R_RISCV_SUB_ULEB128:
      expr = RE_RISCV_LEB128;
      break;

    // CHERI relocations
    case R_RISCV_CHERI_CAPABILITY:
    case R_RISCV_CHERI_CAPABILITY_CODE:
      expr = R_ABS_CAP;
      break;
    case R_RISCV_CHERI_TLS_TGOT_HI20:
    case R_RISCV_CHERI_TLS_TGOT_LO12_I:
      if (rs.checkTlsLe(offset, sym, type))
        continue;
      expr = R_TGOT_TP;
      sym.setFlags(NEEDS_TGOT);
      break;
    case R_RISCV_CHERI_TLS_TGOT_GOT_HI20:
      sym.setFlags(NEEDS_TGOT);
      rs.handleTlsIe<false>(R_TGOT_GOT_PC, type, offset, addend, sym, true);
      continue;
    case R_RISCV_CHERI_TLS_TGOT_GD_HI20:
      sym.setFlags(NEEDS_TGOT);
      rs.handleTlsGd(R_TGOT_TLSGD_PC, R_NONE, R_NONE, type, offset, addend, sym, true);
      continue;
    case R_RISCV_CHERI_TLS_TGOT_ADD:
      sym.setFlags(NEEDS_TGOT);
      if (ctx.arg.relax)
        sec.addReloc(ctx, {R_RELAX_HINT, type, offset, addend, &sym});
      continue;

    // CHERIoT Relocations
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI:
      expr = R_PC;
      break;
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI:
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI:
      expr = RE_CHERIOT1_COMPARTMENT_CGPREL_HI;
      break;
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I:
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S:
      expr = RE_CHERIOT1_COMPARTMENT_CGPREL_LO;
      break;
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I:
      expr = RE_RISCV_PC_INDIRECT;
      break;
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_SIZE:
      expr = RE_CHERIOT1_COMPARTMENT_SIZE;
      break;

    case R_RISCV_VENDOR: {
      auto it1 = it;
      ++it1;
      if (it1 == rels.end() || it1->getType(false) - 192u > 63u) {
        Err(ctx) << getErrorLoc(ctx, sec.content().data() + offset)
                 << "R_RISCV_VENDOR is not followed by a relocation of code "
                    "192 to 255";
        continue;
      }
      vendor = sym.getName();
      continue;
    }
    default:
      auto diag = Err(ctx);
      diag << getErrorLoc(ctx, sec.content().data() + offset);
      if (!vendor.empty()) {
        diag << "unknown vendor-specific relocation (" << type.v
             << ") in namespace '" << vendor << "' against symbol '" << &sym
             << "'";
        vendor = "";
      } else {
        diag << "unknown relocation (" << type.v << ") against symbol " << &sym;
      }
      continue;
    }
    rs.process(expr, type, offset, sym, addend);
  }

  if (ctx.arg.isCheriot) {
    // Insert COMPARTMENT_DATA_HI_NOP relocations after every COMPARTMENT_DATA_HI
    // relocation. This creates space in the relocation array for the relocation
    // needed if we need to turn an auicgp into a cap table load. Do it prior to
    // relocation sorting so that they end up in the correct location.
    for (size_t i = 0, e = sec.relocs().size(); i < e; ++i) {
      Relocation r = sec.relocs()[i];
      // We don't need to check for
      // INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI because they aren't
      // introduced yet.
      if (r.type != INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI)
        continue;
      r.type = INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI_NOP;
      r.offset += 4;
      sec.addReloc(ctx, r);
    }
  }

  // Sort relocations by offset for more efficient searching for
  // R_RISCV_PCREL_HI20.
  llvm::stable_sort(sec.relocs(),
                    [](const Relocation &lhs, const Relocation &rhs) {
                      return lhs.offset < rhs.offset;
                    });

  if (ctx.arg.isCheriot)
    rewriteCheriotLowRelocs(ctx, sec);
}

void RISCV::scanSection(InputSectionBase &sec) {
  if (ctx.arg.is64)
    elf::scanSection1<RISCV, ELF64LE>(*this, sec);
  else
    elf::scanSection1<RISCV, ELF32LE>(*this, sec);
}

void RISCV::relocate(uint8_t *loc, const Relocation &rel, uint64_t val) const {
  const unsigned bits = ctx.arg.wordsize * 8;

  switch (rel.type) {
  case R_RISCV_32:
    write32le(loc, val);
    return;
  case R_RISCV_64:
    write64le(loc, val);
    return;

  case R_RISCV_RVC_BRANCH: {
    checkInt(ctx, loc, val, 9, rel);
    checkAlignment(ctx, loc, val, 2, rel);
    uint16_t insn = read16le(loc) & 0xE383;
    uint16_t imm8 = extractBits(val, 8, 8) << 12;
    uint16_t imm4_3 = extractBits(val, 4, 3) << 10;
    uint16_t imm7_6 = extractBits(val, 7, 6) << 5;
    uint16_t imm2_1 = extractBits(val, 2, 1) << 3;
    uint16_t imm5 = extractBits(val, 5, 5) << 2;
    insn |= imm8 | imm4_3 | imm7_6 | imm2_1 | imm5;

    write16le(loc, insn);
    return;
  }

  case R_RISCV_RVC_JUMP: {
    checkInt(ctx, loc, val, 12, rel);
    checkAlignment(ctx, loc, val, 2, rel);
    uint16_t insn = read16le(loc) & 0xE003;
    uint16_t imm11 = extractBits(val, 11, 11) << 12;
    uint16_t imm4 = extractBits(val, 4, 4) << 11;
    uint16_t imm9_8 = extractBits(val, 9, 8) << 9;
    uint16_t imm10 = extractBits(val, 10, 10) << 8;
    uint16_t imm6 = extractBits(val, 6, 6) << 7;
    uint16_t imm7 = extractBits(val, 7, 7) << 6;
    uint16_t imm3_1 = extractBits(val, 3, 1) << 3;
    uint16_t imm5 = extractBits(val, 5, 5) << 2;
    insn |= imm11 | imm4 | imm9_8 | imm10 | imm6 | imm7 | imm3_1 | imm5;

    write16le(loc, insn);
    return;
  }

  case R_RISCV_JAL: {
    checkInt(ctx, loc, val, 21, rel);
    checkAlignment(ctx, loc, val, 2, rel);

    uint32_t insn = read32le(loc) & 0xFFF;
    uint32_t imm20 = extractBits(val, 20, 20) << 31;
    uint32_t imm10_1 = extractBits(val, 10, 1) << 21;
    uint32_t imm11 = extractBits(val, 11, 11) << 20;
    uint32_t imm19_12 = extractBits(val, 19, 12) << 12;
    insn |= imm20 | imm10_1 | imm11 | imm19_12;

    write32le(loc, insn);
    return;
  }

  case R_RISCV_BRANCH: {
    checkInt(ctx, loc, val, 13, rel);
    checkAlignment(ctx, loc, val, 2, rel);

    uint32_t insn = read32le(loc) & 0x1FFF07F;
    uint32_t imm12 = extractBits(val, 12, 12) << 31;
    uint32_t imm10_5 = extractBits(val, 10, 5) << 25;
    uint32_t imm4_1 = extractBits(val, 4, 1) << 8;
    uint32_t imm11 = extractBits(val, 11, 11) << 7;
    insn |= imm12 | imm10_5 | imm4_1 | imm11;

    write32le(loc, insn);
    return;
  }

  // auipc[c] + [c]jalr pair
  case R_RISCV_CALL:
  case R_RISCV_CALL_PLT: {
    int64_t hi = SignExtend64(val + 0x800, bits) >> 12;
    checkInt(ctx, loc, hi, 20, rel);
    if (isInt<20>(hi)) {
      relocateNoSym(loc,
                    ctx.arg.isCheriot
                        ? INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI
                        : R_RISCV_PCREL_HI20,
                    val);
      relocateNoSym(loc + 4,
                    ctx.arg.isCheriot
                        ? INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I
                        : R_RISCV_PCREL_LO12_I,
                    val);
    }
    return;
  }

  case R_RISCV_CHERI_TLS_TGOT_GOT_HI20:
  case R_RISCV_CHERI_TLS_TGOT_GD_HI20:
  case R_RISCV_CHERI_TLS_TGOT_HI20:
  case R_RISCV_GOT_HI20:
  case R_RISCV_PCREL_HI20:
  case R_RISCV_TLSDESC_HI20:
  case R_RISCV_TLS_GD_HI20:
  case R_RISCV_TLS_GOT_HI20:
  case R_RISCV_TPREL_HI20:
  case R_RISCV_HI20: {
    uint64_t hi = val + 0x800;
    checkInt(ctx, loc, SignExtend64(hi, bits) >> 12, 20, rel);
    write32le(loc, (read32le(loc) & 0xFFF) | (hi & 0xFFFFF000));
    return;
  }

  case R_RISCV_CHERI_TLS_TGOT_LO12_I:
  case R_RISCV_PCREL_LO12_I:
  case R_RISCV_TLSDESC_LOAD_LO12:
  case R_RISCV_TLSDESC_ADD_LO12:
  case R_RISCV_TPREL_LO12_I:
  case R_RISCV_LO12_I: {
    uint64_t hi = (val + 0x800) >> 12;
    uint64_t lo = val - (hi << 12);
    write32le(loc, setLO12_I(read32le(loc), lo & 0xfff));
    return;
  }

  case R_RISCV_PCREL_LO12_S:
  case R_RISCV_TPREL_LO12_S:
  case R_RISCV_LO12_S: {
    uint64_t hi = (val + 0x800) >> 12;
    uint64_t lo = val - (hi << 12);
    write32le(loc, setLO12_S(read32le(loc), lo));
    return;
  }

  case INTERNAL_R_RISCV_X0REL_I:
  case INTERNAL_R_RISCV_X0REL_S: {
    checkInt(ctx, loc, val, 12, rel);
    uint32_t insn = (read32le(loc) & ~(31 << 15)) | (X_X0 << 15);
    if (rel.type == INTERNAL_R_RISCV_X0REL_I)
      insn = setLO12_I(insn, val);
    else
      insn = setLO12_S(insn, val);
    write32le(loc, insn);
    return;
  }

  case INTERNAL_R_RISCV_GPREL_I:
  case INTERNAL_R_RISCV_GPREL_S: {
    Defined *gp = ctx.sym.riscvGlobalPointer;
    int64_t displace = SignExtend64(val - gp->getVA(ctx), bits);
    checkInt(ctx, loc, displace, 12, rel);
    uint32_t insn = (read32le(loc) & ~(31 << 15)) | (X_GP << 15);
    if (rel.type == INTERNAL_R_RISCV_GPREL_I)
      insn = setLO12_I(insn, displace);
    else
      insn = setLO12_S(insn, displace);
    write32le(loc, insn);
    return;
  }

  case R_RISCV_ADD8:
    *loc += val;
    return;
  case R_RISCV_ADD16:
    write16le(loc, read16le(loc) + val);
    return;
  case R_RISCV_ADD32:
    write32le(loc, read32le(loc) + val);
    return;
  case R_RISCV_ADD64:
    write64le(loc, read64le(loc) + val);
    return;
  case R_RISCV_SUB6:
    *loc = (*loc & 0xc0) | (((*loc & 0x3f) - val) & 0x3f);
    return;
  case R_RISCV_SUB8:
    *loc -= val;
    return;
  case R_RISCV_SUB16:
    write16le(loc, read16le(loc) - val);
    return;
  case R_RISCV_SUB32:
    write32le(loc, read32le(loc) - val);
    return;
  case R_RISCV_SUB64:
    write64le(loc, read64le(loc) - val);
    return;
  case R_RISCV_SET6:
    *loc = (*loc & 0xc0) | (val & 0x3f);
    return;
  case R_RISCV_SET8:
    *loc = val;
    return;
  case R_RISCV_SET16:
    write16le(loc, val);
    return;
  case R_RISCV_SET32:
  case R_RISCV_32_PCREL:
  case R_RISCV_PLT32:
  case R_RISCV_GOT32_PCREL:
    checkInt(ctx, loc, val, 32, rel);
    write32le(loc, val);
    return;

  case R_RISCV_TLS_DTPREL32:
    if (ctx.arg.isCheriAbi)
      write32le(loc, val);
    else
      write32le(loc, val - dtpOffset);
    break;
  case R_RISCV_TLS_DTPREL64:
    if (ctx.arg.isCheriAbi)
      write64le(loc, val);
    else
      write64le(loc, val - dtpOffset);
    break;

  case R_RISCV_CHERI_TLS_TGOTREL:
    if (ctx.arg.is64)
      write64le(loc, val);
    else
      write32le(loc, val);
    break;

  case R_RISCV_RELAX:
    return;
  case R_RISCV_TLSDESC:
    // The addend is stored in the second word.
    if (ctx.arg.is64)
      write64le(loc + 8, val);
    else
      write32le(loc + 4, val);
    break;
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I: {
    checkInt(ctx, loc, val, 12, rel);
    write32le(loc, (read32le(loc) & 0x000fffff) | (val << 20));
    break;
  }
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I: {
    // Attach a negative sign bit to LO12 if the offset is negative.
    // However, if HI20 alone is enough to reach the target, then this should
    // not be done and LO14 should just be 0 regardless.
    if (int64_t(val) >= 0 || (val & 0x7ff) == 0)
      val &= 0x7ff;
    else
      val = (uint64_t(-1) & ~0x7ff) | (val & 0x7ff);
    checkInt(ctx, loc, val, 12, rel);
    write32le(loc, (read32le(loc) & 0x000fffff) | (val << 20));
    break;
  }
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_SIZE:
    checkUInt(ctx, loc, val, 12, rel);
    write32le(loc, (read32le(loc) & 0x000fffff) | (val << 20));
    break;
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S: {
    // Stores have their immediate fields split because RISC-V prematurely
    // optimises for small pipelines with no FPU.
    uint32_t insn = read32le(loc) & 0x1fff07f;
    uint32_t val_high = val & 0xfe0;
    uint32_t val_low = val & 0x1f;
    write32le(loc, insn | (val_high << 20) | (val_low << 7));
    break;
  }
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI: {
    // FIXME: Unrelaxed AUICGP relocation should not occur. This is path
    // only needed to retain compatibility with object files that predate
    // COMPARTMENT_DATA_HI relocations, and should be removed in the future.

    // AUICGP
    uint32_t opcode = AUICGP;
    uint32_t existingOpcode = read32le(loc) & 0x7f;
    if ((existingOpcode != AUIPCC) && (existingOpcode != AUICGP))
      warn("R_RISCV_CHERIOT1_COMPARTMENT_CODE_HI relocation applied to instruction "
           "with unexpected opcode " +
           Twine(existingOpcode));
    checkInt(ctx, loc, SignExtend64(val + 0x800, bits) >> 12, 20, rel);
    // Preserve the target register.  We will rewrite the opcode (source
    // register) to either AUICGP or AUIPCC and set the immediate field.
    uint32_t insn = read32le(loc) & 0x00000f80;
    write32le(loc, insn | (val << 12) | opcode);
    break;
  }
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI: {
    // AUIPCC
    uint32_t opcode = AUIPCC;
    if (int64_t(val) < 0)
      val = (val + 0x7ff) & ~0x7ff;
    val = int64_t(val) >> 11;
    uint32_t existingOpcode = read32le(loc) & 0x7f;
    if ((existingOpcode != AUIPCC) && (existingOpcode != AUICGP))
      warn("R_RISCV_CHERIOT1_COMPARTMENT_CODE_HI relocation applied to instruction "
           "with unexpected opcode " +
           Twine(existingOpcode));
    checkInt(ctx, loc, SignExtend64(val + 0x800, bits) >> 12, 20, rel);
    // Preserve the target register.  We will rewrite the opcode (source
    // register) to either AUICGP or AUIPCC and set the immediate field.
    uint32_t insn = read32le(loc) & 0x00000f80;
    write32le(loc, insn | (val << 12) | opcode);
    break;
  }

  case R_RISCV_CHERI_CAPABILITY:
  case R_RISCV_CHERI_CAPABILITY_CODE:
    // Write a word within the capability
    if (ctx.arg.is64)
      write64le(loc, val);
    else
      write32le(loc, val);
    break;

  default:
    llvm_unreachable("unknown relocation");
  }
}

static bool relaxable(ArrayRef<Relocation> relocs, size_t i) {
  return i + 1 != relocs.size() && relocs[i + 1].type == R_RISCV_RELAX;
}

static void tlsdescToIe(Ctx &ctx, uint8_t *loc, const Relocation &rel,
                        uint64_t val) {
  switch (rel.type) {
  case R_RISCV_TLSDESC_HI20:
  case R_RISCV_TLSDESC_LOAD_LO12:
    write32le(loc, 0x00000013); // nop
    break;
  case R_RISCV_TLSDESC_ADD_LO12:
    write32le(loc, utype(AUIPC, X_A0, hi20(val))); // auipc a0,<hi20>
    break;
  case R_RISCV_TLSDESC_CALL:
    if (ctx.arg.is64)
      write32le(loc, itype(LD, X_A0, X_A0, lo12(val))); // ld a0,<lo12>(a0)
    else
      write32le(loc, itype(LW, X_A0, X_A0, lo12(val))); // lw a0,<lo12>(a0)
    break;
  default:
    llvm_unreachable("unsupported relocation for TLSDESC to IE");
  }
}

static void tlsdescToLe(uint8_t *loc, const Relocation &rel, uint64_t val) {
  switch (rel.type) {
  case R_RISCV_TLSDESC_HI20:
  case R_RISCV_TLSDESC_LOAD_LO12:
    write32le(loc, 0x00000013); // nop
    return;
  case R_RISCV_TLSDESC_ADD_LO12:
    if (isInt<12>(val))
      write32le(loc, 0x00000013); // nop
    else
      write32le(loc, utype(LUI, X_A0, hi20(val))); // lui a0,<hi20>
    return;
  case R_RISCV_TLSDESC_CALL:
    if (isInt<12>(val))
      write32le(loc, itype(ADDI, X_A0, 0, val)); // addi a0,zero,<lo12>
    else
      write32le(loc, itype(ADDI, X_A0, X_A0, lo12(val))); // addi a0,a0,<lo12>
    return;
  default:
    llvm_unreachable("unsupported relocation for TLSDESC to LE");
  }
}

uint64_t RISCV::cheriRequiredAlignment(uint64_t len) const {
  if (ctx.arg.isCheriot)
    return llvm::CHERIoTCapabilityFormat::getRequiredAlignment(len).value();
  else if (!ctx.arg.is64)
    return llvm::RV32YCapabilityFormat::getRequiredAlignment(len).value();
  return llvm::RV64YCapabilityFormat::getRequiredAlignment(len).value();
}

void RISCV::relocateAlloc(InputSection &sec, uint8_t *buf) const {
  uint64_t secAddr = sec.getOutputSection()->addr + sec.outSecOff;
  uint64_t tlsdescVal = 0;
  bool tlsdescRelax = false, isToLe = false;
  const ArrayRef<Relocation> relocs = sec.relocs();
  for (size_t i = 0, size = relocs.size(); i != size; ++i) {
    const Relocation &rel = relocs[i];
    assert(rel.type.v <= INTERNAL_R_RISCV_X0REL_S || rel.type.v >= (1<<9));
    uint8_t *loc = buf + rel.offset;
    uint64_t val = sec.getRelocTargetVA(ctx, rel, secAddr + rel.offset);

    switch (rel.type) {
    case R_RISCV_ALIGN:
    case R_RISCV_RELAX:
    case R_RISCV_TPREL_ADD:
    case R_RISCV_CHERI_TLS_TGOT_ADD:
      continue;
    case R_RISCV_TLSDESC_HI20:
      if (rel.expr == R_TLSDESC_PC) {
        // Shared object: store &got(sym)-PC for the following L[DW]/ADDI.
        tlsdescVal = val;
        break;
      }
      // Executable: TLSDESC->LE (R_TPREL) or TLSDESC->IE (R_GOT_PC).
      isToLe = rel.expr == R_TPREL;
      if (isToLe) {
        tlsdescVal = val;
      } else {
        // tlsdescVal will be finalized after we see R_RISCV_TLSDESC_ADD_LO12.
        // The net effect is that tlsdescVal will be smaller than `val` to
        // take into account of NOP instructions (in the absence of
        // R_RISCV_RELAX) before AUIPC.
        tlsdescVal = val + rel.offset;
      }
      tlsdescRelax = relaxable(relocs, i);
      if (!tlsdescRelax) {
        if (isToLe)
          tlsdescToLe(loc, rel, val);
        else
          tlsdescToIe(ctx, loc, rel, val);
      }
      continue;
    case R_RISCV_TLSDESC_LOAD_LO12:
    case R_RISCV_TLSDESC_ADD_LO12:
    case R_RISCV_TLSDESC_CALL:
      if (rel.expr == R_TLSDESC_PC) {
        // Shared object: propagate the stored GOT value.
        val = tlsdescVal;
        break;
      }
      // Executable: IE or LE instruction rewrite.
      if (!isToLe && rel.type == R_RISCV_TLSDESC_ADD_LO12)
        tlsdescVal -= rel.offset;
      val = tlsdescVal;
      // When NOP conversion is eligible and relaxation applies, don't write a
      // NOP in case an unrelated instruction follows the current instruction.
      if (tlsdescRelax &&
          (rel.type == R_RISCV_TLSDESC_LOAD_LO12 ||
           (rel.type == R_RISCV_TLSDESC_ADD_LO12 && isToLe && !hi20(val))))
        continue;
      if (isToLe)
        tlsdescToLe(loc, rel, val);
      else
        tlsdescToIe(ctx, loc, rel, val);
      continue;
    case R_RISCV_SET_ULEB128:
      if (i + 1 < size) {
        const Relocation &rel1 = relocs[i + 1];
        if (rel1.type == R_RISCV_SUB_ULEB128 && rel.offset == rel1.offset) {
          auto val = rel.sym->getVA(ctx, rel.addend) -
                     rel1.sym->getVA(ctx, rel1.addend);
          if (overwriteULEB128(loc, val) >= 0x80)
            Err(ctx) << sec.getLocation(rel.offset) << ": ULEB128 value " << val
                     << " exceeds available space; references '" << rel.sym
                     << "'";
          ++i;
          continue;
        }
      }
      Err(ctx) << sec.getLocation(rel.offset)
               << ": R_RISCV_SET_ULEB128 not paired with R_RISCV_SUB_ULEB128";
      return;
    default:
      break;
    }
    relocate(loc, rel, val);
  }
}

void elf::initSymbolAnchors(Ctx &ctx) {
  SmallVector<InputSection *, 0> storage;
  for (OutputSection *osec : ctx.outputSections) {
    if (!(osec->flags & SHF_EXECINSTR))
      continue;
    for (InputSection *sec : getInputSections(*osec, storage)) {
      if (isa<SyntheticSection>(sec))
        continue;
      sec->relaxAux = make<RelaxAux>();
      if (sec->relocs().size()) {
        sec->relaxAux->relocDeltas =
            std::make_unique<uint32_t[]>(sec->relocs().size());
        sec->relaxAux->relocTypes =
            std::make_unique<RelType[]>(sec->relocs().size());
      }
    }
  }
  // Store symbol anchors for adjusting st_value/st_size during relaxation.
  // We include symbols where d->file == file for the prevailing copies.
  //
  // For a defined symbol foo, we may have `d->file != file` with --wrap=foo.
  // We should process foo, as the defining object file's symbol table may not
  // contain foo after redirectSymbols changed the foo entry to __wrap_foo. Use
  // `d->scriptDefined` to include such symbols.
  //
  // `relaxAux->anchors` may contain duplicate symbols, but that is fine.
  auto addAnchor = [&ctx](Defined *d) {
    if (auto *sec = dyn_cast_or_null<InputSection>(d->section))
      if (sec->flags & SHF_EXECINSTR && sec->relaxAux) {
        // If sec is discarded, relaxAux will be nullptr.
        sec->relaxAux->anchors.push_back({d->value, d, false});
        sec->relaxAux->anchors.push_back(
              {d->value + d->getSize(ctx), d, true});
      }
  };
  for (InputFile *file : ctx.objectFiles)
    for (Symbol *sym : file->getSymbols()) {
      auto *d = dyn_cast<Defined>(sym);
      if (d && (d->file == file || d->scriptDefined))
        addAnchor(d);
    }
  // Add anchors for IRELATIVE symbols (see `handleNonPreemptibleIfunc`).
  // Their values must be adjusted so IRELATIVE addends remain correct.
  for (Defined *d : ctx.irelativeSyms)
    addAnchor(d);
  // Sort anchors by offset so that we can find the closest relocation
  // efficiently. For a zero size symbol, ensure that its start anchor precedes
  // its end anchor. For two symbols with anchors at the same offset, their
  // order does not matter.
  for (OutputSection *osec : ctx.outputSections) {
    if (!(osec->flags & SHF_EXECINSTR))
      continue;
    for (InputSection *sec : getInputSections(*osec, storage)) {
      if (!sec->relaxAux)
        continue;
      llvm::sort(sec->relaxAux->anchors, [](auto &a, auto &b) {
        return std::make_pair(a.offset, a.end) <
               std::make_pair(b.offset, b.end);
      });
    }
  }
}

// Relax R_RISCV_CALL/R_RISCV_CALL_PLT auipc+jalr to c.j, c.jal, or jal.
static void relaxCall(Ctx &ctx, const InputSection &sec, size_t i, uint64_t loc,
                      Relocation &r, uint32_t &remove) {
  const bool rvc = getEFlags(ctx, sec.file) & EF_RISCV_RVC;
  const Symbol &sym = *r.sym;
  const uint64_t insnPair = read64le(sec.content().data() + r.offset);
  const uint32_t rd = extractBits(insnPair, 32 + 11, 32 + 7);
  const uint64_t dest =
      (r.expr == R_PLT_PC ? sym.getPltVA(ctx) : sym.getVA(ctx)) + r.addend;
  const int64_t displace = dest - loc;

  // When the caller specifies the old value of `remove`, disallow its
  // increment.
  if (remove >= 6 && rvc && isInt<12>(displace) && rd == X_X0) {
    sec.relaxAux->relocTypes[i] = R_RISCV_RVC_JUMP;
    sec.relaxAux->writes.push_back(0xa001); // c.[c]j
    remove = 6;
  } else if (remove >= 6 && rvc && isInt<12>(displace) && rd == X_RA &&
             !ctx.arg.is64) { // RV32C only
    sec.relaxAux->relocTypes[i] = R_RISCV_RVC_JUMP;
    sec.relaxAux->writes.push_back(0x2001); // c.jal
    remove = 6;
  } else if (remove >= 4 && isInt<21>(displace)) {
    sec.relaxAux->relocTypes[i] = R_RISCV_JAL;
    sec.relaxAux->writes.push_back(0x6f | rd << 7); // [c]jal
    remove = 4;
  } else {
    remove = 0;
  }
}

// Relax local-exec TLS when hi20 is zero.
static void relaxTlsLe(Ctx &ctx, const InputSection &sec, size_t i,
                       uint64_t loc, Relocation &r, uint32_t &remove) {
  uint64_t val = r.sym->getVA(ctx, r.addend);
  if (hi20(val) != 0)
    return;
  uint32_t insn = read32le(sec.content().data() + r.offset);
  switch (r.type) {
  case R_RISCV_TPREL_HI20:
  case R_RISCV_TPREL_ADD:
    // Remove lui rd, %tprel_hi(x) and add rd, rd, tp, %tprel_add(x).
    sec.relaxAux->relocTypes[i] = R_RISCV_RELAX;
    remove = 4;
    break;
  case R_RISCV_TPREL_LO12_I:
    // addi rd, rd, %tprel_lo(x) => addi rd, tp, st_value(x)
    sec.relaxAux->relocTypes[i] = R_RISCV_32;
    insn = (insn & ~(31 << 15)) | (X_TP << 15);
    sec.relaxAux->writes.push_back(setLO12_I(insn, val));
    break;
  case R_RISCV_TPREL_LO12_S:
    // sw rs, %tprel_lo(x)(rd) => sw rs, st_value(x)(rd)
    sec.relaxAux->relocTypes[i] = R_RISCV_32;
    insn = (insn & ~(31 << 15)) | (X_TP << 15);
    sec.relaxAux->writes.push_back(setLO12_S(insn, val));
    break;
  }
}

static void relaxHi20Lo12(Ctx &ctx, const InputSection &sec, size_t i,
                          uint64_t loc, Relocation &r, uint32_t &remove) {

  // Fold into use of x0+offset
  if (isInt<12>(r.sym->getVA(ctx, r.addend))) {
    switch (r.type) {
    case R_RISCV_HI20:
      // Remove lui rd, %hi20(x).
      sec.relaxAux->relocTypes[i] = R_RISCV_RELAX;
      remove = 4;
      break;
    case R_RISCV_LO12_I:
      sec.relaxAux->relocTypes[i] = INTERNAL_R_RISCV_X0REL_I;
      break;
    case R_RISCV_LO12_S:
      sec.relaxAux->relocTypes[i] = INTERNAL_R_RISCV_X0REL_S;
      break;
    }
    return;
  }

  const Defined *gp = ctx.sym.riscvGlobalPointer;
  if (!gp)
    return;

  if (!isInt<12>(r.sym->getVA(ctx, r.addend) - gp->getVA(ctx)))
    return;

  switch (r.type) {
  case R_RISCV_HI20:
    // Remove lui rd, %hi20(x).
    sec.relaxAux->relocTypes[i] = R_RISCV_RELAX;
    remove = 4;
    break;
  case R_RISCV_LO12_I:
    sec.relaxAux->relocTypes[i] = INTERNAL_R_RISCV_GPREL_I;
    break;
  case R_RISCV_LO12_S:
    sec.relaxAux->relocTypes[i] = INTERNAL_R_RISCV_GPREL_S;
    break;
  }
}

// Relax auicgp + cincoffset/memop to cincoffset/memop cgp
static void relaxCGP(Ctx &ctx, const InputSection &sec, size_t i, uint64_t loc,
                     Relocation &r, uint32_t &remove) {
  uint64_t hival =
      getBiasedCGPOffset(ctx, *r.sym) - getBiasedCGPOffsetLo12(ctx, *r.sym);

  if (hival != 0) {
    if (!ctx.shouldRelaxAuicgpToCapTable)
      return;
    // Once we have concluded that we have no hope of bringing this CGP-relative
    // access in-range, we instead transform the entire sequence into a load
    // through the GOT.
    switch (r.type) {
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI: {
      // AUICGP rd, sym --> AUIPCC rd, hi(got(sym))
      sec.relaxAux->relocTypes[i] =
          INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI;
      uint32_t insn = read32le(sec.content().data() + r.offset);
      uint32_t auipcc = (insn & ~0x7f) | AUIPCC;
      sec.relaxAux->writes.push_back(auipcc);
      break;
    }
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI_NOP: {
      // NOP -> CLC rd, rd, lo(got(sym))
      sec.relaxAux->relocTypes[i] =
          INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I;
      uint32_t insn = read32le(sec.content().data() + r.offset - 4);
      uint32_t reg = insn & 0x00000f80;
      uint32_t clc = CLC_64 | reg | (reg << 8);
      sec.relaxAux->writes.push_back(clc);
      break;
    }
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I:
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S: {
      // CLW rd, ra, lo(sym+off) -> CLW rd, ra, off
      sec.relaxAux->relocTypes[i] = R_RISCV_32;
      uint32_t insn = read32le(sec.content().data() + r.offset);
      int32_t addend = r.addend;
      addend <<= 20;
      insn |= addend;
      sec.relaxAux->writes.push_back(insn);
      break;
    }
    }

    return;
  }

  // Relax away the auicgp when imm == 0 in auicgp rd, imm.
  uint32_t insn = read32le(sec.content().data() + r.offset);
  switch (r.type) {
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI:
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI_NOP:
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI: {
    // Remove auicgp rd, 0 and trailing nop4.
    sec.relaxAux->relocTypes[i] = R_RISCV_RELAX;
    remove = 4;
    break;
  }
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I: {
    // cincoffset/load rd, cs1, %lo(x) => cincoffset/load rd, cgp, %lo(x)
    sec.relaxAux->relocTypes[i] = r.type;
    insn = (insn & ~(31 << 15)) | (3 << 15);
    sec.relaxAux->writes.push_back(insn);
    break;
  }
  case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S:
    // store cs2, cs1, %lo(x) => store cs2, cgp, %lo(x)
    sec.relaxAux->relocTypes[i] = INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S;
    insn = (insn & ~(31 << 15)) | (3 << 15);
    sec.relaxAux->writes.push_back(insn);
    break;
  }
}

static bool relax(Ctx &ctx, int pass, InputSection &sec) {
  const uint64_t secAddr = sec.getVA();
  const MutableArrayRef<Relocation> relocs = sec.relocs();
  auto &aux = *sec.relaxAux;
  bool changed = false;
  ArrayRef<SymbolAnchor> sa = ArrayRef(aux.anchors);
  uint64_t delta = 0;
  bool tlsdescRelax = false, toLeShortForm = false;

  std::fill_n(aux.relocTypes.get(), relocs.size(), R_RISCV_NONE);
  aux.writes.clear();
  for (auto [i, r] : llvm::enumerate(relocs)) {
    const uint64_t loc = secAddr + r.offset - delta;
    uint32_t &cur = aux.relocDeltas[i], remove = 0;
    switch (r.type) {
    case R_RISCV_ALIGN: {
      const uint64_t nextLoc = loc + r.addend;
      const uint64_t align = PowerOf2Ceil(r.addend + 2);
      // All bytes beyond the alignment boundary should be removed.
      remove = nextLoc - ((loc + align - 1) & -align);
      // If we can't satisfy this alignment, we've found a bad input.
      if (LLVM_UNLIKELY(static_cast<int32_t>(remove) < 0)) {
        Err(ctx) << getErrorLoc(ctx, (const uint8_t *)loc)
                 << "insufficient padding bytes for " << r.type << ": "
                 << r.addend
                 << " bytes available "
                    "for requested alignment of "
                 << align << " bytes";
        remove = 0;
      }
      break;
    }
    case R_RISCV_CALL:
    case R_RISCV_CALL_PLT:
      // Prevent oscillation between states by disallowing the increment of
      // `remove` after a few passes. The previous `remove` value is
      // `cur-delta`.
      if (relaxable(relocs, i)) {
        remove = pass < 4 ? 6 : cur - delta;
        relaxCall(ctx, sec, i, loc, r, remove);
      }
      break;
    case R_RISCV_TPREL_HI20:
    case R_RISCV_TPREL_ADD:
    case R_RISCV_TPREL_LO12_I:
    case R_RISCV_TPREL_LO12_S:
      if (relaxable(relocs, i))
        relaxTlsLe(ctx, sec, i, loc, r, remove);
      break;
    case R_RISCV_HI20:
    case R_RISCV_LO12_I:
    case R_RISCV_LO12_S:
      if (relaxable(relocs, i))
        relaxHi20Lo12(ctx, sec, i, loc, r, remove);
      break;
    case R_RISCV_TLSDESC_HI20:
      // For TLSDESC=>LE, we can use the short form if hi20 is zero.
      tlsdescRelax = relaxable(relocs, i);
      toLeShortForm = tlsdescRelax && r.expr == R_TPREL &&
                      !hi20(r.sym->getVA(ctx, r.addend));
      [[fallthrough]];
    case R_RISCV_TLSDESC_LOAD_LO12:
      // For TLSDESC=>LE/IE, AUIPC and L[DW] are removed if relaxable.
      if (tlsdescRelax && r.expr != R_TLSDESC_PC)
        remove = 4;
      break;
    case R_RISCV_TLSDESC_ADD_LO12:
      if (toLeShortForm)
        remove = 4;
      break;
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_CODE_HI:
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI:
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I:
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S:
      if (relaxable(relocs, i))
        relaxCGP(ctx, sec, i, loc, r, remove);
      break;
    case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI_NOP:
      if (isPCCRelative(ctx, nullptr, r.sym)) {
        // Always remove the trailing nop if this follows an AUIPCC
        sec.relaxAux->relocTypes[i] = R_RISCV_RELAX;
        remove = 4;
        break;
      }

      relaxCGP(ctx, sec, i, loc, r, remove);
      break;
    }

    // For all anchors whose offsets are <= r.offset, they are preceded by
    // the previous relocation whose `relocDeltas` value equals `delta`.
    // Decrease their st_value and update their st_size.
    for (; sa.size() && sa[0].offset <= r.offset; sa = sa.slice(1)) {
      if (sa[0].end)
        sa[0].d->setSize(sa[0].offset - delta - sa[0].d->value);
      else
        sa[0].d->value = sa[0].offset - delta;
    }
    delta += remove;
    if (delta != cur) {
      cur = delta;
      changed = true;
    }
  }

  for (const SymbolAnchor &a : sa) {
    if (a.end)
      a.d->setSize(a.offset - delta - a.d->value);
    else
      a.d->value = a.offset - delta;
  }
  // Inform assignAddresses that the size has changed.
  if (!isUInt<32>(delta))
    Err(ctx) << "section size decrease is too large: " << delta;
  sec.bytesDropped = delta;
  return changed;
}

// When relaxing just R_RISCV_ALIGN, relocDeltas is usually changed only once in
// the absence of a linker script. For call and load/store R_RISCV_RELAX, code
// shrinkage may reduce displacement and make more relocations eligible for
// relaxation. Code shrinkage may increase displacement to a call/load/store
// target at a higher fixed address, invalidating an earlier relaxation. Any
// change in section sizes can have cascading effect and require another
// relaxation pass.
bool RISCV::relaxOnce(int pass) const {
  llvm::TimeTraceScope timeScope("RISC-V relaxOnce");
  if (pass == 0)
    initSymbolAnchors(ctx);

  SmallVector<InputSection *, 0> storage;
  bool changed = false;
  for (OutputSection *osec : ctx.outputSections) {
    if (!(osec->flags & SHF_EXECINSTR))
      continue;
    for (InputSection *sec : getInputSections(*osec, storage))
      if (sec->relaxAux)
        changed |= relax(ctx, pass, *sec);
  }

  if (!changed && ctx.arg.isCheriot && !ctx.shouldRelaxAuicgpToCapTable) {
    // Once relaxation has iterated to a fixed point, toggle the option
    // to turn out-of-range CGP-relative accesses into captable (GOT)
    // loads. Then iterate to a fixed point again.
    ctx.shouldRelaxAuicgpToCapTable = true;
    changed = true;
  }

  return changed;
}

// If the section alignment is >= 4, advance `dot` to insert NOPs and synthesize
// an ALIGN relocation. Otherwise, return false to use default handling.
template <class ELFT, class RelTy>
bool RISCV::synthesizeAlignForInput(uint64_t &dot, InputSection *sec,
                                    Relocs<RelTy> rels) {
  if (!baseSec) {
    // Record the first input section with RELAX relocations. We will synthesize
    // ALIGN relocations here.
    for (auto rel : rels) {
      if (rel.getType(false) == R_RISCV_RELAX) {
        baseSec = sec;
        break;
      }
    }
  } else if (sec->addralign >= 4) {
    // If the alignment is > 4, synthesize an ALIGN unless an ALIGN relocation
    // at offset 0 already guarantees `addralign`. Note: A weaker ALIGN at
    // offset 0 from older assemblers do not suppress synthesis (e.g.
    // `.p2align 2; .option norelax; nop; .p2align 3` does not suppress
    // synthesized `.p2align 3`).
    bool covered = llvm::any_of(rels, [&](const RelTy &rel) {
      if (rel.r_offset != 0 || rel.getType(false) != R_RISCV_ALIGN)
        return false;
      if constexpr (RelTy::HasAddend)
        return uint64_t(rel.r_addend) >= sec->addralign - 2;
      return false;
    });
    if (!covered) {
      synthesizedAligns.emplace_back(dot - baseSec->getVA(),
                                     sec->addralign - 2);
      dot += sec->addralign - 2;
      return true;
    }
  }
  return false;
}

// Finalize the relocation section by appending synthesized ALIGN relocations
// after processing all input sections.
template <class ELFT, class RelTy>
void RISCV::finalizeSynthesizeAligns(uint64_t &dot, InputSection *sec,
                                     Relocs<RelTy> rels) {
  auto *f = cast<ObjFile<ELFT>>(baseSec->file);
  auto shdr = f->template getELFShdrs<ELFT>()[baseSec->relSecIdx];
  // Create a copy of InputSection.
  sec = make<InputSection>(*f, shdr, baseSec->name);
  auto *baseRelSec = cast<InputSection>(f->getSections()[baseSec->relSecIdx]);
  *sec = *baseRelSec;
  baseSec = nullptr;

  // Allocate buffer for original and synthesized relocations in RELA format.
  // If CREL is used, OutputSection::finalizeNonAllocCrel will convert RELA to
  // CREL.
  auto newSize = rels.size() + synthesizedAligns.size();
  auto *relas = makeThreadLocalN<typename ELFT::Rela>(newSize);
  sec->size = newSize * sizeof(typename ELFT::Rela);
  sec->content_ = reinterpret_cast<uint8_t *>(relas);
  sec->type = SHT_RELA;
  // Copy original relocations to the new buffer, potentially converting CREL to
  // RELA.
  for (auto [i, r] : llvm::enumerate(rels)) {
    relas[i].r_offset = r.r_offset;
    relas[i].setSymbolAndType(r.getSymbol(0), r.getType(0), false);
    if constexpr (RelTy::HasAddend)
      relas[i].r_addend = r.r_addend;
  }
  // Append synthesized ALIGN relocations to the buffer.
  for (auto [i, r] : llvm::enumerate(synthesizedAligns)) {
    auto &rela = relas[rels.size() + i];
    rela.r_offset = r.first;
    rela.setSymbolAndType(0, R_RISCV_ALIGN, false);
    rela.r_addend = r.second;
  }
  synthesizedAligns.clear();
  // Replace the old relocation section with the new one in the output section.
  // addOrphanSections ensures that the output relocation section is processed
  // after osec.
  for (SectionCommand *cmd : sec->getParent()->commands) {
    auto *isd = dyn_cast<InputSectionDescription>(cmd);
    if (!isd)
      continue;
    for (auto *&isec : isd->sections)
      if (isec == baseRelSec)
        isec = sec;
  }
}

template <class ELFT>
bool RISCV::synthesizeAlignAux(uint64_t &dot, InputSection *sec) {
  bool ret = false;
  if (sec) {
    invokeOnRelocs(*sec, ret = synthesizeAlignForInput<ELFT>, dot, sec);
  } else if (baseSec) {
    invokeOnRelocs(*baseSec, finalizeSynthesizeAligns<ELFT>, dot, sec);
  }
  return ret;
}

// Without linker relaxation enabled for a particular relocatable file or
// section, the assembler will not generate R_RISCV_ALIGN relocations for
// alignment directives. This becomes problematic in a two-stage linking
// process: ld -r a.o b.o -o ab.o; ld ab.o -o ab. This function synthesizes an
// R_RISCV_ALIGN relocation at section start when needed.
//
// When called with an input section (`sec` is not null): If the section
// alignment is >= 4, advance `dot` to insert NOPs and synthesize an ALIGN
// relocation.
//
// When called after all input sections are processed (`sec` is null): The
// output relocation section is updated with all the newly synthesized ALIGN
// relocations.
bool RISCV::synthesizeAlign(uint64_t &dot, InputSection *sec) {
  assert(ctx.arg.relocatable);
  if (ctx.arg.is64)
    return synthesizeAlignAux<ELF64LE>(dot, sec);
  return synthesizeAlignAux<ELF32LE>(dot, sec);
}

void RISCV::finalizeRelax(int passes) const {
  llvm::TimeTraceScope timeScope("Finalize RISC-V relaxation");
  Log(ctx) << "relaxation passes: " << passes;
  SmallVector<InputSection *, 0> storage;
  for (OutputSection *osec : ctx.outputSections) {
    if (!(osec->flags & SHF_EXECINSTR))
      continue;
    for (InputSection *sec : getInputSections(*osec, storage)) {
      if (!sec->relaxAux)
        continue;
      RelaxAux &aux = *sec->relaxAux;
      if (!aux.relocDeltas)
        continue;

      MutableArrayRef<Relocation> rels = sec->relocs();
      ArrayRef<uint8_t> old = sec->content();
      size_t newSize = old.size() - aux.relocDeltas[rels.size() - 1];
      size_t writesIdx = 0;
      uint8_t *p = ctx.bAlloc.Allocate<uint8_t>(newSize);
      uint64_t offset = 0;
      int64_t delta = 0;
      sec->content_ = p;
      sec->size = newSize;
      sec->bytesDropped = 0;

      // Update section content: remove NOPs for R_RISCV_ALIGN and rewrite
      // instructions for relaxed relocations.
      for (size_t i = 0, e = rels.size(); i != e; ++i) {
        uint32_t remove = aux.relocDeltas[i] - delta;
        delta = aux.relocDeltas[i];
        if (remove == 0 && aux.relocTypes[i] == R_RISCV_NONE)
          continue;

        // Copy from last location to the current relocated location.
        Relocation &r = rels[i];
        uint64_t size = r.offset - offset;
        memcpy(p, old.data() + offset, size);
        p += size;

        // For R_RISCV_ALIGN, we will place `offset` in a location (among NOPs)
        // to satisfy the alignment requirement. If both `remove` and r.addend
        // are multiples of 4, it is as if we have skipped some NOPs. Otherwise
        // we are in the middle of a 4-byte NOP, and we need to rewrite the NOP
        // sequence.
        int64_t skip = 0;
        if (r.type == R_RISCV_ALIGN) {
          if (remove % 4 || r.addend % 4) {
            skip = r.addend - remove;
            int64_t j = 0;
            for (; j + 4 <= skip; j += 4)
              write32le(p + j, 0x00000013); // nop
            if (j != skip) {
              assert(j + 2 == skip);
              write16le(p + j, 0x0001); // c.nop
            }
          }
        } else if (RelType newType = aux.relocTypes[i]) {
          switch (newType) {
          case INTERNAL_R_RISCV_GPREL_I:
          case INTERNAL_R_RISCV_GPREL_S:
          case INTERNAL_R_RISCV_X0REL_I:
          case INTERNAL_R_RISCV_X0REL_S:
            break;
          case R_RISCV_RELAX:
            // Used by relaxTlsLe to indicate the relocation is ignored.
            break;
          case R_RISCV_RVC_JUMP:
            skip = 2;
            write16le(p, aux.writes[writesIdx++]);
            break;
          case R_RISCV_JAL:
            skip = 4;
            write32le(p, aux.writes[writesIdx++]);
            break;
          case R_RISCV_32:
            // Used by relaxTlsLe to write a uint32_t then suppress the handling
            // in relocateAlloc.
            skip = 4;
            write32le(p, aux.writes[writesIdx++]);
            aux.relocTypes[i] = R_RISCV_NONE;
            if (r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I ||
                r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S) {
              aux.relocTypes[i] = R_RISCV_RELAX;
            }
            break;
          case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_I:
          case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_LO_S:
            skip = 4;
            write32le(p, aux.writes[writesIdx++]);
            break;
          case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI:
            if (r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI) {
              // In this case, we are mutating a CGP-relative access into a
              // cap-table access.
              skip = 4;
              write32le(p, aux.writes[writesIdx++]);

              for (auto *cmd : sec->getOutputSection()->commands) {
                auto *isd = dyn_cast<InputSectionDescription>(cmd);
                if (!isd)
                  continue;
                if (std::find(isd->sections.begin(), isd->sections.end(),
                              sec) == isd->sections.end())
                  continue;

                // Create a GOT at the end of our OutputSection. The safety of
                // this depends on the fact that the CHERIoT compartment linkage
                // model does not permit any relative relocations across
                // compartment boundaries. Otherwise adding this section could
                // potentially invalidate earlier relocations.
                auto *got = dyn_cast<GotSection>(isd->sections.back());
                if (!got) {
                  got = make<GotSection>(ctx);
                  ctx.inputSections.push_back(got);
                  isd->sections.push_back(got);
                  sec->getOutputSection()->commitSection(got);
                }

                uint64_t gotOff = got->addEntry(*r.sym);
                // FIXME: This is sketchy, but I don't know another solution.
                got->finalizeContents();

                // Construct the GOT entry for the symbol we need to access.
                // FIXME: Deduplicate?
                ctx.in.capRelocs->addReloc(*got, gotOff, *r.sym, 0,
                                                  R_ABS_CAP,
                                                  *ctx.target->symbolicCapRel);
                ctx.in.capRelocs->finalizeContents();

                // Construct an anonymous symbol pointing to the GOT
                r.sym = addSyntheticLocal(ctx, "", STT_NOTYPE, 0, 0, *got);
                r.sym->setFlags(USED);
                r.addend = gotOff;
                r.type = INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_HI;
                r.expr = R_PC;

                break;
              }
            }
            break;
          case INTERNAL_RISCV_CHERIOT1_COMPARTMENT_PCCREL_LO_I: {
            skip = 4;
            write32le(p, aux.writes[writesIdx++]);

            assert(r.type == INTERNAL_RISCV_CHERIOT1_COMPARTMENT_DATA_HI_NOP);
            r.sym = nullptr;
            // Because we are constructions a compartment_lo relocation where
            // there wasn't one before, we need to insert an anonymous label for
            // it to point to as the corresponding compartment_data_hi. Because we
            // know that this NOP reloc was inserted following a compartment_data_hi
            // one, we can just subtract for to get the right offset.
            r.sym = addSyntheticLocal(ctx, "", STT_NOTYPE, r.offset - 4 - delta,
                                      0, *sec);
            assert(r.sym);
            r.addend = 0;
            r.expr = RE_RISCV_PC_INDIRECT;
            break;
          }
          default:
            llvm_unreachable("unsupported type");
          }
        }

        p += skip;
        offset = r.offset + skip + remove;
      }
      memcpy(p, old.data() + offset, old.size() - offset);

      // Subtract the previous relocDeltas value from the relocation offset.
      // For a pair of R_RISCV_CALL/R_RISCV_RELAX with the same offset, decrease
      // their r_offset by the same delta.
      delta = 0;
      for (size_t i = 0, e = rels.size(); i != e;) {
        uint64_t cur = rels[i].offset;
        do {
          rels[i].offset -= delta;
          if (aux.relocTypes[i] != R_RISCV_NONE)
            rels[i].type = aux.relocTypes[i];
        } while (++i != e && rels[i].offset == cur);
        delta = aux.relocDeltas[i - 1];
      }
    }
  }
}

namespace {
// Representation of the merged .riscv.attributes input sections. The psABI
// specifies merge policy for attributes. E.g. if we link an object without an
// extension with an object with the extension, the output Tag_RISCV_arch shall
// contain the extension. Some tools like objdump parse .riscv.attributes and
// disabling some instructions if the first Tag_RISCV_arch does not contain an
// extension.
class RISCVAttributesSection final : public SyntheticSection {
public:
  RISCVAttributesSection(Ctx &ctx)
      : SyntheticSection(ctx, ".riscv.attributes", SHT_RISCV_ATTRIBUTES, 0, 1) {
  }

  size_t getSize() const override { return size; }
  void writeTo(uint8_t *buf) override;

  static constexpr StringRef vendor = "riscv";
  DenseMap<unsigned, unsigned> intAttr;
  DenseMap<unsigned, StringRef> strAttr;
  size_t size = 0;
};
} // namespace

static void mergeArch(Ctx &ctx, RISCVISAUtils::OrderedExtensionMap &mergedExts,
                      unsigned &mergedXlen, const InputSectionBase *sec,
                      StringRef s) {
  auto maybeInfo = RISCVISAInfo::parseNormalizedArchString(s);
  if (!maybeInfo) {
    Err(ctx) << sec << ": " << s << ": " << maybeInfo.takeError();
    return;
  }

  // Merge extensions.
  RISCVISAInfo &info = **maybeInfo;
  if (mergedExts.empty()) {
    mergedExts = info.getExtensions();
    mergedXlen = info.getXLen();
  } else {
    for (const auto &ext : info.getExtensions()) {
      auto p = mergedExts.insert(ext);
      if (!p.second) {
        if (std::tie(p.first->second.Major, p.first->second.Minor) <
            std::tie(ext.second.Major, ext.second.Minor))
          p.first->second = ext.second;
      }
    }
  }
}

static void mergeAtomic(Ctx &ctx, DenseMap<unsigned, unsigned>::iterator it,
                        const InputSectionBase *oldSection,
                        const InputSectionBase *newSection,
                        RISCVAttrs::RISCVAtomicAbiTag oldTag,
                        RISCVAttrs::RISCVAtomicAbiTag newTag) {
  using RISCVAttrs::RISCVAtomicAbiTag;
  // Same tags stay the same, and UNKNOWN is compatible with anything
  if (oldTag == newTag || newTag == RISCVAtomicAbiTag::UNKNOWN)
    return;

  auto reportAbiError = [&]() {
    Err(ctx) << "atomic abi mismatch for " << oldSection->name << "\n>>> "
             << oldSection << ": atomic_abi=" << static_cast<unsigned>(oldTag)
             << "\n>>> " << newSection
             << ": atomic_abi=" << static_cast<unsigned>(newTag);
  };

  auto reportUnknownAbiError = [&](const InputSectionBase *section,
                                   RISCVAtomicAbiTag tag) {
    switch (tag) {
    case RISCVAtomicAbiTag::UNKNOWN:
    case RISCVAtomicAbiTag::A6C:
    case RISCVAtomicAbiTag::A6S:
    case RISCVAtomicAbiTag::A7:
      return;
    };
    Err(ctx) << "unknown atomic abi for " << section->name << "\n>>> "
             << section << ": atomic_abi=" << static_cast<unsigned>(tag);
  };
  switch (oldTag) {
  case RISCVAtomicAbiTag::UNKNOWN:
    it->getSecond() = static_cast<unsigned>(newTag);
    return;
  case RISCVAtomicAbiTag::A6C:
    switch (newTag) {
    case RISCVAtomicAbiTag::A6S:
      it->getSecond() = static_cast<unsigned>(RISCVAtomicAbiTag::A6C);
      return;
    case RISCVAtomicAbiTag::A7:
      reportAbiError();
      return;
    case RISCVAttrs::RISCVAtomicAbiTag::UNKNOWN:
    case RISCVAttrs::RISCVAtomicAbiTag::A6C:
      return;
    };
    break;

  case RISCVAtomicAbiTag::A6S:
    switch (newTag) {
    case RISCVAtomicAbiTag::A6C:
      it->getSecond() = static_cast<unsigned>(RISCVAtomicAbiTag::A6C);
      return;
    case RISCVAtomicAbiTag::A7:
      it->getSecond() = static_cast<unsigned>(RISCVAtomicAbiTag::A7);
      return;
    case RISCVAttrs::RISCVAtomicAbiTag::UNKNOWN:
    case RISCVAttrs::RISCVAtomicAbiTag::A6S:
      return;
    };
    break;

  case RISCVAtomicAbiTag::A7:
    switch (newTag) {
    case RISCVAtomicAbiTag::A6S:
      it->getSecond() = static_cast<unsigned>(RISCVAtomicAbiTag::A7);
      return;
    case RISCVAtomicAbiTag::A6C:
      reportAbiError();
      return;
    case RISCVAttrs::RISCVAtomicAbiTag::UNKNOWN:
    case RISCVAttrs::RISCVAtomicAbiTag::A7:
      return;
    };
    break;
  };

  // If we get here, then we have an invalid tag, so report it.
  // Putting these checks at the end allows us to only do these checks when we
  // need to, since this is expected to be a rare occurrence.
  reportUnknownAbiError(oldSection, oldTag);
  reportUnknownAbiError(newSection, newTag);
}

static RISCVAttributesSection *
mergeAttributesSection(Ctx &ctx,
                       const SmallVector<InputSectionBase *, 0> &sections) {
  using RISCVAttrs::RISCVAtomicAbiTag;
  RISCVISAUtils::OrderedExtensionMap exts;
  const InputSectionBase *firstStackAlign = nullptr;
  const InputSectionBase *firstAtomicAbi = nullptr;
  unsigned firstStackAlignValue = 0, xlen = 0;
  bool hasArch = false;

  ctx.in.riscvAttributes = std::make_unique<RISCVAttributesSection>(ctx);
  auto &merged = static_cast<RISCVAttributesSection &>(*ctx.in.riscvAttributes);

  // Collect all tags values from attributes section.
  const auto &attributesTags = RISCVAttrs::getRISCVAttributeTags();
  for (const InputSectionBase *sec : sections) {
    RISCVAttributeParser parser;
    if (Error e = parser.parse(sec->content(), llvm::endianness::little))
      Warn(ctx) << sec << ": " << std::move(e);
    for (const auto &tag : attributesTags) {
      switch (RISCVAttrs::AttrType(tag.attr)) {
        // Integer attributes.
      case RISCVAttrs::STACK_ALIGN:
        if (auto i = parser.getAttributeValue(tag.attr)) {
          auto r = merged.intAttr.try_emplace(tag.attr, *i);
          if (r.second) {
            firstStackAlign = sec;
            firstStackAlignValue = *i;
          } else if (r.first->second != *i) {
            Err(ctx) << sec << " has stack_align=" << *i << " but "
                     << firstStackAlign
                     << " has stack_align=" << firstStackAlignValue;
          }
        }
        continue;
      case RISCVAttrs::UNALIGNED_ACCESS:
        if (auto i = parser.getAttributeValue(tag.attr))
          merged.intAttr[tag.attr] |= *i;
        continue;

        // String attributes.
      case RISCVAttrs::ARCH:
        if (auto s = parser.getAttributeString(tag.attr)) {
          hasArch = true;
          mergeArch(ctx, exts, xlen, sec, *s);
        }
        continue;

        // Attributes which use the default handling.
      case RISCVAttrs::PRIV_SPEC:
      case RISCVAttrs::PRIV_SPEC_MINOR:
      case RISCVAttrs::PRIV_SPEC_REVISION:
        break;

      case RISCVAttrs::AttrType::ATOMIC_ABI:
        if (auto i = parser.getAttributeValue(tag.attr)) {
          auto r = merged.intAttr.try_emplace(tag.attr, *i);
          if (r.second)
            firstAtomicAbi = sec;
          else
            mergeAtomic(ctx, r.first, firstAtomicAbi, sec,
                        static_cast<RISCVAtomicAbiTag>(r.first->getSecond()),
                        static_cast<RISCVAtomicAbiTag>(*i));
        }
        continue;
      }

      // Fallback for deprecated priv_spec* and other unknown attributes: retain
      // the attribute if all input sections agree on the value. GNU ld uses 0
      // and empty strings as default values which are not dumped to the output.
      // TODO Adjust after resolution to
      // https://github.com/riscv-non-isa/riscv-elf-psabi-doc/issues/352
      if (tag.attr % 2 == 0) {
        if (auto i = parser.getAttributeValue(tag.attr)) {
          auto r = merged.intAttr.try_emplace(tag.attr, *i);
          if (!r.second && r.first->second != *i)
            r.first->second = 0;
        }
      } else if (auto s = parser.getAttributeString(tag.attr)) {
        auto r = merged.strAttr.try_emplace(tag.attr, *s);
        if (!r.second && r.first->second != *s)
          r.first->second = {};
      }
    }
  }

  if (hasArch && xlen != 0) {
    if (auto result = RISCVISAInfo::createFromExtMap(xlen, exts)) {
      merged.strAttr.try_emplace(RISCVAttrs::ARCH,
                                 ctx.saver.save((*result)->toString()));
    } else {
      Err(ctx) << result.takeError();
    }
  }

  // The total size of headers: format-version [ <section-length> "vendor-name"
  // [ <file-tag> <size>.
  size_t size = 5 + merged.vendor.size() + 1 + 5;
  for (auto &attr : merged.intAttr)
    if (attr.second != 0)
      size += getULEB128Size(attr.first) + getULEB128Size(attr.second);
  for (auto &attr : merged.strAttr)
    if (!attr.second.empty())
      size += getULEB128Size(attr.first) + attr.second.size() + 1;
  merged.size = size;
  return &merged;
}

void RISCVAttributesSection::writeTo(uint8_t *buf) {
  const size_t size = getSize();
  uint8_t *const end = buf + size;
  *buf = ELFAttrs::Format_Version;
  write32(ctx, buf + 1, size - 1);
  buf += 5;

  memcpy(buf, vendor.data(), vendor.size());
  buf += vendor.size() + 1;

  *buf = ELFAttrs::File;
  write32(ctx, buf + 1, end - buf);
  buf += 5;

  for (auto &attr : intAttr) {
    if (attr.second == 0)
      continue;
    buf += encodeULEB128(attr.first, buf);
    buf += encodeULEB128(attr.second, buf);
  }
  for (auto &attr : strAttr) {
    if (attr.second.empty())
      continue;
    buf += encodeULEB128(attr.first, buf);
    memcpy(buf, attr.second.data(), attr.second.size());
    buf += attr.second.size() + 1;
  }
}

void elf::mergeRISCVAttributesSections(Ctx &ctx) {
  // Find the first input SHT_RISCV_ATTRIBUTES; return if not found.
  size_t place =
      llvm::find_if(ctx.inputSections,
                    [](auto *s) { return s->type == SHT_RISCV_ATTRIBUTES; }) -
      ctx.inputSections.begin();
  if (place == ctx.inputSections.size())
    return;

  // Extract all SHT_RISCV_ATTRIBUTES sections into `sections`.
  SmallVector<InputSectionBase *, 0> sections;
  llvm::erase_if(ctx.inputSections, [&](InputSectionBase *s) {
    if (s->type != SHT_RISCV_ATTRIBUTES)
      return false;
    sections.push_back(s);
    return true;
  });

  // Add the merged section.
  ctx.inputSections.insert(ctx.inputSections.begin() + place,
                           mergeAttributesSection(ctx, sections));
}

void elf::setRISCVTargetInfo(Ctx &ctx) { ctx.target.reset(new RISCV(ctx)); }

static std::optional<StringRef> getRISCVVendorString(RelType ty) {
  if ((ty.v & INTERNAL_RISCV_VENDOR_MASK) == INTERNAL_RISCV_VENDOR_QUALCOMM)
    return "QUALCOMM";
  if ((ty.v & INTERNAL_RISCV_VENDOR_MASK) == INTERNAL_RISCV_VENDOR_ANDES)
    return "ANDES";
  if ((ty.v & INTERNAL_RISCV_VENDOR_MASK) == INTERNAL_RISCV_VENDOR_CHERIOT1)
    return "CHERIOT1";
  return std::nullopt;
}

namespace lld::elf {

std::string riscvVendorRelocToStr(RelType type) {
  auto VendorString = getRISCVVendorString(type);
  if (!VendorString)
    return "Unknown";

  StringRef str = getRISCVVendorRelocationTypeName(
      type & ~INTERNAL_RISCV_VENDOR_MASK, *VendorString);
  if (str == "Unknown")
    return ("Unknown vendor-specific (" + Twine(type) + ")").str();

  return str.str();
}
}
