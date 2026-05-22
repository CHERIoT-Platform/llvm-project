; RUN: llc @HYBRID_HARDFLOAT_ARGS@ -verify-machineinstrs %s -o - | FileCheck %s --check-prefix=HYBRID

declare ptr addrspace(200) @llvm.cheri.cap.bounds.set.iCAPRANGE(ptr addrspace(200), iCAPRANGE)

define ptr addrspace(200) @setBoundsUnknown(ptr addrspace(200) %c, iCAPRANGE %bounds) nounwind {
entry:
  %0 = tail call ptr addrspace(200) @llvm.cheri.cap.bounds.set.iCAPRANGE(ptr addrspace(200) %c, iCAPRANGE %bounds)
  ret ptr addrspace(200) %0
}

define ptr addrspace(200) @setBoundsConstant1(ptr addrspace(200) %c) nounwind {
entry:
  %0 = tail call ptr addrspace(200) @llvm.cheri.cap.bounds.set.iCAPRANGE(ptr addrspace(200) %c, iCAPRANGE 1)
  ret ptr addrspace(200) %0
}

define ptr addrspace(200) @setBoundsConstantMaxImmediate(ptr addrspace(200) %c) nounwind {
entry:
  %0 = tail call ptr addrspace(200) @llvm.cheri.cap.bounds.set.iCAPRANGE(ptr addrspace(200) %c, iCAPRANGE 2047)
  ret ptr addrspace(200) %0
}

define ptr addrspace(200) @setBoundsConstantTooBigForImm(ptr addrspace(200) %c) nounwind {
entry:
  %0 = tail call ptr addrspace(200) @llvm.cheri.cap.bounds.set.iCAPRANGE(ptr addrspace(200) %c, iCAPRANGE 2048)
  ret ptr addrspace(200) %0
}

define ptr addrspace(200) @setBoundsNegative(ptr addrspace(200) %c) nounwind {
entry:
  %0 = tail call ptr addrspace(200) @llvm.cheri.cap.bounds.set.iCAPRANGE(ptr addrspace(200) %c, iCAPRANGE -2)
  ret ptr addrspace(200) %0
}
