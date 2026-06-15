# REQUIRES: riscv
# RUN: echo '.tbss; .globl evar; evar: .zero 4' > %t.s

# RUN: %riscv64_cheri_purecap_llvm-mc -filetype=obj %t.s -o %t1.64.o
# RUN: ld.lld -shared -soname=t1.so %t1.64.o -o %t1.64.so

# RUN: %riscv64_cheri_purecap_llvm-mc --defsym PIC=0 -filetype=obj %s -o %t.64.o
# RUN: ld.lld %t.64.o %t1.64.so -o %t.64
# RUN: llvm-readobj -r %t.64 | FileCheck --check-prefix=RV64-REL %s
# RUN: llvm-readelf -x .got %t.64 | FileCheck --check-prefix=RV64-GOT %s
# RUN: llvm-objdump -d --no-show-raw-insn --print-imm-hex=false %t.64 | FileCheck --check-prefix=RV64-DIS %s

# RUN: %riscv64_cheri_purecap_llvm-mc --defsym PIC=1 -filetype=obj %s -o %t.64.pico
# RUN: ld.lld -shared %t.64.pico %t1.64.so -o %t.64.so
# RUN: llvm-readobj -r %t.64.so | FileCheck --check-prefix=RV64-SO-REL %s
# RUN: llvm-readelf -x .got %t.64.so | FileCheck --check-prefix=RV64-SO-GOT %s
# RUN: llvm-objdump -d --no-show-raw-insn --print-imm-hex=false %t.64.so | FileCheck --check-prefix=RV64-SO-DIS %s

# RV64-REL:      .rela.dyn {
# RV64-REL-NEXT:   0x123F0 R_RISCV_TLS_DTPMOD64 evar 0x0
# RV64-REL-NEXT:   0x123F8 R_RISCV_TLS_DTPREL64 evar 0x0
# RV64-REL-NEXT:   0x12400 R_RISCV_TLS_TPREL64 evar 0x0
# RV64-REL-NEXT: }

# RV64-SO-REL:      .rela.dyn {
# RV64-SO-REL-NEXT:   0x2460 R_RISCV_TLS_DTPMOD64 - 0x0
# RV64-SO-REL-NEXT:   0x2470 R_RISCV_TLS_TPREL64 - 0x4
# RV64-SO-REL-NEXT:   0x2440 R_RISCV_TLS_DTPMOD64 evar 0x0
# RV64-SO-REL-NEXT:   0x2448 R_RISCV_TLS_DTPREL64 evar 0x0
# RV64-SO-REL-NEXT:   0x2450 R_RISCV_TLS_TPREL64 evar 0x0
# RV64-SO-REL-NEXT: }

# RV64-GOT: section '.got':
# RV64-GOT-NEXT: 0x000123e0 20230100 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x000123f0 00000000 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x00012400 00000000 00000000 00000000 00000000
# RV64-GOT-NEXT: 0x00012410 01000000 00000000 04000000 00000000
# RV64-GOT-NEXT: 0x00012420 04000000 00000000 00000000 00000000

# RV64-SO-GOT: section '.got':
# RV64-SO-GOT-NEXT: 0x00002430 70230000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00002440 00000000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00002450 00000000 00000000 00000000 00000000
# RV64-SO-GOT-NEXT: 0x00002460 00000000 00000000 04000000 00000000
# RV64-SO-GOT-NEXT: 0x00002470 00000000 00000000 00000000 00000000

# 0x123f0 - 0x112f0 = 0x1100 (GD evar)
# RV64-DIS:      112f0: auipcc a0, 1
# RV64-DIS-NEXT:        cincoffset a0, a0, 256

# 0x12400 - 0x112f8 = 0x1108 (IE evar)
# RV64-DIS:      112f8: auipcc a0, 1
# RV64-DIS-NEXT:        cld a0, 264(a0)

# 0x12410 - 0x11300 = 0x1110 (GD lvar)
# RV64-DIS:      11300: auipcc a0, 1
# RV64-DIS-NEXT:        cincoffset a0, a0, 272

# 0x12420 - 0x11308 = 0x1118 (IE lvar)
# RV64-DIS:      11308: auipcc a0, 1
# RV64-DIS-NEXT:        cld a0, 280(a0)

# RV64-DIS:      11310: lui a0, 0
# RV64-DIS-NEXT:        cincoffset a0, tp, a0
# RV64-DIS-NEXT:        cincoffset a0, a0, 4

# 0x2440 - 0x1350 = 0x10f0 (GD evar)
# RV64-SO-DIS:      1350: auipcc a0, 1
# RV64-SO-DIS-NEXT:       cincoffset a0, a0, 240

# 0x2450 - 0x1358 = 0x10f8 (IE evar)
# RV64-SO-DIS:      1358: auipcc a0, 1
# RV64-SO-DIS-NEXT:       cld a0, 248(a0)

# 0x2460 - 0x1360 = 0x1100 (GD lvar)
# RV64-SO-DIS:      1360: auipcc a0, 1
# RV64-SO-DIS-NEXT:       cincoffset a0, a0, 256

# 0x2470 - 0x1368 = 0x1108 (IE lvar)
# RV64-SO-DIS:      1368: auipcc a0, 1
# RV64-SO-DIS-NEXT:       cld a0, 264(a0)

.global _start
_start:
	clc.tls.gd a0, evar

	cla.tls.ie a0, evar

	clc.tls.gd a0, lvar

	cla.tls.ie a0, lvar

.if PIC == 0
	lui a0, %tprel_hi(lvar)
	cincoffset a0, tp, a0, %tprel_add(lvar)
	cincoffset a0, a0, %tprel_lo(lvar)
.endif

.tbss
	.zero 4
lvar:
	.zero 4
