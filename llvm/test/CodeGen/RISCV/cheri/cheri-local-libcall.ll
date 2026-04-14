; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap,+xcheriot < %s | FileCheck %s
target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

@f = dso_local local_unnamed_addr addrspace(200) global ptr addrspace(200) null, align 8

define dso_local void @_Z11id_functionv() addrspace(200) #0 {
entry:
  ret void
}

define dso_local cheriot_compartmentcalleecc void @_Z9say_hellov() local_unnamed_addr addrspace(200) #1 {
entry:
  store ptr addrspace(200) @_Z11id_functionv, ptr addrspace(200) @f, align 8
  ret void
}

attributes #0 = { minsize mustprogress nofree norecurse nosync nounwind optsize willreturn memory(none) "cheri-compartment"="hello" "interrupt-state"="disabled" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+xcheripurecap,+xcheriot,+e,+m,+relax,+xcheri,+xcheriot" }
attributes #1 = { minsize mustprogress nofree norecurse nosync nounwind optsize willreturn memory(write, argmem: none, inaccessiblemem: none) "cheri-compartment"="hello" "interrupt-state"="enabled" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="cheriot" "target-features"="+32bit,+c,+xcheripurecap,+xcheriot,+e,+m,+relax,+xcheri,+xcheriot" }

; CHECK: 	.section	.compartment_exports,"aR",@progbits
; CHECK-NEXT:	.type	__library_export_hello__Z11id_functionv,@object
; CHECK-NEXT:	.p2align	2, 0x0
; CHECK-NEXT: __library_export_hello__Z11id_functionv:
; CHECK-NEXT:	.half	_Z11id_functionv-__compartment_pcc_start
; CHECK-NEXT:	.byte	0
; CHECK-NEXT:	.byte	16
; CHECK-NEXT:	.size	__library_export_hello__Z11id_functionv, 4
; CHECK-NEXT:	.type	__export_hello__Z9say_hellov,@object
; CHECK-NEXT:	.globl	__export_hello__Z9say_hellov
; CHECK-NEXT:	.p2align	2, 0x0
; CHECK-NEXT:__export_hello__Z9say_hellov:
; CHECK-NEXT:	.half	_Z9say_hellov-__compartment_pcc_start
; CHECK-NEXT:	.byte	0
; CHECK-NEXT:	.byte	8
; CHECK-NEXT:	.size	__export_hello__Z9say_hellov, 4
; CHECK-NEXT:	.section	.compartment_imports._Z11id_functionv,"aG",@progbits,__library_import_hello__Z11id_functionv,comdat
; CHECK-NEXT:	.type	__library_import_hello__Z11id_functionv,@object
; CHECK-NEXT:	.weak	__library_import_hello__Z11id_functionv
; CHECK-NEXT:	.p2align	3, 0x0
; CHECK-NEXT:__library_import_hello__Z11id_functionv:
; CHECK-NEXT:	.word	__library_export_hello__Z11id_functionv+1
; CHECK-NEXT:	.word	0
; CHECK-NEXT:	.size	__library_import_hello__Z11id_functionv, 8
