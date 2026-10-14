; RUN: llc --filetype=asm --mtriple=riscv32-unknown-cheriotrtos -target-abi cheriot -mcpu cheriot-ibex -mattr=+xcheri,+xcheripurecap,+xcheriot -verify-machineinstrs -o - %s | FileCheck %s
target datalayout = "e-m:e-p:32:32-pfe200:64:64:64:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

;; Cheriot Ibex has latency penalty of two extra cycles on capability loads.
;; Verify that the scheduling model correctly fills the load shadow.

define i32 @test(ptr addrspace(200) %b, i32 %c, i32 %d, i32 %e) addrspace(200) {
; CHECK-LABEL: test:
; CHECK:         ct.clc
; CHECK-NEXT:    add
; CHECK-NEXT:    add
; CHECK-NEXT:    ct.clw
; CHECK-NEXT:    add
; CHECK-NEXT:    ct.cret
entry:
  %0 = load ptr addrspace(200), ptr addrspace(200) %b, align 8
  %1 = load i32, ptr addrspace(200) %0, align 4
  %add = add i32 %d, %c
  %add1 = add i32 %add, %e
  %add2 = add i32 %add1, %1
  ret i32 %add2
}
