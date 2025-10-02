; RUN: %riscv64_cheri_purecap_llc -stop-after=finalize-isel %s -o - | FileCheck %s

declare ptr addrspace(200) @llvm.ptrmask.p200.i64(ptr addrspace(200), i64);

; CHECK-LABEL: name: test1
; CHECK: %0:ygpr = COPY $x10_y
; CHECK-NEXT: %1:gpr = PseudoCGetAddr %0
; CHECK-NEXT: %2:gpr = ADDI $x0, -1793
; CHECK-NEXT: %3:gpr = SRLI killed %2, 8
; CHECK-NEXT: %4:gpr = AND killed %1, killed %3
; CHECK-NEXT: %5:ygpr = CSetAddr %0, killed %4
; CHECK-NEXT: $x10_y = COPY %5
; CHECK-NEXT: PseudoCRET implicit $x10_y

define ptr addrspace(200) @test1(ptr addrspace(200) %src) {
  %ptr = call ptr addrspace(200) @llvm.ptrmask.p200.i64(ptr addrspace(200) %src, i64 72057594037927928)
  ret ptr addrspace(200) %ptr
}
