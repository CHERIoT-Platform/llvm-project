; DO NOT EDIT -- This file was generated from test/CodeGen/CHERI-Generic/Inputs/memcpy-nobuiltin.ll
; Check that llvm.memcpy() and llvm.memmove() intrinisics with must_preserve_cheri_tags
; attribute are always lowered to libcalls
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi purecap -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,PURECAP
; RUN: llc -mtriple=mips64 -mcpu=cheri128 -mattr=+cheri128 --relocation-model=pic -target-abi n64 -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,HYBRID

declare void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memmove.p200.p200.i64(ptr addrspace(200) nocapture writeonly, ptr addrspace(200) nocapture readonly, i64, i1 immarg)

define void @memcpy_too_big_unaligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 1 %dst, ptr addrspace(200) align 16 %src, i64 2048, i1 false)
  ret void
  ; CHECK-LABEL: memcpy_too_big_unaligned:
  ; PURECAP: clcbi $c12, %capcall20(memcpy)
  ; HYBRID: ld $25, %call16(memcpy_c)($gp)
}

define void @memcpy_aligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %dst, ptr addrspace(200) align 16 %src, i64 16, i1 false)
  ret void
  ; This can be inlined;
  ; CHECK-LABEL: memcpy_aligned:
  ; CHECK: # %bb.0:
  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
  ; PURECAP-NEXT: cjr $c17
  ; HYBRID-NEXT: jr $ra
  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
}

define void @memcpy_aligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 16 %dst, ptr addrspace(200) align 16 %src, i64 16, i1 false) must_preserve_cheri_tags
  ret void
  ; Correctly aligned -> no need for libcall (even with attribute)
  ; CHECK-LABEL: memcpy_aligned_must_preserve_tags:
  ; CHECK: # %bb.0:
  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
  ; PURECAP-NEXT: cjr $c17
  ; HYBRID-NEXT: jr $ra
  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
}

define void @memcpy_underaligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 8 %dst, ptr addrspace(200) align 16 %src, i64 16, i1 false) must_preserve_cheri_tags
  ret void
  ; The memmove could be inlined but was tagged with nobuiltin -> should call memmove()
  ; CHECK-LABEL: memcpy_underaligned_must_preserve_tags:
  ; PURECAP: clcbi $c12, %capcall20(memcpy)
  ; HYBRID: ld $25, %call16(memcpy_c)($gp)
}


; Same again in hybrid:

define void @memmove_too_big_unaligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 1 %dst, ptr addrspace(200) align 16 %src, i64 2048, i1 false)
  ret void
  ; CHECK-LABEL: memmove_too_big_unaligned:
  ; PURECAP: clcbi $c12, %capcall20(memmove)
  ; HYBRID: ld $25, %call16(memmove_c)($gp)
}

define void @memmove_aligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 16 %dst, ptr addrspace(200) align 16 %src, i64 16, i1 false)
  ret void
  ; This can be inlined;
  ; CHECK-LABEL: memmove_aligned:
  ; CHECK: # %bb.0:
  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
  ; PURECAP-NEXT: cjr $c17
  ; HYBRID-NEXT: jr $ra
  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
}

define void @memmove_underaligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 8 %dst, ptr addrspace(200) align 16 %src, i64 16, i1 false) must_preserve_cheri_tags
  ret void
  ; The memmove could be inlined but was tagged with nobuiltin -> should call memmove()
  ; CHECK-LABEL: memmove_underaligned_must_preserve_tags:
  ; PURECAP: clcbi $c12, %capcall20(memmove)
  ; HYBRID: ld $25, %call16(memmove_c)($gp)
}

define void @memmove_aligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 16 %dst, ptr addrspace(200) align 16 %src, i64 16, i1 false) must_preserve_cheri_tags
  ret void
  ; Correctly aligned -> no need for libcall (even with attribute)
  ; CHECK-LABEL: memmove_aligned_must_preserve_tags:
  ; CHECK: # %bb.0:
  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
  ; PURECAP-NEXT: cjr $c17
  ; HYBRID-NEXT: jr $ra
  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
}
