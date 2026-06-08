; RUN: opt -S -passes=instcombine %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; CHECK-LABEL: define ptr addrspace(200) @test1
; CHECK: getelementptr inbounds i32, {{.*}}, i32 %count.i9.i
; CHECK-NOT: getelementptr i8, {{.*}}, i32 20
define ptr addrspace(200) @test1(ptr addrspace(200) %xs, i32 %count.i9.i) addrspace(200) {
start:
  %_6.i.i3 = getelementptr i32, ptr addrspace(200) %xs, i32 6
  %_35.i.i = getelementptr inbounds i32, ptr addrspace(200) %_6.i.i3, i32 %count.i9.i
  %_47.i.i = getelementptr i8, ptr addrspace(200) %_35.i.i, i32 -4
  ret ptr addrspace(200) %_47.i.i
}

; CHECK-LABEL: define void @test2
; CHECK: getelementptr
; CHECK-NOT: getelementptr
define void @test2(ptr addrspace(200) noalias noundef nonnull align 8 captures(none) dereferenceable(8) %self, i32 noundef signext range(i32 1, 4) %data) unnamed_addr addrspace(200) {
start:
  %_5 = load ptr addrspace(200), ptr addrspace(200) %self, align 8
  %0 = ptrtoint ptr addrspace(200) %_5 to i32
  %_8 = and i32 %0, -4
  %.neg = sub i32 0, %0
  %_7 = add i32 %.neg, %data
  %_0.i = add i32 %_7, %_8
  %1 = getelementptr i8, ptr addrspace(200) %_5, i32 %_0.i
  store ptr addrspace(200) %1, ptr addrspace(200) %self, align 8
  ret void
}

!llvm.ident = !{!0}
!llvm.module.flags = !{!1}

!0 = !{!"rustc version 1.95.0-dev"}
!1 = !{i32 1, !"target-abi", !"cheriot"}
