; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -O1 %s -o - | FileCheck %s --check-prefix=PURECAP

@va_copy = common addrspace(200) global ptr addrspace(200) null, align @CAP_BYTES@

define void @copy_to_global(ptr addrspace(200) nocapture readnone %y, i32 signext %x, ...) addrspace(200) nounwind {
  %1 = alloca ptr addrspace(200), align @CAP_BYTES@, addrspace(200)
  call void @llvm.va_start.p200(ptr addrspace(200) %1)
  call void @llvm.va_copy.p200.p200(ptr addrspace(200) @va_copy, ptr addrspace(200) %1)
  call void @llvm.va_end.p200(ptr addrspace(200) %1)
  ret void
}

define ptr addrspace(200) @copy_from_global() addrspace(200) nounwind {
  %1 = alloca ptr addrspace(200), align @CAP_BYTES@, addrspace(200)
  call void @llvm.va_copy.p200.p200(ptr addrspace(200) %1, ptr addrspace(200) @va_copy)
  %2 = load ptr addrspace(200), ptr addrspace(200) %1, align @CAP_BYTES@
  ret ptr addrspace(200) %2
}

declare void @llvm.va_start.p200(ptr addrspace(200)) addrspace(200)
declare void @llvm.va_copy.p200.p200(ptr addrspace(200), ptr addrspace(200)) addrspace(200)
declare void @llvm.va_end.p200(ptr addrspace(200)) addrspace(200)
