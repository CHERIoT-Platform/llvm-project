; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot -mattr=+xcheri,+xcheripurecap -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32cheriotv1-unknown-cheriotrtos"

; CHECK-NOT: addi	a0, a0, -1
; CHECK: addi	a0, a0, 1
; CHECK-NOT: addi	a0, a0, -1

define fastcc void @test() addrspace(200) {
entry:
  br label %for.cond

for.cond:                                         ; preds = %for.cond, %entry
  %storemerge = phi i32 [ 0, %entry ], [ %inc, %for.cond ]
  %exitcond.not = icmp eq i32 %storemerge, 0
  %inc = add i32 %storemerge, 1
  br label %for.cond
}
