# REQUIRES: riscv
# RUN: echo '.globl bar, weak; .type bar,@function; .type weak,@function; bar: weak:' > %t1.s

# RUN: %riscv64_cheri_purecap_llvm-mc -filetype=obj %t1.s -o %t1.64.o
# RUN: ld.lld -shared %t1.64.o -soname=t1.64.so -o %t1.64.so
# RUN: %riscv64_cheri_purecap_llvm-mc -filetype=obj %s -o %t.64.o
# RUN: ld.lld %t.64.o %t1.64.so -z separate-code -o %t.64
# RUN: llvm-readelf -S -s %t.64 | FileCheck --check-prefixes=SEC,NM %s
# RUN: llvm-readobj -r --cap-relocs %t.64 | FileCheck --check-prefix=RELOC64 %s
# RUN: llvm-readelf -x .got.plt %t.64 | FileCheck --check-prefix=GOTPLT64 %s
# RUN: llvm-objdump -d --no-show-raw-insn --print-imm-hex=false %t.64 | FileCheck --check-prefixes=DIS,DIS64 %s

# SEC: .plt PROGBITS {{0*}}00011030

## A canonical PLT has a non-zero st_value. bar and weak are called but their
## addresses are not taken, so a canonical PLT is not necessary.
# NM: {{0*}}00000000 0 FUNC GLOBAL DEFAULT UND bar
# NM: {{0*}}00000000 0 FUNC WEAK   DEFAULT UND weak

# RELOC32:      .rela.plt {
# RELOC32-NEXT:   0x13088 R_RISCV_JUMP_SLOT bar 0x0
# RELOC32-NEXT:   0x13090 R_RISCV_JUMP_SLOT weak 0x0
# RELOC32-NEXT: }
# RELOC32:      __cap_relocs {
# RELOC32-NEXT:   0x13088 CODE - 0x11030 [0x10400-0x13400]
# RELOC32-NEXT:   0x13090 CODE - 0x11030 [0x10400-0x13400]
# RELOC32-NEXT: }
# GOTPLT32:      section '.got.plt'
# GOTPLT32-NEXT: 0x00013078 00000000 00000000 00000000 00000000
# GOTPLT32-NEXT: 0x00013088 00000000 00000000 00000000 00000000

# RELOC64:      .rela.plt {
# RELOC64-NEXT:   0x13110 R_RISCV_JUMP_SLOT bar 0x0
# RELOC64-NEXT:   0x13120 R_RISCV_JUMP_SLOT weak 0x0
# RELOC64-NEXT: }
# RELOC64:      __cap_relocs {
# RELOC64-NEXT:   0x13110 CODE - 0x11030 [0x10240-0x13130]
# RELOC64-NEXT:   0x13120 CODE - 0x11030 [0x10240-0x13130]
# RELOC64-NEXT: }
# GOTPLT64:      section '.got.plt'
# GOTPLT64-NEXT: 0x000130f0 00000000 00000000 00000000 00000000
# GOTPLT64-NEXT: 0x00013100 00000000 00000000 00000000 00000000
# GOTPLT64-NEXT: 0x00013110 00000000 00000000 00000000 00000000
# GOTPLT64-NEXT: 0x00013120 00000000 00000000 00000000 00000000

# DIS:      <_start>:
## Direct call
## foo - . = 0x11020-0x11000 = 32
# DIS-NEXT:   11000: auipcc ra, 0
# DIS-NEXT:          cjalr 32(ra)
## bar@plt - . = 0x11050-0x11008 = 72
# DIS-NEXT:   11008: auipcc ra, 0
# DIS-NEXT:          cjalr 72(ra)
## bar@plt - . = 0x11050-0x11010 = 64
# DIS-NEXT:   11010: auipcc ra, 0
# DIS-NEXT:          cjalr 64(ra)
## weak@plt - . = 0x11060-0x11018 = 72
# DIS-NEXT:   11018: auipcc ra, 0
# DIS-NEXT:          cjalr 72(ra)
# DIS:      <foo>:
# DIS-NEXT:   11020:

# DIS:      Disassembly of section .plt:
# DIS:      <.plt>:
# DIS-NEXT:     auipcc t2, 2
# DIS-NEXT:     sub t1, t1, t3
## 32-bit: .got.plt - .plt = 0x13078 - 0x11030 = 4096*2+72
## 64-bit: .got.plt - .plt = 0x130f0 - 0x11030 = 4096*2+192
# DIS32-NEXT:   lc t3, 72(t2)
# DIS64-NEXT:   lc t3, 192(t2)
# DIS-NEXT:     addi t1, t1, -44
# DIS32-NEXT:   cincoffset t0, t2, 72
# DIS64-NEXT:   cincoffset t0, t2, 192
# DIS32-NEXT:   srli t1, t1, 1
# DIS32-NEXT:   lc t0, 8(t0)
# DIS64-NEXT:   lc t0, 16(t0)
# DIS-NEXT:     jr t3
# DIS64-NEXT:   nop

## 32-bit (.got.plt): &.got.plt[bar]-. = 0x13088-0x11050 = 4096*2+56
## 64-bit (.got.plt): &.got.plt[bar]-. = 0x13110-0x11050 = 4096*2+192
# DIS:        11050: auipcc t3, 2
# DIS32-NEXT:   lc t3, 56(t3)
# DIS64-NEXT:   lc t3, 192(t3)
# DIS-NEXT:     jalr t1, t3
# DIS-NEXT:     nop

## 32-bit (.got.plt): &.got.plt[weak]-. = 0x13090-0x11060 = 4096*2+48
## 64-bit (.got.plt): &.got.plt[weak]-. = 0x13120-0x11060 = 4096*2+192
# DIS:        11060: auipcc t3, 2
# DIS32-NEXT:   lc t3, 48(t3)
# DIS64-NEXT:   lc t3, 192(t3)
# DIS-NEXT:     jalr t1, t3
# DIS-NEXT:     nop

.global _start, foo, bar
.weak weak

.type _start, @function
_start:
  ccall foo
  ccall bar
  ccall bar@plt
  ccall weak
.size _start, . - _start

## foo is local and non-preemptale, no PLT is generated.
.type foo, @function
foo:
  cret
.size foo, . - foo
