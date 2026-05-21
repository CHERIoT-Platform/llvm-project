; RUN: llc @HYBRID_HARDFLOAT_ARGS@ %s -o - | FileCheck %s --check-prefix=HYBRID

@n = common global ptr addrspace(200) null, align 32

define void @set(ptr addrspace(200) %h) nounwind {
  %1 = load ptr addrspace(200), ptr @n, align 32
  store ptr addrspace(200) %h, ptr addrspace(200) %1, align 32
  ret void
}
