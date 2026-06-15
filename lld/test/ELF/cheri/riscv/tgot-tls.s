# REQUIRES: riscv
# RUN: echo '.tbss; .globl evar; evar: .zero 4' > %t.s

# RUN: %riscv64_cheri_purecap_llvm-mc -cheri-tgot-tls -filetype=obj %t.s -o %t1.64.o
# RUN: ld.lld -shared -soname=t1.so %t1.64.o -o %t1.64.so

# RUN: %riscv64_cheri_purecap_llvm-mc --defsym PIC=0 -cheri-tgot-tls -filetype=obj %s -o %t.64.o
# RUN: ld.lld %t.64.o %t1.64.so -o %t.64
# RUN: llvm-readobj -r --cap-relocs %t.64 | FileCheck --check-prefix=RV64-REL %s
# RUN: llvm-readelf -x .got -x .tgot %t.64 | FileCheck --check-prefix=RV64-GOT %s
# RUN: llvm-objdump -d --no-show-raw-insn %t.64 | FileCheck --check-prefix=RV64-DIS %s

# RUN: %riscv64_cheri_purecap_llvm-mc --defsym PIC=1 -cheri-tgot-tls -filetype=obj %s -o %t.64.pico
# RUN: ld.lld -shared %t.64.pico %t1.64.so -o %t.64.so
# RUN: llvm-readobj -r --cap-relocs %t.64.so | FileCheck --check-prefix=RV64-SO-REL %s
# RUN: llvm-readelf -x .got -x .tgot %t.64.so | FileCheck --check-prefix=RV64-SO-GOT %s
# RUN: llvm-objdump -d --no-show-raw-insn %t.64.so | FileCheck --check-prefix=RV64-SO-DIS %s

# RV32-REL:      .rela.tgot {
# RV32-REL-NEXT:   0x13800 R_RISCV_CHERI_TLS_TGOT_SLOT evar 0x0
# RV32-REL-NEXT: }
# RV32-REL:      __tgot_cap_relocs {
# RV32-REL-NEXT:   0x13808 DATA - 0x4 [0x4-0x8]
# RV32-REL-NEXT: }

# RV32-SO-REL:      .rela.dyn {
# RV32-SO-REL-NEXT:   0x2578 R_RISCV_CHERI_TLS_TGOTREL - 0x0
# RV32-SO-REL-NEXT:   0x2580 R_RISCV_TLS_DTPMOD32 - 0x0
# RV32-SO-REL-NEXT:   0x2588 R_RISCV_CHERI_TLS_TGOTREL - 0x8
# RV32-SO-REL-NEXT:   0x2590 R_RISCV_TLS_DTPMOD32 - 0x0
# RV32-SO-REL-NEXT: }
# RV32-SO-REL:      .rela.tgot {
# RV32-SO-REL-NEXT:   0x3800 R_RISCV_CHERI_TLS_TGOT_SLOT evar 0x0
# RV32-SO-REL-NEXT: }
# RV32-SO-REL:      __tgot_cap_relocs {
# RV32-SO-REL-NEXT:   0x3808 DATA - 0x4 [0x4-0x8]
# RV32-SO-REL-NEXT: }

# RV32-GOT: section '.got':
# RV32-GOT-NEXT: 0x00012518 a8240100 00000000 00000000 00000000
# RV32-GOT-NEXT: 0x00012528 01000000 00000000 08000000 00000000
# RV32-GOT-NEXT: 0x00012538 01000000 08000000
# RV32-GOT: section '.tgot':
# RV32-GOT-NEXT: 0x00013800 00000000 00000000 00000000 00000000

# RV32-SO-GOT: section '.got':
# RV32-SO-GOT-NEXT: 0x00002570 f0240000 00000000 00000000 00000000
# RV32-SO-GOT-NEXT: 0x00002580 00000000 00000000 00000000 00000000
# RV32-SO-GOT-NEXT: 0x00002590 00000000 08000000
# RV32-SO-GOT: section '.tgot':
# RV32-SO-GOT-NEXT: 0x00003800 00000000 00000000 00000000 00000000

# 0x124c0 - 0x11414 = 0x10ac (GD evar)
# RV32-DIS:      1147c: auipcc a0, 0x1
# RV32-DIS-NEXT:        cincoffset a0, a0, 0xac

# 0x124b8 - 0x1141c = 0x109c (IE evar)
# RV32-DIS:      11484: auipcc a0, 0x1
# RV32-DIS-NEXT:        lw a0, 0x9c(a0)

# 0x124d0 - 0x11424 = 0x10ac (GD lvar)
# RV32-DIS:      1148c: auipcc a0, 0x1
# RV32-DIS-NEXT:        cincoffset a0, a0, 0xac

# 0x124c8 - 0x1142c = 0x109c (IE lvar)
# RV32-DIS:      11494: auipcc a0, 0x1
# RV32-DIS-NEXT:        lw a0, 0x9c(a0)

# RV32-DIS:      1149c: lui a0, 0x0
# RV32-DIS-NEXT:        cincoffset a0, tp, a0
# RV32-DIS-NEXT:        lc a0, 0x8(a0)

# 0x24d0 - 0x1414 = 0x10bc (GD evar)
# RV32-SO-DIS:      14d0: auipcc a0, 0x1
# RV32-SO-DIS-NEXT:       cincoffset a0, a0, 0xb0

