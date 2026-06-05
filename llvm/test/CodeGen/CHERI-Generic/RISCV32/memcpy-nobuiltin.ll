; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/memcpy-nobuiltin.ll
; Check that llvm.memcpy() and llvm.memmove() intrinisics with must_preserve_cheri_tags
; attribute are always lowered to libcalls
; RUN: llc -mtriple=riscv32 --relocation-model=pic -target-abi il32pc64f -mattr=+xcheri,+xcheripurecap,+f -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,PURECAP
; RUN: llc -mtriple=riscv32 --relocation-model=pic -target-abi ilp32f -mattr=+xcheri,+f -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,HYBRID

declare void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memmove.p200.p200.i64(ptr addrspace(200) nocapture writeonly, ptr addrspace(200) nocapture readonly, i64, i1 immarg)

define void @memcpy_too_big_unaligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 1 %dst, ptr addrspace(200) align 8 %src, i64 2048, i1 false)
  ret void
  ; CHECK-LABEL: memcpy_too_big_unaligned:
  ; PURECAP: ccall memcpy
  ; HYBRID: call memcpy_c
}

define void @memcpy_aligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 8 %dst, ptr addrspace(200) align 8 %src, i64 8, i1 false)
  ret void
  ; This can be inlined;
  ; CHECK-LABEL: memcpy_aligned:
  ; PURECAP: # %bb.0:
  ; PURECAP-NEXT: clc a1, 0(a1)
  ; PURECAP-NEXT: csc a1, 0(a0)
  ; PURECAP-NEXT: cret
  ; HYBRID: # %bb.0:
  ; HYBRID-NEXT: lc.cap a1, (a1)
  ; HYBRID-NEXT: sc.cap a1, (a0)
  ; HYBRID-NEXT: ret
}

define void @memcpy_aligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 8 %dst, ptr addrspace(200) align 8 %src, i64 8, i1 false) must_preserve_cheri_tags
  ret void
  ; Correctly aligned -> no need for libcall (even with attribute)
  ; CHECK-LABEL: memcpy_aligned_must_preserve_tags:
  ; PURECAP: # %bb.0:
  ; PURECAP-NEXT: clc a1, 0(a1)
  ; PURECAP-NEXT: csc a1, 0(a0)
  ; PURECAP-NEXT: cret
  ; HYBRID: # %bb.0:
  ; HYBRID-NEXT: lc.cap a1, (a1)
  ; HYBRID-NEXT: sc.cap a1, (a0)
  ; HYBRID-NEXT: ret
}

define void @memcpy_underaligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 4 %dst, ptr addrspace(200) align 8 %src, i64 8, i1 false) must_preserve_cheri_tags
  ret void
  ; The memmove could be inlined but was tagged with nobuiltin -> should call memmove()
  ; CHECK-LABEL: memcpy_underaligned_must_preserve_tags:
  ; PURECAP: ccall memcpy
  ; HYBRID: call memcpy_c
}


; Same again in hybrid:

define void @memmove_too_big_unaligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 1 %dst, ptr addrspace(200) align 8 %src, i64 2048, i1 false)
  ret void
  ; CHECK-LABEL: memmove_too_big_unaligned:
  ; PURECAP: ccall memmove
  ; HYBRID: call memmove_c
}

define void @memmove_aligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 8 %dst, ptr addrspace(200) align 8 %src, i64 8, i1 false)
  ret void
  ; This can be inlined;
  ; CHECK-LABEL: memmove_aligned:
  ; PURECAP: # %bb.0:
  ; PURECAP-NEXT: clc a1, 0(a1)
  ; PURECAP-NEXT: csc a1, 0(a0)
  ; PURECAP-NEXT: cret
  ; HYBRID: # %bb.0:
  ; HYBRID-NEXT: lc.cap a1, (a1)
  ; HYBRID-NEXT: sc.cap a1, (a0)
  ; HYBRID-NEXT: ret
}

define void @memmove_underaligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 4 %dst, ptr addrspace(200) align 8 %src, i64 8, i1 false) must_preserve_cheri_tags
  ret void
  ; The memmove could be inlined but was tagged with nobuiltin -> should call memmove()
  ; CHECK-LABEL: memmove_underaligned_must_preserve_tags:
  ; PURECAP: ccall memmove
  ; HYBRID: call memmove_c
}

define void @memmove_aligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 8 %dst, ptr addrspace(200) align 8 %src, i64 8, i1 false) must_preserve_cheri_tags
  ret void
  ; Correctly aligned -> no need for libcall (even with attribute)
  ; CHECK-LABEL: memmove_aligned_must_preserve_tags:
  ; PURECAP: # %bb.0:
  ; PURECAP-NEXT: clc a1, 0(a1)
  ; PURECAP-NEXT: csc a1, 0(a0)
  ; PURECAP-NEXT: cret
  ; HYBRID: # %bb.0:
  ; HYBRID-NEXT: lc.cap a1, (a1)
  ; HYBRID-NEXT: sc.cap a1, (a0)
  ; HYBRID-NEXT: ret
}
