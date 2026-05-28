; RUN: llc @HYBRID_HARDFLOAT_ARGS@ -O2 -verify-machineinstrs -relocation-model=static %s -o - | FileCheck %s
; https://github.com/CTSRD-CHERI/llvm/issues/228

; Function Attrs: inlinehint nounwind
define void @CHERIABI_SYS_mknodat_fill_uap() nounwind {
cheriabi_fetch_syscall_arg.exit119:
  %0 = ptrtoint ptr addrspace(200) undef to i16
  store i16 %0, ptr undef, align 2
  unreachable
}

define void @CHERIABI_SYS_mknodat_fill_uap1() nounwind {
cheriabi_fetch_syscall_arg.exit119:
  %0 = getelementptr i8, ptr addrspace(200) null, i64 undef
  store ptr addrspace(200) %0, ptr undef, align 16
  unreachable
}

define void @CHERIABI_SYS_mknodat_fill_uap2(ptr addrspace(200) %arg) nounwind {
cheriabi_fetch_syscall_arg.exit119:
  %0 = ptrtoint ptr addrspace(200) %arg to i16
  store i16 %0, ptr undef, align 2
  unreachable
}

define void @CHERIABI_SYS_mknodat_fill_uap3(ptr addrspace(200) %arg, ptr %ptr) nounwind {
cheriabi_fetch_syscall_arg.exit119:
  %0 = ptrtoint ptr addrspace(200) %arg to i16
  store i16 %0, ptr %ptr, align 2
  ret void
}
