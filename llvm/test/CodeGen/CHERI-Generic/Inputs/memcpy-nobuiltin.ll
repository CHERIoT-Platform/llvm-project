; !DO NOT AUTOGEN!
; Check that llvm.memcpy() and llvm.memmove() intrinisics with must_preserve_cheri_tags
; attribute are always lowered to libcalls
; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,PURECAP
; RUN: llc @HYBRID_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefixes=CHECK,HYBRID

declare void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) noalias nocapture writeonly, ptr addrspace(200) noalias nocapture readonly, i64, i1 immarg)
declare void @llvm.memmove.p200.p200.i64(ptr addrspace(200) nocapture writeonly, ptr addrspace(200) nocapture readonly, i64, i1 immarg)

define void @memcpy_too_big_unaligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align 1 %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 2048, i1 false)
  ret void
  ; CHECK-LABEL: memcpy_too_big_unaligned:
@IF-MIPS@  ; PURECAP: clcbi $c12, %capcall20(memcpy)
@IF-MIPS@  ; HYBRID: ld $25, %call16(memcpy_c)($gp)
@IF-RISCV@  ; PURECAP: ccall memcpy
@IF-RISCV@  ; HYBRID: call memcpy_c
}

define void @memcpy_aligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align @CAP_BYTES@ %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 @CAP_BYTES@, i1 false)
  ret void
  ; This can be inlined;
  ; CHECK-LABEL: memcpy_aligned:
@IF-MIPS@  ; CHECK: # %bb.0:
@IF-MIPS@  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
@IF-MIPS@  ; PURECAP-NEXT: cjr $c17
@IF-MIPS@  ; HYBRID-NEXT: jr $ra
@IF-MIPS@  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
@IF-RISCV@  ; PURECAP: # %bb.0:
@IF-RISCV@  ; PURECAP-NEXT: clc a1, 0(a1)
@IF-RISCV@  ; PURECAP-NEXT: csc a1, 0(a0)
@IF-RISCV@  ; PURECAP-NEXT: cret
@IF-RISCV@  ; HYBRID: # %bb.0:
@IF-RISCV@  ; HYBRID-NEXT: lc.cap a1, (a1)
@IF-RISCV@  ; HYBRID-NEXT: sc.cap a1, (a0)
@IF-RISCV@  ; HYBRID-NEXT: ret
}

define void @memcpy_aligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align @CAP_BYTES@ %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 @CAP_BYTES@, i1 false) must_preserve_cheri_tags
  ret void
  ; Correctly aligned -> no need for libcall (even with attribute)
  ; CHECK-LABEL: memcpy_aligned_must_preserve_tags:
@IF-MIPS@  ; CHECK: # %bb.0:
@IF-MIPS@  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
@IF-MIPS@  ; PURECAP-NEXT: cjr $c17
@IF-MIPS@  ; HYBRID-NEXT: jr $ra
@IF-MIPS@  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
@IF-RISCV@  ; PURECAP: # %bb.0:
@IF-RISCV@  ; PURECAP-NEXT: clc a1, 0(a1)
@IF-RISCV@  ; PURECAP-NEXT: csc a1, 0(a0)
@IF-RISCV@  ; PURECAP-NEXT: cret
@IF-RISCV@  ; HYBRID: # %bb.0:
@IF-RISCV@  ; HYBRID-NEXT: lc.cap a1, (a1)
@IF-RISCV@  ; HYBRID-NEXT: sc.cap a1, (a0)
@IF-RISCV@  ; HYBRID-NEXT: ret
}

define void @memcpy_underaligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memcpy.p200.p200.i64(ptr addrspace(200) align @CAP_RANGE_BYTES@ %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 @CAP_BYTES@, i1 false) must_preserve_cheri_tags
  ret void
  ; The memmove could be inlined but was tagged with nobuiltin -> should call memmove()
  ; CHECK-LABEL: memcpy_underaligned_must_preserve_tags:
