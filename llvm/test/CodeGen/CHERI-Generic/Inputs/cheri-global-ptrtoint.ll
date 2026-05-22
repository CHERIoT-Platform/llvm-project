; !DO NOT AUTOGEN!
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -o - | \
@IF-MIPS@; RUN:   FileCheck %s --check-prefix=ASM -DPTR_DIRECTIVE=.8byte
@IF-RISCV64@; RUN:   FileCheck %s --check-prefix=ASM -DPTR_DIRECTIVE=.quad
@IF-RISCV32@; RUN:   FileCheck %s --check-prefix=ASM -DPTR_DIRECTIVE=.word

@ext_array = external addrspace(200) global [0 x i8], align 1
; ASM: some_var:
; ASM-NEXT: [[PTR_DIRECTIVE]] ext_array{{$}}
@some_var = addrspace(200) global iCAPRANGE ptrtoint (ptr addrspace(200) @ext_array to iCAPRANGE), align @CAP_RANGE_BYTES@
