# REQUIRES: mips
# RUN: llvm-mc -position-independent -filetype=obj -triple=mips64-none-freebsd %s -o %t.o
# RUN: not ld.lld %t.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=MIPS
# These symbols are not automatically added for non-purecapL
# MIPS: error: undefined symbol: __start___cap_relocs
# MIPS: error: undefined symbol: __stop___cap_relocs
# RUN: %cheri128_llvm-mc -position-independent -filetype=obj -target-abi purecap %s -o %t.o
# RUN: ld.lld %t.o -o - | llvm-objdump -h -t - | FileCheck %s

#
# This used to cause LLD to create a symbol table with invalid entries
# LLD now outputs an empty section with valid start/stop symbols

.text
.global __start
.option pic0
__start:
  dla $a0, __start___cap_relocs
  dla $a1, __stop___cap_relocs
  
# CHECK-LABEL: Sections:
# CHECK:  3 __cap_relocs   00000000 [[VMA:[0-9a-f]+]] DATA
# CHECK-LABEL: SYMBOL TABLE:
# CHECK-NEXT: 0{{[0-9a-f]+}} l       .got		0000000000000000 .hidden _gp
# CHECK-NEXT: 0{{[0-9a-f]+}} g       .text	0000000000000000 __start
# CHECK-NEXT: [[VMA]] g       __cap_relocs   0000000000000000 .protected __start___cap_relocs
# CHECK-NEXT: [[VMA]] g       __cap_relocs   0000000000000000 .protected __stop___cap_relocs
