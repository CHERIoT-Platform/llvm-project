; RUN: llc --mcpu=cheriot --mtriple=riscv32-unknown-cheriotrtos -target-abi cheriot -mattr=+xcheri,+xcheripurecap,+xcheriot -verify-machineinstrs -o - %s | FileCheck %s

;; Previously crashed when LLVM tried to materialize the immediate for the csetbounds in a register.

%struct.M = type { i32, i32, i32, i32 }
@model = dso_local local_unnamed_addr addrspace(200) global [320 x %struct.M] zeroinitializer, align 4

; CHECK-LABEL: f:
; CHECK: li [[SIZE:[a-z0-9]+]], 5
; CHECK: slli [[SIZE]], [[SIZE]], 10
; CHECK: csetbounds {{[a-z0-9]+}}, {{[a-z0-9]+}}, [[SIZE]]
define dso_local void @f(ptr addrspace(200) noundef %p) addrspace(200) {
entry:
  br label %for.cond

for.cond:
  %j = phi i32 [ 0, %entry ], [ %inc, %for.body ]
  %done = icmp eq i32 %j, 320
  br i1 %done, label %exit, label %for.body

for.body:
  %src = getelementptr inbounds ptr addrspace(200), ptr addrspace(200) %p, i32 %j
  %v = load volatile i32, ptr addrspace(200) %src, align 4
  %dst = getelementptr inbounds %struct.M, ptr addrspace(200) @model, i32 %j
  store i32 %v, ptr addrspace(200) %dst, align 4
  %inc = add nuw nsw i32 %j, 1
  br label %for.cond

exit:
  ret void
}
