; RUN: opt < %s -S -passes=licm | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; Because of the tightness of representable bounds on Cheriot, LICM must not reorder
; GEP indices if they are not guaranteed to be the same sign.
; CHECK-LABEL: @foo1
; CHECK: entry:
; CHECK-NOT: getelementptr
; CHECK: do.body:
; CHECK: getelementptr
; CHECK-SAME: i32 %inc
; CHECK: getelementptr
; CHECK-SAME: i32 -8

define void @foo1(ptr addrspace(200) noundef %r) {
entry:
  br label %do.body

do.body:
  %ctr.0 = phi i32 [ 0, %entry ], [ %inc, %do.body ]
  %inc = add nuw nsw i32 %ctr.0, 1
  %add.ptr = getelementptr inbounds nuw i16, ptr addrspace(200) %r, i32 %inc
  %add.ptr1 = getelementptr inbounds i8, ptr addrspace(200) %add.ptr, i32 -8
  tail call void @bar(ptr addrspace(200) noundef nonnull %add.ptr1)
  %exitcond.not = icmp eq i32 %inc, 256
  br i1 %exitcond.not, label %do.end, label %do.body

do.end:
  ret void
}
; CHECK-LABEL: @zot
; CHECK: bb:
; CHECK-NOT: getelementptr
; CHECK: bb12:
; CHECK: getelementptr
; CHECK: getelementptr
; CHECK: call addrspace(200) void @bar

define void @zot(ptr addrspace(200) %arg) {
bb:
  %alloca = alloca [25 x i64], align 8, addrspace(200)
  br label %bb8

bb8:
  %phi = phi i32 [ 0, %bb ], [ %add16, %bb14 ]
  %phi9 = phi i32 [ 0, %bb ], [ %select, %bb14 ]
  %icmp = icmp ult i32 %phi9, 256
  br i1 %icmp, label %bb10, label %bb17

bb10:
  %icmp11 = icmp eq i32 %phi, 4
  br i1 %icmp11, label %bb12, label %bb14

bb12:
  %getelementptr = getelementptr inbounds nuw i16, ptr addrspace(200) %arg, i32 %phi9
  %getelementptr13 = getelementptr inbounds i8, ptr addrspace(200) %getelementptr, i32 -6
  call void @bar(ptr addrspace(200) %getelementptr13)
  br label %bb14

bb14:
  %icmp15 = icmp ne i32 %phi9, 255
  %add = add nuw nsw i32 %phi9, 2
  %zext = zext i1 %icmp15 to i32
  %add16 = add i32 %zext, %phi
  %select = select i1 %icmp15, i32 %add, i32 256
  br label %bb8

bb17:
  ret void
}

declare void @bar(ptr addrspace(200))

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"cheriot"}
