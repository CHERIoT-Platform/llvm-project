; RUN: llc --filetype=asm --mcpu=cheriot --mtriple=riscv32-unknown-cheriotrtos -target-abi cheriot  %s -mattr=+xcheri,+xcheripurecap,+xcheriot,+b -o - | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-n32-S128-pf200:64:64:64:32-A200-P200-G200"
target triple = "riscv32-unknown-cheriotrtos"

%struct.mlk_polymat = type { [3 x %struct.mlk_polyvec] }
%struct.mlk_polyvec = type { [3 x %struct.mlk_poly] }
%struct.mlk_poly = type { [256 x i16] }

; Make sure we don't accidentally try to use a sh3add to compute a capability frame offset.
; CHECK-LABEL: PQCP_MLKEM_NATIVE_MLKEM768_indcpa_keypair_derand:
; CHECK-NOT: sh3add
define ptr addrspace(200) @PQCP_MLKEM_NATIVE_MLKEM768_indcpa_keypair_derand() addrspace(200) #0 {
entry:
  %a = alloca %struct.mlk_polymat, align 32, addrspace(200)
  %e = alloca %struct.mlk_polyvec, align 32, addrspace(200)
  ret ptr addrspace(200) %e
}

attributes #0 = { "target-cpu"="cheriot-ibex" }
