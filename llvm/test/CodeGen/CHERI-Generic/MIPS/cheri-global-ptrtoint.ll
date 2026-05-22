; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/cheri-global-ptrtoint.ll
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi purecap %s -o - | \
; RUN:   FileCheck %s --check-prefix=ASM -DPTR_DIRECTIVE=.8byte

@ext_array = external addrspace(200) global [0 x i8], align 1
; ASM: some_var:
; ASM-NEXT: [[PTR_DIRECTIVE]] ext_array{{$}}
@some_var = addrspace(200) global i64 ptrtoint (ptr addrspace(200) @ext_array to i64), align 8