@IF-MIPS@  ; PURECAP: clcbi $c12, %capcall20(memcpy)
@IF-MIPS@  ; HYBRID: ld $25, %call16(memcpy_c)($gp)
@IF-RISCV@  ; PURECAP: ccall memcpy
@IF-RISCV@  ; HYBRID: call memcpy_c
}


; Same again in hybrid:

define void @memmove_too_big_unaligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align 1 %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 2048, i1 false)
  ret void
  ; CHECK-LABEL: memmove_too_big_unaligned:
@IF-MIPS@  ; PURECAP: clcbi $c12, %capcall20(memmove)
@IF-MIPS@  ; HYBRID: ld $25, %call16(memmove_c)($gp)
@IF-RISCV@  ; PURECAP: ccall memmove
@IF-RISCV@  ; HYBRID: call memmove_c
}

define void @memmove_aligned(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align @CAP_BYTES@ %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 @CAP_BYTES@, i1 false)
  ret void
  ; This can be inlined;
  ; CHECK-LABEL: memmove_aligned:
@IF-MIPS@  ; CHECK: # %bb.0:
@IF-MIPS@  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
@IF-MIPS@  ; PURECAP-NEXT: cjr $c17
@IF-MIPS@  ; HYBRID-NEXT: jr $ra
@IF-MIPS@  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
@IF-RISCV@  ; PURECAP: # %bb.0:
@IF-RISCV@  ; PURECAP-NEXT: clc a1, 0(a1)
@IF-RISCV@  ; PURECAP-NEXT: csc a1, 0(a0)
@IF-RISCV@  ; PURECAP-NEXT: cret
@IF-RISCV@  ; HYBRID: # %bb.0:
@IF-RISCV@  ; HYBRID-NEXT: lc.cap a1, (a1)
@IF-RISCV@  ; HYBRID-NEXT: sc.cap a1, (a0)
@IF-RISCV@  ; HYBRID-NEXT: ret
}

define void @memmove_underaligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align @CAP_RANGE_BYTES@ %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 @CAP_BYTES@, i1 false) must_preserve_cheri_tags
  ret void
  ; The memmove could be inlined but was tagged with nobuiltin -> should call memmove()
  ; CHECK-LABEL: memmove_underaligned_must_preserve_tags:
@IF-MIPS@  ; PURECAP: clcbi $c12, %capcall20(memmove)
@IF-MIPS@  ; HYBRID: ld $25, %call16(memmove_c)($gp)
@IF-RISCV@  ; PURECAP: ccall memmove
@IF-RISCV@  ; HYBRID: call memmove_c
}

define void @memmove_aligned_must_preserve_tags(ptr addrspace(200) %dst, ptr addrspace(200) %src) noinline nounwind {
entry:
  call void @llvm.memmove.p200.p200.i64(ptr addrspace(200) align @CAP_BYTES@ %dst, ptr addrspace(200) align @CAP_BYTES@ %src, i64 @CAP_BYTES@, i1 false) must_preserve_cheri_tags
  ret void
  ; Correctly aligned -> no need for libcall (even with attribute)
  ; CHECK-LABEL: memmove_aligned_must_preserve_tags:
@IF-MIPS@  ; CHECK: # %bb.0:
@IF-MIPS@  ; CHECK-NEXT: clc $c1, $zero, 0($c4)
@IF-MIPS@  ; PURECAP-NEXT: cjr $c17
@IF-MIPS@  ; HYBRID-NEXT: jr $ra
@IF-MIPS@  ; CHECK-NEXT: csc $c1, $zero, 0($c3)
@IF-RISCV@  ; PURECAP: # %bb.0:
@IF-RISCV@  ; PURECAP-NEXT: clc a1, 0(a1)
@IF-RISCV@  ; PURECAP-NEXT: csc a1, 0(a0)
@IF-RISCV@  ; PURECAP-NEXT: cret
@IF-RISCV@  ; HYBRID: # %bb.0:
@IF-RISCV@  ; HYBRID-NEXT: lc.cap a1, (a1)
@IF-RISCV@  ; HYBRID-NEXT: sc.cap a1, (a0)
@IF-RISCV@  ; HYBRID-NEXT: ret
}
