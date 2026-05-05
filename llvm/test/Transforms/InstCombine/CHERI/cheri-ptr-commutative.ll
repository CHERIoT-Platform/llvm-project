; RUN: opt -S -passes=instcombine %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; CHECK-LABEL: define ptr addrspace(200) @test_iterator_chain_nth_back
; CHECK: getelementptr inbounds i32, {{.*}}, i32 %count.i9.i
; CHECK-NOT: getelementptr i8, {{.*}}, i32 20
define ptr addrspace(200) @test_iterator_chain_nth_back(ptr addrspace(200) %xs, i32 %count.i9.i) addrspace(200) {
start:
  %_6.i.i3 = getelementptr i32, ptr addrspace(200) %xs, i32 6
  %_35.i.i = getelementptr inbounds i32, ptr addrspace(200) %_6.i.i3, i32 %count.i9.i
  %_47.i.i = getelementptr i8, ptr addrspace(200) %_35.i.i, i32 -4
  ret ptr addrspace(200) %_47.i.i
}

!llvm.ident = !{!0}
!llvm.module.flags = !{!1}

!0 = !{!"rustc version 1.95.0-dev"}
!1 = !{i32 1, !"target-abi", !"cheriot"}
