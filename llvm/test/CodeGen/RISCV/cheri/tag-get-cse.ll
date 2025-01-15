; RUN: %riscv32_cheri_purecap_llc < %s | FileCheck %s
target datalayout = "e-m:e-pf200:64:64:64:32-p:32:32-i64:64-n32-S128-A200-P200-G200"
target triple = "riscv32-unknown-unknown"

; Function Attrs: nounwind
define dso_local i32 @foo() local_unnamed_addr addrspace(200) {
; CHECK-LABEL: foo:
; CHECK:    cgettag
; CHECK:    ccall print
; CHECK:    ccall might_free
; CHECK:    cgettag
; CHECK:    ccall print
; CHECK:    cret
entry:
  %call = tail call  dereferenceable_or_null(128) addrspace(200) ptr addrspace(200) @malloc(i32 noundef 128)
  %0 = tail call addrspace(200) i1 @llvm.cheri.cap.tag.get.temporal(ptr addrspace(200) %call)
  tail call addrspace(200) void @print(i1 noundef zeroext %0) #4
  tail call addrspace(200) void @might_free(ptr addrspace(200) noundef %call) #4
  %1 = tail call addrspace(200) i1 @llvm.cheri.cap.tag.get.temporal(ptr addrspace(200) %call)
  tail call addrspace(200) void @print(i1 noundef zeroext %1) #4
  ret i32 0
}

declare dso_local  noalias noundef ptr addrspace(200) @malloc(i32 noundef) local_unnamed_addr addrspace(200)

declare i1 @llvm.cheri.cap.tag.get.temporal(ptr addrspace(200)) addrspace(200)

declare dso_local void @print(i1 noundef zeroext) local_unnamed_addr addrspace(200)

declare dso_local void @might_free(ptr addrspace(200) noundef) local_unnamed_addr addrspace(200)

