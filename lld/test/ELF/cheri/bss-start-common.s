# REQUIRES: mips
# RUN: llvm-mc -filetype=obj -triple=cheri-unknown-freebsd -target-abi=purecap -mcpu=cheri128 %s -o %t
# RUN: ld.lld %t -o %t-cheri --verbose
# RUN: llvm-objdump -t --section-headers %t-cheri | FileCheck %s -check-prefix CHERI

# CHERI: Sections:
# CHERI: Idx Name          Size      VMA          Type
# CHERI:   7 .bss          00000010  [[ADDR:[0-9a-f]+]] BSS
# CHERI: SYMBOL TABLE:
# CHERI: [[ADDR]] g        .bss 0000000000000010 __bss_start

## Test __bss_start is defined at the start of .bss for CHERI-MIPS (split out
## of the upstream x86-only bss-start-common.s).

.global __bss_start
.text
_start:
.comm sym1,4,4
