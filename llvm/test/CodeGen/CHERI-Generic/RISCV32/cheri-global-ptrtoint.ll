; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/cheri-global-ptrtoint.ll
; RUN: llc -mtriple=riscv32 --relocation-model=pic -target-abi il32pc64f -mattr=+xcheri,+xcheripurecap,+f %s -o - | \
; RUN:   FileCheck %s --check-prefix=ASM -DPTR_DIRECTIVE=.word

@ext_array = external addrspace(200) global [0 x i8], align 1
; ASM: some_var:
; ASM-NEXT: [[PTR_DIRECTIVE]] ext_array{{$}}
@some_var = addrspace(200) global i32 ptrtoint (ptr addrspace(200) @ext_array to i32), align 4
