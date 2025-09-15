# RUN: llvm-mc -triple=mips64-unknown-freebsd -mcpu=cheri128 -filetype=obj %s -o - | llvm-readobj -h - | FileCheck --check-prefix=CHERI128-NO-ABI %s
# CHERI128-NO-ABI: Flags [ (0x30C10004)
# RUN: llvm-mc -triple=mips64-unknown-freebsd -mcpu=cheri128 -filetype=obj -target-abi n64 %s -o - | llvm-readobj -h - | FileCheck --check-prefix=CHERI128-N64 %s
# CHERI128-N64: Flags [ (0x30C10004)
# RUN: llvm-mc -triple=mips64-unknown-freebsd -mcpu=cheri128 -position-independent -filetype=obj -target-abi purecap %s -o - | llvm-readobj -h - | FileCheck --check-prefix=CHERI128-PURECAP %s
# CHERI128-PURECAP: Flags [ (0x30C1C006)
