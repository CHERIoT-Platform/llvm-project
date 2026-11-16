# The cllc/clgc pseudo-instructions expand to auipcc with %pcrel_hi /
# %got_pcrel_hi and a %pcrel_lo second instruction. Check that both textual
# output (which prints the specifiers) and object emission work.
# RUN: llvm-mc %s -triple=riscv64 -mattr=+xcheri,+xcheripurecap -target-abi l64pc128 \
# RUN:     | FileCheck %s --check-prefix=ASM
# RUN: llvm-mc %s -triple=riscv64 -mattr=+xcheri,+xcheripurecap -target-abi l64pc128 -filetype=obj \
# RUN:     | llvm-readobj -r - | FileCheck %s --check-prefix=RELOC
# RUN: llvm-mc %s -triple=riscv32-unknown-cheriotrtos -mcpu=cheriot -target-abi cheriot \
# RUN:     | FileCheck %s --check-prefix=ASM
# RUN: llvm-mc %s -triple=riscv32-unknown-cheriotrtos -mcpu=cheriot -target-abi cheriot -filetype=obj \
# RUN:     | llvm-readobj -r - | FileCheck %s --check-prefix=RELOC

cllc ca0, local
# ASM: {{(ct\.)?}}auipcc a0, %pcrel_hi(local)
# ASM-NEXT: {{(ct\.)?}}cincoffset a0, a0, %pcrel_lo(.Lpcrel_hi0)
clgc ca1, global
# ASM: {{(ct\.)?}}auipcc a1, %got_pcrel_hi(global)
# ASM-NEXT: {{(ct\.)?}}clc a1, %pcrel_lo(.Lpcrel_hi1)(a1)

# RELOC:      R_RISCV_PCREL_HI20 local 0x0
# RELOC-NEXT: R_RISCV_PCREL_LO12_I .Lpcrel_hi0 0x0
# RELOC-NEXT: R_RISCV_GOT_HI20 global 0x0
# RELOC-NEXT: R_RISCV_PCREL_LO12_I .Lpcrel_hi1 0x0
