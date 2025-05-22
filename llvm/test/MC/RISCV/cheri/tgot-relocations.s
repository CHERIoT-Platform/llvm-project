# RUN: llvm-mc -triple=riscv64 -mattr=+xcheri,+xcheripurecap \
# RUN:     -target-abi l64pc128 -cheri-tgot-tls < %s \
# RUN:     | FileCheck --check-prefix=CHECK-ASM %s
# RUN: llvm-mc -filetype=obj -triple riscv64 -mattr=+xcheri,+xcheripurecap \
# RUN:     -target-abi l64pc128 -cheri-tgot-tls < %s \
# RUN:     | llvm-objdump --mattr=+xcheri,+xcheripurecap -d - \
# RUN:     | FileCheck --check-prefix=CHECK-OBJ %s
# RUN: llvm-mc -filetype=obj -triple riscv64 -mattr=+xcheri,+xcheripurecap \
# RUN:     -target-abi l64pc128 -cheri-tgot-tls < %s \
# RUN:     | llvm-readobj -r - \
# RUN:     | FileCheck --check-prefix=CHECK-REL %s

.global x

## Local Exec
# CHECK-ASM: lui a0, %tgot_tprel_hi(x)
# CHECK-OBJ: lui a0, 0
# CHECK-REL: R_RISCV_CHERI_TLS_TGOT_HI20 x
lui a0, %tgot_tprel_hi(x)
# CHECK-ASM: cincoffset a0, tp, a0, %tgot_tprel_add(x)
# CHECK-OBJ: cincoffset a0, tp, a0
# CHECK-REL: R_RISCV_CHERI_TLS_TGOT_ADD x
cincoffset ca0, ctp, a0, %tgot_tprel_add(x)
# CHECK-ASM: lc a0, %tgot_tprel_lo(x)(a0)
# CHECK-OBJ: clc a0, 0x0(a0)
# CHECK-REL: R_RISCV_CHERI_TLS_TGOT_LO12_I x
lc ca0, %tgot_tprel_lo(x)(ca0)

## Initial Exec
# CHECK-ASM-LABEL: .Lpcrel_hi0:
# CHECK-ASM: auipcc a0, %tls_ie_pcrel_hi(x)
# CHECK-OBJ: auipcc a0, 0
# CHECK-REL: R_RISCV_CHERI_TLS_TGOT_GOT_HI20 x
# CHECK-ASM: ld a0, %pcrel_lo(.Lpcrel_hi0)(a0)
# CHECK-OBJ: cld a0, 0x0(a0)
# CHECK-REL: R_RISCV_PCREL_LO12_I .Lpcrel_hi0
cla.tls.ie a0, x
# CHECK-ASM: cincoffset a0, tp, a0
# CHECK-OBJ: cincoffset a0, tp, a0
cincoffset ca0, ctp, a0
# CHECK-ASM: lc a0, 0(a0)
# CHECK-OBJ: clc a0, 0x0(a0)
lc ca0, 0(ca0)

## Initial Exec
# CHECK-ASM-LABEL: .Ltmp0:
1:
# CHECK-ASM: auipcc a0, %tls_ie_pcrel_hi(x)
# CHECK-OBJ: auipcc a0, 0
# CHECK-REL: R_RISCV_CHERI_TLS_TGOT_GOT_HI20 x
auipcc ca0, %tls_ie_pcrel_hi(x)
# CHECK-ASM: ld a0, %pcrel_lo(.Ltmp0)(a0)
# CHECK-OBJ: ld a0, 0x0(a0)
# CHECK-REL: R_RISCV_PCREL_LO12_I .Ltmp0
ld a0, %pcrel_lo(1b)(ca0)
# CHECK-ASM: cincoffset a0, tp, a0
# CHECK-OBJ: cincoffset a0, tp, a0
cincoffset ca0, ctp, a0
# CHECK-ASM: lc a0, 0(a0)
# CHECK-OBJ: lc a0, 0x0(a0)
lc ca0, 0(ca0)

## General Dynamic
# CHECK-ASM-LABEL: .Lpcrel_hi1:
# CHECK-ASM: auipcc a0, %tls_gd_pcrel_hi(x)
# CHECK-OBJ: auipcc a0, 0
# CHECK-REL: R_RISCV_CHERI_TLS_TGOT_GD_HI20 x
# CHECK-ASM: cincoffset a0, a0, %pcrel_lo(.Lpcrel_hi1)
# CHECK-OBJ: cincoffset a0, a0, 0
# CHECK-REL: R_RISCV_PCREL_LO12_I .Lpcrel_hi1
clc.tls.gd ca0, x
# CHECK-ASM: ccall __tls_get_addr
# CHECK-OBJ: auipcc ra, 0
# CHECK-OBJ: jalr ra
# CHECK-REL: R_RISCV_CALL_PLT __tls_get_addr
ccall __tls_get_addr

## General Dynamic
# CHECK-ASM-LABEL: .Ltmp1:
1:
# CHECK-ASM: auipcc a0, %tls_gd_pcrel_hi(x)
# CHECK-OBJ: auipcc a0, 0
# CHECK-REL: R_RISCV_CHERI_TLS_TGOT_GD_HI20 x
auipcc ca0, %tls_gd_pcrel_hi(x)
# CHECK-ASM: cincoffset a0, a0, %pcrel_lo(.Ltmp1)
# CHECK-OBJ: cincoffset a0, a0, 0
# CHECK-REL: R_RISCV_PCREL_LO12_I .Ltmp1
cincoffset ca0, ca0, %pcrel_lo(1b)
# CHECK-ASM: ccall __tls_get_addr
# CHECK-OBJ: auipcc ra, 0
# CHECK-OBJ: jalr ra
# CHECK-REL: R_RISCV_CALL_PLT __tls_get_addr
ccall __tls_get_addr
