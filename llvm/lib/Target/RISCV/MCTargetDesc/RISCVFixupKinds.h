//===-- RISCVFixupKinds.h - RISC-V Specific Fixup Entries -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_RISCV_MCTARGETDESC_RISCVFIXUPKINDS_H
#define LLVM_LIB_TARGET_RISCV_MCTARGETDESC_RISCVFIXUPKINDS_H

#include "llvm/BinaryFormat/ELF.h"
#include "llvm/MC/MCFixup.h"
#include <utility>

#undef RISCV

namespace llvm::RISCV {
enum Fixups {
  // 20-bit fixup corresponding to %hi(foo) for instructions like lui
  fixup_riscv_hi20 = FirstTargetFixupKind,
  // 12-bit fixup corresponding to %lo(foo) for instructions like addi
  fixup_riscv_lo12_i,
  // 12-bit fixup corresponding to foo-bar for instructions like addi
  fixup_riscv_12_i,
  // 12-bit fixup corresponding to %lo(foo) for the S-type store instructions
  fixup_riscv_lo12_s,
  // 20-bit fixup corresponding to %pcrel_hi(foo) for instructions like auipc
  fixup_riscv_pcrel_hi20,
  // 12-bit fixup corresponding to %pcrel_lo(foo) for instructions like addi
  fixup_riscv_pcrel_lo12_i,
  // 12-bit fixup corresponding to %pcrel_lo(foo) for the S-type store
  // instructions
  fixup_riscv_pcrel_lo12_s,
  // 20-bit fixup for symbol references in the jal instruction
  fixup_riscv_jal,
  // 12-bit fixup for symbol references in the branch instructions
  fixup_riscv_branch,
  // 11-bit fixup for symbol references in the compressed jump instruction
  fixup_riscv_rvc_jump,
  // 8-bit fixup for symbol references in the compressed branch instruction
  fixup_riscv_rvc_branch,
  // 6-bit fixup for symbol references in instructions like c.li
  fixup_riscv_rvc_imm,
  // Fixup representing a legacy no-pic function call attached to the auipc
  // instruction in a pair composed of adjacent auipc+jalr instructions.
  fixup_riscv_call,
  // Fixup representing a function call attached to the auipc instruction in a
  // pair composed of adjacent auipc+jalr instructions.
  fixup_riscv_call_plt,

  // Qualcomm specific fixups
  // 12-bit fixup for symbol references in the 48-bit Xqcibi branch immediate
  // instructions
  fixup_riscv_qc_e_branch,
  // 32-bit fixup for symbol references in the 48-bit qc.e.li instruction
  fixup_riscv_qc_e_32,
  // 20-bit fixup for symbol references in the 32-bit qc.li instruction
  fixup_riscv_qc_abs20_u,
  // 32-bit fixup for symbol references in the 48-bit qc.j/qc.jal instructions
  fixup_riscv_qc_e_call_plt,

  // Andes specific fixups
  // 10-bit fixup for symbol references in the xandesperf branch instruction
  fixup_riscv_nds_branch_10,

  // fixup_riscv_captab_pcrel_hi20 - 20-bit fixup corresponding to
  // captab_pcrel_hi(foo) for instructions like auipcc
  fixup_riscv_captab_pcrel_hi20,
  // fixup_riscv_capability - CLen-bit fixup corresponding to .chericap
  fixup_riscv_capability,
  // fixup_riscv_tprel_cincoffset - A fixup corresponding to
  // %tprel_cincoffset(foo) for the cincoffset_tls instruction. Used to provide
  // a hint to the linker.
  fixup_riscv_tprel_cincoffset,
  // fixup_riscv_tls_ie_captab_pcrel_hi20 - 20-bit fixup corresponding to
  // tls_ie_captab_pcrel_hi(foo) for instructions like auipcc
  fixup_riscv_tls_ie_captab_pcrel_hi20,
  // fixup_riscv_tls_gd_captab_pcrel_hi20 - 20-bit fixup corresponding to
  // tls_gd_captab_pcrel_hi(foo) for instructions like auipcc
  fixup_riscv_tls_gd_captab_pcrel_hi20,
  // fixup_riscv_cjal - 20-bit fixup for symbol references in the cjal
  // instruction
  fixup_riscv_cjal,
  // fixup_riscv_call - A fixup representing a ccall attached to the auipcc
  // instruction in a pair composed of adjacent auipcc+cjalr instructions.
  fixup_riscv_ccall,
  // fixup_riscv_rvc_cjump - 11-bit fixup for symbol references in the
  // compressed capability jump instruction
  fixup_riscv_rvc_cjump,

  // $cgp- or $pcc-relative global, used with auicgp / auipcc instructions
  fixup_riscv_cheriot_compartment_hi,
  // $cgp- or $pcc-relative global, used with RV32 I instructions
  fixup_riscv_cheriot_compartment_lo_i,
  // $cgp-relative global, used with RV32 S instructions
  fixup_riscv_cheriot_compartment_lo_s,
  // Size of the symbol.
  fixup_riscv_cheriot_compartment_size,

  // Used as a sentinel, must be the last
  fixup_riscv_invalid,
  NumTargetFixupKinds = fixup_riscv_invalid - FirstTargetFixupKind
};
} // end namespace llvm::RISCV

#endif
