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

; CHECK-LABEL: @foo2
; CHECK: entry:
; CHECK: getelementptr
; CHECK-SAME: i32 8
; CHECK: do.body:
; CHECK: getelementptr
; CHECK-SAME: i32 %inc
; CHECK-NOT: getelementptr

define void @foo2(ptr addrspace(200) noundef %r) {
entry:
  br label %do.body

do.body:
  %ctr.0 = phi i32 [ 0, %entry ], [ %inc, %do.body ]
  %inc = add nuw nsw i32 %ctr.0, 1
  %add.ptr = getelementptr inbounds nuw i16, ptr addrspace(200) %r, i32 %inc
  %add.ptr1 = getelementptr inbounds i8, ptr addrspace(200) %add.ptr, i32 8
  tail call void @bar(ptr addrspace(200) noundef nonnull %add.ptr1)
  %exitcond.not = icmp eq i32 %inc, 256
  br i1 %exitcond.not, label %do.end, label %do.body

do.end:
  ret void
}


declare void @bar(ptr addrspace(200) noundef)

!llvm.module.flags = !{!0}
!0 = !{i32 1, !"target-abi", !"cheriot"}
