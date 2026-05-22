; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/cheri-global-ptrtoint.ll
; RUN: llc -mtriple=riscv64 --relocation-model=pic -target-abi l64pc128d -mattr=+xcheri,+xcheripurecap,+f,+d %s -o - | \
; RUN:   FileCheck %s --check-prefix=ASM -DPTR_DIRECTIVE=.quad

@ext_array = external addrspace(200) global [0 x i8], align 1
; ASM: some_var:
; ASM-NEXT: [[PTR_DIRECTIVE]] ext_array{{$}}
@some_var = addrspace(200) global i64 ptrtoint (ptr addrspace(200) @ext_array to i64), align 8
