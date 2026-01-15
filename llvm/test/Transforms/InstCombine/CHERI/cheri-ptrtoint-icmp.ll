; Check that
; RUN: %cheri_opt -S -passes=instcombine %s -o - | FileCheck %s
source_filename = "vec_optimizes_away.a42c4a0c72f4b4c3-cgu.0"
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

define dso_local noundef i1 @foo() unnamed_addr addrspace(200) {
; CHECK-LABEL: @foo()
; CHECK-NEXT: start:
start:
; CHECK-NEXT:   %0 = tail call noundef align 4 dereferenceable_or_null(12) ptr addrspace(200) @alloc(i32 noundef signext 12, i32 noundef signext 4)
  %0 = tail call noundef align 4 dereferenceable_or_null(12) ptr addrspace(200) @alloc(i32 noundef signext 12, i32 noundef signext 4)

; CHECK-NEXT:   %1 = icmp eq ptr addrspace(200) %0, null
  %1 = ptrtoint ptr addrspace(200) %0 to i32
  %2 = icmp eq i32 %1, 0

; CHECK-NEXT:   ret i1 %1
  ret i1 %2
}

declare dso_local noalias noundef ptr addrspace(200) @alloc(i32 noundef signext, i32 allocalign noundef signext) unnamed_addr addrspace(200)

