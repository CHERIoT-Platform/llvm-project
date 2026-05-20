; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/cheri-blockaddress.ll
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi purecap %s -o - | FileCheck %s --check-prefix=ASM
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi purecap -filetype=obj %s -o - | llvm-readobj -r - | \
; RUN:   FileCheck %s --check-prefix=OBJ '-DCAP_RELOC=R_MIPS_CHERI_CAPABILITY/R_MIPS_NONE/R_MIPS_NONE' -DADDEND=0x1C
; See address-of-label-crash.c in clang/test/CodeGen/cheri for the C source code

; Function Attrs: noinline nounwind optnone
define i32 @addrof_label_in_static() addrspace(200) {
entry:
  %0 = load ptr addrspace(200), ptr addrspace(200) @addrof_label_in_static.b, align 32
  br label %indirectgoto

label1:                                           ; preds = %indirectgoto
  ret i32 2

indirectgoto:                                     ; preds = %entry
  %indirect.goto.dest = phi ptr addrspace(200) [ %0, %entry ]
  indirectbr ptr addrspace(200) %indirect.goto.dest, [label %label1]
}

@addrof_label_in_static.b = internal addrspace(200) global ptr addrspace(200) blockaddress(@addrof_label_in_static, %label1), align 32

; Create a global containing the address of the label:
; ASM-LABEL: addrof_label_in_static:
; ASM:      cjr
; ASM:      .Ltmp0: # Block address taken
; ASM-NEXT: .LBB0_1: # %label1


; Function Attrs: noinline nounwind optnone
define i32 @addrof_label_in_local() addrspace(200) {
entry:
  %d = alloca ptr addrspace(200), align 16, addrspace(200)
  store ptr addrspace(200) blockaddress(@addrof_label_in_local, %label2), ptr addrspace(200) %d, align 32
  %0 = load ptr addrspace(200), ptr addrspace(200) %d, align 32
  br label %indirectgoto

label2:                                           ; preds = %indirectgoto
  ret i32 2

indirectgoto:                                     ; preds = %entry
  %indirect.goto.dest = phi ptr addrspace(200) [ %0, %entry ]
  indirectbr ptr addrspace(200) %indirect.goto.dest, [label %label2]
}

; ASM-LABEL: addrof_label_in_local:
; ASM:      cjr
; ASM:      .Ltmp1: # Block address taken
; ASM-NEXT: .LBB1_1: # %label2

; ASM-LABEL: addrof_label_in_static.b:
; ASM-NEXT:    .chericap
; ASM-NEXT:    .size addrof_label_in_static.b, 16

; The .o file should contain a relocation against the function with a constant addend (per-arch)
; OBJ-LABEL: .rela.data {
; OBJ-NEXT:    0x0 [[CAP_RELOC]] .Laddrof_label_in_static$local [[ADDEND]]
; OBJ-NEXT:  }
