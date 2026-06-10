; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s

;; Crash found while compiling rust-generated code:
declare { ptr addrspace(200), ptr addrspace(200), ptr addrspace(200) } @"_ZN13libcore_cheri5slice29_$LT$impl$u20$$u5b$T$u5d$$GT$4iter17h36a7eda044ca512cE"(ptr addrspace(200), i128) addrspace(200)

define void @a() nounwind {
  %1 = call { ptr addrspace(200), ptr addrspace(200), ptr addrspace(200) } @"_ZN13libcore_cheri5slice29_$LT$impl$u20$$u5b$T$u5d$$GT$4iter17h36a7eda044ca512cE"(ptr addrspace(200) null, i128 6)
  ret void
}
