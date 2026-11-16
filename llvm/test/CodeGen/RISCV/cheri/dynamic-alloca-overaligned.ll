; RUN: %riscv32_cheri_purecap_llc -verify-machineinstrs < %s | FileCheck %s
; RUN: %riscv64_cheri_purecap_llc -verify-machineinstrs < %s | FileCheck %s

; A dynamic alloca must be aligned to the requested alignment, not just to the
; representable-alignment mask, when that's more than the stack alignment.

declare void @use(ptr addrspace(200))

define void @overaligned(i32 %n) nounwind {
; CHECK-LABEL: overaligned:
; CHECK:      cram [[MASK:a[0-9]+]], {{a[0-9]+}}
; CHECK-NEXT: and [[ADDR:a[0-9]+]], {{a[0-9]+}}, [[MASK]]
; CHECK-NEXT: andi [[ALIGNED:a[0-9]+]], [[ADDR]], -64
; CHECK-NEXT: csetaddr {{a[0-9]+}}, sp, [[ALIGNED]]
  %p = alloca i8, i32 %n, align 64, addrspace(200)
  call void @use(ptr addrspace(200) %p)
  ret void
}

define void @stack_aligned(i32 %n) nounwind {
; CHECK-LABEL: stack_aligned:
; CHECK:      cram [[MASK:a[0-9]+]], {{a[0-9]+}}
; CHECK-NEXT: and [[ADDR:a[0-9]+]], {{a[0-9]+}}, [[MASK]]
; CHECK-NEXT: csetaddr {{a[0-9]+}}, sp, [[ADDR]]
  %p = alloca i8, i32 %n, align 16, addrspace(200)
  call void @use(ptr addrspace(200) %p)
  ret void
}
