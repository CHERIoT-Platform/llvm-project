; !DO NOT AUTOGEN!
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -relocation-model=static %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -filetype=obj %s -o - | llvm-readobj -r - | \
@IF-MIPS@; RUN:   FileCheck %s --check-prefix=RELOCS '-DCAP_RELOC=R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE'
@IF-RISCV@; RUN:   FileCheck %s --check-prefix=RELOCS -DCAP_RELOC=R_RISCV_CHERI_CAPABILITY

@a = common addrspace(200) global [5 x i32] zeroinitializer, align 4
@b = addrspace(200) global [3 x ptr addrspace(200)] [
    ptr addrspace(200) getelementptr (i8, ptr addrspace(200) @a, i64 8),
    ptr addrspace(200) getelementptr (i8, ptr addrspace(200) @a, i64 4),
    ptr addrspace(200) @a
  ], align 32
@c = addrspace(200) constant [3 x ptr addrspace(200)] [
    ptr addrspace(200) getelementptr (i8, ptr addrspace(200) @a, i64 16),
    ptr addrspace(200) getelementptr (i8, ptr addrspace(200) @a, i64 12),
    ptr addrspace(200) @a
  ], align 32

; ASM:      .data
; ASM-NEXT: .globl b
; ASM-NEXT: .p2align 5, 0x0
; ASM-NEXT: b:
; ASM-NEXT: .chericap a+8
; ASM-NEXT: .chericap a+4
; ASM-NEXT: .chericap a
; ASM-NEXT: .size b, {{48|24}}

; ASM:      .section .data.rel.ro,"aw",@progbits
; ASM-NEXT: .globl c
; ASM-NEXT: .p2align 5, 0x0
; ASM-NEXT: c:
; ASM-NEXT: .chericap a+16
; ASM-NEXT: .chericap a+12
; ASM-NEXT: .chericap a
; ASM-NEXT: .size c, {{48|24}}

; RELOCS-LABEL: .rela.data {
; RELOCS-NEXT:    0x0 [[CAP_RELOC]] a 0x8
; RELOCS-NEXT:    0x{{10|8}} [[CAP_RELOC]] a 0x4
; RELOCS-NEXT:    0x{{20|10}} [[CAP_RELOC]] a 0x0
; RELOCS-NEXT:  }
; RELOCS-LABEL: .rela.data.rel.ro {
; RELOCS-NEXT:    0x0 [[CAP_RELOC]] a 0x10
; RELOCS-NEXT:    0x{{10|8}} [[CAP_RELOC]] a 0xC
; RELOCS-NEXT:    0x{{20|10}} [[CAP_RELOC]] a 0x0
; RELOCS-NEXT:  }
