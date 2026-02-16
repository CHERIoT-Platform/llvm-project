; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-unknown -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap -o - | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

; This test was previously generating an indirect tail call using s1 as the
; destination register, which doesn't work because s1 needs to be restored.

; CHECK-NOT: cjr s{{[01]}}

define fastcc zeroext i1 @_RNvNvMsa_NtCsi5kCOIVVOQy_4core3fmtNtB7_9Formatter12pad_integral12write_prefix(ptr addrspace(200) %f.0.val, ptr addrspace(200) readonly captures(none) %f.8.val, i32 %0, ptr addrspace(200) %1, i32 %2) local_unnamed_addr addrspace(200) #0 {
start:
  %.not = icmp eq i32 %0, 1
  br i1 %.not, label %bb4, label %bb1

bb1:                                              ; preds = %start
  %3 = load ptr addrspace(200), ptr addrspace(200) %f.8.val, align 8
  %self = tail call zeroext i1 %3(ptr addrspace(200) null, i32 0)
  br i1 %self, label %common.ret, label %bb4

common.ret:                                       ; preds = %bb1
  ret i1 false

bb4:                                              ; preds = %bb1, %start
  %4 = load ptr addrspace(200), ptr addrspace(200) %f.8.val, align 8
  %5 = tail call zeroext i1 %4(ptr addrspace(200) %f.0.val, ptr addrspace(200) %1, i32 %2)
  ret i1 %5
}

attributes #0 = { "frame-pointer"="all" }
