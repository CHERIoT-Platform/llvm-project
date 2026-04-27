; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-cheriotrtos -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap,+xcheriot -verify-machineinstrs -o - | FileCheck %s
target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

@glob = addrspace(200) constant <{ ptr addrspace(200), [4 x i8], [4 x i8] }> <{ ptr addrspace(200) null, [4 x i8] c"\0B\00\00\00", [4 x i8] undef }>

; Verify that the +8 offset to @glob is preserved
; CHECK-LABEL: test:
; CHECK: auipcc {{.*}}, %cheriot_compartment_hi(glob+8)
define ptr addrspace(200) @test() addrspace(200) {
start:
  %a = load ptr addrspace(200), ptr addrspace(200) getelementptr inbounds nuw (i8, ptr addrspace(200) @glob, i32 8), align 8
  ret ptr addrspace(200) %a
}