# 0x24c8 - 0x141c = 0x10ac (IE evar)
# RV32-SO-DIS:      14d8: auipcc a0, 0x1
# RV32-SO-DIS-NEXT:       lw a0, 0xa0(a0)

# 0x24e0 - 0x1424 = 0x10bc (GD lvar)
# RV32-SO-DIS:      14e0: auipcc a0, 0x1
# RV32-SO-DIS-NEXT:       cincoffset a0, a0, 0xb0

# 0x24d8 - 0x142c = 0x10ac (IE lvar)
# RV32-SO-DIS:      14e8: auipcc a0, 0x1
# RV32-SO-DIS-NEXT:       lw a0, 0xa0(a0)

# RV64-REL:      .rela.tgot {
# RV64-REL-NEXT:   0x134C0 R_RISCV_CHERI_TLS_TGOT_SLOT evar 0x0
# RV64-REL-NEXT: }
# RV64-REL:      __tgot_cap_relocs {
# RV64-REL-NEXT:   0x134D0 DATA - 0x4 [0x4-0x8]
# RV64-REL-NEXT: }

# RV64-SO-REL:      .rela.dyn {
# RV64-SO-REL-NEXT:   0x2520 R_RISCV_CHERI_TLS_TGOTREL - 0x0
# RV64-SO-REL-NEXT:   0x2530 R_RISCV_TLS_DTPMOD64 - 0x0
# RV64-SO-REL-NEXT:   0x2540 R_RISCV_CHERI_TLS_TGOTREL - 0x10
# RV64-SO-REL-NEXT:   0x2550 R_RISCV_TLS_DTPMOD64 - 0x0
# RV64-SO-REL-NEXT: }
# RV64-SO-REL:      .rela.tgot {
# RV64-SO-REL-NEXT:   0x3560 R_RISCV_CHERI_TLS_TGOT_SLOT evar 0x0
# RV64-SO-REL-NEXT: }
# RV64-SO-REL:      __tgot_cap_relocs {
# RV64-SO-REL-NEXT:   0x3570 DATA - 0x4 [0x4-0x8]
# RV64-SO-REL-NEXT: }

# RV64-GOT: section '.got':
# RV64-GOT-NEXT: 0x00012470 90230100 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x00012480 00000000 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x00012490 01000000 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x000124a0 10000000 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x000124b0 01000000 00000000 10000000 00000000
# RV64-GOT: section '.tgot':
# RV64-GOT-NEXT: 0x000134c0 00000000 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x000134d0 00000000 00000000 00000000 00000000

# RV64-SO-GOT: section '.got':
# RV64-SO-GOT-NEXT: 0x00002510 10240000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00002520 00000000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00002530 00000000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00002540 00000000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00002550 00000000 00000000 10000000 00000000
# RV64-SO-GOT: section '.tgot':
# RV64-SO-GOT-NEXT: 0x00003560 00000000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00003570 00000000 00000000 00000000 00000000

# 0x12490 - 0x11358 = 0x1138 (GD evar)
# RV64-DIS:      11360: auipcc a0, 0x1
# RV64-DIS-NEXT:        cincoffset a0, a0, 0x130

# 0x12480 - 0x11360 = 0x1120 (IE evar)
# RV64-DIS:      11368: auipcc a0, 0x1
# RV64-DIS-NEXT:        ld a0, 0x118(a0)

# 0x124b0 - 0x11368 = 0x1148 (GD lvar)
# RV64-DIS:      11370: auipcc a0, 0x1
# RV64-DIS-NEXT:        cincoffset a0, a0, 0x140

# 0x124a0 - 0x11370 = 0x1130 (IE lvar)
# RV64-DIS:      11378: auipcc a0, 0x1
# RV64-DIS-NEXT:        ld a0, 0x128(a0)

# RV64-DIS:      11380: lui a0, 0x0
# RV64-DIS-NEXT:        cincoffset a0, tp, a0
# RV64-DIS-NEXT:        lc a0, 0x10(a0)

# 0x2540 - 0x13e8 = 0x1158 (GD evar)
# RV64-SO-DIS:      13f0: auipcc a0, 0x1
# RV64-SO-DIS-NEXT:       cincoffset a0, a0, 0x140

# 0x2530 - 0x13f0 = 0x1140 (IE evar)
# RV64-SO-DIS:      13f8: auipcc a0, 0x1
# RV64-SO-DIS-NEXT:       ld a0, 0x128(a0)

# 0x2560 - 0x13f8 = 0x1168 (GD lvar)
# RV64-SO-DIS:      1400: auipcc a0, 0x1
# RV64-SO-DIS-NEXT:       cincoffset a0, a0, 0x150

# 0x2550 - 0x1400 = 0x1150 (IE lvar)
# RV64-SO-DIS:      1408: auipcc a0, 0x1
# RV64-SO-DIS-NEXT:       ld a0, 0x138(a0)

.global _start
_start:
	clc.tls.gd a0, evar

	cla.tls.ie a0, evar

	clc.tls.gd a0, lvar

	cla.tls.ie a0, lvar

.if PIC == 0
	lui a0, %tgot_tprel_hi(lvar)
	cincoffset a0, ctp, a0, %tgot_tprel_add(lvar)
	clc a0, %tgot_tprel_lo(lvar)(a0)
.endif

.tbss
	.zero 4
lvar:
	.zero 4
	.size lvar, . - lvar
