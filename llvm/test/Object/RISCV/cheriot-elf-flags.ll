; RUN: llc --filetype=obj -mtriple=riscv32-unknown-unknown -mcpu=cheriot -target-abi cheriot %s -o - | llvm-readelf --file-header - | FileCheck %s

; Ensure CHERIoT targets have the correct ELF flags.
; CHECK: Flags: 0x30009, RVC, RVE, cheriabi, capability mode
