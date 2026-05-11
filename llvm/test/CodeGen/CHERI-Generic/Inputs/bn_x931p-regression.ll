;; After a C13==NULL optimization in the MIPS backend,
;; openssl/crypto/bn/bn_x931p.c was failing.
;; Check that it works as expected
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=PURECAP

declare i32 @b() addrspace(200)

; Calling a function with 9+ capability parameters would previously cause verification errors
define i32 @a(ptr addrspace(200) %0, ptr addrspace(200) %1, ptr addrspace(200) %2, ptr addrspace(200) %3, ptr addrspace(200) %4, ptr addrspace(200) %5, ptr addrspace(200) %6, ptr addrspace(200) %7, ptr addrspace(200) %8) addrspace(200) nounwind {
  %ret = call i32 @b()
  ret i32 %ret
}
