; Check the element type chosen when expanding a memset of a capability-aligned
; buffer: a zero memset can use capability stores of the null capability, but a
; non-zero memset cannot, so it uses the generic expansion (inline integer
; stores on 128-bit-cap targets, a library call on RV32).
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs -O1 %s -o - | FileCheck %s -check-prefix=CHECK

declare void @llvm.memset.p200.i64(ptr addrspace(200) nocapture writeonly, i8, i64, i1 immarg) addrspace(200)

; Check that the zero memset is expanded to capability stores + a single tail store.
define void @align64(ptr addrspace(200) nocapture %out) addrspace(200) {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 64 %out, i8 0, i64 36, i1 false)
  ret void
}

; Can't use capability stores for a non-zero fill, so fall back to the generic
; expansion (inline integer stores here, a library call on RV32).
define void @align64_nonzero_value(ptr addrspace(200) nocapture %out) addrspace(200) {
entry:
  call void @llvm.memset.p200.i64(ptr addrspace(200) align 64 %out, i8 1, i64 36, i1 false)
  ret void
}
