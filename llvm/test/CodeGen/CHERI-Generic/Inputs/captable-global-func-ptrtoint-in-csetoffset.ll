; RUN: llc @PURECAP_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=PURECAP
; Check that we don't accidentally create a nonsense COPY node ($at_64 = COPY killed renamable $c1)
; when the result of the GlobalAddr goes through a ptrtoint

declare i32 @foo(ptr addrspace(200)) addrspace(200)

declare ptr addrspace(200) @llvm.cheri.cap.offset.set.iCAPRANGE(ptr addrspace(200), iCAPRANGE) addrspace(200)

define void @main(ptr addrspace(200) %arg) addrspace(200) nounwind {
entry:
  %0 = call ptr addrspace(200) @llvm.cheri.cap.offset.set.iCAPRANGE(ptr addrspace(200) %arg, iCAPRANGE ptrtoint (ptr addrspace(200) @foo to iCAPRANGE))
  %call3 = call i32 %0(ptr addrspace(200) undef)
  ret void
}
