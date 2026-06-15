# REQUIRES: riscv

# RUN: echo '.globl b; b:' | %riscv64_cheri_purecap_llvm-mc -filetype=obj - -o %t1.o
# RUN: ld.lld -shared %t1.o -soname=t1.so -o %t1.so

# RUN: %riscv64_cheri_purecap_llvm-mc -filetype=obj -position-independent %s -o %t.o
# RUN: ld.lld %t.o %t1.so -o %t
# RUN: llvm-readelf -S %t | FileCheck --check-prefix=SEC64 %s
# RUN: llvm-readobj -r --cap-relocs %t | FileCheck --check-prefix=RELOC64 %s
# RUN: llvm-nm %t | FileCheck --check-prefix=NM64 %s
# RUN: llvm-readobj -x .got %t | FileCheck --check-prefix=HEX64 %s
# RUN: llvm-objdump -d --no-show-raw-insn %t | FileCheck --check-prefix=DIS64 %s

# SEC32: .got PROGBITS         00012500 000500 000018
# SEC64: .got PROGBITS 00000000000123e0 0003e0 000030

# RELOC32:      .rela.dyn {
# RELOC32-NEXT:   0x12508 R_RISCV_CHERI_CAPABILITY b 0x0
# RELOC32-NEXT: }
# RELOC32:      __cap_relocs {
# RELOC32-NEXT:   0x12510 DATA - 0x13800 [0x13800-0x13804]
# RELOC32-NEXT: }

# RELOC64:      .rela.dyn {
# RELOC64-NEXT:   0x123F0 R_RISCV_CHERI_CAPABILITY b 0x0
# RELOC64-NEXT: }
# RELOC64:      __cap_relocs {
# RELOC64-NEXT:   0x12400 DATA - 0x13410 [0x13410-0x13414]
# RELOC64-NEXT: }

# NM32: 00013800 d a
# NM64: 0000000000013410 d a

## .got[0] = _DYNAMIC
## .got[1] = 0 (relocated by R_RISCV_CHERI_CAPABILITY at run time)
## .got[2] = 0 (relocated by __cap_relocs at run time)
# HEX32: section '.got':
# HEX32: 0x00012500 8c240100 00000000 00000000 00000000
# HEX32: 0x00012510 00000000 00000000

# HEX64: section '.got':
# HEX64: 0x000123e0 00230100 00000000 00000000 00000000
# HEX64: 0x000123f0 00000000 00000000 00000000 00000000
# HEX64: 0x00012400 00000000 00000000 00000000 00000000

## &.got[2]-. = 0x124a8-0x11414 = 4096*1+148
# DIS32:      1147c: auipcc a0, 0x1
# DIS32-NEXT:        lc a0, 0x94(a0)
## &.got[1]-. = 0x124a0-0x1141c = 4096*1+132
# DIS32:      11484: auipcc a0, 0x1
# DIS32-NEXT:        lc a0, 0x84(a0)

## &.got[2]-. = 0x12400-0x112e8 = 4096*1+280
# DIS64:      112f0: auipcc a0, 0x1
# DIS64-NEXT:        lc a0, 0x110(a0)
## &.got[1]-. = 0x123f0-0x112f0 = 4096*1+256
# DIS64:      112f8: auipcc a0, 0x1
# DIS64-NEXT:        lc a0, 0xf8(a0)

clgc a0, a
clgc a0, b

.data
a:
## An undefined reference of _GLOBAL_OFFSET_TABLE_ causes .got[0] to be
## allocated to store _DYNAMIC.
.long _GLOBAL_OFFSET_TABLE_ - .
