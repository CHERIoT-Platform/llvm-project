; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - -O2 | FileCheck %s

define ptr addrspace(200) @wrap_mempcpy(ptr addrspace(200) %dst, ptr addrspace(200) %src, i64 %n) nounwind {
  %1 = tail call ptr addrspace(200) @mempcpy(ptr addrspace(200) %dst, ptr addrspace(200) %src, i64 %n)
  ret ptr addrspace(200) %1
}

declare ptr addrspace(200) @mempcpy(ptr addrspace(200), ptr addrspace(200), i64)
