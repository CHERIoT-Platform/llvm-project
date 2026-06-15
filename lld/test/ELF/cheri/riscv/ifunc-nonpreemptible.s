# REQUIRES: riscv

# RUN: %riscv64_cheri_purecap_llvm-mc -filetype=obj %s -o %t.64.o
# RUN: ld.lld %t.64.o -z separate-code -o %t.64
# RUN: llvm-readelf -S -s %t.64 | FileCheck --check-prefixes=SEC,NM %s
# RUN: llvm-readobj -r --cap-relocs -x .got.plt %t.64 | FileCheck --check-prefix=RELOC64 %s
# RUN: llvm-readelf -x .got.plt %t.64 | FileCheck --check-prefix=GOTPLT64 %s
# RUN: llvm-objdump -d --no-show-raw-insn %t.64 | FileCheck --check-prefixes=DIS,DIS64 %s

# SEC: .iplt PROGBITS {{0*}}00011010

## Canonical PLT in .iplt
# NM: {{0*}}00011010 16 FUNC GLOBAL DEFAULT {{.*}} func

# RELOC32:      Relocations [
# RELOC32-NEXT: ]
# RELOC32:      __cap_relocs {
# RELOC32-NEXT:   0x12000 FUNC - 0x11010 [0x10400-0x12200]
# RELOC32-NEXT:   0x12008 IFUNC - 0x11000 [0x10400-0x12200]
# RELOC32-NEXT: }
# GOTPLT32:      section '.got.plt':
# GOTPLT32-NEXT: 0x00012008 00000000 00000000

# RELOC64:      Relocations [
# RELOC64-NEXT: ]
# RELOC64:      __cap_relocs {
# RELOC64-NEXT:   0x12000 FUNC - 0x11010 [0x10190-0x12020]
# RELOC64-NEXT:   0x12010 IFUNC - 0x11000 [0x10190-0x12020]
# RELOC64-NEXT: }
# GOTPLT64:      section '.got.plt':
# GOTPLT64-NEXT: 0x00012010 00000000 00000000 00000000 00000000

# DIS:      <_start>:
## func - . = 0x11010-0x11004 = 12
# DIS-NEXT: 11004: auipcc a0, 0
# DIS-NEXT:        cincoffset a0, a0, 0xc

# DIS:      Disassembly of section .iplt:
# DIS:      <func>:
## 32-bit: &.got.plt[func]-. = 0x12008-0x11010 = 4096*1-8
## 64-bit: &.got.plt[func]-. = 0x12010-0x11010 = 4096*1+0
# DIS-NEXT: 11010: auipcc t3, 0x1
# DIS32-NEXT:      lc t3, -0x8(t3)
# DIS64-NEXT:      lc t3, 0x0(t3)
# DIS-NEXT:        jalr t1, t3
# DIS-NEXT:        nop

.text
.globl func
.type func, @gnu_indirect_function
func:
  cret
  .size func, . - func

.globl _start
.type _start, @function
_start:
  cllc ca0, func
  .size _start, . - _start

.data
.globl fptr
.type fptr, @object
fptr:
  .chericap func
  .size fptr, . - fptr
